#pragma once

#include <future>
#include <memory>
#include <CL/cl.hpp>
#include "megaflare/defs.hpp"
#include "megaflare/host/event.hpp"
#include "task.hpp"


namespace megaflare {
    namespace host {
            template <typename T>
            struct exec_and_unlock
            {
                template <typename Func>
                void operator() (Func i_func, std::promise<T>& i_promise)
                {
                    i_promise.set_value(i_func());
                }
            };

            template <>
            struct exec_and_unlock<void>
            {
                template <typename Func>
                void operator() (Func i_func, std::promise<void>& i_promise)
                {
                    i_func();
                    i_promise.set_value();
                }
            };

        template <typename Ret, typename Iterator>
        struct exec_and_unmap {
            cl_command_queue m_queue;
            std::function<Ret (Iterator, Iterator)> m_routine;
            boost::intrusive_ptr<_cl_mem> m_buf;
            Iterator m_pvMapped;
            std::size_t m_size;
            std::promise<Ret> m_promise;
            host::event::checked_out_t m_evToUnmap;


            void operator () ()
            {
                //値を返す
                exec_and_unlock<Ret>() (
                    [=]() {
                        return m_routine(m_pvMapped, 
                                         m_pvMapped + m_size);
                    },
                    m_promise
                );

                //イベントを開放
                ::clSetUserEventStatus(m_evToUnmap.get(), CL_COMPLETE);
            }

            static void run(exec_and_unmap<Ret, Iterator>* i_pThis)
            {
            }
            static void run_as_callback(cl_event, cl_int, void* i_pData){
                exec_and_unmap<Ret, Iterator>* pThis =
                    static_cast<exec_and_unmap<Ret, Iterator>*>(i_pData);

                auto routine = [ pThis ]() {
                    (*pThis)();
                    delete pThis;
                };

                std::thread th(std::move(routine));
                th.detach();
            }

            static void run_as_thread(cl_event i_ev, void* i_pData){
                exec_and_unmap<Ret, Iterator>* pThis =
                    static_cast<exec_and_unmap<Ret, Iterator>*>(i_pData);

                auto routine = [ pThis, i_ev ]() {
                    clWaitForEvents(1, &i_ev);
                    clReleaseEvent(i_ev);

                    (*pThis)();
                    delete pThis;
                };

                std::thread th(std::move(routine));
                th.detach();
            }
        };

        template <typename Routine, 
                  typename HostType //TODO: ポインタ込みに
                  >
        struct with_range_args {
            boost::intrusive_ptr<_cl_mem> m_buf;
            event& m_ev;
            std::size_t m_size;
            int m_offset;
            Routine m_routine;
            cl_map_flags m_flags;
        };


        template <typename Routine, 
                  typename HostType,
                  typename Iterator>
        struct with_range_task : 
            task<with_range_task<Routine, HostType, Iterator>>
        {
            typedef  typename 
                std::result_of<Routine(Iterator, Iterator)>::type Ret;

            with_range_args <Routine, HostType> m_args;

            with_range_task(with_range_args<Routine, HostType> i_args) 
            : m_args(i_args) {}

            std::future<Ret>
            operator() (cl_command_queue i_queue)
            {
                cl_context context;
                cl_int err = clGetCommandQueueInfo(
                    i_queue, CL_QUEUE_CONTEXT, sizeof(context), 
                    &context, NULL);

                err = CL_SUCCESS;

                //バッファの待っているイベントを取得
                host::event::checked_out_t evPre = 
                    m_args.m_ev.lock_and_checkout();
                cl_event pEvPre = evPre.get();
                cl_event evMap = NULL;



                //マッピング
                void * pvMapped = 
                    clEnqueueMapBuffer (    
                        i_queue,
                        m_args.m_buf.get(),
                        CL_FALSE,
                        m_args.m_flags,
                        sizeof(HostType) * m_args.m_offset, 
                        sizeof(HostType) * m_args.m_size,
                        (pEvPre != NULL)?1:0,
                        (pEvPre != NULL)?&pEvPre:NULL,
                        &evMap,
                        &err);

                cl_event evToUnmap = 
                    clCreateUserEvent(context, &err);
                cl_event pUnmap;
                //アンマップイベントを積みつつ、走り出さないようにしておく
                clEnqueueUnmapMemObject(
                    i_queue,
                    m_args.m_buf.get(), 
                    pvMapped,
                    1,
                    &evToUnmap,
                    &pUnmap);
                m_args.m_ev.checkin_and_unlock(pUnmap);
                                     
                //futureの準備
                std::promise<Ret> promise;
                std::future<Ret> future = promise.get_future();
                
                //コールバックに渡すオブジェクト
                //コールバック側でdelete                
                exec_and_unmap<Ret, Iterator> *pRoutine = 
                    new exec_and_unmap<Ret, Iterator>
                    {
                        i_queue,
                        std::forward<Routine>(m_args.m_routine), 
                        m_args.m_buf,
                        (HostType*)pvMapped,
                        m_args.m_size,
                        std::move(promise),
                        evToUnmap
                    };
                
#if 0
                evMap.setCallback(CL_COMPLETE, 
                                  exec_and_unmap<host_type, Ret>::run_as_callback, 
                                  pRoutine);
#else
                exec_and_unmap<Ret, Iterator>::run_as_thread(evMap, pRoutine);
#endif

                return future;

            }
        };
    }
}

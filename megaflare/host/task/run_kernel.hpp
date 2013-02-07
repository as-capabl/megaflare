#pragma once

#include "task.hpp"
#include <algorithm>
#include "megaflare/code/size.hpp"

namespace megaflare {
    namespace host {
        struct param_handler {
            virtual ~param_handler() {}
            virtual event::checked_out_t lock_and_set_arg(cl_kernel, int) = 0;
            virtual void unlock(event::checked_out_t &&) = 0;

            static int 
            compare(param_handler const* i_lhs,
                    param_handler const* i_rhs) {  
                return (i_lhs->get_ptr() < i_rhs->get_ptr()) ? (-1) :
                    ((i_lhs->get_ptr() == i_rhs->get_ptr()) ? 0 : 1 );
            }
        protected:
            virtual cl_mem get_ptr() const = 0;
        };

        template <typename TypeSpec, typename Policy>
        struct buffer_handler : param_handler {
            typedef host::buffer<TypeSpec, Policy> impl_type;

            buffer_handler(impl_type & i_buffer) 
            : m_buffer(i_buffer) {}
            virtual ~buffer_handler() {}

            virtual event::checked_out_t 
            lock_and_set_arg(cl_kernel i_kern, int i_nArg) {
                event::checked_out_t ev = m_buffer.m_ev.lock_and_checkout();
                if( ev ) {
                    BOOST_LOG_TRIVIAL(MF_LOG_QUEUING)
                        << "kernel  " << i_kern
                        << ": pre event set(buffer " << m_buffer.m_buf
                        << ", event " << ev <<").";
                }

                cl_mem rawBuf = m_buffer.m_buf.get();
                clSetKernelArg(i_kern, i_nArg, sizeof(cl_mem), &rawBuf);
                return ev;
            }

            virtual void unlock(event::checked_out_t && i_ev) {
                m_buffer.m_ev.checkin_and_unlock(std::move(i_ev));
            }

        protected:
            virtual cl_mem get_ptr() const {
                return m_buffer.m_buf.get();
            }
        private:
            impl_type & m_buffer;
        };

        struct make_handler_op {
            template <typename TypeSpec, typename Policy>
            buffer_handler<TypeSpec, Policy>
            operator () (host::buffer<TypeSpec, Policy> & i_buf) {
                return {i_buf};
            }
        };

        template <typename Tuple, sprout::index_t... Indices>
        inline void upcast_handlers(param_handler* o_ptrs[], 
                                    Tuple & i_tp, 
                                    sprout::index_tuple<Indices...>)
        {
            param_handler* ptrsTemp[] = {(&tuples::get<Indices>(i_tp))...};
            std::copy(ptrsTemp, ptrsTemp + sizeof...(Indices), o_ptrs);
        }

        template <typename Calling>
        struct run_kernel_task : task<run_kernel_task<Calling>> {

            cl::Program const & m_program;
            Calling m_pack;
            std::size_t m_size; //本当はNDRange

            run_kernel_task(cl::Program const & i_program, Calling i_pack, std::size_t i_size) :
                m_program(i_program),
                m_pack(i_pack),
                m_size(i_size)
            {
            }


            //template<std::size_t NName, typename... Args>
            cl_int
            operator() (cl_command_queue i_pQueue)
            {
                constexpr cl_int numParam = 
                    sprout::tuples::tuple_size<decltype(Calling::m_paramsAlt)>::value;
                cl_event events[numParam];

                cl_int err = CL_SUCCESS;
                cl::Kernel kernel(m_program, m_pack.m_name.c_str(), &err);
                if( err != CL_SUCCESS ) return err;

                auto tpHandlers = tuples::transform2(make_handler_op(), m_pack.m_paramsAlt);
                param_handler* ptrHandlers[numParam] = {nullptr};
                upcast_handlers(ptrHandlers, tpHandlers, sprout::index_range<0, numParam>::make());

                //食事する哲学者の問題を避けるため、アドレス順にソート
                std::sort(ptrHandlers, 
                          ptrHandlers + numParam, 
                          param_handler::compare);

                cl_int numEvents = 0;
                for( int i = 0; i < numParam; ++i ) {
                    host::event::checked_out_t coOld =
                        ptrHandlers[i]->lock_and_set_arg(kernel(), i);
                    if( coOld.get() ) {
                        //同じバッファを二度ロックしようとすると固まるので
                        if(i != 0 && 
                           param_handler::compare(
                               ptrHandlers[i - 1],
                               ptrHandlers[i]) == 0) {
                            continue;
                        }
                           
                        events[numEvents] = coOld.get();
                        clRetainEvent(events[numEvents]);
                        ++numEvents;
                    }
                }


                cl_event evDone;
                err = clEnqueueNDRangeKernel(
                    i_pQueue,
                    kernel(), 
                    1,
                    NULL,
                    &m_size,
                    NULL, 
                    numEvents,
                    ((numEvents != 0)?events:0),
                    &evDone
                );
                if( err != CL_SUCCESS ) return err;
                BOOST_LOG_TRIVIAL(MF_LOG_QUEUING)
                    << "kernel " << kernel()
                    << ": executed. post event is " << evDone << ".";

                for( int i = 0; i < numEvents; ++i ) {
                    ::clReleaseEvent(events[i]);
                }

                for( int i = numParam - 1; i >= 0 ; --i ) {
                    host::event::checked_out_t coDone = evDone;
                    ptrHandlers[i]->unlock(std::move(coDone));
                }

                return CL_SUCCESS;
            }
        };

        template <typename Calling>
        inline run_kernel_task<Calling>
        run_kernel (cl::Program const & i_program, Calling i_pack, std::size_t i_size)
        {
            return {i_program, i_pack, i_size};
        }        
    }
}

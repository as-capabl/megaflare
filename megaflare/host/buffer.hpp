#pragma once

#include <future>
#include <memory>
#include <boost/intrusive_ptr.hpp>
#include <CL/cl.hpp>
#include "megaflare/defs.hpp"
#include "event.hpp"
#include "task/with_range.hpp"
#include "buffer_policy.hpp"

void intrusive_ptr_add_ref(_cl_mem* i_ptr)
{
    ::clRetainMemObject(i_ptr);
}

void intrusive_ptr_release(_cl_mem* i_ptr)
{
    ::clReleaseMemObject(i_ptr);
}

namespace megaflare {
    namespace host {
        template <typename TypeSpec, 
                  typename Policy = policies::buffer_default>
        struct buffer {
            typedef boost::intrusive_ptr<_cl_mem> ptr_t;
#if 0
            buffer(cl::Buffer const& i_buf) 
                : m_buf(i_buf), m_ev()
            {
            }
#endif
            buffer(const cl::Context& context,
                   cl_int size,
                   void* host_ptr = NULL,
                   cl_int* err = NULL)
                : m_buf(
                    ::clCreateBuffer(
                        context(), 
                        policies::mem_flag<Policy>(), 
                        size * sizeof(host_type),
                        host_ptr,
                        err)) ,
                  m_ev(),
                  m_size(size)
            {
            }

            buffer(buffer const&) = delete;
            buffer(buffer &&) = delete;

#if 0
            buffer(buffer && i_moved) 
            : m_buf(i_moved.m_buf), m_ev(i_moved.m_ev)
            {
            }
#endif
            template <typename T>
            buffer<TypeSpec>&
            subst_to_param(T const &)
            {
                static_assert(
                    std::is_same<typename T::host_type, 
                                 typename TypeSpec::host_type*>::value, 
                    "parameter type mismatch");
                return *this;
            }            

            typedef typename TypeSpec::host_type host_type;
            typedef host_type* iterator;
            typedef host_type const * const_iterator;

            template <typename Ret>
            static void with_range_callback(cl_event, cl_int, void* i_pvBody)
            {
                std::function<void ()>* pRoutine = 
                    static_cast<std::function<void ()>*>(i_pvBody);
                (*pRoutine)();
                delete pRoutine;
            }

            sprout::tuple<cl_int, std::size_t>
            range_check(cl_int i_offset, 
                     std::size_t i_size) const
            {
                if (i_offset < 0 || i_offset >= m_size) {
                    throw std::out_of_range("host::buffer::with_range");
                }
                if (i_size <= 0) {
                    i_size = m_size - i_offset;
                }

                return sprout::make_tuple(i_offset, i_size);
            }

            template <typename Routine>
            with_range_task<Routine, host_type, iterator>
            with_range(Routine i_routine,
                     cl_int i_offset = 0,
                     std::size_t i_size = 0)
            {
                sprout::tie(i_offset, i_size) =
                    range_check(i_offset, i_size);
                return with_range_args<Routine, host_type> {
                    m_buf, m_ev, i_size, i_offset, i_routine, 
                        policies::map_flag<Policy>() };
            }

            template <typename Routine>
            with_range_task<Routine, host_type, const_iterator>
            with_range(Routine i_routine,
                     cl_int i_offset = 0,
                     std::size_t i_size = 0) const
            {
                sprout::tie(i_offset, i_size) =
                    range_check(i_offset, i_size);

                return with_range_args<Routine, host_type> {
                    m_buf, m_ev, i_size, i_offset, i_routine, 
                        policies::const_map_flag<Policy>() };
            }

            //friend struct buffer_handler<TypeSpec, Policy>;
            
            //cl::Buffer& get() { return m_buf; }
        public: //run_packがスタブの間はpublic
            friend struct set_arg_op;
            friend struct set_event_op;

            ptr_t mutable m_buf;
            ::megaflare::host::event mutable m_ev;
            int m_size;
        };
    }
}

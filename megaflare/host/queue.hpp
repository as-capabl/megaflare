#pragma once

#include <boost/intrusive_ptr.hpp>

void intrusive_ptr_add_ref(_cl_command_queue* i_ptr)
{
    ::clRetainCommandQueue(i_ptr);
}

void intrusive_ptr_release(_cl_command_queue* i_ptr)
{
    ::clReleaseCommandQueue(i_ptr);
}

namespace megaflare {
    namespace host {
        struct queue {
            queue(cl_context i_ctx, 
                  cl_device_id i_dev) : m_ptr(NULL)
            {
                cl_int err;
                m_ptr =::clCreateCommandQueue(
                    i_ctx, 
                    i_dev,
                    0,
                    &err
                );
            }

            cl_command_queue get() const {
                return m_ptr.get();
            }


            //template <typename Derived>
            //struct task {
            //    //void operator() (cl_queue i_pQueue);
            //};
            template <typename Derived>
            friend struct task;
            
            template <typename Derived, 
                      typename Enabler = 
                      typename std::enable_if<
                          std::is_base_of< ::megaflare::host::task<Derived>, Derived>::value
                          >::type >
            auto operator() (Derived i_task)
                -> typename std::result_of<Derived(cl_command_queue)>::type
            {
                return i_task(m_ptr.get());
            }
        private:
            typedef boost::intrusive_ptr<_cl_command_queue> impl_t;
            impl_t m_ptr;
        };
    }
}

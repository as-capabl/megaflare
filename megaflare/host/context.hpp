#pragma once

#include <boost/intrusive_ptr.hpp>
#include "context_policy.hpp"

void intrusive_ptr_add_ref(_cl_context * i_ptr)
{
    ::clRetainContext(i_ptr);
}

void intrusive_ptr_release(_cl_context * i_ptr)
{
    ::clReleaseContext(i_ptr);
}

namespace megaflare {
    namespace host {
        template <typename Policy = policy::buffer_default>
        struct context {
            program()
                : m_ptr()
            {
            }
        private:
            boost::intrusive_ptr<_cl_context> m_ptr;
        };
    }
}

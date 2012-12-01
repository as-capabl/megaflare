#pragma once

#include <boost/intrusive_ptr.hpp>

void intrusive_ptr_add_ref(_cl_program * i_ptr)
{
    ::clRetainProgram(i_ptr);
}

void intrusive_ptr_release(_cl_program * i_ptr)
{
    ::clReleaseProgram(i_ptr);
}

namespace megaflare {
    namespace host {
        template <typename Source>
        struct program {
            program(cl_context i_ctx, Source i_src)
                : m_ptr()
            {
            }
        private:
            boost::intrusive_ptr<_cl_program> m_ptr;
        };
    }
}

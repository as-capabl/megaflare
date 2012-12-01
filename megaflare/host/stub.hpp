#pragma once
#include <CL/cl.hpp>
namespace megaflare {
    namespace host {
        typedef cl::Context context;
        typedef cl::Platform platform;
        typedef cl::Device device;

        template <typename Program>
        inline cl::Program make_program(context i_context, Program i_prog)
        {
            auto str = code::get_cl_string(i_prog);

            cl::Program::Sources source(1, std::make_pair(str.c_str(), str.length()));
            return cl::Program{i_context, source};
        }
    }
}

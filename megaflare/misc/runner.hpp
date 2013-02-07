#pragma once

#include <megaflare/host.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <cstdlib>

namespace megaflare {
    namespace misc {
        namespace popt = boost::program_options;

        enum struct running_error {
            no_platform,
        };

        std::ostream& operator<<(std::ostream& io_os, running_error i_err)
        {
            switch(i_err) {
            case running_error::no_platform:
                io_os << "No platform"; break;
            };
            return io_os;
        }


        // runner
        struct runner {
            runner(int i_argc, char** i_argv)
                : m_platforms(),
                  m_context(init_context(m_platforms)),
                  m_devices(m_context.getInfo<CL_CONTEXT_DEVICES>()),
                  m_iDeviceNo(),
                  m_queue(m_context(), m_devices[m_iDeviceNo]()) //TODO: 例外拾おう
            {
                static_cast<void>(i_argc);
                static_cast<void>(i_argv);
            }

            ~runner()
            {
                ::clFinish(m_queue.get()); //TODO; メンバ関数にしましょう
            }

            template <typename ProgramCode>
            void
            with_program(
                ProgramCode i_prog,
                std::function<void(runner const &, 
                                   host::program<ProgramCode>)> i_func)
            {
                host::program<ProgramCode> program = host::make_program(m_context, i_prog);

                try {
                    program.build(m_devices);
                }
                catch(cl::Error err) {
                    if(err.err() == CL_BUILD_PROGRAM_FAILURE)
                    {
                        std::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_devices[m_iDeviceNo]);

                        std::cerr << "Compilation error";
                        std::cerr << "Source:\n"
                                  << get_cl_string(i_prog);
                        std::cerr << "Build log:\n"
                                  << str;
                        return;
                    }
                    else {
                        throw err;
                    }
                }

                i_func(*this, program);
            }

        private:
            static host::context 
            init_context(std::vector<host::platform> & i_platforms);

        public:
            std::vector<host::platform> m_platforms;
            host::context m_context;
            std::vector<host::device> m_devices;
            cl_int m_iDeviceNo;
            host::queue mutable m_queue;
        };

        host::context 
        runner::init_context(std::vector<host::platform> & o_platforms)
        {
            host::platform::get(&o_platforms);
            if (o_platforms.size() == 0) {
                throw running_error::no_platform;
            }


            cl_context_properties properties[] = {
                CL_CONTEXT_PLATFORM, 
                (cl_context_properties)(o_platforms[0])(), 
                0};

            host::context context(CL_DEVICE_TYPE_GPU, properties);
            return context; //TODO: 例外拾おう
        }
    }
}

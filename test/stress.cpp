#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <cstdlib>


#include <megaflare/code.hpp>
#include <megaflare/platform.hpp>
#include <megaflare/host.hpp>
#include <megaflare/misc/runner.hpp>

namespace pfm = megaflare::platform;
namespace chrono = std::chrono;
namespace code = megaflare::code;
namespace host = megaflare::host;
namespace mtpl = megaflare::tuples;
namespace misc = megaflare::misc;

auto fill_index = code::func(
    "fill_index",
    code::returns<pfm::void_>()|
    code::param<code::global<pfm::int_>*>("pInt"),
    "int id = get_global_id(0);\n"
    "pInt[id] = id;"
);

auto add = code::func(
    "add",
    code::returns<pfm::void_>()|
    code::param<code::global<pfm::int_>*>("pLhs")|
    code::param<code::global<pfm::int_>*>("pValue"),
    "int id = get_global_id(0);\n"
    "pLhs[id] += pValue[id];"
);

auto subtract = code::func(
    "subtract",
    code::returns<pfm::void_>()|
    code::param<code::global<pfm::int_>*>("pLhs")|
    code::param<code::global<pfm::int_>*>("pValue"),
    "int id = get_global_id(0);\n"
    "pLhs[id] -= pValue[id];"
);



auto prog = code::program(
    code::kernel(fill_index),
    code::kernel(add),
    code::kernel(subtract)
);

static constexpr int item_count = 20;




void stress(misc::runner const & i_runner, 
            host::program<decltype(prog)> i_program) 
{        
    host::context const& context = i_runner.m_context;
    host::queue & queue = i_runner.m_queue;

    host::buffer<pfm::int_> bufWrite(context, item_count);
    host::buffer<pfm::int_> bufDiff(context, item_count);
    queue(host::run_kernel(i_program, fill_index(bufWrite), item_count));
    queue(host::run_kernel(i_program, fill_index(bufDiff), item_count));
        
    for (int i = 0; i < 100; ++i) {
        std::thread th(
            [&] () {
                queue(host::run_kernel(i_program, add(bufWrite, bufDiff), item_count));
                queue(host::run_kernel(i_program, subtract(bufWrite, bufDiff), item_count));
            }
        );
        th.detach();
    }

    typedef decltype(bufWrite)::iterator iterator;
    for (int i = 0; i < 20; ++i) {
        auto future2 = 
            queue(bufWrite.with_range(
                      [](iterator i_begin, iterator i_end){
                          return std::accumulate(i_begin, i_end, 0);
                      }));
        cl_int retval = future2.get();
        //std::cout << retval << std::endl;
        assert( retval %  (item_count * (item_count - 1) / 2) == 0);
    }
}

int main(int i_argc, char** i_argv) try 
{
    misc::runner runner(i_argc, i_argv);
    runner.with_program<decltype(prog)>(prog, stress);
    return EXIT_SUCCESS;
}
catch (cl::Error err) {
    std::cerr << err.what() << "(" << err.err() << ")" ;
    return -1;
}
catch (misc::running_error err) {
    std::cerr << err;
    return -1;
}

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <unistd.h>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>

#include <megaflare/code.hpp>
#include <megaflare/platform.hpp>
#include <megaflare/host.hpp>


namespace pfm = megaflare::platform;
namespace chrono = std::chrono;
namespace code = megaflare::code;
namespace host = megaflare::host;
namespace stpl = sprout::tuples;
namespace mtpl = megaflare::tuples;
namespace logging = boost::log;
namespace flt = boost::log::filters;

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


void init()
{
    logging::core::get()->set_filter
    (
        flt::attr< logging::trivial::severity_level >("Severity") 
        >= logging::trivial::info
    );
}

auto prog = code::program(
    code::kernel(fill_index),
    code::kernel(add),
    code::kernel(subtract)
);

static constexpr int item_count = 20;

cl_command_queue g_pAtExit = NULL;
void at_exit_my()
{
    if ( g_pAtExit ){
        ::clFinish(g_pAtExit);
    }
}



int main() try {
    init();

    std::vector<host::platform> platforms;
    host::platform::get(&platforms);
    if (platforms.size() == 0) {
        BOOST_LOG_TRIVIAL(fatal) << "no platform";
        return -1;
    }
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
    host::context context(CL_DEVICE_TYPE_GPU, properties);

    auto devices = context.getInfo<CL_CONTEXT_DEVICES>();

    auto program = host::make_program(context, prog);

    try {
        program.build(devices);
    }
    catch(cl::Error err) {
        if(err.err() == CL_BUILD_PROGRAM_FAILURE)
        {
            std::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);

            BOOST_LOG_TRIVIAL(fatal) << "Compilation error";
            BOOST_LOG_TRIVIAL(info) << "Source:\n"
                                    << get_cl_string(prog);
            BOOST_LOG_TRIVIAL(info) << "Build log:\n"
                                    << str;
            return -1;
        }
        else {
            throw err;
        }
    }
    host::queue queue(context(), devices[0]());
    g_pAtExit = queue.get();
    ::at_quick_exit(at_exit_my);
        
    host::buffer<pfm::int_> bufWrite(context, item_count);
    host::buffer<pfm::int_> bufDiff(context, item_count);
    queue(host::run_kernel(program, fill_index(bufWrite), item_count));
    queue(host::run_kernel(program, fill_index(bufDiff), item_count));
        
    for (int ii = 0; ii < 100; ++ii) {
        std::thread th(
            [&] () {
                queue(host::run_kernel(program, add(bufWrite, bufDiff), item_count));
                queue(host::run_kernel(program, subtract(bufWrite, bufDiff), item_count));
            }
        );
        th.detach();
    }

    typedef decltype(bufWrite)::iterator iterator;
    for (int ii = 0; ii < 20; ++ii) {
        auto future2 = 
            queue(bufWrite.with_range(
                      [](iterator i_begin, iterator i_end){
                          return std::accumulate(i_begin, i_end, 0);
                      }));
        cl_int retval = future2.get();
        std::cout << retval << std::endl;
        assert( retval %  (item_count * (item_count - 1) / 2) == 0);
    }

    ::quick_exit(0);
} catch (cl::Error err) {
    BOOST_LOG_TRIVIAL(fatal) << err.what() << "(" << err.err() << ")" ;
    return -1;
}

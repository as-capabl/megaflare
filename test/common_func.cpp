// stub

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <numeric>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>

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


auto twice = code::func(
    "twice",
    code::returns<pfm::int_>()|
    code::param<pfm::int_>("num"),
    "return num * 2;"
);

auto fill_index = code::func(
    "fill_index",
    code::returns<pfm::void_>()|
    code::param<code::global<pfm::int_>*>("pInt"),
    "int id = get_global_id(0);\n"
    "pInt[id] = twice(id);"
);



auto prog = code::program (
    code::common_func(twice),
    code::kernel(fill_index)
);

static constexpr int item_count = 1000;


inline constexpr int 
arith(int i_diff, int i_num)
{
    return i_diff * i_num * (i_num - 1) / 2;
}



bool
check(misc::runner const & i_runner, host::generic_program i_program)
{
    chrono::steady_clock::time_point tp = chrono::steady_clock::now();  

    host::buffer<pfm::int_> bufWrite(i_runner.m_context, item_count);
    typedef host::buffer<pfm::int_>::const_iterator iterator;

    // kernel内で使用できる事の確認
    i_runner.m_queue(
        run_kernel(
            i_program, 
            fill_index(bufWrite), 
            item_count));


    auto future = 
        i_runner.m_queue(
            bufWrite.with_range(
                [](iterator i_begin, iterator i_end){
                    return std::accumulate(i_begin, i_end, 0);
                }));
    std::future_status result = 
        future.wait_until(tp + chrono::seconds(5));

    assert(result == std::future_status::ready);
    assert(future.get() == arith(2, item_count));

    // ホストから呼べない事の確認
    try {
        int const a = 1; //gcc-4.7.2 twice(1)と書くと内部エラー
        i_runner.m_queue(
            host::run_kernel(
                i_program,
                twice(a),
                1)
        );
        assert(false); 
    }
    catch (cl::Error err) {
        assert(err.err() == CL_INVALID_KERNEL_NAME);
    }

    return true;
}



int main(int i_argc, char** i_argv) try 
{
    misc::runner runner(i_argc, i_argv);

    runner.with_program<decltype(prog)>(prog, check);
        
    return EXIT_SUCCESS;
}
catch (cl::Error err) {
    BOOST_LOG_TRIVIAL(fatal) << err.what() << "(" << err.err() << ")" ;
    return -1;
}




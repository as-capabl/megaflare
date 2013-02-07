#define __CL_ENABLE_EXCEPTIONS
#include <megaflare/host/buffer.hpp>

#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <numeric>
//#include <unistd.h>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>

#include <megaflare/code.hpp>
#include <megaflare/platform.hpp>
#include <megaflare/host.hpp>
#include <megaflare/misc/runner.hpp>
#include "pair_range.hpp"

namespace pfm = megaflare::platform;
namespace chrono = std::chrono;
namespace code = megaflare::code;
namespace host = megaflare::host;
namespace stpl = sprout::tuples;
namespace mtpl = megaflare::tuples;
namespace misc = megaflare::misc;

auto fill_index = code::func(
    "fill_index",
    code::returns<pfm::void_>()|
    code::param<code::global<pfm::int_>*>("pInt"),
    "int id = get_global_id(0);\n"
    "pInt[id] = id;"
);

auto twice = code::func(
    "twice",
    code::returns<pfm::void_>()|
    code::param<code::global<pfm::int_>*>("pInt"),
    "int id = get_global_id(0);\n"
    "pInt[id] = pInt[id] * 2;"
);


auto prog = code::program (
    code::kernel(fill_index),
    code::kernel(twice)
);

static constexpr int item_count = 1000;

inline constexpr int 
arith(int i_diff, int i_num)
{
    return i_diff * i_num * (i_num - 1) / 2;
}

void
run(misc::runner const & i_runner, host::program<decltype(prog)> i_program)
{
    chrono::steady_clock::time_point tp = chrono::steady_clock::now();  


    host::buffer<pfm::int_> bufWrite(i_runner.m_context, item_count);
    typedef host::buffer<pfm::int_>::const_iterator iterator;


    i_runner.m_queue(run_kernel(i_program, fill_index(bufWrite), item_count));
    i_runner.m_queue(run_kernel(i_program, twice(bufWrite), item_count));


    // std::function渡し
    std::function<cl_int(cl_int*, cl_int*)> funcSum =
        [](cl_int* outMapPtr, cl_int* end) -> cl_int 
        {
            cl_int sum = 0;
            for( cl_int i : std::make_pair(outMapPtr, end) ) {
                sum += i;
            }
            return sum;
        };
    auto future = i_runner.m_queue(bufWrite.with_range(funcSum));

    std::future_status result = future.wait_until(tp + chrono::seconds(3));
    assert(result == std::future_status::ready);
    assert(future.get() == arith(2, item_count));

    // const
    host::buffer<pfm::int_> const & bufConst = bufWrite;
    // ラムダ式渡し
    auto future2 = 
        i_runner.m_queue(bufConst.with_range(
                  [](iterator i_begin, iterator i_end){
                      return std::accumulate(i_begin, i_end, 0);
                  }));
    std::future_status result2 = future2.wait_until(tp + chrono::seconds(5));
    assert(result2 == std::future_status::ready);
    assert(future2.get() == arith(2, item_count));    

    // void戻り
    int sum = 0;
    auto future3 =
        i_runner.m_queue(
            bufConst.with_range(
                [&sum](iterator i_begin, iterator i_end) {
                    for( cl_int i : std::make_pair(i_begin, i_end)) {
                        sum += i;
                    }
                }));
    std::future_status result3 = future3.wait_until(tp + chrono::seconds(3));
    assert(result3 == std::future_status::ready);
    assert(sum == arith(2, item_count));
}


int main(int i_argc, char** i_argv) try 
{
    misc::runner runner(i_argc, i_argv);
    runner.with_program<decltype(prog)>(prog, run);
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



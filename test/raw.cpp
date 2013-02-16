// stub

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <numeric>

#include <megaflare/code.hpp>
#include <megaflare/platform.hpp>
#include <megaflare/host.hpp>
#include <megaflare/misc/runner.hpp>
#include "./pair_range.hpp"

namespace pfm = megaflare::platform;
namespace chrono = std::chrono;
namespace code = megaflare::code;
namespace host = megaflare::host;
namespace stpl = sprout::tuples;
namespace mtpl = megaflare::tuples;
namespace misc = megaflare::misc;


auto coeff_def = sprout::to_string(
    "#define COEFF 2\n"
);

auto coeff_undef = sprout::to_string(
    "#undef COEFF \n"
);


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
    "pInt[id] = pInt[id] * COEFF;"
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


    i_runner.m_queue(
        run_kernel(i_program, 
                   fill_index(bufWrite), 
                   item_count));
    i_runner.m_queue(
        run_kernel(i_program, 
                   twice(bufWrite), 
                   item_count));


    auto future = 
        i_runner.m_queue(
            bufWrite.with_range(
                [](iterator i_begin, iterator i_end){
                    return std::accumulate(i_begin, i_end, 0);
                }));
    std::future_status result = future.wait_until(tp + chrono::seconds(5));
    assert(result == std::future_status::ready);
    assert(future.get() == arith(2, item_count));    

    return true;
}


int main(int i_argc, char** i_argv) try 
{
    misc::runner runner(i_argc, i_argv);

    // 先頭
    auto prog1 = code::program (
        code::raw(coeff_def),
        code::kernel(twice),
        code::kernel(fill_index)
    );
    runner.with_program<decltype(prog1)>(prog1, check);

    // 末尾
    auto prog2 = code::program (
        code::kernel(fill_index),
        code::raw(coeff_def),
        code::kernel(twice),
        code::raw(coeff_undef)
    );
    runner.with_program<decltype(prog2)>(prog2, check);

    // 複数
    auto prog3 = code::program (
        code::kernel(fill_index),
        code::raw(coeff_def),
        code::raw(coeff_undef),
        code::raw(coeff_def),
        code::kernel(twice)
    );
    runner.with_program<decltype(prog3)>(prog3, check);

    // rawだけ
    auto prog4 = code::program (
        code::raw(
            sprout::to_string(
                "__kernel void twice(int __global* pInt)\n"
                "{\n"
                "int id = get_global_id(0);\n"
                "pInt[id] = pInt[id] * 2;\n"
                "}\n"
                "__kernel void fill_index(int __global* pInt)\n"
                "{\n"
                "int id = get_global_id(0);\n"
                "pInt[id] = id;\n"
                "}\n"
            )
        )
    );
    runner.with_program<decltype(prog4)>(prog4, check);

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


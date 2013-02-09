#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <numeric>
#include <chrono>
#include <iostream>
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
namespace stpl = sprout::tuples;
namespace mtpl = megaflare::tuples;
namespace misc = megaflare::misc;

using namespace pfm;

auto fill_zero = code::func(
    "fill_zero",
    code::returns<pfm::void_>()|
    code::param<code::global<int_>*>("a"),
    "int id = get_global_id(0);\n"
    "a[id] = 0;"
);


auto step_dp = code::func(
    "step_dp",
    code::returns<pfm::void_>()|
    code::param<code::global<int_>*>("aCurrent")|
    code::param<code::global<int_>*>("aNext")|
    code::param<int_>("iCap")|
    code::param<int_>("iPrice"),
    "int id = get_global_id(0);\n"
    "if (iCap <= id) {"
    "  int v0 = aCurrent[id];"
    "  int v1 = aCurrent[id - iCap] + iPrice;"
    "  aNext[id] = max(v0, v1);"
    "}"
);


auto prog = code::program (
    code::kernel(step_dp),
    code::kernel(fill_zero)    
);

constexpr int item_count = 10;
constexpr int price_max = 10000;
constexpr int total_cap = 200;

struct item_type {
    int cap;
    int price;
};

int
knapsack(misc::runner const & i_runner, host::program<decltype(prog)> i_program, std::vector<item_type> const & i_aItem, int i_nTotalCap)
{
    host::context const& context = i_runner.m_context;
    host::queue & queue = i_runner.m_queue;

    host::buffer<int_> buffers[2] = {
        {context, i_nTotalCap + 1},
        {context, i_nTotalCap + 1}
    };
    for(int i = 0; i < 2; ++i) {
        queue(
            run_kernel(
                i_program,
                fill_zero(buffers[i]),
                i_nTotalCap + 1
            )
        );
    }

    
    for(int i = 0; i < (int)i_aItem.size(); ++i) {
        queue(
            run_kernel(i_program, 
                       step_dp(buffers[i % 2], 
                               buffers[(i+1) % 2], 
                               i_aItem[i].cap,
                               i_aItem[i].price),
                       i_nTotalCap + 1
            )
        );
    }
    auto ret = queue(
        buffers[i_aItem.size() % 2].with_range(
            [](cl_int* begin, cl_int* end) {
                return *(end - 1);
            }
        )
    );
    return ret.get();
}

void
run(misc::runner const & i_runner, host::program<decltype(prog)> i_program)
{
    // small test
    std::vector<item_type> aItemSmall = {
        {10, 1},
        {20, 3}
    };
    int retSmall = knapsack(i_runner, i_program, aItemSmall, 30);
    assert(retSmall == 4);

    // random
    std::vector<item_type> aItem(item_count);
    std::srand(std::chrono::steady_clock::now().time_since_epoch().count());
    for (int i = 0; i < item_count; ++i) {
        aItem[i] = item_type{std::rand() % total_cap,
                             std::rand() % price_max};
    }
    int retRandom = knapsack(i_runner, i_program, aItem, total_cap);
    std::cout << retRandom << std::endl;
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



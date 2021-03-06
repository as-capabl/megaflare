#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <numeric>

#include <megaflare/code.hpp>
#include <megaflare/platform.hpp>
#include <megaflare/host.hpp>
#include "pair_range.hpp"

namespace pfm = megaflare::platform;
namespace chrono = std::chrono;
namespace code = megaflare::code;
namespace host = megaflare::host;
namespace stpl = sprout::tuples;
namespace mtpl = megaflare::tuples;

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


int main() try 
{
    std::vector<host::platform> platforms;
    host::platform::get(&platforms);

    if (platforms.size() == 0) {
        return -1;
    }
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
    host::context context(CL_DEVICE_TYPE_GPU, properties);

    std::vector<host::device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

    auto program = host::make_program(context, prog);

    try {
        program.build(devices);
    }
    catch(cl::Error err) {
        if(err.err() == CL_BUILD_PROGRAM_FAILURE)
        {
            return -1;
        }
        else {
            throw err;
        }
    }
    host::queue queue(context(), devices[0]());
        
    host::buffer<pfm::int_> bufWrite(context, item_count);


    queue(run_kernel(program, fill_index(bufWrite), item_count));
    queue(run_kernel(program, twice(bufWrite), item_count));


    // std::function渡し
    std::function<cl_int(cl_int*, cl_int*)> funcSum =
        [](cl_int* outMapPtr, cl_int* end) -> cl_int 
        {
            cl_int sum = 0;
            auto rangeOut = std::make_pair(outMapPtr, end);
            
            for( cl_int i : rangeOut ) {
                sum += i;
            }
            return sum;
        };

    auto future = queue(bufWrite.with_range(funcSum));

    chrono::steady_clock::time_point tp = chrono::steady_clock::now();  
    std::future_status result = future.wait_until(tp + chrono::seconds(3));

    assert(result == std::future_status::ready);
    assert(future.get() == item_count * (item_count - 1) / 2 * 2);

    // const
    host::buffer<pfm::int_> const & bufConst = bufWrite;
    // ラムダ式渡し
    typedef host::buffer<pfm::int_>::const_iterator iterator;
    auto future2 = 
        queue(bufConst.with_range(
                  [](iterator i_begin, iterator i_end){
                      return std::accumulate(i_begin, i_end, 0);
                  }));
    chrono::steady_clock::time_point tp2 = chrono::steady_clock::now();
    std::future_status result2 = future2.wait_until(tp2 + chrono::seconds(5));

    assert(result2 == std::future_status::ready);
    assert(future2.get() == item_count * (item_count - 1) / 2 * 2);

    return EXIT_SUCCESS;
}
catch (cl::Error err) {
    return -1;
}




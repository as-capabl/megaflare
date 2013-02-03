#include <megaflare/code.hpp>
#include <megaflare/platform.hpp>
#include <iostream>
#include <cassert>

namespace pfm = megaflare::platform;
namespace code = megaflare::code;

int main() try
{
#if 0
    auto x = func(
        code::sgnt<int (int, int)>(), 
        "return _1 + _2;");
#endif
#if 0
    auto y = func(
        code::returns<pfm::void_>() | 
        code::param<0, code::global<pfm::int_>*>("a") | 
        code::param<1, pfm::float_>("b"), 
        "return a + b;");
#endif

    auto z = func(
        "x_kern",
        code::returns<pfm::void_>(),
        "");

    auto prog = code::program(
#if 0
        code::kernel(x, "x_kern"), 
#endif
        code::kernel(z),
        code::kernel(z)//,
//        code::kernel(z, "z_kern")
    );


    std::cout << code::get_cl_string(prog) << std::endl;
#if 0
    auto compiled_prog = prog.compile();
    auto compiled_kern = compiled_prog.get_kernel<decltype(x)>("x_kern");
#endif    


    return 0;
}
catch (...) {
    assert(0);
    return 1;
}


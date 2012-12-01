#include <megaflare/code/detail/building_kernel.hpp>
#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <cassert>
#include <iostream>

namespace stpl = sprout::tuples;
namespace bk = megaflare::code::detail::bk;

int main()
{

    auto int_void = 
        bk::modify<bk::elems::body_str>(
            bk::modify<bk::elems::result_str>(
                bk::construct(bk::symbol(), sprout::to_string("foo")), 
                sprout::to_string("int")),
            sprout::to_string("return 0;"));

    assert(bk::to_string(int_void) == "int foo()\n{\nreturn 0;\n}\n");

    auto long_intint_part = 
            bk::modify<bk::elems::arg_tuple>(
                bk::modify<bk::elems::result_str>(
                    bk::construct(bk::symbol(), sprout::to_string("bar")), 
                    sprout::to_string("long")),
                stpl::make_tuple(
                    sprout::to_string("int a"),
                    sprout::to_string("int b")));
    auto long_intint = 
        bk::modify<bk::elems::body_str>(
            bk::modify<bk::elems::result_str>(
                long_intint_part, 
                sprout::to_string("long")),
            sprout::to_string("return 0;"));
        

    assert(bk::to_string(long_intint) == "long bar(int a, int b)\n{\nreturn 0;\n}\n");


    return 0;
}

#include <megaflare/text.hpp>
#include <megaflare/tuples.hpp>
#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <cassert>
#include <iostream>

namespace lit =  megaflare::text;
namespace mtpl = megaflare::tuples;
namespace stpl = sprout::tuples;

int main()
{
    // unwords
    constexpr auto foo = sprout::to_string("foo");
    constexpr auto bar = sprout::to_string("bar");

    auto str_tuple1 = sprout::make_tuple(foo);
    auto str_tuple2 = sprout::make_tuple(foo, bar);
    
    assert(lit::unwords(str_tuple1) == sprout::to_string("foo"));
    assert(lit::unwords(str_tuple2) == sprout::to_string("foo bar"));

    //unlines
    assert(lit::unlines(str_tuple1) == sprout::to_string("foo\n"));
    assert(lit::unlines(str_tuple2) == sprout::to_string("foo\nbar\n"));    

    //comma_list
    //unlines
    assert(lit::comma_list(str_tuple1) == sprout::to_string("foo"));
    assert(lit::comma_list(str_tuple2) == sprout::to_string("foo, bar"));

    //drop_last_type_matched
    assert(lit::drop_last_type_matched<int>(str_tuple2) == str_tuple2);

    assert(lit::drop_last_type_matched<decltype(bar)>(str_tuple2) == str_tuple1);

    auto ret2 = sprout::tuples::push_back(
        str_tuple2, 
        sprout::make_tuple(std::false_type(), 
                           sprout::tuple<>() ) );
    assert(sprout::tuples::get<0>(ret2) == foo);
}

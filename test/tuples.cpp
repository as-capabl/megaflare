#include <megaflare/text.hpp>
#include <megaflare/tuples.hpp>
#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <cassert>
#include <iostream>

namespace lit =  megaflare::text;
namespace mtpl = megaflare::tuples;
namespace stpl = sprout::tuples;

struct test_un {
    template <typename T1>
    constexpr auto  operator () (T1 t1) -> decltype(t1 + 1)
        { return t1 + 1; }
};

struct test_bi {
    template <typename T1, typename T2>
    constexpr auto  operator () (T1 t1, T2 t2) -> decltype(t1 + t2)
        { return t1 + t2; }
};

struct test_mapaccum {
    template <typename T>
    constexpr auto  operator () (int i_st, T i_in) -> stpl::tuple<int, T>
    { return stpl::make_tuple(i_st + 1, i_in * i_st); }
};

int main()
{
    //fold
    static_assert(std::is_same<mtpl::detail::foldr1_type<test_bi, 3, 3, sprout::tuple<int, double, int, float>>::type, float>::value, "fail!");

    auto tuple4 = sprout::make_tuple(1, 4.0f, 1, 4);
    assert(mtpl::foldr1(test_bi(), tuple4) == 10.0);
    assert(mtpl::foldl(test_bi(), 0, tuple4) == 10.0);
    
    auto tuple1 = sprout::make_tuple(1);
    assert(mtpl::foldr1(test_bi(), tuple1) == 1);
    assert(mtpl::foldl(test_bi(), 1, tuple1) == 2);
    //map_accum
    auto map_accum_result4 = stpl::get<1>(mtpl::map_accum_l(test_mapaccum(), tuple4, 1));
    assert(stpl::get<0>(map_accum_result4) == 1);
    assert(stpl::get<1>(map_accum_result4) == 8.0f);
    assert(stpl::get<2>(map_accum_result4) == 3);
    assert(stpl::get<3>(map_accum_result4) == 16);

    auto map_accum_result1 = stpl::get<1>(mtpl::map_accum_l(test_mapaccum(), tuple1, 1));
    assert(stpl::get<0>(map_accum_result1) == 1);

    //transform
    assert(sprout::get<0>(mtpl::transform(test_un(), tuple4)) == 2);
    assert(sprout::get<2>(mtpl::transform(test_un(), tuple4)) == 2);
    assert(sprout::get<3>(mtpl::transform(test_un(), tuple4)) == 5);
    assert(sprout::get<0>(mtpl::transform(test_un(), tuple1)) == 2);

    //mtpl::modify
    auto modtuple4 = mtpl::modify<2>(tuple4, sprout::to_string("hoge"));
    assert(sprout::get<0>(modtuple4) == 1);
    assert(sprout::get<1>(modtuple4) == 4.0f);
    assert(sprout::get<2>(modtuple4) == "hoge");
    assert(sprout::get<3>(modtuple4) == 4);

    auto modtuple1 = mtpl::modify<0>(tuple1, sprout::to_string("fuga"));
    assert(sprout::get<0>(modtuple1) == "fuga");

    //pad
    auto padtuple4 = mtpl::pad<10>(tuple4, 99);
    static_assert(std::tuple_size<decltype(padtuple4)>::value == 10, "size of pad tuple");
    assert(sprout::tuples::get<3>(padtuple4) != 99);
    assert(sprout::tuples::get<4>(padtuple4) == 99);
    assert(sprout::tuples::get<9>(padtuple4) == 99);
}

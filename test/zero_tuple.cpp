#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include <cassert>
int main()
{
    assert(sprout::tuples::push_back(sprout::tuple<>(), 1)
           == sprout::make_tuple(1));
    return 0;
}

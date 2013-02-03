#pragma once 

#include "./func.hpp"

namespace megaflare {
    namespace code {
        template <typename Func>
        constexpr Func
        common_func(Func const & i_func) {
            return i_func;
        }
    }
}

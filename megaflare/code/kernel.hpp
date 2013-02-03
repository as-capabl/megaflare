#pragma once

#include <sprout/string.hpp>
#include "detail/building_kernel.hpp"
#include "megaflare/text.hpp"
#include "megaflare/tuples.hpp"
#include "./param.hpp"
#include "./mods.hpp"

namespace megaflare {
    namespace code {
        // kernel


        template <typename Func>
        constexpr auto
        kernel(Func const & i_func)
            -> decltype(i_func.modify(kernel()))
        {
            return i_func.modify(kernel());
        }
    }
}

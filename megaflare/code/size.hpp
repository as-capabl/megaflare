#pragma once

#include <type_traits>

namespace megaflare {
    namespace code {
        template <typename TypeSpec>
        struct size 
            : std::integral_constant<std::size_t, 
                                     sizeof(typename TypeSpec::host_type)> 
        {
        };

    }
}

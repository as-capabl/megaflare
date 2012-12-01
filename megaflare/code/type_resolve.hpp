#pragma once

#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include "detail/building_kernel.hpp"
#include "megaflare/text.hpp"
#include "megaflare/tuples.hpp"

namespace megaflare {
    namespace code {
        // 型の補完
        template <typename T> 
        struct type_resolve : public T {};

        namespace detail {
            static constexpr auto ss_aster = sprout::to_string("*");
            static constexpr auto ss_global = sprout::to_string(" __global");
        }

        template <typename T>
        struct type_resolve<T*> {
        private:
            typedef type_resolve<T> parent;
        public:
            static constexpr decltype(parent::str() + detail::ss_aster)
                str() 
            {
                return parent::str() + detail::ss_aster;
            }
            typedef typename parent::host_type* host_type;
        };

        template <typename T>
        struct global {
        private:
            typedef type_resolve<T> parent;
        public:
            static constexpr decltype(parent::str() + detail::ss_global)
                str() 
            {
                return parent::str() + detail::ss_global;
            }
            typedef typename parent::host_type host_type;
        };
    }
}

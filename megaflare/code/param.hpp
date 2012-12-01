#pragma once

#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include "megaflare/text.hpp"
#include "megaflare/tuples.hpp"
#include "./type_resolve.hpp"

namespace megaflare {
    namespace code {
        template <typename T, int N>
        struct param_t {
            sprout::string<N> m_param_str;
            typedef T type_spec;
            typedef typename type_resolve<T>::host_type host_type;

            constexpr auto
            decl_str()
                -> decltype(type_resolve<T>::str() + text::ss_space + m_param_str)
            {
                return type_resolve<T>::str() + text::ss_space + m_param_str;
            }

        };

        template <int N, typename TpParams>
        struct calling_pack {
            sprout::string<N> m_name;
            TpParams m_paramsAlt;
        };
    }
}

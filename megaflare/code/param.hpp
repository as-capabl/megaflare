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

#if 0
        // 即値
        template <typename TypeSpec>
        struct immediate {
            typedef typename TypeSpec::host_type host_type;
            host_type value;
        };
#endif

        template <typename Type, typename TypeSpec, int N>
        auto
        subst_to_param(Type& i_type, param_t<TypeSpec, N> i_param)
            -> decltype(i_type.subst_to_param(i_param))
        {
            return i_type.subst_to_param(i_param);
        }

        template <typename TypeSpec, int N>
        typename TypeSpec::host_type const & 
        subst_to_param(typename TypeSpec::host_type const & i_type, 
                       param_t<TypeSpec, N>)
        {
            return i_type;
        }

    }
}

#pragma once 

#include "detail/building_kernel.hpp"

namespace megaflare {
    namespace code {
        template <typename Str>
        struct raw_t {
            Str m_str;
            constexpr tuples::tuple<detail::bk::symbol, Str>
            get_cl_string(detail::bk::symbol i_sym) {
                return tuples::make_tuple(i_sym, m_str);
            }
        };

        template <typename Str>
        constexpr raw_t<Str>
        raw(Str const & i_str) {
            return {i_str};
        }
    }
}

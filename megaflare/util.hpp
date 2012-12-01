#pragma once
namespace megaflare {
    namespace utility {
        struct add {
            template <typename T, typename U>
            constexpr auto
            operator() (T const& i_t, U const& i_u)
                -> decltype(i_t + i_u)
            {
                return i_t + i_u;
            }
        };

        struct id {
            template <typename T>
            constexpr T operator() (T i_t) { return i_t; }
        };
    }
}

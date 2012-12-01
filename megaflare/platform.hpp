#pragma once

#include <CL/cl_platform.h>
#include <sprout/string.hpp>
#include <type_traits>

namespace megaflare {    
    namespace platform {        
        constexpr static auto void_str = sprout::to_string("void");
        constexpr static auto bool_str = sprout::to_string("bool");
        constexpr static auto char_str = sprout::to_string("char");
        constexpr static auto uchar_str = sprout::to_string("uchar");
        constexpr static auto short_str = sprout::to_string("short");
        constexpr static auto ushort_str = sprout::to_string("ushort");
        constexpr static auto int_str = sprout::to_string("int");
        constexpr static auto uint_str = sprout::to_string("uint");
        constexpr static auto long_str = sprout::to_string("long");
        constexpr static auto ulong_str = sprout::to_string("ulong");
        constexpr static auto float_str = sprout::to_string("float");
        constexpr static auto half_str = sprout::to_string("half");


        struct void_ {
            static constexpr decltype(void_str) str() { return void_str; }
        };
        struct bool_ {
            static constexpr decltype(bool_str) str() { return bool_str; }
        };
        struct char_ {
            static constexpr decltype(char_str) str() { return char_str; }
            typedef cl_char host_type;
        };
        struct uchar_ {
            static constexpr decltype(uchar_str) str() { return uchar_str; }
            typedef cl_uchar host_type;
        };
        struct short_ {
            static constexpr decltype(short_str) str() { return short_str; }
            typedef cl_short host_type;
        };
        struct ushort_ {
            static constexpr decltype(ushort_str) str() { return ushort_str; }
            typedef cl_ushort host_type;
        };
        struct int_ {
            static constexpr decltype(int_str) str() { return int_str; }
            typedef cl_int host_type;
        };
        struct uint_ {
            static constexpr decltype(uint_str) str() { return uint_str; }
            typedef cl_uint host_type;
        };
        struct long_ {
            static constexpr decltype(long_str) str() { return long_str; }
            typedef cl_long host_type;
        };
        struct ulong_ {
            static constexpr decltype(ulong_str) str() { return ulong_str; }
            typedef cl_ulong host_type;
        };
        struct float_ {
            static constexpr decltype(float_str) str() { return float_str; }
            typedef cl_float host_type;
        };
        struct half_ {
            static constexpr decltype(half_str) str() { return half_str; }
            typedef cl_half host_type;
        };

    }
}

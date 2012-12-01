#pragma once

namespace megaflare {
    namespace host {
        namespace policies {
            struct buffer_default {};

            template <typename Policy>
            constexpr inline cl_mem_flags
            mem_flag() {
                return CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR;
            }

            template <typename Policy>
            constexpr inline cl_map_flags
            map_flag() {
                return CL_MAP_READ | CL_MAP_WRITE;
            }

            template <typename Policy>
            constexpr inline cl_map_flags
            const_map_flag() {
                return CL_MAP_READ;
            }

        }
    }
}

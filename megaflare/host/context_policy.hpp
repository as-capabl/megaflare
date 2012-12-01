#pragma once

namespace megaflare {
    namespace host {
        namespace policies {
            struct context_default {};

            template <typename Policy>
            cl_mem_flags
            mem_flag() {
                return CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR;
            }

            template <typename Policy>
            cl_map_flags
            map_flag() {
                return CL_MAP_READ | CL_MAP_WRITE;
            }

            template <typename Policy>
            cl_map_flags
            const_map_flag() {
                return CL_MAP_READ;
            }

        }
    }
}

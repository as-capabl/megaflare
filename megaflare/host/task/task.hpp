#pragma once

namespace megaflare {
    namespace host {
		//CRTPにより継承
		//……にしたいのだが、上手く型を合わせられなかったので現状ただの飾り
        template <typename Derived>
        struct task {
            //void operator() (cl_queue i_pQueue);
        };
    }
}

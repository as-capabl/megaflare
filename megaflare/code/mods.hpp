#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include "detail/building_kernel.hpp"
#include "megaflare/text.hpp"
#include "megaflare/tuples.hpp"
#include "./type_resolve.hpp"
#include "./param.hpp"





namespace megaflare {
    namespace code {
        // |演算子による結合をするクラス
		// CRTPにより継承
        template <typename Derived>
        struct modifier_t {
            template <typename RhsT, 
                      typename Enabler = typename std::enable_if<
                          std::is_base_of<modifier_t<RhsT>, RhsT>::value>::type >
            constexpr sprout::tuple<Derived, RhsT>
            operator| (RhsT const& rhs) 
            {
                return sprout::tuple<Derived, RhsT>(*static_cast<const Derived*>(this), rhs);
            }

            // パラメータタプルの作成
            // デフォルトでは値を返さない
            template <typename T>
            constexpr T get_params(T const& i_tp) { return i_tp; }
        };

        template <typename Rhs, typename... Lhs>
        inline constexpr 
        tuples::tuple<Lhs..., Rhs>
        operator| (tuples::tuple<Lhs...> const& i_tp, modifier_t<Rhs> const& i_rhs)
        {
            return tuples::push_back(i_tp, static_cast<Rhs const&>(i_rhs));
        }

        template <typename... Mods>
        inline constexpr tuples::tuple<Mods...>
        modifier_tuple(tuples::tuple<Mods...> const& i_tp)
        {
            return i_tp;
        }

        template <typename Derived>
        inline constexpr tuples::tuple<Derived>
        modifier_tuple(Derived const& i_mod)
        {
            static_assert(
                std::is_base_of<modifier_t<Derived>, Derived>::value,
                "need modifier");
            return tuples::make_tuple(i_mod);
        }


        template <typename T>
        struct result_mod : modifier_t<result_mod<T>>{
            template <typename BK>
                constexpr auto get_cl_string(BK const& i_bk) 
                -> decltype(detail::bk::modify<detail::bk::elems::result_str>(
                                i_bk, 
                                type_resolve<T>::str()))
            {
                return detail::bk::modify<detail::bk::elems::result_str>(
                    i_bk, 
                    type_resolve<T>::str());
            }
        };


        template <typename T>
        constexpr result_mod<T>
        returns()
        {
            return result_mod<T>();
        }


        template <typename T, int N>
        struct param_mod : modifier_t<param_mod<T, N>>{
            param_t<T, N> m_content;

            constexpr param_mod(sprout::string<N> s) : m_content{s} {}


#if 0
            template <typename BK, typename PADDED_ARG> 
                static constexpr auto
                get_cl_string_(BK i_bk, PADDED_ARG i_arg) 
                -> decltype(detail::bk::modify<detail::bk::elems::arg_tuple>(i_bk, type_resolve<T>::str()))
            {
                static_assert(
                    !std::is_same<typename tuples::tuple_element<N, PADDED_ARG>::type, detail::bk::blank_param>::value,
                    "parameter duplication."
                );
                return detail::bk::modify<detail::bk::elems::arg_tuple>(i_bk, type_resolve<T>::str());
            }
#endif
        
        
            template <typename BK>
                constexpr auto get_cl_string(BK const& i_bk)
                -> decltype(detail::bk::modify<detail::bk::elems::arg_tuple>(
                                i_bk, 
                                tuples::push_back(
                                    detail::bk::get<detail::bk::elems::arg_tuple>(i_bk),
                                    m_content.decl_str()
                                )))
            {
                return detail::bk::modify<detail::bk::elems::arg_tuple>(
                    i_bk, 
                    tuples::push_back(
                        detail::bk::get<detail::bk::elems::arg_tuple>(i_bk),
                        m_content.decl_str()
                    ));
            } 

            template <typename Tuple>
                constexpr auto get_params(Tuple const & i_tp)
                -> decltype(tuples::push_back(i_tp, m_content))
            {
                return tuples::push_back(i_tp, m_content);
            }

        };


        template <typename T, int N>
        constexpr param_mod<T, N> 
        param(char const (&arr)[N])
        {
            return {sprout::to_string(arr)};
        }

    }
}

#pragma once

#include <type_traits>
#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include "util.hpp"



namespace megaflare {
    namespace tuples {
        using sprout::tuples::tuple;
        using sprout::tuples::make_tuple;
        using sprout::tuples::tie;
        using sprout::tuples::get;
        using sprout::tuples::push_back;
        using sprout::tuples::push_front;
        using sprout::tuples::tuple_size;
        using sprout::tuples::tuple_element;

        namespace types {
            using sprout::types::push_back;
            using sprout::types::push_front;
        }

        namespace result_of {
            using sprout::tuples::result_of::push_back;
            using sprout::tuples::result_of::push_front;
        }

// foldr1 -------------------------------------
        template <typename Signature>
        struct type_error_on_fold {};

        namespace detail {
            struct apply_impl {
                template <typename Err, typename Bi, typename Lhs, typename Rhs>
                static constexpr 
                auto
                impl2(Err, Bi bi, Lhs lhs, Rhs rhs)
                    -> decltype(bi(lhs, rhs))
                {
                    return bi(lhs, rhs);
                }

                // 型エラー発生
                template <typename Err>
                static constexpr
                Err
                impl2(Err, ...) {
                    return {};
                }

                // 既に型エラー(fordr)
                template <typename Err, typename Bi, typename Lhs, typename Signature>
                static constexpr 
                auto
                impl2(Err, Bi bi, Lhs lhs, type_error_on_fold<Signature> rhs)
                    -> decltype(rhs)
                {
                    return rhs;
                }

                // 既に型エラー(foldl)
                template <typename Err, typename Bi, typename Signature, typename Rhs>
                static constexpr 
                auto
                impl2(Err, Bi bi, type_error_on_fold<Signature> lhs, Rhs rhs)
                    -> decltype(lhs)
                {
                    return lhs;
                }


                template <typename Bi, typename Lhs, typename Rhs, typename Err = type_error_on_fold<Bi(Lhs, Rhs)> >
                static constexpr
                auto
                impl(Bi bi, Lhs lhs, Rhs rhs)
                    -> decltype(impl2(Err(), bi, lhs, rhs))
                {
                    return impl2(Err(), bi, lhs, rhs);
                }
            };


            template <typename Bi, std::size_t N, std::size_t IdxMax, typename Tuple>
            struct
            foldr1_type
            {
                typedef typename std::tuple_element<N, Tuple>::type t1_t;
                typedef typename std::enable_if<(std::tuple_size<Tuple>::value > N)
                    , foldr1_type<Bi, N+1, IdxMax, Tuple> >::type t2_t;
                
                
                
                static constexpr 
                auto
                impl(Bi bi, const Tuple& tp)
                    -> decltype(
                        apply_impl::impl(bi, sprout::get<N>(tp), 
                            t2_t::impl(bi, tp)))
                {
                    return 
                        apply_impl::impl(bi, sprout::get<N>(tp), t2_t::impl(bi, tp));
                }
                
                typedef decltype(
                    impl(std::declval<Bi>(),
                         std::declval<Tuple>()))
                type;
            };
            
            template <typename Bi, std::size_t IdxMax, typename Tuple>
            struct
            foldr1_type<Bi, IdxMax, IdxMax, Tuple>
            {
                static constexpr std::size_t N = IdxMax;
                typedef typename std::tuple_element<N, Tuple>::type type;
    
                static constexpr 
                type impl(Bi bi, const Tuple& tp){
                    return sprout::get<N>(tp);
                }
            };
        }

        template <typename Bi, typename Tuple,
                  typename Impl = detail::foldr1_type<Bi, 0, std::tuple_size<Tuple>::value - 1, Tuple> >
        inline constexpr typename Impl::type foldr1(Bi bi, Tuple tp)
        {
            return Impl::impl(bi, tp);
        }

// foldl
// myFoldl :: (a -> b -> a) -> a -> [b] -> a
// myFoldl f z xs = foldr step id xs z
//  where step x g a = g (f a x)
//     -- Real World Haskell p100 4.6.8 右からの畳み込み
        namespace detail {
            template <typename Bi, typename Tuple>
            struct foldl_type {
                Bi bi;
                foldl_type(Bi i_bi) : bi(i_bi) {}

                template <typename Later, typename Packed>
                struct step_in {
                    Bi bi;
                    Later later;
                    Packed packed;

                    template <typename Former>
                    constexpr auto
                    operator() (Former former)
                        -> decltype(packed(
                                        apply_impl::impl(bi,former, later)))
                    {
                        return packed(apply_impl::impl(bi,former, later));
                    }
                };


                //foldr1から呼ばれる
                template <typename Later, typename Packed>
                constexpr step_in<Later, Packed>
                operator() (Later later, Packed packed) 
                {
                    return {bi, later, packed};
                }
            };
        }

        template <typename Bi, typename Tuple, typename First
                  , typename Impl = detail::foldl_type<Bi, Tuple> >
        inline constexpr auto
        foldl(Bi bi, First first, Tuple tp)
            -> decltype(foldr1(Impl(bi), push_back(tp, utility::id())) (first))
        {
            return foldr1(Impl(bi), push_back(tp, utility::id())) (first);
        }

// map_accum
        namespace detail {
            template <typename Bi>
            struct map_accum_op {
                Bi m_bi;

                template <typename State, 
                          typename Value,
                          typename... Args>
                static constexpr auto 
                eval_bi(Bi const& i_bi,
                        tuple<State, tuple<Args...>> const& i_former,
                        Value const& i_value)
                    -> typename std::result_of<Bi(State, Value)>::type
                {
                    return i_bi(get<0>(i_former), i_value);
                }
        
                template <typename NextState,
                          typename Value,
                          typename NowTuple>
                static constexpr auto //tuple<NextState, NextTuple> 
                repack(NextState const& i_st, Value const& i_val, NowTuple const& i_tp)
                    -> tuple<
                        NextState,
                        typename types::push_back<NowTuple, Value>::type>
                {
                    return make_tuple(i_st, push_back(i_tp, i_val));
                }
        
                template <typename State, 
                          typename Value,
                          typename NowTuple
                          >
                constexpr auto 
                operator() (tuple<State, NowTuple> const& i_former,
                            Value const& i_value)
                    -> decltype(
                        repack(get<0>(eval_bi(m_bi, i_former, i_value)), 
                               get<1>(eval_bi(m_bi, i_former, i_value)), 
                               get<1>(i_former)))
                {
                    return repack(get<0>(eval_bi(m_bi, i_former, i_value)), 
                                  get<1>(eval_bi(m_bi, i_former, i_value)), 
                                  get<1>(i_former));
                }
            };
        }

        template <typename Bi, typename Tuple, typename State>
        inline constexpr auto map_accum_l(Bi const& i_bi, Tuple const& i_tp, State const& i_st)
            -> decltype(foldl(detail::map_accum_op<Bi>{i_bi}, make_tuple(i_st, tuple<>()), i_tp))
        {
            return foldl(detail::map_accum_op<Bi>{i_bi}, make_tuple(i_st, tuple<>()), i_tp);
        }

        
        // transform
        namespace detail {
            template <typename Un, int I, int IdxMax, typename Tuple>
            struct transform_impl {
                Tuple m_before;

                typedef typename std::tuple_element<I, Tuple>::type head_t;
                typedef typename Un::template nth_type<I, typename tuple_element<I, Tuple>::type>::type tr_head_t;

                typedef transform_impl<Un, I+1, IdxMax, Tuple> impl_tail_t;
      

            
                inline constexpr tr_head_t
                tr_head(Un i_un) { return i_un.template nth<I>(get<I>(m_before)); }

                constexpr transform_impl(Tuple const& i_tp)
                : m_before(i_tp){}


                inline constexpr auto impl(Un i_un)
                -> decltype(push_front(impl_tail_t(m_before).impl(i_un), this->tr_head(i_un))) {
                    return push_front(impl_tail_t(m_before).impl(i_un), tr_head(i_un));
                }
            };

            template <typename Un, int IdxMax, typename Tuple>
            struct transform_impl<Un, IdxMax, IdxMax, Tuple> {
                Tuple m_before;
            
                constexpr transform_impl(Tuple const& i_tp)
                : m_before(i_tp){}

                inline constexpr sprout::tuple<>
                impl(Un i_un) {
                    return sprout::tuple<>();
                }
            };
        
            template <typename Un>
            struct transform_op {
                Un m_un;
                constexpr transform_op(Un const& i_un) : m_un(i_un) {}

                template <int I, typename T> 
                struct nth_type {
                    typedef typename std::result_of<Un(T)>::type  type;
                };

                template <int I, typename T>
                constexpr typename std::result_of<Un(T)>::type
                nth(T i_t) { return m_un(i_t); }
            };
        }
    
        template <typename Un, typename Tuple>
        inline constexpr auto transform(Un i_un, Tuple const& i_tp) 
            -> decltype(detail::transform_impl<detail::transform_op<Un>, 0, std::tuple_size<Tuple>::value, Tuple>(i_tp)
                        .impl(detail::transform_op<Un>(i_un)))
        {
            return detail::transform_impl<detail::transform_op<Un>, 0, std::tuple_size<Tuple>::value, Tuple>(i_tp)
                .impl(detail::transform_op<Un>(i_un));
        }

        //参照版transformはindex_tupleで作ってみた
        namespace detail {
            template <typename Ret, typename Un, typename Tuple, sprout::index_t... Indices>
            inline constexpr Ret
            transform2_(Un i_un, Tuple & i_tp, sprout::index_tuple<Indices...>) 
            {
                return make_tuple(i_un(get<Indices>(i_tp))...);
            }

        }

        template <typename Un, typename... Args>
        inline constexpr auto 
        transform2(Un i_un, tuple<Args...> & i_tp) 
            -> tuple<typename std::result_of<Un(Args)>::type...>
        {
            return detail::transform2_<tuple<typename std::result_of<Un(Args)>::type...>> (i_un, i_tp, sprout::index_range<0, sizeof...(Args)>::make());
        }
        // pad
        namespace detail {
            template <std::size_t N, bool, typename T, typename... Args>
            struct pad_impl {
                typedef pad_impl<N, 
                                 (sizeof...(Args) + 1 < N), 
                    T, 
                    Args..., T> next;
                typedef typename next::type type;
            
                static constexpr type
                impl(sprout::tuple<Args...> const& i_tp, T const& i_t){
                    return next::impl(push_back(i_tp, i_t), i_t);
                }
            };

            template <std::size_t N, typename T, typename... Args>
            struct pad_impl<N, false, T, Args...> {
                typedef sprout::tuple<Args...> type;
            
                static constexpr type
                impl(sprout::tuple<Args...> const& i_tp, T const& i_t){
                    return i_tp;
                }
            };
        }
        template <std::size_t N, typename T, typename... Args>
        inline 
        constexpr
        typename detail::pad_impl<N, sizeof...(Args) < N, T, Args...>::type
            pad(sprout::tuple<Args...> const& i_tp, T const& i_t) 
        {
            return detail::pad_impl<N, sizeof...(Args) < N, T, Args...>::impl(i_tp, i_t);
        }

        // modify_tuple                
        namespace detail {
            template <int I, typename T>
            struct modify_tuple_op {
                T m_t;

                constexpr modify_tuple_op(T const& i_t)
                : m_t(i_t) {}

                template <int J, typename U>
                struct impl {
                    typedef U type;
                    static constexpr U choose (T const& i_t, U const& i_u) {
                        return i_u;
                    }
                };
                template <typename U>
                struct impl<I, U> {
                    typedef T type;
                    static constexpr T choose (T const& i_t, U const& i_u) {
                        return i_t;
                    }
                };

                template <int J, typename U>
                struct nth_type {
                    typedef typename impl<J, U>::type type;
                };

                template <int J, typename U>
                constexpr typename impl<J, U>::type
                nth(U i_u) {
                    return impl<J, U>::choose(m_t, i_u);
                }
            };
        }

        template <std::size_t I, typename T, typename Tuple>
        inline constexpr auto modify(Tuple const& i_tp, T const& i_t) 
            -> decltype(detail::transform_impl<detail::modify_tuple_op<I, T>, 0, std::tuple_size<Tuple>::value, Tuple>(i_tp)
                        .impl(detail::modify_tuple_op<I, T>(i_t)))
        {
            return detail::transform_impl<detail::modify_tuple_op<I, T>, 0, std::tuple_size<Tuple>::value, Tuple>(i_tp)
                .impl(detail::modify_tuple_op<I, T>(i_t));
        }

        // zip
        namespace detail {
            template <typename Tp1, 
                      typename Tp2 ,
                      sprout::index_t... Indexes>
                inline constexpr auto 
                zip_(Tp1 const & i_tp1, 
                     Tp2 const & i_tp2, 
                     sprout::index_tuple<Indexes...>)
                -> sprout::tuple<sprout::tuple<typename tuple_element<Indexes, Tp1>::type, typename tuple_element<Indexes, Tp2>::type> ...>
            {
                return 
                    sprout::tuple<sprout::tuple<typename tuple_element<Indexes, Tp1>::type, typename tuple_element<Indexes, Tp2>::type>...>(
                        sprout::tuple<typename tuple_element<Indexes, Tp1>::type, typename tuple_element<Indexes, Tp2>::type>(get<Indexes>(i_tp1),  get<Indexes>(i_tp2))...
                    );
            }
        }
        template <typename Tp1, 
                  typename Tp2, 
                  std::size_t N = sprout::min(
                      tuple_size<Tp1>::value,
                      tuple_size<Tp2>::value
                  )>
        inline constexpr auto zip(Tp1 const & i_tp1, Tp2 const & i_tp2)
            -> decltype(detail::zip_(i_tp1, i_tp2, sprout::index_range<0, N>::make()))
        {
            return detail::zip_(i_tp1, i_tp2, sprout::index_range<0, N>::make());
        }
    }
}

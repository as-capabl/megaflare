#pragma once

#include <type_traits>
#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include "util.hpp"
#include "tuples.hpp"

namespace megaflare {
    namespace text {
// word processing------------

        namespace detail {
            template <typename Str> struct split_op {
                Str split;
                constexpr split_op(Str i_split) : split(i_split) {}
    
                template <typename Str1, typename Str2>
                constexpr auto operator() (Str1 str1, Str2 str2)
                    -> decltype(str1 + split + str2)
                { 
                    return str1 + split + str2; 
                }
            };

#if 0
            // not implemented to gcc
            template <std::size_t N>
                constexpr auto make_split_op(char const(&s)[N])
                -> split_op<decltype(sprout::to_string(s))>
            {
                return sprout::to_string(s);
            }
#endif
    
            template <typename T>
                constexpr split_op<T> make_split_op(T s)
            {
                return s;
            }
        }
        static constexpr auto ss_space = sprout::to_string(" ");
        static constexpr auto ss_lf = sprout::to_string("\n");
        static constexpr auto ss_blank = sprout::to_string("");
        static constexpr auto ss_comma = sprout::to_string(", ");

// unwords
        template <typename StrTuple>
        inline constexpr auto unwords(StrTuple tp) -> 
        decltype( tuples::foldr1(detail::make_split_op(ss_space), tp) )
        {
            return tuples::foldr1(detail::make_split_op(ss_space), tp);
        }

// unlines
        template <typename StrTuple>
        inline constexpr auto unlines(StrTuple i_tp) -> 
        decltype( 
            tuples::foldr1(
                detail::make_split_op(ss_lf), 
                tuples::push_back(i_tp, ss_blank)
            )
        )
        {
            return 
                tuples::foldr1(
                    detail::make_split_op(ss_lf), 
                    tuples::push_back(i_tp, ss_blank)
                );
        }

// comma_list
        inline constexpr decltype(ss_blank)
        comma_list(tuples::tuple<>)
        {
            return ss_blank;
        }

        template <typename T, typename... Rest>
        inline constexpr auto comma_list(tuples::tuple<T, Rest...> tp) -> 
            decltype( tuples::foldr1(detail::make_split_op(ss_comma), tp) )
        {
            return tuples::foldr1(detail::make_split_op(ss_comma), tp);
        }

// drop_last_type_matched
        namespace detail {
            template <typename T>
            struct drop_last_type_matched_op {
                // true_type 既に削除済みなので無視
                template <typename U, typename Tuple>
                auto operator() (U u, sprout::tuple<std::true_type, Tuple> tp) ->
                    sprout::tuple<std::true_type, 
                                  typename tuples::result_of::push_front<Tuple, U>::type>
                {
                    return sprout::make_tuple(std::true_type(), 
                                              tuples::push_front(tuples::get<1>(tp), u));
                }

                // false_type 削除対象ではない
                template <typename U, typename Tuple >
                auto operator() (U u, sprout::tuple<std::false_type, Tuple>  tp) ->
                    sprout::tuple<std::false_type, 
                                  typename tuples::result_of::push_front<Tuple, U>::type>
                {
                    return sprout::make_tuple(std::false_type(), 
                                              tuples::push_front(tuples::get<1>(tp), u));
                }

                // false_type 削除対象
                template <typename Tuple >
                auto operator() (T t, sprout::tuple<std::false_type, Tuple> tp) ->
                    sprout::tuple<std::true_type, Tuple>
                {
                    return sprout::make_tuple(std::true_type(), 
                                              tuples::get<1>(tp));
                }        
            };
        }

// 
        namespace detail {
            template <typename T, typename Tuple>
            inline constexpr auto drop_last_type_matched_(Tuple tp) ->
            decltype(tuples::foldr1(drop_last_type_matched_op<T>(), 
                           tuples::push_back(tp, sprout::make_tuple(std::false_type(), sprout::tuple<>()))))
            {
                return 
                    tuples::foldr1(drop_last_type_matched_op<T>(), 
                          tuples::push_back(tp,  sprout::make_tuple(std::false_type(),  sprout::tuple<>()))
                    );
            }
        }

        template <typename T, typename Tuple>
        inline constexpr auto drop_last_type_matched(Tuple tp) ->
        typename sprout::tuple_element<1, decltype(detail::drop_last_type_matched_<T>(tp))>::type
        {
            return tuples::get<1>(detail::drop_last_type_matched_<T>(tp));
        }


        // substitute_tuple
        // tuple内の型がintegral_constant<std::size_t, N>ならば、
        // rhsのN番要素に置換する。
        template <size_t N>
        struct to_substitute 
            : std::integral_constant<std::size_t, N>
        {};

        namespace detail {
            template <typename... RhsElems>
            struct substitute_tuple_op {
                typedef tuples::tuple<RhsElems...> rhs_t;
#if 0
                substitute_tuple_op(rhs_t const& i_rhs)
                : 
                m_rhs(i_rhs) {}
#endif
                rhs_t m_rhs;

                template <typename T>
                constexpr T operator() (T const& i_t) 
                {
                    return i_t; 
                }

                template <std::size_t N>
                constexpr typename tuples::tuple_element<N, rhs_t>::type
                operator() (to_substitute<N>)
                {
                    return tuples::get<N>(m_rhs);
                }
            };
        }
        template <typename LhsTuple, typename... RhsElems>
        inline constexpr auto 
        substitute_tuple(LhsTuple const& i_lhs, tuples::tuple<RhsElems...> const& i_rhs)
            -> decltype(tuples::transform(detail::substitute_tuple_op<RhsElems...>{i_rhs}, i_lhs))
        {
            return tuples::transform(detail::substitute_tuple_op<RhsElems...>{i_rhs}, i_lhs);
        }

    } //namespace
}

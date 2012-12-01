#pragma once

#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include "megaflare/text.hpp"
#include "megaflare/tuples.hpp"
#include "megaflare/util.hpp"


namespace megaflare {
    namespace code {
        namespace detail {
            namespace bk {
                // making -----------------------------
                class  symbol {
                    int m_val;

                    static constexpr sprout::string<4> m_STRHEADER = 
                        sprout::string<4>{"sym"};
                public:
                    constexpr symbol() : m_val(0) {}
                    constexpr symbol next() {
                        return m_val + 1;
                    }
                    constexpr 
                    decltype(m_STRHEADER + sprout::int_to_string<char>(m_val)) 
                        to_string() 
                    {
                        return m_STRHEADER + sprout::int_to_string<char>(m_val);
                    }
                private:
                    constexpr symbol(int i_val) : m_val(i_val) {}

                };

#if 0    
                template <typename ArgTuple, int NHeader, int NFooter, int NBody>
                struct kernel_helper {
                    symbol symbol;
                    ArgTuple m_arg;
                    sprout::string<NHeader> m_header;
                    sprout::string<NFooter> m_footer;
                    sprout::string<NBody> m_body;
                };
#endif

                // tupleによる疑似構造体
                enum  class elems : std::size_t {
                    symbol = 0,
                        arg_tuple,
                        header_str,
                        footer_str,
                        result_str,
                        name_str,
                        body_str,
                        max // 添字の最大値
                        };

                struct blank_param {
#if 0
                    constexpr operator sprout::string<0>() {
                        return false ? sprout::to_string("") : (throw std::logic_error("blank argument."));
                    }
#endif
                };



                template <elems Elem, typename T, typename Tuple>
                constexpr inline auto modify(Tuple const& i_tp, T const& i_t)
                    -> decltype(
                        tuples::modify<static_cast<std::size_t>(Elem)>(i_tp, i_t)
                    )
                {
                    return tuples::modify<static_cast<std::size_t>(Elem)>(i_tp, i_t);
                }

                template <elems Elem, typename Tuple, std::size_t N  = static_cast<std::size_t>(Elem)> constexpr 
                inline auto get(Tuple const& i_tp)
                    -> typename tuples::tuple_element<N, Tuple>::type
                { 
                    return tuples::get<N>(i_tp); 
                }

                template <typename NameStrT>
                constexpr inline tuples::tuple<
                    symbol, 
                    tuples::tuple<>, 
                    sprout::string<0>,
                    sprout::string<0>,
                    sprout::string<0>,
                    NameStrT,
                    sprout::string<0>
                    >
                construct(symbol const& i_sym, NameStrT const& i_name)
                    {
                        return 
                            tuples::make_tuple( 
                                i_sym,
                                tuples::tuple<>(),
                                sprout::string<0>(),
                                sprout::string<0>(),
                                sprout::string<0>(),
                                i_name,
                                sprout::string<0>());
                    }

                //resolve_arg_tuple-----------------------
                template <typename Tuple>
                constexpr auto resolve_arg_tuple(Tuple const& i_tp)
                ->decltype(modify<elems::arg_tuple>(i_tp, text::comma_list(get<elems::arg_tuple>(i_tp))))
                {
                    return modify<elems::arg_tuple>(i_tp, text::comma_list(get<elems::arg_tuple>(i_tp)));
                }

                //to_string-------------------------------
                template <elems Elem>
                constexpr text::to_substitute<static_cast<std::size_t>(Elem)>
                to_subst_elem() { return {}; }

                constexpr auto bk_seq = 
                    tuples::make_tuple(
                        to_subst_elem<elems::header_str>(),
                        to_subst_elem<elems::result_str>(),
                        text::ss_space,
                        to_subst_elem<elems::name_str>(),
                        sprout::to_string("("),
                        to_subst_elem<elems::arg_tuple>(),
                        sprout::to_string(")"),
                        sprout::to_string("\n{\n"),
                        to_subst_elem<elems::body_str>(),
                        sprout::to_string("\n}\n"),
                        to_subst_elem<elems::footer_str>()
                    );
                template <typename BK>
                constexpr auto
                to_string_(BK i_bk)
                ->decltype(
                    tuples::foldr1(
                        utility::add(), 
                        text::substitute_tuple(bk_seq, i_bk)
                    )
                )
                {
                    return tuples::foldr1(
                        utility::add(), 
                        text::substitute_tuple(bk_seq, i_bk)
                    );
                }

                template <typename BK>
                constexpr auto
                to_string(BK i_bk)
                ->decltype(
                    to_string_(resolve_arg_tuple(i_bk))
                )
                {
                    return to_string_(resolve_arg_tuple(i_bk));
                }
            }
        }
    }
}

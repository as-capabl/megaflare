#pragma once

#include <sprout/string.hpp>
#include <sprout/tuple.hpp>
#include <sprout/tuple/operation.hpp>
#include "detail/building_kernel.hpp"
#include "megaflare/text.hpp"
#include "megaflare/tuples.hpp"
#include "./type_resolve.hpp"
#include "./param.hpp"
#include "./raw.hpp"
#include "mods.hpp"

namespace megaflare {
    namespace code {
        template <typename ModTuple, std::size_t NBody, std::size_t NName>
        struct func_t {
            ModTuple m_mods;
            sprout::string<NBody> m_body;
            sprout::string<NName> m_name;

        private:
            template <typename ModTuple_>
            struct ensure_tuple {
                typedef tuples::tuple<ModTuple_> type;
                constexpr type operator() (ModTuple_ const & i_mod)
                {
                    return type(i_mod);
                }
            };
            template <typename... Mods>
            struct ensure_tuple<tuples::tuple<Mods...>> {
                typedef tuples::tuple<Mods...> type;
                constexpr type operator() (type const & i_tuple)
                {
                    return i_tuple;
                }
            };

        public:
            
            template <typename Mod>
            constexpr 
            func_t<typename tuples::types::push_back<
                       typename ensure_tuple<ModTuple>::type, 
                       Mod>::type,
                   NBody,
                   NName>
            modify(Mod const & i_mod)
            {
                static_assert(
                std::is_base_of<modifier_t<Mod>, Mod>::value,
                "need modifier");

                return {tuples::push_back(
                        ensure_tuple<ModTuple>()(m_mods), i_mod),
                        m_body,
                        m_name};
            }

            struct get_params_op {
                template <class Tuple, class T>
                constexpr auto
                operator () (Tuple i_tp, T i_t)
                    -> decltype(i_t.get_params(i_tp))
                {
                    return i_t.get_params(i_tp);
                }
            };


            static constexpr auto get_params(ModTuple const & i_mods)
            -> decltype(tuples::foldl(get_params_op(), 
                                      tuples::tuple<>(), 
                                      modifier_tuple(i_mods)))
            {
                return tuples::foldl(get_params_op(), 
                                     tuples::tuple<>(), 
                                     modifier_tuple(i_mods));
            }

            struct arg_substitute_op {
                template <typename Type, typename Applied>
                constexpr auto operator() (Type const & i_type, Applied & i_applied) 
                    -> decltype(i_applied.subst_to_param(i_type))&
                {
                    return i_applied.subst_to_param(i_type);
                }
            };


#if 0
            template <typename TpParams, 
                      std::size_t N, 
                      std::size_t NMax,
                      typename... TpArgs>
            struct arg_subst_impl {
                typedef arg_subst_impl<TpParams, N+1, NMax, TpArgs...>
                next;
                typedef typename next::type type;

                static type run(TpParams const& i_param, TpArgs... & i_args)
                {
                    return next::run(i_params, i_args)
                        }
            };

            template <typename TpParams, std::size_t NMax, typename... TpArgs>
            struct arg_subst_impl<N, NMax, TpArgs...>
            {
            
            };
#endif

            template <typename TpParams, 
                      typename TpArgs, 
                      sprout::index_t... Indexes>
            static constexpr auto arg_substitute_all(TpParams const & i_params, 
                                                     TpArgs i_args, 
                                                     sprout::index_tuple<Indexes...>)
                -> decltype(
                    tuples::tie(
                        (tuples::get<Indexes>(i_args)
                         .subst_to_param(tuples::get<Indexes>(i_params)))...))

            {
                // 引数の個数チェック
                static_assert(sprout::tuple_size<TpParams>::value
                              == sprout::tuple_size<TpArgs>::value,
                              "wrong number of argument."
                );

                return 
                    tuples::tie(
                        (tuples::get<Indexes>(i_args)
                         .subst_to_param(tuples::get<Indexes>(i_params)))...);
            }        

            template <typename... Args>
            auto operator() (Args&... i_args)
                -> calling_pack<
                NName, 
                decltype(
                    arg_substitute_all(
                        get_params(m_mods), 
                        tuples::tie(i_args...), 
                        sprout::index_range<0, sizeof...(Args)>::make()))>
            { /*<ModTuple, tuples::tuple<Args&...>>*/
                return calling_pack<
                    NName, 
                    decltype(arg_substitute_all(
                                 get_params(m_mods), 
                                 tuples::tie(i_args...), 
                                 sprout::index_range<0, sizeof...(Args)>::make()))>
                {
                    m_name, arg_substitute_all(
                        get_params(m_mods), 
                        tuples::tie(i_args...), 
                        sprout::index_range<0, sizeof...(Args)>::make())
                        };
            }


        private:
            struct get_cl_string_op {
                template <class BK, class T>
                constexpr auto
                operator () (BK i_bk, T i_t)
                    -> decltype(i_t.get_cl_string(i_bk))
                {
                    return i_t.get_cl_string(i_bk);
                }
            };



            template <typename... Mods>
            static constexpr auto 
            get_cl_string_impl(sprout::tuple<Mods...> const& i_mods, 
                               sprout::string<NBody> const& i_body, 
                               sprout::string<NName> const& i_name, 
                               detail::bk::symbol i_sym)
                ->decltype (
                    detail::bk::modify<detail::bk::elems::name_str>(
                        detail::bk::modify<detail::bk::elems::body_str>(
                            tuples::foldl(
                                get_cl_string_op(),
                                detail::bk::construct(i_sym, text::ss_blank),
                                i_mods),
                            i_body), 
                        i_name))
            {
                return 
                    detail::bk::modify<detail::bk::elems::name_str>(
                        detail::bk::modify<detail::bk::elems::body_str>(
                            tuples::foldl(
                                get_cl_string_op(),
                                detail::bk::construct(i_sym, text::ss_blank),
                                i_mods),
                            i_body), 
                        i_name);
            }

            template <typename BK>
            static constexpr auto expand_bk(BK i_bk)
                -> tuples::tuple<detail::bk::symbol,
                decltype (detail::bk::to_string(
                                 detail::bk::modify<detail::bk::elems::result_str>(
                                     i_bk, 
                                     detail::bk::get<detail::bk::elems::result_str>(i_bk))))>
            {
                return tuples::make_tuple(
                    detail::bk::get<detail::bk::elems::symbol>(i_bk),
                    detail::bk::to_string(
                        detail::bk::modify<detail::bk::elems::result_str>(
                            i_bk, 
                            detail::bk::get<detail::bk::elems::result_str>(i_bk))))
                    ;
            }

        public:        
            constexpr auto get_cl_string(detail::bk::symbol i_sym)
                -> decltype(
                    expand_bk(
                        get_cl_string_impl(
                            modifier_tuple(m_mods), 
                            m_body, 
                            m_name,
                            i_sym
                        )))
            {
                return 
                    expand_bk(
                        get_cl_string_impl(
                            modifier_tuple(m_mods), 
                            m_body, 
                            m_name,
                            i_sym
                        )
                    );
            }

        };

        template <typename Signature, std::size_t NBody, std::size_t NName>
        constexpr func_t<Signature, NBody, NName> 
        func(char const (&i_name)[NName],
             Signature i_mods, 
             char const (&i_body)[NBody])
        {
            return { i_mods, 
                    sprout::to_string(i_body), 
                    sprout::to_string(i_name)};
        }


    
        // program
        template <typename... Args>
        using program_t = tuples::tuple<Args...>;

        struct program_op {
            template <typename ModTuple, std::size_t NBody, std::size_t NName, typename... Args>
            constexpr auto 
            operator() (tuples::tuple<Args...> i_lhs, 
                        func_t<ModTuple, NBody, NName> i_rhs)
                -> typename tuples::tuple<Args..., decltype(i_rhs)>
            {
                return tuples::push_back(i_lhs, i_rhs);
            }

            template <typename Raw, typename... Args>
            constexpr auto 
            operator() (tuples::tuple<Args...> i_lhs, 
                        Raw i_rhs)
                -> typename tuples::tuple<Args..., Raw>
            {
                //TODO: get_cl_stringの有無をチェックしてほしい
                return tuples::push_back(i_lhs, i_rhs);
            }
        };

        template <typename... Args>
        constexpr auto
        program(Args... i_args)
            -> decltype(
                tuples::foldl(program_op(), 
                              tuples::tuple<>(),
                              sprout::make_tuple(i_args...)
                              ))
        {
            return tuples::foldl(program_op(),
                                 tuples::tuple<>(),
                                 sprout::make_tuple(i_args...)
                                 );
        }

        struct get_cl_string_helper {
            template <typename T>
            constexpr auto operator() (detail::bk::symbol i_sym, T const& i_t)->
                decltype(i_t.get_cl_string(i_sym))
            {
                static_assert(
                    std::is_same<
                    typename tuples::tuple_element<0, decltype(i_t.get_cl_string(i_sym))>::type,
                    detail::bk::symbol>::value,
                    "get_cl_string(i_sym) must return sprout::tuple<detail::bk::symbol, T>");
                return i_t.get_cl_string(i_sym);
            }
        };


        template <typename... Args>
        inline constexpr auto
        get_cl_string(program_t<Args...> const& i_program)
            -> decltype(
                text::unlines(
                    tuples::get<1>(
                        tuples::map_accum_l(get_cl_string_helper(),
                                            i_program, 
                                            detail::bk::symbol()))))
        {
            return text::unlines(
                tuples::get<1>(
                    tuples::map_accum_l(get_cl_string_helper(), 
                                        i_program, 
                                        detail::bk::symbol())));
        }


        // translation helper ------------------------------
        namespace helper {
            template <int N, int I, typename... Args>
            struct extend_tuple_impl {
                typedef extend_tuple_impl<N, I+1, Args..., detail::bk::blank_param> next_t;
                typedef typename next_t::type type;
                static constexpr type impl (sprout::tuple<Args...> const& i_tp) {
                    return next_t::impl(tuples::push_back(i_tp, detail::bk::blank_param()));
                }
            };

            template <int N, typename... Args>
            struct extend_tuple_impl<N, N, Args...> {
                typedef sprout::tuple<Args...> type;
                static constexpr type impl (sprout::tuple<Args...> const& i_tp) {
                    return i_tp;
                }
            };

            template <int N, typename... Args>
            inline constexpr 
            typename extend_tuple_impl<N, sizeof...(Args), Args...>::type
            extend_tuple(sprout::tuple<Args...> const& i_tp) {
                return extend_tuple_impl<N, sizeof...(Args), Args...>::impl(i_tp);
            }

#if 0
            template <int N>
            struct get_arg_op {
                template <typename TParam>
                constexpr TParam impl(TParam const& i_t_param, detail::bk::blank_param const&) 
                {
                    return i_t_param;
                }

                template <typename UParam>
                constexpr UParam impl(detail::bk::blank_param const&, UParam const& i_u_param) 
                {
                    return i_u_param;
                }

                template <typename T, typename U>
                constexpr auto operator() (T const& i_t, U const& i_u) 
                    -> decltype( impl(i_t.template get_param<N>(), i_u.template get_param<N>()) )
                {
                    return i_t.get_param<N>();
                }            
            };
#endif
        }
    }
}

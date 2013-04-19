/*!
 * @file
 * This file defines utilities to perform metafunction forwarding.
 */

#if !defined(BOOST_PP_IS_ITERATING)

//////////////////////////////////////////////////////////////////////////////
// Normal header
//////////////////////////////////////////////////////////////////////////////
#   ifndef DYNO_FORWARD_METAFUNCTION_HPP
#   define DYNO_FORWARD_METAFUNCTION_HPP

#   include <dyno/detail/doxygen.hpp>

#   include <boost/config.hpp>
#   include <boost/mpl/has_xxx.hpp>
#   include <boost/preprocessor/cat.hpp>
#   include <boost/preprocessor/iteration/iterate.hpp>
#   include <boost/preprocessor/repetition/enum.hpp>
#   include <boost/preprocessor/repetition/enum_params.hpp>
#   include <boost/utility/enable_if.hpp>


    namespace dyno {

#   if !defined(DYNO_METAFUNCTION_FORWARDING_MAX_ARITY) ||                  \
        defined(DYNO_DOXYGEN_INVOKED)
        /*!
         * Configuration macro controlling the maximum arity of automatically
         * forwarded metafunctions.
         *
         * If variadic templates are supported, this is not used.
         */
#       define DYNO_METAFUNCTION_FORWARDING_MAX_ARITY 10
#   endif

    template <typename T>
    struct unwrap {
        typedef T type;
    };

    namespace forward_metafunction_detail {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(type)

        template <typename Metafunction>
        struct unwrap_args;

#   ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

        template <template <typename ...> class Metafunction, typename ...Args>
        struct unwrap_args<Metafunction<Args...> > {
            typedef Metafunction<typename unwrap<Args>::type...> type;
        };

#   else // BOOST_NO_CXX11_VARIADIC_TEMPLATES

#       define BOOST_PP_ITERATION_PARAMS_1                                  \
            (3, (1, DYNO_METAFUNCTION_FORWARDING_MAX_ARITY,                 \
                    <dyno/forward_metafunction.hpp>))                       \
        /**/
#       include BOOST_PP_ITERATE()

#   endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES
    }

    /*!
     * Metafunction forwarding the call to `F`, possibly modifying the
     * value of some arguments while doing so.
     *
     * @todo Explain how to use it and be more precise.
     */
    template <typename F>
    struct forward_metafunction
        : boost::lazy_enable_if<
            forward_metafunction_detail::has_type<
                typename forward_metafunction_detail::unwrap_args<F>::type
            >,
            typename forward_metafunction_detail::unwrap_args<F>::type
        >
    { };

    /*!
     *
     */
    template <typename F>
    struct can_be_forwarded
        : forward_metafunction_detail::has_type<
            typename forward_metafunction_detail::unwrap_args<F>::type
        >
    { };

#   define DYNO_ENABLE_METAFUNCTION_FORWARDING(Arity, Metafunction)         \
        template <                                                          \
            BOOST_PP_ENUM_PARAMS(Arity, typename BOOST_PP_CAT(A, __LINE__)) \
        >                                                                   \
        struct Metafunction<                                                \
            BOOST_PP_ENUM_PARAMS(Arity, BOOST_PP_CAT(A, __LINE__)),         \
            typename ::boost::enable_if<                                    \
                ::dyno::can_be_forwarded<                                   \
                    Metafunction<                                           \
                        BOOST_PP_ENUM_PARAMS(Arity,BOOST_PP_CAT(A,__LINE__))\
                    >                                                       \
                >                                                           \
            >::type                                                         \
        >                                                                   \
            : ::dyno::forward_metafunction<                                 \
                Metafunction<                                               \
                    BOOST_PP_ENUM_PARAMS(Arity, BOOST_PP_CAT(A, __LINE__))  \
                >                                                           \
            >                                                               \
        { };                                                                \
    /**/

#   define DYNO_ENABLE_UNARY_METAFUNCTION_FORWARDING(Metafunction)  \
        DYNO_ENABLE_METAFUNCTION_FORWARDING(1, Metafunction)

#   define DYNO_ENABLE_BINARY_METAFUNCTION_FORWARDING(Metafunction)  \
        DYNO_ENABLE_METAFUNCTION_FORWARDING(2, Metafunction)

#   define DYNO_ENABLE_TERNARY_METAFUNCTION_FORWARDING(Metafunction)  \
        DYNO_ENABLE_METAFUNCTION_FORWARDING(3, Metafunction)

    } // end namespace dyno

#   endif // !DYNO_FORWARD_METAFUNCTION_HPP

#else // BOOST_PP_IS_ITERATING

//////////////////////////////////////////////////////////////////////////////
// Preprocessor repetition code
//////////////////////////////////////////////////////////////////////////////

#   define N BOOST_PP_ITERATION()
#   define A BOOST_PP_CAT(A, __LINE__)

    template <
        template <BOOST_PP_ENUM_PARAMS(N, typename x)> class Metafunction,
        BOOST_PP_ENUM_PARAMS(N, typename A)
    >
    struct unwrap_args<Metafunction<BOOST_PP_ENUM_PARAMS(N, A)> > {

#   define DYNO_I_UNWRAP(z, n, _) typename unwrap<BOOST_PP_CAT(A, n)>::type
        typedef Metafunction<BOOST_PP_ENUM(N, DYNO_I_UNWRAP, ~)> type;
#   undef DYNO_I_UNWRAP

    };

#   undef A
#   undef N

#endif // !BOOST_PP_IS_ITERATING

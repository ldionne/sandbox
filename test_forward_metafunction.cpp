/*!
 * @file
 * This file contains unit tests for metafunction forwarding.
 */

#include <dyno/sandbox/forward_metafunction.hpp>

#include <boost/mpl/apply.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>


template <typename T> struct supported;
template <typename T> struct forwarder;

// Binary metafunction with forwarding enabled
template <typename First, typename Second, typename Enable = void>
struct first;
DYNO_ENABLE_BINARY_METAFUNCTION_FORWARDING(first)
template <typename First, typename Second>
struct first<supported<First>, supported<Second> > {
    typedef First type;
};

// Then, we declare a forwarder that will be able to use the metafunction
// without defining it himself, just because it is wrapping a `supported<T>`.
namespace dyno {
    template <typename T>
    struct unwrap<forwarder<T> > {
        typedef T type;
    };
}

// Finally, we call the metafunction on the supported type and on the
// forwarder, and we make sure that the results are the same.
template <typename First, typename Second, typename Pred>
struct all_perms {
#   define DO_ASSERT(F, S) \
        BOOST_MPL_ASSERT((::boost::mpl::apply<Pred, first<F, S> >))

    // These are all the permutations up to 2x nested forwarders
    DO_ASSERT(First, Second);

    DO_ASSERT(forwarder<First>, Second);
    DO_ASSERT(First, forwarder<Second>);
    DO_ASSERT(forwarder<First>, forwarder<Second>);

    DO_ASSERT(forwarder<forwarder<First> >, Second);
    DO_ASSERT(First, forwarder<forwarder<Second> >);
    DO_ASSERT(forwarder<forwarder<First> >, forwarder<Second>);
    DO_ASSERT(forwarder<First>, forwarder<forwarder<Second> >);
    DO_ASSERT(forwarder<forwarder<First> >, forwarder<forwarder<Second> >);

    // Try some irregularities
    DO_ASSERT(forwarder<forwarder<forwarder<First> > >, Second);
    DO_ASSERT(forwarder<forwarder<forwarder<forwarder<First> > > >, Second);
    DO_ASSERT(forwarder<forwarder<forwarder<First> > >, forwarder<Second>);
};

BOOST_MPL_HAS_XXX_TRAIT_DEF(type)

typedef boost::mpl::not_<has_type<boost::mpl::_1> > is_not_defined;
typedef boost::is_same<
            boost::mpl::eval_if<boost::mpl::true_, boost::mpl::_1, void>,
            struct the_first
        > result_is_the_first;

template struct all_perms<
    struct anything, struct anything, is_not_defined
>;
template struct all_perms<
    supported<struct anything>, struct anything, is_not_defined
>;
template struct all_perms<
    struct anything, supported<struct anything>, is_not_defined
>;
template struct all_perms<
    supported<struct the_first>, supported<struct the_second>,
    result_is_the_first
>;

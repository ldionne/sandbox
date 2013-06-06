
//////////////////////////////////////////////////////////////////////////////
// super_lambda.hpp
//////////////////////////////////////////////////////////////////////////////

#include <boost/mpl/at.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/map.hpp>
#include <boost/type_traits/is_same.hpp>


namespace boost { namespace mpl {
namespace detail {
template <typename Expression>
struct eval_inner;

template <template <typename ...> class Expression, typename ...T>
struct eval_inner<Expression<T...> > {
    typedef Expression<typename T::type...> type;
};


template <typename SubstitutionMap>
class substitution_context {
    // Helpers
    template <typename Expression, typename Otherwise>
    struct do_substitution_or
        : eval_if<has_key<SubstitutionMap, Expression>,
            at<SubstitutionMap, Expression>,
            Otherwise
        >
    { };

    template <typename Expression>
    struct do_substitution
        : do_substitution_or<Expression, identity<Expression> >
    { };

    template <typename Expression>
    struct do_lazy_substitution
        : do_substitution<typename Expression::type>
    { };


    // The recursive substitution process.
    template <typename Expression>
    struct substitute
        : do_substitution<Expression>
    { };

    template <template <typename ...> class Expression, typename ...T>
    struct substitute<Expression<T...> >
        : do_substitution_or<
            // Try substituting the expression as a whole.
            Expression<T...>,

            // If that fails, recursively substitute the inner types.
            eval_inner<Expression<substitute<T>...> >
        >
    { };

public:
    template <typename Expression>
    using apply = substitute<Expression>;
};
} // end namespace detail

template <typename Expression, typename SubstitutionMap>
struct substitute
    : detail::substitution_context<SubstitutionMap>::template apply<Expression>
{ };

template <typename Expression, typename Result, typename ...Substitutions>
struct is_same_after_substitution
    : is_same<
        typename substitute<Expression, map<Substitutions...> >::type,
        Result
    >
{ };
}} // end namespace boost::mpl



//////////////////////////////////////////////////////////////////////////////
// main.cpp
//////////////////////////////////////////////////////////////////////////////

#include <boost/mpl/map.hpp>
#include <boost/mpl/pair.hpp>


// clang++ -I /usr/lib/c++/v1 -I ~/code/boost-trunk -std=c++11 -stdlib=libc++ -o /dev/null ~/code/sandbox/super_lambda.cpp

using namespace boost::mpl;

template <typename ...> struct expression;

template <int> struct a;
typedef a<0> a0;
typedef a<1> a1;
typedef a<2> a2;
typedef a<3> a3;


static_assert(is_same_after_substitution<
    expression<a0, a1>,
    expression<a0, a1>,

    pair<int, float>,
    pair<float, char>
>::value, "");


static_assert(is_same_after_substitution<
    expression<a0, a1>,
    expression<a1, a2>,

    pair<a0, a1>,
    pair<a1, a2>
>::value, "");


static_assert(is_same_after_substitution<
    expression<a0, a1>,
    a2,

    pair<expression<a0, a1>, a2>
>::value, "");


// Make sure the pre-substitution happens: `a1` is never substituted for `a3`
// because `expression<a0, a1>` is substituted for `a2` before.
static_assert(is_same_after_substitution<
    expression<a0, a1>,
    a2,

    pair<expression<a0, a1>, a2>,
    pair<a1, a3>
>::value, "");


// Make sure that the substitution does not happen twice. After the
// pre-substitution attempt fails, `a1` is substituted for `a2` and
// the expression is not substituted for `a3` after.
static_assert(is_same_after_substitution<
    expression<a0, a1>,
    expression<a0, a2>,

    pair<a1, a2>,
    pair<expression<a0, a2>, a3>
>::value, "");


// To show that this named placeholders solution is a generalization of
// the current MPL mechanism, the current mechanism can be implemented
// with the following SubstitutionMap:
// map<pair<arg<0>, Args[0]>, ..., pair<arg<i>, Args[i]> >


int main() { }

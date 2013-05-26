
#include <boost/mpl/all_of.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/sum.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>


namespace boost { namespace traverse {
    template <typename Node>
    struct is_leaf
        : mpl::true_
    { };

    template <template <typename ...> class Node, typename ...Children>
    struct is_leaf
        : mpl::false_
    { };


    template <typename Node>
    struct arity
        : mpl::unsigned_<0>
    { };

    template <template <typename ...> class Node, typename ...Children>
    struct arity<Node<Children...> >
        : mpl::unsigned_<sizeof(Children...)>
    { };


    template <unsigned n, typename Node>
    struct nth_child;

    template <unsigned n, template <typename ...> class Node, typename ...Children>
    struct nth_child<n, Node<Children...> >
        : mpl::at_c<mpl::vector<Children...>, n>
    { };


    template <typename Tree1, typename Tree2>
    struct matches
        : is_same<Tree1, Tree2>
    { };

    struct _;
    template <typename Tree> struct matches<_, Tree> : mpl::true_ { };
    template <typename Tree> struct matches<Tree, _> : mpl::true_ { };
    // remove ambiguity for matches<_, _>
    template <> struct matches<_, _> : mpl::true_ { };

    template <template <typename ...> class Node, typename ...Children1, typename ...Children2>
    struct matches<Node<Children1...>, Node<Children2...> >
        : mpl::and_<
            mpl::bool_<sizeof(Children1...) == sizeof(Children2...)>,
            mpl::all_of<
                mpl::joint_view<
                    mpl::vector<
                        mpl::vector<Children1...>,
                        mpl::vector<Children2...>
                    >
                >,
                matches<mpl::_1, mpl::_2>
            >
        >
    { };


    template <typename Leaf, typename F>
    struct dfs
        : mpl::apply<F, Leaf>
    { };

    template <template <typename ...> class Node, typename ...Children, typename F>
    struct dfs<Node<Children...>, F> {
        typedef Node<
            typename mpl::apply<
                F,
                typename mpl::apply<
                    dfs<mpl::_1, F>, Children
                >::type
            >::type...
        > type;
    };


    template <typename Leaf>
    struct size
        : mpl::unsigned_<1>
    { };

    template <template <typename ...> class Node, typename ...Children>
    struct size
        : mpl::sum<
            typename mpl::transform<
                mpl::vector<struct add_1_for_this_node, Children...>,
                size<mpl::_1>
            >::type
        >
    { };
}}

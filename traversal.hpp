
#ifndef SANDBOX_TRAVERSAL_HPP
#define SANDBOX_TRAVERSAL_HPP

#include <boost/mpl/accumulate.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/unpack_args.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/zip_view.hpp>
#include <boost/type_traits/is_same.hpp>
#include <dyno/detail/mpl_extensions.hpp>


namespace boost { namespace mpl {
    template <typename Sequence>
    struct sum
        : accumulate<Sequence, int_<0>, plus<_1, _2> >
    { };
}}

namespace boost { namespace traverse {
    template <typename Node>
    struct is_leaf
        : mpl::true_
    { };

    template <template <typename ...> class Node, typename ...Children>
    struct is_leaf<Node<Children...> >
        : mpl::false_
    { };


    template <typename Node>
    struct arity
        : mpl::int_<0>
    { };

    template <template <typename ...> class Node, typename ...Children>
    struct arity<Node<Children...> >
        : mpl::int_<sizeof...(Children)>
    { };


    template <unsigned n, typename Node>
    struct nth_child;

    template <unsigned n, template <typename ...> class Node, typename ...Children>
    struct nth_child<n, Node<Children...> >
        : mpl::at_c<mpl::vector<Children...>, n>
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
        : mpl::int_<1>
    { };

    template <template <typename ...> class Node, typename ...Children>
    struct size<Node<Children...> >
        : mpl::sum<
            typename mpl::transform<
                mpl::vector<struct add_1_for_this_node, Children...>,
                size<mpl::_1>
            >::type
        >
    { };
}}

#endif // !SANDBOX_TRAVERSAL_HPP

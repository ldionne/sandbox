
#include "traversal.hpp"

#include <boost/mpl/assert.hpp>


using namespace boost::traverse;


struct Leaf;
template <typename...> struct Node;


// boost::traverse::matches
BOOST_MPL_ASSERT((matches<Leaf, Leaf>));
BOOST_MPL_ASSERT_NOT((matches<Leaf, struct some_other_type>));

BOOST_MPL_ASSERT((matches<_, Leaf>));
BOOST_MPL_ASSERT((matches<Leaf, _>));
BOOST_MPL_ASSERT((matches<_, _>));

BOOST_MPL_ASSERT((matches<Node<Leaf>, Node<Leaf> >));
BOOST_MPL_ASSERT((matches<Node<Leaf>, Node<_> >));
BOOST_MPL_ASSERT((matches<Node<_>, Node<Leaf> >));
BOOST_MPL_ASSERT((matches<Node<_>, Node<_> >));
BOOST_MPL_ASSERT_NOT((matches<Node<>, Node<_> >));
BOOST_MPL_ASSERT_NOT((matches<Node<_>, Node<> >));

BOOST_MPL_ASSERT((matches<
    Node<Node<Leaf>, Node<>, Node<Leaf> >,
    Node<Node<_>, Node<>, Node<Leaf> >
>));

BOOST_MPL_ASSERT_NOT((matches<
    Node<Node<>, Node<> >,
    Node<Node<Leaf>, Node<> >
>));


int main() { }


// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic -o/dev/null ~/code/sandbox/test_traverse.cpp
// g++-4.8 -std=c++11 -ftemplate-backtrace-limit=0 -I /usr/local/include -Wall -Wextra -pedantic -I ~/code/dyno/include -o/dev/null ~/code/sandbox/test_traverse.cpp

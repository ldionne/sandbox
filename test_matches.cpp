
#include "dyno_v3.hpp"

#include <boost/mpl/assert.hpp>


using namespace dyno;


struct Leaf;
template <typename...> struct Node;


// dyno::matches
BOOST_MPL_ASSERT((matches<Leaf, Leaf>));
BOOST_MPL_ASSERT_NOT((matches<Leaf, struct some_other_type>));
BOOST_MPL_ASSERT((matches<Leaf, _>));

BOOST_MPL_ASSERT((matches<Node<Leaf>, Node<Leaf> >));
BOOST_MPL_ASSERT((matches<Node<Leaf>, Node<_> >));
BOOST_MPL_ASSERT_NOT((matches<Node<>, Node<_> >));

BOOST_MPL_ASSERT((matches<
    Node<Node<Leaf>, Node<>, Node<Leaf> >,
    Node<Node<_>, Node<>, Node<Leaf> >
>));

BOOST_MPL_ASSERT_NOT((matches<
    Node<Node<>, Node<> >,
    Node<Node<Leaf>, Node<> >
>));


BOOST_MPL_ASSERT_NOT((matches<
    struct some_type,
    and_<struct another_type, struct some_type>
>));
BOOST_MPL_ASSERT((matches<
    Node<struct leaf2, struct leaf0, struct leaf1>,
    Node<and_<struct leaf0, struct leaf1, struct leaf2> >
>));


int main() { }


// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic -o/dev/null ~/code/sandbox/test_matches.cpp
// g++-4.8 -std=c++11 -ftemplate-backtrace-limit=0 -I /usr/local/include -Wall -Wextra -pedantic -I ~/code/dyno/include -o/dev/null ~/code/sandbox/test_matches.cpp

// (C) Copyright 2013 Louis Dionne
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LDIONNE_SANDBOX_OPTIMIZED_MAP_HPP
#define LDIONNE_SANDBOX_OPTIMIZED_MAP_HPP

#include <boost/iterator/iterator_traits.hpp>
#include <boost/proto/proto.hpp>
#include <utility> // for std::make_pair


namespace sandbox {

namespace oms_detail {
struct do_insert {
    template <typename Sig>
    struct result;

    template <typename This, typename Container, typename Key, typename Value>
    struct result<This(Container, Key, Value)>
        : boost::iterator_reference<typename Container::iterator>
    { };

    template <typename Container, typename Key, typename Value>
    typename result<do_insert(Container, Key, Value)>::type
    operator()(Container& container, Key const& key, Value const& value) {
        return *container.insert(std::make_pair(key, value)).first;
    }
};

using namespace boost::proto;
struct optimize_map_subscripts
    : or_<
        // foo[bar] = x -> don't default-construct an object for nothing,
        //                 perform an insertion instead.
        when<
            assign<subscript<_, _>, _>,
            //   do_insert(map,          key,           value)
            call<do_insert(_left(_left), _right(_left), _right)>
        >,

        // anything else -> normal behavior
        _
    >
{ };
} // end namespace oms_detail

template <typename Map>
struct optimized
    : boost::proto::extends<
        typename boost::proto::terminal<Map>::type,
        optimized<Map>
    >
{
private:
    typedef typename boost::proto::terminal<Map>::type Terminal;

    // Seriously? There must be a way to do that inline.
    static Terminal make_terminal(Map const& map) {
        Terminal t = {map};
        return t;
    }

public:
    optimized(Map const& map = Map())
        : optimized::proto_extends(make_terminal(map))
    { }
};

template <typename Map>
optimized<Map> optimize(Map const& map) {
    return optimized<Map>(map);
}
} // end namespace sandbox

#endif // !LDIONNE_SANDBOX_OPTIMIZED_MAP_HPP

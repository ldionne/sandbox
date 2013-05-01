/*!
 * @file
 * This file implements the `dyno::detail::aggregate_map`
 * template class and related metafunctions.
 */

#ifndef DYNO_DETAIL_AGGREGATE_MAP_HPP
#define DYNO_DETAIL_AGGREGATE_MAP_HPP

#include <dyno/detail/doxygen.hpp>

#include <boost/move/utility.hpp>


namespace dyno {
namespace aggregate_map_detail {
//! @internal Tag signalling the end of a chain of `aggregate_cell`s.
struct last_in_chain DYNO_DOXYGEN_FORWARD_DECL;

//! @internal Cell optimized for holding an aggregate type.
template <typename Tag, typename Data, typename Next>
struct aggregate_cell {
    typedef Tag tag_type;
    typedef Next next_type;
    typedef Data data_type;

    data_type data;
    next_type next;

    template <typename ThisData, typename ...MoreData>
    static aggregate_cell
    create(BOOST_FWD_REF(ThisData) data, BOOST_FWD_REF(MoreData) ...more) {
        aggregate_cell cell = {
            boost::forward<ThisData>(data),
            Next::create(boost::forward<MoreData>(more)...)
        };
        return cell;
    }
};

/*!
 * @internal
 * Specialization of `aggregate_cell` for the last cell in the chain.
 *
 * It does not have a `Next` member.
 */
template <typename Tag, typename Data>
struct aggregate_cell<Tag, Data, last_in_chain> {
    typedef Tag tag_type;
    typedef Data data_type;

    data_type data;

    template <typename ThisData>
    static aggregate_cell create(BOOST_FWD_REF(ThisData) data) {
        aggregate_cell cell = { boost::forward<ThisData>(data) };
        return cell;
    }
};
} // end namespace aggregate_map_detail

namespace detail {
template <typename ...Cells>
struct aggregate_map;

template <typename ...Cells>
aggregate_sequence<Cells...> make_aggregate_sequence()
} // end namespace detail
} // end namespace dyno

#endif // !DYNO_DETAIL_AGGREGATE_MAP_HPP

/*!
 * @file
 * Defines `react::parameters_of`.
 */

#ifndef REACT_PARAMETERS_OF_HPP
#define REACT_PARAMETERS_OF_HPP

#include <react/sandbox/detail/fetch_nested.hpp>
#include <react/sandbox/detail/placeholder.hpp>

#include <boost/mpl/fold.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/set_insert_range.hpp>
#include <boost/mpl/vector.hpp>


namespace react {
    namespace parameters_of_detail {
        REACT_FETCH_NESTED(
            required_features,
            required_features,
            boost::mpl::set0<>
        )

        template <typename T, typename IfNotPlaceholder>
        struct placeholder_dependencies
            : boost::mpl::if_<detail::is_placeholder<T>,
                required_features<T>,
                IfNotPlaceholder
            >::type
        { };
    } // end namespace parameters_of_detail

    /*!
     * Returns the features directly parameterizing an unbound computation.
     *
     * The parameters of an unbound computation are the features that are
     * required directly in order to `bind` the unbound computation.
     *
     * @return A Boost.MPL Associative Sequence of @ref Feature "Features"
     */
    template <typename UnboundComputation>
    struct parameters_of
        : parameters_of_detail::placeholder_dependencies<
            UnboundComputation, boost::mpl::set0<>
        >
    { };

    template <template <typename ...> class F, typename ...T>
    struct parameters_of<F<T...>>
        : parameters_of_detail::placeholder_dependencies<
            F<T...>,
            boost::mpl::fold<
                typename boost::mpl::vector<T...>::type,
                boost::mpl::set0<>,
                boost::mpl::set_insert_range<
                    boost::mpl::_1,
                    parameters_of<boost::mpl::_2>
                >
            >
        >
    { };
} // end namespace react

#endif // !REACT_PARAMETERS_OF_HPP

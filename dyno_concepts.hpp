
#ifndef DYNO_CONCEPTS_HPP
#define DYNO_CONCEPTS_HPP

//////////////////////////////////////////////////////////////////////////////
// DYNO_INHERIT_CONSTRUCTORS macro for portability
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/utility.hpp>

#define DYNO_INHERIT_CONSTRUCTORS(Type, Base)                               \
    template <typename ...Args>                                             \
    Type(BOOST_FWD_REF(Args) ...args)                                       \
        : Base(::boost::forward<Args>(args)...)                             \
    { }                                                                     \
/**/



//////////////////////////////////////////////////////////////////////////////
// matches metafunction
//////////////////////////////////////////////////////////////////////////////
#include "traversal.hpp"

namespace dyno {
// template <typename T, typename Pattern>
// struct matches;

using boost::traverse::matches;
using boost::traverse::_;
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// domain_of metafunction
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
    /*!
     *
     */
    template <typename Event>
    struct domain_of {
        typedef typename Event::dyno_domain type;
    };
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// instance_of
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
    /*!
     * We need a way to deal with the lifetime of listeners. While we access
     * the instance with this function, we should _not_ only return a
     * reference to a static inside of it.
     */
    template <typename Listener>
    Listener& instance_of() {
        static Listener the_only_instance_in_the_program;
        return the_only_instance_in_the_program;
    }
}



//////////////////////////////////////////////////////////////////////////////
// Event concept
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
/*!
 * Specification of the `Event` concept.
 *
 * An event is a type carrying information about something that happens
 * in a program. The sole purpose of an event is to serve as a type tag
 * to find the right overload when calling a `Listener`.
 *
 *
 * ## Notation
 * | Expression | Description
 * | ---------- | -----------
 * | `E`        | A type modeling the `Event` concept
 * | `Pattern`  | Any type
 *
 *
 * ## Valid expressions
 * In addition to be `DefaultConstructible` and `CopyConstructible`, the
 * following expressions must be valid and have the described semantics.
 *
 * | Expression                  | Return type                      | Semantics
 * | ----------                  | -----------                      | ---------
 * | `matches<E, Pattern>::type` | See `matches`                    | See `matches`
 * | `domain_of<E>::type`        | A model of the `Domain` concept  | Returns the domain categorizing `E`.
 *
 *
 * @tparam E
 *         The type to be tested for modeling of the `Event` concept.
 */
template <typename E>
struct Event;
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// predefined environment variables
//////////////////////////////////////////////////////////////////////////////
namespace dyno { namespace env {
/*!
 * Key to an optional environment variable containing the `this` pointer of
 * the object that triggered the current event.
 */
struct _this;

/*!
 * Key to an optional environment variable containing the arguments passed
 * to the function that triggered the current event as a tuple.
 */
struct _args;
}} // end namespace dyno::env



//////////////////////////////////////////////////////////////////////////////
// generate
//////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/include/map.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/utility/enable_if.hpp>

namespace dyno {
namespace generate_detail {
    template <typename Pattern>
    struct convertible_if_matches {
        template <typename T, typename =
            typename boost::enable_if<matches<T, Pattern> >::type>
        operator T() const { return T(); }
    };

    template <typename Event, typename Environment>
    struct call_listener {
        Environment& env_;
        template <typename Listener>
        void operator()(Listener*) {
            instance_of<Listener>()(convertible_if_matches<Event>(), env_);
        }
    };
} // end namespace generate_detail

/*!
 *
 */
template <typename Event, typename Environment>
void generate(Environment const& env) {
    generate_detail::call_listener<Event, Environment const> call = {env};
    typedef typename domain_of<Event>::type Domain;
    typedef typename boost::mpl::transform<
        typename Domain::static_listeners,
        boost::add_pointer<boost::mpl::_1>,
        boost::mpl::back_inserter<boost::mpl::vector<> >
    >::type ListenersWithoutInstantiation;
    boost::mpl::for_each<ListenersWithoutInstantiation>(call);
}

/*!
 *
 */
template <typename Event>
void generate() {
    generate<Event>(boost::fusion::map<>());
}
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// Domain concept
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
/*!
 * Specification of the `Domain` concept.
 *
 * Conceptually, a domain regroups the listeners to notify when any event
 * from a set of events is triggered.
 *
 * All domains have a super-domain whose events are more general. Members of
 * a domain are also automatically members of all the sub-domains of that
 * domain. The most general domain is `root_domain` and it is its own
 * super-domain.
 *
 *
 * ## Notation
 * | Expression | Description
 * | ---------- | -----------
 * | `D`        | A type modeling the `Domain` concept
 *
 *
 * ## Valid expressions
 * | Expression            | Return type                                                     | Semantics
 * | ----------            | -----------                                                     | ---------
 * | `D::static_listeners` | A Boost.MPL `Sequence` of types modeling the `Listener` concept | Returns the set of listeners that were registered to that domain or its super-domain at compile-time.
 * | `D::super_domain`     | A type modeling the `Domain` concept                            | Returns the super-domain of a domain.
 *
 *
 * @tparam D
 *         The type to be tested for modeling of the `Domain` concept.
 */
template <typename D>
struct Domain;
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// default implementation of a domain
//////////////////////////////////////////////////////////////////////////////
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/vector.hpp>

namespace dyno {
/*!
 *
 */
template <typename Superdomain, typename ...Listeners>
struct domain {
    typedef Superdomain super_domain;

    typedef boost::mpl::joint_view<
        boost::mpl::vector<Listeners...>,
        typename super_domain::static_listeners
    > static_listeners;
};
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// root_domain
//////////////////////////////////////////////////////////////////////////////
#include <boost/mpl/vector.hpp>

namespace dyno {
    /*!
     *
     */
    struct root_domain {
        typedef root_domain super_domain;
        typedef boost::mpl::vector<> static_listeners;
    };
} // end namespace dyno




/*!
 * @section Tutorial
 * 1. define events
 * 2. instrument your code (mixins, wrappers, raw calls to generate or other means)
 * 3. implement your listener, i.e. what will you do on those events
 *
 *
 * 4. define what domain(s) of events your listener is subscribed to
 *    your listener will be called whenever any event from those domains
 *    is generated.
 *    ^^^^^^^^^^^^^^^^ domains are useless (make sure of it) ^^^^^^^^^^^^^
 */

#endif // !DYNO_CONCEPTS_HPP

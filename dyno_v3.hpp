
#ifndef SANDBOX_DYNO_V3_HPP
#define SANDBOX_DYNO_V3_HPP

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
#include <boost/mpl/and.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/unpack_args.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/zip_view.hpp>
#include <boost/type_traits/is_same.hpp>
#include <dyno/detail/mpl_extensions.hpp>

namespace dyno {
//! By default, a type matches a pattern if they are the same.
//! Note: specializing matches<T, T> makes it ambiguous with the Node<...>
//!       specialization when Children1 is the same as Children2.
template <typename T, typename Pattern>
struct matches
    : boost::is_same<T, Pattern>
{ };

//! Two template classes match if their template parameters match side by side.
template <template <typename ...> class Node, typename ...Children1, typename ...Children2>
struct matches<Node<Children1...>, Node<Children2...> >
    : boost::mpl::and_<
        boost::mpl::bool_<sizeof...(Children1) == sizeof...(Children2)>,
        boost::mpl::all_of<
            boost::mpl::zip_view<
                boost::mpl::vector<
                    boost::mpl::vector<Children1...>,
                    boost::mpl::vector<Children2...>
                >
            >,
            boost::mpl::unpack_args<matches<boost::mpl::_1, boost::mpl::_2> >
        >
    >
{ };

//! Matches anything.
struct _;
template <typename T>
struct matches<T, _>
    : boost::mpl::true_
{ };

//! Matches if any of the patterns match.
template <typename ...> struct or_;
template <typename T, typename ...Patterns>
struct matches<T, or_<Patterns...> >
    : boost::mpl::any_of<
        boost::mpl::vector<Patterns...>,
        matches<T, boost::mpl::_1>
    >
{ };

//! Matches if all of the patterns match.
template <typename ...> struct and_;
template <typename T, typename ...Patterns>
struct matches<T, and_<Patterns...> >
    : boost::mpl::all_of<
        boost::mpl::vector<Patterns...>,
        matches<T, boost::mpl::_1>
    >
{ };

//! Matches iff the pattern does not match.
template <typename> struct not_;
template <typename T, typename Pattern>
struct matches<T, not_<Pattern> >
    : boost::mpl::not_<matches<T, Pattern> >
{ };

namespace matches_detail {
    template <typename ...Patterns>
    struct match_any_of {
        template <typename T>
        struct apply
            : boost::mpl::any_of<
                boost::mpl::vector<Patterns...>,
                matches<T, boost::mpl::_1>
            >
        { };
    };
}

//! Matches if all of the children of the node matches any of the patterns.
template <template <typename ...> class Node, typename ...Children, typename ...Patterns>
struct matches<Node<Children...>, Node<and_<Patterns...> > >
    : boost::mpl::all_of<
        boost::mpl::vector<Children...>,
        matches_detail::match_any_of<Patterns...>
    >
{ };
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
    template <typename Event>
    struct convertible_if_matches {
        template <typename Pattern, typename =
            typename boost::enable_if<matches<Event, Pattern> >::type>
        operator Pattern() const { return Pattern(); }
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

#endif // !SANDBOX_DYNO_V3_HPP
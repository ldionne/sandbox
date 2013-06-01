
#ifndef SANDBOX_DYNO_V3_HPP
#define SANDBOX_DYNO_V3_HPP

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
 * Related events are categorized into `Domain`s. When creating a new _family_
 * of events, it is a good idea to create a new domain for those events.
 * Events are aware of their domain, but the converse is not true.
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
// Listener concept
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
/*!
 * Specification of the `Listener` concept.
 *
 * A listener is a function object called whenever an event is generated in
 * one of the domains it is a member of.
 *
 * The first parameter must be a model of the `Event` concept. Pattern
 * matching may be used when declaring the parameter. That pattern must
 * be matched by an `Event` in order for this overload of `operator()` to
 * be picked. See `matches` for more information on patterns.
 *
 * The second parameter must be a Boost.MPL `Sequence` of types representing
 * the minimal set of keys that must be present in the environment in order
 * for this overload to be picked.
 *
 * The third parameter is an unspecified type modeling the Boost.Fusion
 * `AssociativeSequence` concept. When the listener is called, this will
 * be a compile-time map containing all of the environment variables
 * accessible to the listener. This environment is shared by all the
 * listeners of a domain.
 *
 *
 * ## Notation
 * | Expression    | Description
 * | ----------    | -----------
 * | `L`           | A type modeling the `Listener` concept
 * | `listener`    | An instance of type `L`
 * | `Event`       | A type convertible to any type matching some model of the `Event` concept
 * | `MinimalKeys` | A type convertible to any `Sequence` whose elements are all keys in the `Environment`
 * | `Environment` | A type modeling the Boost.Fusion `AssociativeSequence` concept
 * | `env`         | An instance of type `Environment`
 *
 *
 * ## Valid expressions
 * | Expression                              | Return type | Semantics
 * | ----------                              | ----------- | ---------
 * | `listener(Event(), MinimalKeys(), env)` | `void`      | Notify the listener that an event has occured.
 *
 *
 * @tparam L
 *         The type to be tested for modeling of the `Listener` concept.
 */
template <typename L>
struct Listener;
} // end namespace dyno



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
// minimal_env
//////////////////////////////////////////////////////////////////////////////
#include <boost/mpl/vector.hpp>

namespace dyno {
    template <typename ...Keys>
    using minimal_env = boost::mpl::vector<Keys...>;
}



//////////////////////////////////////////////////////////////////////////////
// generate
//////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/include/has_key.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/copy_if.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/utility/enable_if.hpp>
#include <dyno/detail/mpl_extensions.hpp>
#include <utility>

namespace dyno {
namespace generate_detail {
    namespace fsn = boost::fusion;
    namespace mpl = boost::mpl;

    template <typename Condition>
    struct convertible_if {
        template <typename T, typename = typename boost::enable_if<
            typename mpl::apply<typename mpl::lambda<Condition>::type, T>::type
        >::type>
        operator T() const { return T(); }
    };

    template <typename Listener, typename Event, typename Environment, typename Key>
    class uses_key {
        template <typename K>
        static mpl::true_ test(decltype(
            std::declval<Listener>()(
                convertible_if<matches<Event, mpl::_1> >(),
                convertible_if<fsn::result_of::has_key<mpl::_1, K> >(),
                std::declval<Environment>()
        ))*);

        template <typename K> static mpl::false_ test(...);

    public:
        typedef decltype(test<Key>(0)) type;
    };

    // For optimization purposes, it might be interesting to know what is
    // the subset of the environment that is actually used by the listener.
    template <typename Listener, typename Event, typename Environment>
    struct deduce_used_keys
        : mpl::copy_if<
            typename mpl::keys<Environment>::type,
            uses_key<Listener, Event, Environment, mpl::_1>
        >
    { };

    template <typename Sequence>
    struct pointers_to
        : mpl::transform<
            Sequence,
            boost::add_pointer<mpl::_1>,
            mpl::back_inserter<mpl::vector<> >
        >
    { };

    template <typename Event, typename Environment>
    struct call_listener {
        typedef typename mpl::lambda<
            fsn::result_of::has_key<Environment, mpl::_1>
        >::type EnvironmentHasKey;

        Environment& shared_env;
        template <typename Listener>
        void operator()(Listener*) {
            instance_of<Listener>()(
                convertible_if<matches<Event, mpl::_1> >(),
                convertible_if<mpl::all_of<mpl::_1, EnvironmentHasKey> >(),
                shared_env);
        }
    };
} // end namespace generate_detail

/*!
 *
 */
template <typename Event, typename Environment>
void generate(Environment env) {
    typedef typename domain_of<Event>::type Domain;
    typedef typename generate_detail::pointers_to<
        typename Domain::static_listeners
    >::type ListenersWithoutInstantiation;

    generate_detail::call_listener<Event, Environment> call = {env};
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

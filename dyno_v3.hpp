
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
// Environment concept
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
/*!
 * Specification of the `Environment` concept.
 *
 * An environment ...
 *
 *
 * ## Notation
 * | Expression | Description
 * | ---------- | -----------
 * | `Env`      | A type modeling the `Environment` concept
 * | `env`      | An instance of type `Env`
 * | `ext`      | An arbitrary instance of a model of the Boost.Fusion `AssociativeSequence` concept
 * | `var`      | An instance of a type modeling the `EnvironmentVariable` concept
 *
 *
 * ## Valid expressions
 * | Expression        | Return type                                               | Semantics
 * | ----------        | -----------                                               | ---------
 * | `Env env(ext)`    |                                                           | Create an environment whose `externals()` method returns an `AssociativeSequence` with the same content as `ext`.
 * | `env[var]`        | Any type                                                  | `Var::get(env)`, where `Var` is the type of `var`.
 * | `env.externals()` | A model of the Boost.Fusion `AssociativeSequence` concept | Returns the set of `(key, value)` pairs with which the environment was initially created.
 *
 *
 * @tparam Env
 *         The type to be tested for modeling of the `Environment` concept.
 */
template <typename Env>
struct Environment;
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// EnvironmentVariable concept
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
/*!
 * Specification of the `EnvironmentVariable` concept.
 *
 * An environment variable ...
 *
 *
 * ## Notation
 * | Expression | Description
 * | ---------- | -----------
 * | `Var`      | A type modeling the `EnvironmentVariable` concept
 * | `Env`      | A type modeling the `Environment` concept
 * | `env`      | An instance of type `Env`
 *
 *
 * ## Valid expressions
 * | Expression          | Return type | Semantics
 * | ----------          | ----------- | ---------
 * | `Var::get(env)`     | Any type    | Retrieve the value associated to `Var` in `env`.
 *
 *
 * @tparam Var
 *         The type to be tested for modeling of the `EnvironmentVariable`
 *         concept.
 */
template <typename Var>
struct EnvironmentVariable;
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// environment
//////////////////////////////////////////////////////////////////////////////
#include <utility>

namespace dyno {
template <typename Externals>
struct environment {
    explicit environment(Externals const& ext) : externals_(ext) { }
    explicit environment(Externals&& ext) : externals_(std::move(ext)) { }

    template <typename Var>
    decltype(Var::get(std::declval<environment>())) operator[](Var)
    { return Var::get(*this); }

    template <typename Var>
    decltype(Var::get(std::declval<environment const>())) operator[](Var) const
    { return Var::get(*this); }

    Externals const& externals() const
    { return externals_; }

private:
    Externals const externals_;
};
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// external_env_var
//////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/include/at_key.hpp>

namespace dyno {
    template <typename Key>
    struct external_env_var {
        template <typename Environment>
        static auto get(Environment& env)
        -> decltype(boost::fusion::at_key<Key>(env.externals())) {
            return boost::fusion::at_key<Key>(env.externals());
        }
    };
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
struct _ { };
template <typename T>
struct matches<T, _>
    : boost::mpl::true_
{ };

//! Matches if any of the patterns match.
template <typename ...> struct or_ { };
template <typename T, typename ...Patterns>
struct matches<T, or_<Patterns...> >
    : boost::mpl::any_of<
        boost::mpl::vector<Patterns...>,
        matches<T, boost::mpl::_1>
    >
{ };

//! Matches if all of the patterns match.
template <typename ...> struct and_ { };
template <typename T, typename ...Patterns>
struct matches<T, and_<Patterns...> >
    : boost::mpl::all_of<
        boost::mpl::vector<Patterns...>,
        matches<T, boost::mpl::_1>
    >
{ };

//! Matches iff the pattern does not match.
template <typename> struct not_ { };
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
 * Environment variable containing the `this` pointer of the object that
 * triggered the current event.
 */
struct _this : external_env_var<_this> { };

/*!
 * Environment variable containing the arguments passed to the function that
 * triggered the current event as a tuple.
 */
struct _args : external_env_var<_args> { };
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
#include <boost/fusion/include/as_map.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/has_key.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/always.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/copy_if.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>
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

    template <typename Listener, typename Event, typename Var>
    class uses_env_var {
        template <typename K>
        static mpl::true_ test(decltype(
            std::declval<Listener>()(
                convertible_if<matches<Event, mpl::_1> >(),
                convertible_if<mpl::contains<mpl::_1, K> >(),
                convertible_if<mpl::always<mpl::true_> >()
        ))*);

        template <typename K> static mpl::false_ test(...);

    public:
        typedef decltype(test<Var>(0)) type;
    };

    template <typename Sequence>
    struct pointers_to
        : mpl::transform<
            Sequence,
            boost::add_pointer<mpl::_1>,
            mpl::back_inserter<mpl::vector<> >
        >
    { };

    struct push_back_env_var {
        template <typename Sig> struct result;

        template <typename This, typename Environment, typename Var>
        struct result<This(Environment&, Var)>
            : fsn::result_of::as_map<
                typename fsn::result_of::push_back<
                    Environment,
                    typename fsn::result_of::make_pair<
                        Var, typename Var::type
                    >::type
                >::type
            >
        { };

        template <typename Environment, typename Var>
        typename result<push_back_env_var(Environment&, Var)>::type
        operator()(Environment& env, Var) const {
            return fsn::as_map(fsn::push_back(env, fsn::make_pair<Var>(Var::retrieve_from(env))));
        }
    };

    template <typename Event, typename Environment>
    struct trigger_listener {
        Environment& shared_env;

        template <typename Listener>
        void operator()(Listener*) {
            typedef typename mpl::lambda<
                fsn::result_of::has_key<Environment, mpl::_1>
            >::type EnvironmentHasKey;

            instance_of<Listener>()(
                convertible_if<matches<Event, mpl::_1> >(),
                convertible_if<mpl::all_of<mpl::_1, EnvironmentHasKey> >(),
                augmented_env);
        }
    };
} // end namespace generate_detail

/*!
 *
 */
template <typename Event, typename Externals>
void generate(Externals&& ext) {
    typedef typename domain_of<Event>::type Domain;
    typedef typename generate_detail::pointers_to<
        typename Domain::static_listeners
    >::type ListenersWithoutInstantiation;

    typedef environment<
        typename boost::remove_reference<Externals>::type
    > Environment;

    Environment env(std::forward<Externals>(ext));
    generate_detail::trigger_listener<Event, Environment> trigger = {env};
    boost::mpl::for_each<ListenersWithoutInstantiation>(trigger);
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

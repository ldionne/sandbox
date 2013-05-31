
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
// Spy concept
//////////////////////////////////////////////////////////////////////////////
namespace dyno {
/*!
 * Specification of the `Spy` concept.
 *
 * A spy is a small class tweaking some aspect of another class. Besides
 * the targeted aspect, a spied-on class should be indistinguishable from
 * the original class.
 *
 * A spied-on class is created by using layered inheritance. The general
 * layering goes like this:
 * `topmost facade | one or more spies | basemost facade`
 *
 * The roles of `basemost facade` and `topmost facade` are to provide,
 * respectively hide the `facade()` protected method so that it can be
 * used exclusively by the spies in between. The `facade()` protected
 * method returns a reference to the most derived class available when
 * creating the spied-on type.
 *
 * When creating spies, one should be _very_ careful about correctly
 * forwarding the constructors and handling the additional state when
 * applicable.
 *
 * @warning
 * This way of spying may not be suitable for classes used in a polymorphic
 * manner. Be warry of object slicing!
 *
 *
 * ## Notation
 * | Expression      | Description
 * | ----------      | -----------
 * | `S`             | A type modeling the `Spy` concept
 * | `Facade, Spied` | Arbitrary types
 *
 *
 * ## Valid expressions
 * | Expression                | Return type | Semantics
 * | ----------                | ----------- | ---------
 * | `S::apply<Facade, Spied>` | Any type    | Returns a type deriving publicly from `Spied` tweaking an aspect of `Spied`. `Facade` is the derived class
 *
 *
 * @tparam S
 *         The type to be tested for modeling of the `Spy` concept.
 */
template <typename S>
struct Spy;
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// spy_as_mixin and spy_as_wrapper
//////////////////////////////////////////////////////////////////////////////
#include <boost/mpl/bind.hpp>
#include <boost/mpl/empty_base.hpp>
#include <boost/mpl/inherit_linearly.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/vector.hpp>

namespace dyno {
namespace spy_detail {
    /*!
     * @internal
     * Wrapper around `boost::mpl::inherit_linearly` to make the usage more
     * intuitive.
     *
     * Each base is formed by applying `Op` to the rest of the chain and the
     * current base. The rest of the chain (first parameter to `Op`) should
     * be inherited.
     *
     * Bases are inherited from left to right. The achieved effect is the
     * following:
     * @code
     *  struct Bases[0] : Bases[1] { };
     *  struct Bases[1] : Bases[2] { };
     *  ...
     *  struct Bases[last] : Basemost { };
     * @endcode
     */
    template <typename Op, typename Bases,
              typename Basemost = boost::mpl::empty_base>
    struct inherit_linearly
        : boost::mpl::inherit_linearly<
            typename boost::mpl::reverse<Bases>::type, Op, Basemost
        >
    { };

    /*!
     *
     */
    struct basemost_spy {
        template <typename Facade, typename Spied>
        struct apply : Spied {
            DYNO_INHERIT_CONSTRUCTORS(apply, Spied)

        protected:
            Facade& facade()
            { return static_cast<Facade&>(*this); }

            Facade const& facade() const
            { return static_cast<Facade const&>(*this); }
        };
    };

    /*!
     *
     */
    struct topmost_spy {
        template <typename Facade, typename Spied>
        struct apply : Spied {
            DYNO_INHERIT_CONSTRUCTORS(apply, Spied)

        protected:
            Facade& facade() = delete;
            Facade const& facade() const = delete;
        };
    };

    template <typename Facade>
    struct make_spy {
        template <typename Next, typename Spy>
        struct apply {
            typedef typename Spy::template apply<Facade, Next> type;
        };
    };

    /*!
     *
     */
    template <typename Facade, typename Basemost, typename ...Spies>
    struct make_spy_chain
        : inherit_linearly<
            make_spy<Facade>,
            boost::mpl::vector<topmost_spy, Spies..., basemost_spy>,
            Basemost
        >
    { };
} // end namespace spy_detail

/*!
 *
 */
template <typename Derived, typename ...Spies>
struct spy_as_mixin
    : spy_detail::make_spy_chain<
        Derived, boost::mpl::empty_base, Spies...
    >::type
{ };

/*!
 *
 */
template <typename Wrapped, typename ...Spies>
struct spy_as_wrapper
    : spy_detail::make_spy_chain<
        spy_as_wrapper<Wrapped, Spies...>, Wrapped, Spies...
    >::type
{
private:
    typedef typename spy_detail::make_spy_chain<
        spy_as_wrapper<Wrapped, Spies...>, Wrapped, Spies...
    >::type Base;

public:
    DYNO_INHERIT_CONSTRUCTORS(spy_as_wrapper, Base)
};
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// generate
//////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/map.hpp>
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
        void operator()(Listener const& listener) const {
            listener(convertible_if_matches<Event>(), env_);
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
    boost::fusion::for_each(Domain::static_listeners(), call);
    // boost::for_each(Domain::dynamic_listeners, call); // not ready yet
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
 * A domain regroups listeners interested into being notified when any event
 * from a set of events is triggered.
 *
 *
 * ## Notation
 * | Expression | Description
 * | ---------- | -----------
 * | `D`        | A type modeling the `Domain` concept
 *
 *
 * ## Valid expressions
 * | Expression              | Return type                                                        | Semantics
 * | ----------              | -----------                                                        | ---------
 * | `D::static_listeners()` | A Boost.Fusion `Sequence` of types modeling the `Listener` concept | Returns the set of listeners that were registered to that domain at compile-time.
 *
 *
 * @tparam D
 *         The type to be tested for modeling of the `Domain` concept.
 */
template <typename D>
struct Domain;
} // end namespace dyno



//////////////////////////////////////////////////////////////////////////////
// domain default implementation
//////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/include/vector.hpp>

namespace dyno {
/*!
 *
 */
template <typename ...Listeners>
struct domain {
    static boost::fusion::vector<Listeners&...> static_listeners() {
        return boost::fusion::vector<Listeners&...>(
                                        instance_of<Listeners>()...);
    }
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

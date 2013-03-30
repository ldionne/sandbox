
#if 0
namespace custom {

// Generate analysis rules.
typedef analysis_rules<
            when<acquire_event,
                add_edge_in_lock_graph
            >,
            when<start_event,
                add_edge_in_segmentation_graph
            >
        > my_analysis_rules;

typedef set<acquire_event, release_event, start_event, join_event> my_events;

// Generate a dynamic analysis framework for grouping a set of events to
// our dispatch rules. We need the set of events to be specified if we
// want to use variant to serialize the events. I think this is a good
// compromise between ease of extension and ease of implementation.
typedef framework<
            my_events, my_dispatch_rules, my_analysis_rules
        > my_framework;
}

static custom::my_framework d2;

int main() {
    enable(d2, "foobar");
        acquire_event acquire;
        dispatch(d2, acquire);
    disable(d2);

    // these are ignored because the framework is disabled
    acquire_event acquire2;
    dispatch(d2, acquire2);

    // compile-time error because foobar_event is not in the framework's domain
    typedef event<...> foobar_event;
    foobar_event foo;
    dispatch(d2, foo);

    // re-enable the framework with the last settings
    enable(d2);
}

struct map_by_thread_id {
    template <typename Sig>
    struct result;

    template <typename This, typename Event, typename State, typename Data>
    struct result<This(Event&, State&, Data&)> {
        typedef typename boost::remove_reference<
                    typename boost::proto::result_of::env_var<
                        Data, boost::proto::data_type
                    >::type
                >::type Bundle;
        typedef typename Bundle::map_type::mapped_type& type;
    };

    template <typename Event, typename State, typename Data>
    typename result<map_by_thread_id(Event const&, State const&, Data&)>::type
    operator()(Event const& event, State const&, Data& data) const {
        typedef typename result<
                    map_by_thread_id(Event const&, State const&, Data&)
                >::Bundle Bundle;
        typedef typename Bundle::mutex_type Mutex;

        d2::detail::scoped_lock<Mutex> lock(data[boost::proto::data].mutex);
        return data[boost::proto::data].map[thread_of(event)];
    }
};

template <typename ExtractMapFromData = boost::multi_index::identity,
          typename ExtractEventKey = boost::multi_index::identity>
struct map_factory {
    template <typename Sig>
    struct result;

    template <typename This, typename Event, typename State, typename Data>
    struct result<This(Event&, State&, Data&)> {
        typedef typename boost::remove_reference<
                    typename boost::result_of<ExtractMapFromData(Data&)>::type
                >::type Map;
        typedef typename Map::mapped_type& type;
    };

    template <typename Event, typename State, typename Data>
    typename result<map_factory(Event const&, State const&, Data const&)>::type
    operator()(Event const& event, State const&, Data const& data) const {
        return ExtractMapFromData()(data)[ExtractEventKey()(event)];
    }

    template <typename Event, typename State, typename Data>
    typename result<map_factory(Event const&, State const&, Data&)>::type
    operator()(Event const& event, State const&, Data& data) const {
        return ExtractMapFromData()(data)[ExtractEventKey()(event)];
    }
};

struct Bundle {
    typedef d2::detail::basic_mutex mutex_type;
    mutex_type mutex;

    struct lockable_ofstream {
        std::ofstream stream;
        mutex_type mutex;

        template <typename T>
        friend lockable_ofstream& operator<<(lockable_ofstream& self, T const& t) {
            d2::detail::scoped_lock<mutex_type> lock(self.mutex);
            self.stream << t;
            return self;
        }
    };

    typedef boost::unordered_map<d2::ThreadId, lockable_ofstream> map_type;
    map_type map;
};

struct lock_stream {
    d2::detail::basic_mutex& mutex_;

    template <typename Event, typename State, typename Data>
    lock_stream(Event const&, State const&, Data& env)
                                            : mutex_(env[d2::stream_].mutex) {
        mutex_.lock();
    }

    ~lock_stream() { mutex_.unlock(); }
};
#endif


#include <d2/detail/basic_mutex.hpp>
#include <d2/sandbox/dispatch_policy.hpp>
#include <d2/sandbox/dispatch_rules.hpp>
#include <d2/sandbox/event.hpp>
#include <d2/sandbox/event_traits.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/functional.hpp>
#include <boost/fusion/iterator.hpp>
#include <boost/fusion/support.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/fusion/view.hpp>


struct null_factory {
    struct result_type {
        template <typename T>
        friend void operator<<(result_type const&, T const&) {
            std::cout << "yup";
        }
    };

    template <typename Event, typename State, typename Data>
    result_type operator()(Event const&, State const&, Data const&) const {
        return result_type();
    }

    template <typename Event>
    result_type operator()(Event const&) const {
        return result_type();
    }
};

namespace tag {
    struct thread { };
    struct lock { };
    struct parent_segment { };
    struct new_parent_segment { };
    struct child_segment { };
}

struct acquire_event
    : d2::event<
        d2::scope<d2::thread_scope>,
        d2::members<
            tag::thread, unsigned long,
            tag::lock, unsigned long
        >
    >
{ };

struct release_event : acquire_event { };

struct start_event
    : d2::event<
        d2::scope<d2::process_scope>,
        d2::members<
            tag::parent_segment, unsigned long,
            tag::new_parent_segment, unsigned long,
            tag::child_segment, unsigned long
        >
    >
{ };

struct join_event : start_event { };

// Generate dispatch rules.
struct my_dispatch_rules
    : d2::dispatch_rules<
        d2::when<
            d2::if_<d2::has_thread_scope<d2::_event>()>,

            d2::dispatch_policy<
                d2::stream_factory<null_factory>
            >(d2::_event)
        >,
        d2::when<
            d2::if_<d2::has_process_scope<d2::_event>()>,

            d2::dispatch_policy<
                d2::stream_factory<null_factory>
            >(d2::_event)
        >
    >
{ };

struct my_load_rules
    : d2::event_processor< // isnt it the same thing as dispatch_rules?
        d2::when<
            d2::matches<d2::_event, start_event>,
            process_start_event(d2::_event, d2::_data)
        >,
        d2::otherwise<
            d2::call<some_visitor>
        >
    >
{ };


// Avoir des "attributs" qui vont chercher de l'information spécifique. Par
// exemple, un event peut avoir l'attribut "thread", auquel cas il va
// automatiquement connaître la thread dans laquelle il est généré (ça peut
// s'implémenter de plusieurs manières).

// Ça prend un module qui permet de spécifier des dispatch rules génériques
// qui n'ont pas rapport avec le loading ou le saving d'events. Ça, ça se
// trouve à être un petit wrapper par dessus proto qui permet d'utiliser
// du vocabulaire d'analyse dynamique plutôt que de traversal d'expression
// template. En essence, c'est la même chose. Le dispatcher call simplement
// un foncteur lorsqu'il a terminé de dispatcher.


namespace tag {
    struct acquire;
    struct release;

    struct segment {
        typedef unsigned type;
    };
    struct other_info;
}

namespace events {
    typedef dyno::event<
                // tag identifying the event type
                tag::acquire,

                // segment::type is used automatically as the type of the attribute
                dyno::attribute<tag::segment, unsigned, dyno::provided>,

                // the other_info type is associated to an attribute of type
                // std::string for this event
                dyno::attribute<tag::other_info, std::string, dyno::provided>

            > acquire;
}

// functor with a static_visitor-like interface
struct lock_graph_builder {
    // The real Event type is not known, so it is easy to change the
    // implementation. The event is only manipulated through its attribute tags.
    // Internal note: It is not excluded that the `Event` type actually
    // holds a reference to the framework that generated it, so that it
    // can ask for information there. That's clever.
    template <typename Event>
    void operator()(tag::acquire, Event const& event) const {
        dyno::getattr<tag::segment>(event);
        dyno::hasattr<tag::segment>(event);
    }
};


namespace d2 {
    // type of the backend that will be used for d2.
    // D2SpecificMapping is a functor that will dispatch event tags on
    // the filesystem. it will have access to the event's information.
    typedef dyno::filesystem<D2SpecificMapping> filesystem;

    typedef dyno::dynamic_analysis_framework<
                // list of events created with dyno::event<>
                dyno::events<events::acquire>,

                // complicated part:
                // the backend must be specified. are there generic ways of
                // thinking about a backend, or will it always have to be
                // implementation specific? For now, let's say we have
                // something very implementation specific, to simplify the
                // things.
                dyno::backend<filesystem>

            > framework;
}

int main() {
    d2::framework d2_framework;
    dyno::generate<tag::acquire>(d2_framework,
        dyno::init_attribute<tag::segment>(0),

            /* information used to construct the event, if required */);


    unsigned long t = get(tag::thread(), acquire);

    int state, data;
    dispatch(acquire, state, data);
    dispatch(start, state, data);

    my_load_rules load;
    load(some_repository);

    dyno::program<dyno::basic_filesystem_db> prog("path to repository");
    prog.threads();
    prog.threads()[0].segments();
    for (auto event : prog.threads()[0].events()) {
        load(event); // constructs the graph
    }
}

// clang++ -std=c++98 -Wall -Wextra -pedantic -I ~/Documents/Ordi/boost-trunk -I ~/Documents/Ordi/d2/include ~/Desktop/brainstorm_d2.cpp -o ~/Desktop/brainstorm


template <typename Key, typename Value>
typename boost::enable_if<
    boost::mpl::or_<
        has_key_value_enabled<typename boost::remove_reference<Key>::type>,
        has_key_value_enabled<typename boost::remove_reference<Value>::type>
    >,
boost::tuple<BOOST_FWD_REF(Key), BOOST_FWD_REF(Value)> >::type
operator=>(BOOST_FWD_REF(Key) key, BOOST_FWD_REF(Value) value) {
    return boost::forward_as_tuple(boost::forward<Key>(key),
                                   boost::forward<Value>(value));
}

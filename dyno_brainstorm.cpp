
namespace tag {
    struct acquire;
    struct release;
}

typedef dyno::event<
            dyno::tagged_as<tag::acquire>,
            dyno::computation<
                dyno::tagged_as<tag::thread_id>
            >
        > acquire;

dyno::optional<
    dyno::computation<
        dyno::tagged_as<tag::optional_tag>,
        dyno::type<maybe_void>,
        dyno::computed<how>
    >
>

dyno::generate<tag::my_event>(D2_FRAMEWORK,
    dyno::omit<tag::optional_tag>(),
    dyno::mapto<tag::other_tag>(args));

typedef dyno::event<
            dyno::tagged_as<tag::acquire>,

            dyno::computes<dyno::call_stack>,
            dyno::computes<dyno::serialized_execution_block>,
            dyno::computes<dyno::thread_id>,
            dyno::computes<
                dyno::optional<
                    dyno::computation<
                        dyno::tagged_as<tag::info_string>,
                        dyno::type<std::string>,
                        dyno::already_computed
                    >
                >
            >,
            dyno::records<
                dyno::attribute<
                    tag::other_info_string, std::string, dyno::computed<dyno::lazily>
                >
            >,

            dyno::in_parallel<
                dyno::in_order<
                    dyno::records<
                        dyno::attribute<
                            tag::system_state_string, std::string, dyno::computed<dyno::strictly>
                        >
                    >,
                    dyno::triggers<
                        some_action
                    >
                >,

                dyno::triggers<
                    // within a triggers<> block, stuff is by default executed
                    // in order, so we have to explicitly make it parallel.
                    dyno::in_parallel<
                        print_stuff_to_a_stream,
                        do_something_unrelated
                    >
                >
            >
        > acquire;


// No need to repeat the whole definition
// When the second argument is used, tagged_as becomes the definition
// of an event. As such, using this form would trigger a compilation
// error when defining an event like above:
//  dyno::event<
//      dyno::tagged_as<tag::release, acquire>, // <-- compilation error
//      dyno::records<...>,
//  >
typedef dyno::tagged_as<tag::release, acquire> release;


struct my_backend {
    thread_local std::ofstream per_thread_events;

    mutex start_and_join_events_mutex;
    std::ofstream start_and_join_events;

    // Now, this is very specific to our application, but we know that we only
    // need to load acquire and release events thread by thread to build the
    // lock graph correctly. In other words, we don't have to order the
    // acquires and releases between different threads. Thus, it is
    // unnecessary to save all the acquire and release events to the same
    // file. Since we save acquire and release events per-thread, we don't
    // need to synchronize the file because it is unique to this thread.
    template <typename Event>
    void save(tag::acquire, Event const& e) {
        typedef typename event_tag<Event>::type event_tag; // == tag::acquire
        typedef typename event_attribute<Event, tag::info_string>::type info_string;
        // etc...

        if (!per_thread_events) {
            per_thread_events.reset(new std::ofstream("file for this thread"));
        }
        *per_thread_events << make_variant(e); // not serialized for simplicity
    }

    // As for the start and join events, we have to load them all at once
    // in the segmentation graph. For this reason, we will simply save them
    // in a file shared by all the threads and use conventional synchronization.
    template <typename Event>
    void save(tag::start, Event const& e) {
        boost::lock_guard<mutex> lock(start_and_join_events_mutex);
        start_and_join_events << make_variant(e); // not serialized for simplicity
    }
};


// When all the events and the backend are defined, we can create a framework.
typedef dyno::framework<
            dyno::events<acquire, release>,
            dyno::backend<my_backend>
        > d2_framework;


//////////////////////// deep in a .cpp file //////////////////////////
namespace d2 {
    void trackable_sync_object::notify_acquire() const {
        dyno::generate<tag::acquire>(
            D2_FRAMEWORK,
            dyno::mapto<tag::lock_id>(this->id_),
            dyno::omit<dyno::call_stack>());
    }
}
//////////////////////////////////////////////////////////////



///////////////// begin header distributed to users ///////////////////
namespace d2 {
    template <typename Lockable>
    struct lockable : trackable_sync_object {
        void lock() {
            Lockable::lock();
            this->notify_acquire();
        }
    };
}
/////////////// end header distributed to users ////////////////////








/*********** COULD THIS BE USEFUL OR IS IT JUST OUT OF LINE? **************/

// Any object can be tracked by dyno. Since the only thing shared by all
// objects is creation and destruction, they are the only lifetime points
// that can reasonably be defined by dyno.
//
// Arbitrary custom events can be attached to the entity.
// This allows to typecheck that the only events that will be generated by an
// entity are those supported by that entity.
//
// Contrarily to the above events, these entities must have some kind of
typedef dyno::tracked_entity<
            dyno::on_creation<
                dyno::record<dyno::call_stack>
            >,
            dyno::on_destruction<
                dyno::do_nothing
            >,
            dyno::generates<
                acquire, release
            >
        > tracked_lock;






/***************************************************************************/
#define DYNO_VARIADIC_FUNCTION_TEMPLATE_ARG(Arg) BOOST_FWD_REF(Arg)
#define DYNO_VARIADIC_FUNCTION_TEMPLATE(typename_Args, arglist)             \
    template <typename Tag, typename Framework typename_Args>               \
    void generate(arglist) {                                                \
        ::boost::forward<Args>(args)                                                                    \
    }                                                                       \
/**/

#include <dyno/detail/variadic_templates.hpp>

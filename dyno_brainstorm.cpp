

// Create an event. It is identified by a tag and that is the only way to
// interact with the event, because the actual implementation of the event
// is completely unspecified. The actual type of the event will be generated
// when the framework is generated, at the end.
//
// We specify information to be recorded by the event when it is triggered.
// There are some predefined actions provided by dyno, such as recording the
// call stack.
//
// It is also possible to define arbitrary custom information to be gathered
// when the event is triggered. This information can be gathered by several
// different means:
//  - It can be provided when the event is generated, by passing it to
//    the dyno::generate front-end function. It is possible to mark such an
//    attribute as 'optional'. If it is optional, it does not have to be
//    provided when the event is generated.
//
//  - It can be computed lazily using a function. It will only be computed
//    when the information is accessed within the event. It is possible to
//    specify whether the computed attribute should be cached or not, i.e.
//    whether it should be recomputed everytime it is accessed. Additional
//    information from the framework may be provided to the function when
//    it is called.
//
//  - It can be computed strictly using a function. It will be computed once
//    by calling the function when the event is generated. This is equivalent
//    to providing a non-optional attribute that is the result of calling the
//    function with the same arguments. However, it can be more convenient to
//    use this form because additional information from the framework may be
//    made available to the function.
//
// Arbitrary custom actions (functors) can be triggered when the event is
// generated. This is different from attributes in the sense that the result
// of the action is discarded, as if it were `void`.
//
// All the described actions, whether it be strictly computing an attribute
// or executing an arbitrary action, are executed in their order of appearance.
// This does not hold for lazily computed attribtues because we have no way
// to know when, or if, they will be computed. This also does not hold for
// provided attributes because they are already computed anyway.
//
// It is also possible to specify which groups could be executed in parallel,
// but the framework is not required to follow that recommendation.
// However, if a group of actions is specified to be executed serially, this
// indication is guaranteed to be respected.
typedef dyno::event<
            dyno::tagged_as<tag::acquire>,

            dyno::records<dyno::call_stack>,
            dyno::records<dyno::serialized_execution_block>,
            dyno::records<dyno::thread_id>,
            dyno::records<
                dyno::optional<
                    dyno::attribute<
                        tag::info_string, std::string, dyno::provided
                    >
                >
            >,
            dyno::records<
                dyno::attribute<
                    tag::other_info_string, std::string, dyno::computed<dyno::lazily>
                >
            >,

            dyno::in_parallel<
                dyno::serial<
                    dyno::records<
                        // we could also say dyno::strictly_computed_attribute, or
                        // dyno::computed_attribute<..., ..., dyno::strictly>
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
                    // in serie, so we have to explicitly make it parallel.
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
            my_backend
        > d2_framework;





//////////////////////// deep in a .cpp file //////////////////////////
namespace d2 {
    void trackable_sync_object::notify_acquire() const {
        dyno::generate<tag::acquire>(D2_FRAMEWORK, this->id_, other info);
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

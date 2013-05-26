
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



// note: generate() should not have to deal with events. dealing with
//       arbitrary taggables instead of events should do the trick, yet
//       it makes the concept requirements smaller. also, arbitrary types
//       should do the trick for representing the tags of computations.
//       actually, generate() should not be aware that an event will
//       perform computations.

// note2: We should maybe generalize computations so that an event does
//        not have to 'hold' computations, which makes no sense. i think
//        something like 'Information' is a concept that would be more
//        suited. Then, we can have Information that performs a computation
//        in order to be obtained, and information that is externally
//        provided, which makes more sense. We could get rid of the
//        not-quite-right `externally` evaluation policy. However, we
//        should __not__ call the concept Information, which is WAYY
//        too general to mean something. I am sure there is a name that
//        represents exactly what I am thinking about. self-note: Data is
//        not any better than Information.




typedef dyno::event<
    tagged_as<tag::my_event>,
    trigger<
        some_action(tag_of_info_to_pass, other_tag,
                    dyno::special_tag_representing_the_framework_like_in_proto,
                    dyno::special_tag_representing_all_the_args_like_in_proto)
    >
> my_event;


typedef dyno::framework<
            events<my_event>
        > my_framework1;

typedef dyno::bind_to<
            tag::my_event, this_action(dyno::whole_event)
        > framework_with_additional_action_triggered_when_my_event_is_generated;

dyno::enable_dynamic_binding_for<tag::my_event, framework> framework_with_dynamic_binding;

{
    dyno::framework_modifier fm(MY_FRAMEWORK);
    // modify it freely... gotta <3 RAII
}


/*!
 * Represents an event that can be generated by a framework.
 *
 * An event is identified by a tag provided with the `dyno::tagged_as`
 * protocol. This tag is the only way to identify that event type with
 * the framework.
 *
 * @note This is so because the actual implementation of the event is
 *       completely unspecified. The instantiation of the event's
 *       implementation will generally be delayed until later in the
 *       framework creation process, when enough information is known
 *       to generate the optimal event representation.
 *
 * The event definition also contains computations to be performed when the
 * event is generated. Computations are specified with `dyno::computation`
 * or any of the computation modifiers.
 *
 * By default, all computations are performed in their order of appearance in
 * the definition of the event, as if all the computations had been declared
 * within a `dyno::in_order` modifier. The same restrictions applying to
 * `dyno::in_order` applies within the top level of the event definition
 * as well.
 *
 * All the template parameters must be specified using named template
 * parameters. Providing a tag using `dyno::tagged_as` is mandatory.
 * Anything else is optional.
 *
 * It is an error to provide more than one computation with a given tag.
 *
 * @tparam `dyno::tagged_as<Tag>`
 *         A type used to uniquely identify that event among the other
 *         events of a framework.
 *
 * @tparam Computations...
 *         Zero or more computations to perform when the event is generated.
 */



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
        ::boost::forward<Args>(args)                                        \
    }                                                                       \
/**/

#include <dyno/detail/variadic_templates.hpp>






//////////////////////////////////////////////////////////////////////////////


struct event {
    context& ctx;

    ... getattr<tag>() {
        auto attribute = fusion_map.at_key<tag>(*this);
        return attribute(ctx);
    }
};


framework {

}



#include "traversal.hpp"
#include <boost/fusion/include/as_map.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/zip_view.hpp>
#include <boost/mpl/bind.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/inherit_linearly.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/utility/enable_if.hpp>
#include <dyno/model/easy_map.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>
#include <utility>


struct my_mutex;

#define RETURNS(...) -> decltype(__VA_ARGS__) { return __VA_ARGS__; }

// clang3.2 does not have inheriting constructors
#define INHERIT_CONSTRUCTORS(Type, Base)                                    \
    template <typename ...Args>                                             \
    Type(Args&& ...args) : Base(std::forward<Args>(args)...) { }            \
/**/

template <typename MemberFunctionPointer, MemberFunctionPointer mfp>
struct call_member_fn;

template <typename R, typename T, R T::*mfp>
struct call_member_fn<R T::*, mfp> {
    template <typename ...Args>
    auto operator()(T& t, Args&& ...args) const RETURNS(
        (t.*mfp)(std::forward<Args>(args)...)
    )

    template <typename ...Args>
    auto operator()(T const& t, Args&& ...args) const RETURNS(
        (t.*mfp)(std::forward<Args>(args)...)
    )

    template <typename ...Args>
    auto operator()(T&& t, Args&& ...args) const RETURNS(
        (std::move(t).*mfp)(std::forward<Args>(args)...)
    )
};


namespace d2 { struct goodlock_analysis; }


namespace dyno {
    typedef boost::mpl::vector<d2::goodlock_analysis> FRAMEWORKS;

    using boost::traverse::matches;
    using boost::traverse::_;

    template <typename Pattern>
    struct convertible_to_matches {
        template <typename T, typename = typename boost::enable_if<matches<Pattern, T> >::type>
        operator T() const { return T(); }
    };

    template <typename Event, typename Environment>
    struct call_it {
        Environment& env;

        template <typename Framework>
        void operator()(Framework const& framework) const {
            framework(convertible_to_matches<Event>(), env);
        }
    };

    template <typename Event, typename Environment>
    void generate(Environment const& env) {
        boost::mpl::for_each<FRAMEWORKS>(call_it<Event, Environment const>{env});
    }

    template <typename Event>
    void generate() {
        generate<Event>(boost::fusion::map<>());
    }


    //////////////////////////////////////////////////////////////////////////
    // keys.hpp (find another name)
    //////////////////////////////////////////////////////////////////////////
    struct _self; // the object (*this) that generated the event
    struct _args; // the arguments to the function that triggered the event (should map to a tuple or a fusion::map)


    //////////////////////////////////////////////////////////////////////////
    // program_states.hpp (i want these to be predefined points in the execution of a program where you can do stuff)
    //////////////////////////////////////////////////////////////////////////
    struct start_of_program { };
    struct end_of_program { };


    //////////////////////////////////////////////////////////////////////////
    // mixin_and_wrapper.hpp
    //////////////////////////////////////////////////////////////////////////
    template <typename Framework, typename Derived>
    struct state_for {
        struct type { };
    };

    struct base_tracker {
        template <typename Derived, typename Next>
        struct apply : Next {
            INHERIT_CONSTRUCTORS(apply, Next)

        protected:
            Derived& derived()
            { return static_cast<Derived&>(*this); }

            Derived const& derived() const
            { return static_cast<Derived const&>(*this); }

        private:
            typedef typename boost::fusion::result_of::as_vector<
                typename boost::fusion::result_of::transform<
                    FRAMEWORKS, state_for<boost::mpl::_1, Derived>
                >::type
            >::type States;
            States states;
            // typedef typename boost::fusion::result_of::as_map<
            //     boost::fusion::zip_view<boost::fusion::vector<FRAMEWORKS, States> >
            // >::type Framework_to_State;
            // Framework_to_State framework_to_state;
        };
    };

    template <typename Derived>
    struct form_tracker {
        template <typename Next, typename Tracker>
        struct apply {
            typedef typename Tracker::template apply<Derived, Next> type;
        };
    };

    template <typename Derived, typename Basemost, typename ...Trackers>
    struct inherit_trackers
        : boost::mpl::inherit_linearly<
            typename boost::mpl::reverse<
                boost::mpl::vector<Trackers..., base_tracker>
            >::type,
            form_tracker<Derived>,
            Basemost
        >
    { };

    template <typename Derived, typename ...Trackers>
    struct mixin
        : inherit_trackers<
            Derived, boost::mpl::empty_base, Trackers...
        >::type
    { };

    template <typename Wrapped, typename ...Trackers>
    struct wrapper
        : inherit_trackers<
            wrapper<Wrapped, Trackers...>, Wrapped, Trackers...
        >::type
    {
    private:
        typedef typename inherit_trackers<
                    wrapper<Wrapped, Trackers...>, Wrapped, Trackers...
                >::type Base;

    public:
        INHERIT_CONSTRUCTORS(wrapper, Base)
    };
} // end namespace dyno



namespace d2 {
    //////////////////////////////////////////////////////////////////////////
    // access.hpp
    //////////////////////////////////////////////////////////////////////////
    class access {
#define D2_I_ACCESS_USE(METHOD_NAME)                                        \
        struct use_ ## METHOD_NAME {                                        \
            template <typename T, typename ...Args>                         \
            auto operator()(T&& t, Args&& ...args) const RETURNS(           \
                std::forward<T>(t).METHOD_NAME(std::forward<Args>(args)...) \
            )                                                               \
        };                                                                  \
    /**/

    public:
        D2_I_ACCESS_USE(lock_impl)
        D2_I_ACCESS_USE(unlock_impl)
        D2_I_ACCESS_USE(try_lock_impl)
        D2_I_ACCESS_USE(join_impl)
        D2_I_ACCESS_USE(detach_impl)
#undef D2_I_ACCESS_USE
    };


    //////////////////////////////////////////////////////////////////////////
    // mutex_events.hpp
    //////////////////////////////////////////////////////////////////////////
    template <typename ...Characteristics> struct mutex;
    // characteristics of mutexes:
    template <typename> struct ownership; // how many threads can own the mutex at a time?
    struct none;
    struct shared;
    struct exclusive;
    struct upgradable; // this is functionally the same as shared, but only one thread may have it at a time and it is not possible for a thread to have it if a thread has exclusive ownership (and vice-versa)

    template <typename> struct recursiveness; // can the mutex be re-acquired by the same thread?
    struct non_recursive;
    struct recursive;


    template <typename ...Characteristics> struct mutex_operation { };
    // characteristics of all operations on mutexes:
    template <typename> struct synchronization_object;
    template <typename> struct previous_ownership;
    template <typename> struct new_ownership;


    //////////////////////////////////////////////////////////////////////////
    // basic_lockable.hpp
    //////////////////////////////////////////////////////////////////////////
    template <typename LockImplementation, typename UnlockImplementation>
    struct track_lock_unlock {
        template <typename Derived, typename Next>
        struct apply : Next {
            INHERIT_CONSTRUCTORS(apply, Next)

            void lock() {
                LockImplementation()(this->derived());
                dyno::generate<
                    mutex_operation<
                        previous_ownership<none>,
                        new_ownership<exclusive>,
                        synchronization_object<Derived>
                    >
                >(dyno::key<dyno::_self>() = boost::cref(this->derived()));
            }

            void unlock() {
                UnlockImplementation()(this->derived());
                dyno::generate<
                    mutex_operation<
                        previous_ownership<exclusive>,
                        new_ownership<none>,
                        synchronization_object<Derived>
                    >
                >(dyno::key<dyno::_self>() = boost::cref(this->derived()));
            }
        };
    };

    template <typename Derived, typename ...Trackers>
    using basic_lockable_mixin =
        dyno::mixin<
            Derived,
            track_lock_unlock<access::use_lock_impl, access::use_unlock_impl>,
            Trackers...
        >;

    template <typename BasicLockable, typename ...Trackers>
    using basic_lockable_wrapper =
        dyno::wrapper<
            BasicLockable,
            track_lock_unlock<
                call_member_fn<decltype(&BasicLockable::lock), &BasicLockable::lock>,
                call_member_fn<decltype(&BasicLockable::unlock), &BasicLockable::unlock>
            >,
            Trackers...
        >;


    //////////////////////////////////////////////////////////////////////////
    // lockable.hpp
    //////////////////////////////////////////////////////////////////////////
    template <typename TryLockImplementation>
    struct track_try_lock {
        template <typename Derived, typename Next>
        struct apply : Next {
            INHERIT_CONSTRUCTORS(apply, Next)

            bool try_lock() noexcept {
                return TryLockImplementation()(this->derived());
            }
        };
    };

    template <typename Derived, typename ...Trackers>
    using lockable_mixin =
        basic_lockable_mixin<
            Derived,
            track_try_lock<access::use_try_lock_impl>,
            Trackers...
        >;

    template <typename Lockable, typename ...Trackers>
    using lockable_wrapper =
        basic_lockable_wrapper<
            Lockable,
            track_try_lock<call_member_fn<decltype(&Lockable::try_lock), &Lockable::try_lock> >,
            Trackers...
        >;


    //////////////////////////////////////////////////////////////////////////
    // code_segment_events.hpp (find better name)
    //////////////////////////////////////////////////////////////////////////
    template <typename ...Characteristics> struct code_segment;
    // Characteristics of code segments
    template <typename> struct parallelism_level;
    struct task;
    struct thread;
    struct process; // don't think about this right now

    template <typename Level> struct start { };   // semantics: current segment must complete before the two new segments can begin (fork semantics)
    template <typename Level> struct join { };    // semantics: current segment waits for the two other segments to complete
    template <typename Level> struct detach { };  // semantics: segment still running but no one will wait for it to complete before the end of the execution


    //////////////////////////////////////////////////////////////////////////
    // std_thread.hpp
    //////////////////////////////////////////////////////////////////////////
    template <typename JoinImplementation, typename DetachImplementation>
    struct track_start_join_detach {
        template <typename Derived, typename Next>
        struct apply : Next {
            template <typename F, typename ...Args>
            explicit apply(F&& f, Args&& ...args)
                // Here, we would wrap the thread function
                : Next([&] {
                    return dyno::generate<start<parallelism_level<thread> > >(),
                           void(), // bypass any `operator,` overload
                           std::forward<F>(f)(std::forward<Args>(args)...);
                })
            { }

            using Next::operator=;

            void join() {
                JoinImplementation()(this->derived());
                dyno::generate<
                    d2::join<parallelism_level<thread> >
                >(dyno::key<dyno::_self>() = boost::cref(this->derived()));
            }

            void detach() {
                DetachImplementation()(this->derived());
                dyno::generate<
                    d2::detach<parallelism_level<thread> >
                >(dyno::key<dyno::_self>() = boost::cref(this->derived()));
            }
        };
    };

    template <typename Derived, typename ...Trackers>
    using std_thread_mixin =
        dyno::mixin<
            Derived,
            track_start_join_detach<access::use_join_impl, access::use_detach_impl>,
            Trackers...
        >;

    template <typename Thread, typename ...Trackers>
    using std_thread_wrapper =
        dyno::wrapper<
            Thread,
            track_start_join_detach<
                call_member_fn<decltype(&Thread::join), &Thread::join>,
                call_member_fn<decltype(&Thread::detach), &Thread::detach>
            >,
            Trackers...
        >;


    //////////////////////////////////////////////////////////////////////////
    // build_lock_graph.hpp
    //////////////////////////////////////////////////////////////////////////
    struct build_lock_graph {
        template <typename Environment>
        void operator()(mutex_operation<
                            previous_ownership<none>,
                            new_ownership<exclusive>,
                            synchronization_object<dyno::_>
                        >,
                        Environment const&) const
        {
            std::cout << "mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<dyno::_> >" << std::endl;
            #if 0
            auto mutex = env[tags::mutex_id];
            add_vertex(mutex, lock_graph);
            for (auto gatelock: env[tags::gatelocks]) {
                // TODO: If the code location of this acquire and that of the
                //       other acquire making `mutex` and `gatelock` adjacent
                //       are different, then we would like to record this acquire
                //       because it is not redundant to do so.
                if (!is_adjacent(mutex, gatelock, lock_graph))
                    // we might want to tag the edge with the whole environment
                    add_edge(gatelock, mutex, lock_graph);
            }
            #endif
        }

        template <typename Environment>
        void operator()(mutex_operation<
                            previous_ownership<exclusive>,
                            new_ownership<none>,
                            synchronization_object<dyno::_>
                        >,
                        Environment const&) const
        {
            std::cout << "mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<dyno::_> >" << std::endl;
            // nothing to do
        }

        template <typename Environment>
        void operator()(mutex_operation<
                            previous_ownership<none>,
                            new_ownership<exclusive>,
                            synchronization_object<mutex<recursiveness<recursive> > >
                        >,
                        Environment const&) const
        {
            std::cout << "mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<mutex<recursiveness<recursive> > > >" << std::endl;
            #if 0
            if (env[tags::recursive_lock_count]++)
                operator()(mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<_> >(), env);
            #endif
        }

        template <typename Environment>
        void operator()(mutex_operation<
                            previous_ownership<exclusive>,
                            new_ownership<none>,
                            synchronization_object<mutex<recursiveness<recursive> > >
                        >,
                        Environment const&) const
        {
            std::cout << "mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<mutex<recursiveness<recursive> > > >" << std::endl;
            #if 0
            if (--env[tags::recursive_lock_count] == 0)
                operator()(mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<_> >(), env);
            #endif
        }
    };

    // Now let's define what to do when these operations happen
    struct build_segmentation_graph {
        template <typename Environment>
        void operator()(start<parallelism_level<thread> >, Environment const&) const
        {
            std::cout << "start<parallelism_level<thread> >" << std::endl;
            #if 0
            add_vertex(env[tags::new_parent_segment], segmentation_graph);
            add_vertex(env[tags::child_segment], segmentation_graph);
            add_edge(env[tags::parent_segment], env[tags::new_parent_segment], segmentation_graph);
            add_edge(env[tags::parent_segment], env[tags::child_segment], segmentation_graph);
            env[tags::parent_segment] = env[tags::new_parent_segment];
            #endif
        }

        template <typename Environment>
        void operator()(join<parallelism_level<thread> >, Environment const&) const
        {
            std::cout << "join<parallelism_level<thread> >" << std::endl;
            #if 0
            add_vertex(env[tags::new_parent_segment], segmentation_graph);
            add_edge(env[tags::parent_segment], env[tags::new_parent_segment], segmentation_graph);
            add_edge(env[tags::child_segment], env[tags::new_parent_segment], segmentation_graph);
            #endif
        }

        template <typename Environment>
        void operator()(detach<parallelism_level<thread> >, Environment const&) const
        {
            std::cout << "detach<parallelism_level<thread> >" << std::endl;
        }
    };

    struct goodlock_analysis : build_lock_graph, build_segmentation_graph {
        using build_lock_graph::operator();
        using build_segmentation_graph::operator();

        template <typename Environment>
        void operator()(dyno::end_of_program, Environment const&) const {
            std::cout << "dyno::end_of_program" << std::endl;
            #if 0
            return boost::graph::all_cycles(lock_graph)
                    | boost::adaptors::filtered(single_threaded_cycle)
                    | boost::adaptors::filtered(explicitly_ordered_segments)
                    | boost::adaptors::filtered(overlapping_gatelocks)
                    ;
            #endif
        }
    };
} // end namespace d2

// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic -o/dev/null ~/code/sandbox/d2_brainstorm.cpp
// g++-4.8 -std=c++11 -ftemplate-backtrace-limit=0 -I /usr/local/include -Wall -Wextra -pedantic -I ~/code/dyno/include -o/dev/null ~/code/sandbox/d2_brainstorm.cpp

struct MutexMixin : d2::lockable_mixin<MutexMixin> {
    void lock_impl() { }
    void unlock_impl() { }
    bool try_lock_impl() { return true; }
};

typedef d2::lockable_wrapper<std::mutex> MutexWrapper;
typedef d2::std_thread_wrapper<std::thread> Thread;


int main() {
    MutexMixin mix;
    mix.lock();
    mix.unlock();
    mix.try_lock();

    MutexWrapper wrap;
    wrap.lock();
    wrap.unlock();
    wrap.try_lock();

    Thread t1([] {});
    t1.join();

    Thread t2([] {});
    t2.detach();
}




#if 0
namespace d2 {
    // SharedLockable
    typedef mutex_operation<
        previous_ownership<none>, new_ownership<shared>, synchronization_object<_1>
    > lock_shared;

    typedef mutex_operation<
        previous_ownership<shared>, new_ownership<none>, synchronization_object<_1>
    > unlock_shared;

    // UpgradeLockable
    typedef mutex_operation<
        previous_ownership<none>, new_ownership<upgradable>, synchronization_object<_1>
    > lock_upgrade;

    typedef mutex_operation<
        previous_ownership<upgradable>, new_ownership<none>, synchronization_object<_1>
    > unlock_upgrade;

    typedef mutex_operation<
        previous_ownership<shared>, new_ownership<exclusive>, synchronization_object<_1>
    > unlock_shared_and_lock;

    typedef mutex_operation<
        previous_ownership<exclusive>, new_ownership<shared>, synchronization_object<_1>
    > unlock_and_lock_shared;

    typedef mutex_operation<
        previous_ownership<shared>, new_ownership<upgradable>, synchronization_object<_1>
    > unlock_shared_and_lock_upgrade;

    typedef mutex_operation<
        previous_ownership<exclusive>, new_ownership<upgradable>, synchronization_object<_1>
    > unlock_and_lock_upgrade;

    typedef mutex_operation<
        previous_ownership<upgradable>, new_ownership<exclusive>, synchronization_object<_1>
    > unlock_upgrade_and_lock;

    typedef mutex_operation<
        previous_ownership<upgradable>, new_ownership<shared>, synchronization_object<_1>
    > unlock_upgrade_and_lock_shared;
} // end namespace operations

#endif


template <typename Framework>
struct type_erased_event_base {
    virtual void call(Framework&) = 0;
};

template <typename Event, typename Environment, typename Framework>
struct type_erased_event : type_erased_event_base<Framework> {
    explicit type_erased_event(Environment& env) : env_(env) { }

    virtual void call(Framework& framework) {
        framework(Event(), env_);
    }

private:
    Environment& env_;
};

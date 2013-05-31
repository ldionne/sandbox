
#include "dyno_concepts.hpp"
#include <atomic>
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
    struct mutex_event_domain;

    template <typename ...Characteristics>
    struct mutex { typedef mutex_event_domain dyno_domain; };
    // characteristics of mutexes:
    template <typename> struct ownership; // how many threads can own the mutex at a time?
    struct none;
    struct shared;
    struct exclusive;
    struct upgradable; // this is functionally the same as shared, but only one thread may have it at a time and it is not possible for a thread to have it if a thread has exclusive ownership (and vice-versa)

    template <typename> struct recursiveness; // can the mutex be re-acquired by the same thread?
    struct non_recursive;
    struct recursive;


    template <typename ...Characteristics>
    struct mutex_operation { typedef mutex_event_domain dyno_domain; };
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
                LockImplementation()(this->facade());
                dyno::generate<
                    mutex_operation<
                        previous_ownership<none>,
                        new_ownership<exclusive>,
                        synchronization_object<Derived>
                    >
                >(dyno::key<dyno::env::_this>() = &this->facade());
            }

            void unlock() {
                UnlockImplementation()(this->facade());
                dyno::generate<
                    mutex_operation<
                        previous_ownership<exclusive>,
                        new_ownership<none>,
                        synchronization_object<Derived>
                    >
                >(dyno::key<dyno::env::_this>() = &this->facade());
            }
        };
    };

    template <typename Derived, typename ...Trackers>
    using basic_lockable_mixin =
        dyno::spy_as_mixin<
            Derived,
            track_lock_unlock<access::use_lock_impl, access::use_unlock_impl>,
            Trackers...
        >;

    template <typename BasicLockable, typename ...Trackers>
    using basic_lockable_wrapper =
        dyno::spy_as_wrapper<
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
                return TryLockImplementation()(this->facade());
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

    struct thread_sync_domain;
    // semantics: current segment must complete before the two new segments can begin (fork semantics)
    template <typename Level> struct start { typedef thread_sync_domain dyno_domain; };
    // semantics: current segment waits for the two other segments to complete
    template <typename Level> struct join { typedef thread_sync_domain dyno_domain; };
    // semantics: segment still running but no one will wait for it to complete before the end of the execution
    template <typename Level> struct detach { typedef thread_sync_domain dyno_domain; };


    //////////////////////////////////////////////////////////////////////////
    // std_thread.hpp
    //////////////////////////////////////////////////////////////////////////
    template <typename JoinImplementation, typename DetachImplementation>
    struct track_start_join_detach {
        template <typename Derived, typename Next>
        struct apply : Next {
            template <typename F, typename ...Args>
            explicit apply(F&& f, Args&& ...args)
                : Next([=] { // need to copy everything because it will be run in a new thread
                    return dyno::generate<start<parallelism_level<thread> > >(),
                           void(), // bypass any `operator,` overload
                           std::move(f)(std::move(args)...);
                })
            { }

            using Next::operator=;

            void join() {
                JoinImplementation()(this->facade());
                dyno::generate<
                    d2::join<parallelism_level<thread> >
                >(dyno::key<dyno::env::_this>() = &this->facade());
            }

            void detach() {
                DetachImplementation()(this->facade());
                dyno::generate<
                    d2::detach<parallelism_level<thread> >
                >(dyno::key<dyno::env::_this>() = &this->facade());
            }
        };
    };

    template <typename Derived, typename ...Trackers>
    using std_thread_mixin =
        dyno::spy_as_mixin<
            Derived,
            track_start_join_detach<access::use_join_impl, access::use_detach_impl>,
            Trackers...
        >;

    template <typename Thread, typename ...Trackers>
    using std_thread_wrapper =
        dyno::spy_as_wrapper<
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
        static unsigned long get_new_id() {
            static std::atomic<unsigned long> lock_id;
            return lock_id++;
        }

        template <typename Int>
        struct get_new_id_on_construction {
            get_new_id_on_construction() : value_(get_new_id()) { }
            operator Int() const { return value_; }
            Int value_;
        };

        template <typename SynchronizationObject, typename Next, typename Enable = void>
        struct tracker : Next { };

        template <typename SynchronizationObject, typename Next>
        struct tracker<SynchronizationObject, Next, typename boost::enable_if<
            dyno::matches<mutex<dyno::_> , SynchronizationObject>
        >::type> : Next
        {
            get_new_id_on_construction<unsigned long> lock_id;
        };

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

    struct end_of_program { };

    struct goodlock_analysis : build_lock_graph, build_segmentation_graph {
        using build_lock_graph::operator();
        using build_segmentation_graph::operator();

        template <typename Environment>
        void operator()(end_of_program, Environment const&) const {
            std::cout << "end_of_program" << std::endl;
            #if 0
            return boost::graph::all_cycles(lock_graph)
                    | boost::adaptors::filtered(single_threaded_cycle)
                    | boost::adaptors::filtered(explicitly_ordered_segments)
                    | boost::adaptors::filtered(overlapping_gatelocks)
                    ;
            #endif
        }
    };

    struct mutex_event_domain
        : dyno::domain<dyno::root_domain, goodlock_analysis>
    { };

    struct thread_sync_domain
        : dyno::domain<dyno::root_domain, goodlock_analysis>
    { };
} // end namespace d2

// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic ~/code/sandbox/d2_brainstorm.cpp -o/dev/null
// g++-4.8 -std=c++11 -ftemplate-backtrace-limit=0 -I /usr/local/include -Wall -Wextra -pedantic -I ~/code/dyno/include ~/code/sandbox/d2_brainstorm.cpp -o/dev/null

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

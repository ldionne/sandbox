
#include "dyno_v3.hpp"
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/has_key.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/vector.hpp>
#include <dyno/model/easy_map.hpp>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <utility>


namespace d2 {
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


    struct mutex_event_domain;
    template <typename ...Characteristics>
    struct mutex_operation { typedef mutex_event_domain dyno_domain; };
    // characteristics of all operations on mutexes:
    template <typename> struct synchronization_object;
    template <typename> struct previous_ownership;
    template <typename> struct new_ownership;


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
    // build_lock_graph.hpp
    //////////////////////////////////////////////////////////////////////////
    namespace env {
        struct _lock_id {
            typedef boost::mpl::vector<> depends_on;
            template <typename Event, typename Environment>
            void operator()(Event const&, Environment const&) { }
        };

        struct _gatelocks {
            typedef boost::mpl::vector<_lock_id> depends_on;

            // Whenever a lock is acquired, we add its lock_id to the gatelocks.
            template <typename Environment>
            void operator()(mutex_operation<dyno::and_<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<dyno::_> > >,
                            Environment& env) {
                BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<
                    Environment, _gatelocks
                >));
                BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<
                    Environment, _lock_id
                >));
                // auto& held_locks = boost::fusion::at_key<_gatelocks>(env);
                // held_locks.insert(boost::fusion::at_key<_lock_id>(env));
            }

            // Whenever a lock is released, we remove its lock_id from the gatelocks.
            template <typename Environment>
            void operator()(mutex_operation<dyno::and_<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<dyno::_> > >,
                            Environment& env) {
                BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<
                    Environment, _gatelocks
                >));
                BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<
                    Environment, _lock_id
                >));
                // auto& held_locks = boost::fusion::at_key<_gatelocks>(env);
                // held_locks.erase(boost::fusion::at_key<_lock_id>(env));
            }
        };
    }

    struct build_lock_graph {
        boost::directed_graph<> lock_graph;

        typedef boost::mpl::vector<env::_gatelocks> depends_on;

        template <typename Environment>
        void operator()(mutex_operation<dyno::and_<
                            previous_ownership<none>,
                            new_ownership<exclusive>,
                            synchronization_object<
                                mutex<dyno::and_<
                                    ownership<exclusive>,
                                    recursiveness<non_recursive>
                                > >
                            >
                        > >,
                        Environment const&) const
        {
            BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<
                Environment, d2::env::_gatelocks
            >));
            BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<
                Environment, d2::env::_lock_id
            >));

            std::cout << "mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<mutex<ownership<exclusive>, recursiveness<non_recursive> > > >" << std::endl;
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
        void operator()(mutex_operation<dyno::and_<
                            previous_ownership<exclusive>,
                            new_ownership<none>,
                            synchronization_object<
                                mutex<dyno::and_<
                                    ownership<exclusive>,
                                    recursiveness<non_recursive>
                                > >
                            >
                        > >,
                        Environment const&) const
        {
            std::cout << "mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<mutex<ownership<exclusive>, recursiveness<non_recursive> > > >" << std::endl;
            // nothing to do
        }

        template <typename Environment>
        void operator()(mutex_operation<dyno::and_<
                            previous_ownership<none>,
                            new_ownership<exclusive>,
                            synchronization_object<
                                mutex<dyno::and_<
                                    ownership<dyno::_>,
                                    recursiveness<recursive>
                                > >
                            >
                        > >,
                        Environment const&) const
        {
            std::cout << "mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<mutex<ownership<dyno::_>, recursiveness<recursive> > > >" << std::endl;
            #if 0
            if (env[tags::recursive_lock_count]++)
                operator()(mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<_> >(), env);
            #endif
        }

        template <typename Environment>
        void operator()(mutex_operation<dyno::and_<
                            previous_ownership<exclusive>,
                            new_ownership<none>,
                            synchronization_object<
                                mutex<dyno::and_<
                                    ownership<dyno::_>,
                                    recursiveness<recursive>
                                > >
                            >
                        > >,
                        Environment const&) const
        {
            std::cout << "mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<mutex<ownership<dyno::_>, recursiveness<recursive> > > > >" << std::endl;
            #if 0
            if (--env[tags::recursive_lock_count] == 0)
                operator()(mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<_> >(), env);
            #endif
        }
    };

    // Now let's define what to do when these operations happen
    struct build_segmentation_graph {
        typedef boost::mpl::vector<> depends_on;

        boost::directed_graph<> segmentation_graph;

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
        typedef boost::mpl::joint_view<
            build_lock_graph::depends_on,
            build_segmentation_graph::depends_on
        > depends_on;

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

// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic ~/code/sandbox/d2_brainstorm.cpp -o d2_brainstorm.out && ./d2_brainstorm.out && rm d2_brainstorm.out
// g++-4.8 -std=c++11 -ftemplate-backtrace-limit=0 -I /usr/local/include -Wall -Wextra -pedantic -I ~/code/dyno/include ~/code/sandbox/d2_brainstorm.cpp -o d2_brainstorm.out && ./d2_brainstorm.out && rm d2_brainstorm.out


// Wrapper over a std::mutex (or any other mutex with the same interface and semantics)
template <typename Mutex>
struct std_mutex_wrapper : Mutex {
private:
    typedef d2::mutex<
        d2::ownership<d2::exclusive>,
        d2::recursiveness<d2::non_recursive>
    > DescriptionOfSemantics;

public:
    void lock() {
        Mutex::lock();
        dyno::generate<
            d2::mutex_operation<
                d2::previous_ownership<d2::none>,
                d2::new_ownership<d2::exclusive>,
                d2::synchronization_object<DescriptionOfSemantics>
            >
        >((
            dyno::key<dyno::env::_this>() = this,
            dyno::key<d2::env::_gatelocks>() = 1,
            dyno::key<d2::env::_lock_id>() = 2
        ));
    }

    void unlock() {
        Mutex::unlock();
        dyno::generate<
            d2::mutex_operation<
                d2::previous_ownership<d2::exclusive>,
                d2::new_ownership<d2::none>,
                d2::synchronization_object<DescriptionOfSemantics>
            >
        >((
            dyno::key<dyno::env::_this>() = this,
            dyno::key<d2::env::_gatelocks>() = 1,
            dyno::key<d2::env::_lock_id>() = 2
        ));
    }
};

// Wrapper over a std::thread (or any other thread with the same interface and semantics)
template <typename Thread>
struct std_thread_wrapper : Thread {
    template <typename F, typename ...Args>
    explicit std_thread_wrapper(F&& f, Args&& ...args)
        : Thread(std::forward<F>(f), std::forward<Args>(args)...)
    { }
};

typedef std_thread_wrapper<std::thread> WrappedThread;
typedef std_mutex_wrapper<std::mutex> WrappedMutex;


int main() {
    WrappedMutex wrap;
    wrap.lock();
    wrap.unlock();
    wrap.try_lock();

    WrappedThread t1([]{});
    t1.join();
}

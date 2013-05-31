
#include "dyno_v3.hpp"
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


namespace d2 {
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

struct WrappedMutex : std::mutex {
    void lock() {
        std::mutex::lock();
        dyno::generate<
            d2::mutex_operation<
                d2::previous_ownership<d2::none>,
                d2::new_ownership<d2::exclusive>,
                d2::synchronization_object<WrappedMutex>
            >
        >(dyno::key<dyno::env::_this>() = this);
    }

    void unlock() {
        std::mutex::unlock();
        dyno::generate<
            d2::mutex_operation<
                d2::previous_ownership<d2::exclusive>,
                d2::new_ownership<d2::none>,
                d2::synchronization_object<WrappedMutex>
            >
        >(dyno::key<dyno::env::_this>() = this);
    }
};

int main() {
    WrappedMutex wrap;
    wrap.lock();
    wrap.unlock();
    wrap.try_lock();
}

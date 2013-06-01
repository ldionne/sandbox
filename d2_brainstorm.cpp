
#include "dyno_v3.hpp"
#include <boost/config.hpp>
#include <boost/graph/directed_graph.hpp>
#include <dyno/model/easy_map.hpp>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <utility>


namespace d2 {
#ifdef BOOST_CLANG
#   define D2_THREAD_LOCAL __thread
#else
#   define D2_THREAD_LOCAL thread_local
#endif
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

// the two types of operations currently supported by the d2 analysis
template <typename Recursiveness>
using acquire =
mutex_operation<
    dyno::and_<
        previous_ownership<none>,
        new_ownership<exclusive>,
        synchronization_object<
            mutex<
                dyno::and_<
                    ownership<dyno::_>,
                    recursiveness<Recursiveness>
                >
            >
        >
    >
>;

template <typename Recursiveness>
using release =
mutex_operation<
    dyno::and_<
        previous_ownership<exclusive>,
        new_ownership<none>,
        synchronization_object<
            mutex<
                dyno::and_<
                    ownership<dyno::_>,
                    recursiveness<Recursiveness>
                >
            >
        >
    >
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
// lock_id.hpp
//////////////////////////////////////////////////////////////////////////
typedef unsigned long LockId;
namespace env {
    struct _lock_id : dyno::external_env_var<_lock_id> { };
    static const _lock_id lock_id{};
}



//////////////////////////////////////////////////////////////////////////
// gatelocks.hpp
//////////////////////////////////////////////////////////////////////////
namespace env {
struct _gatelocks {
    template <typename Environment>
    static std::set<LockId>& get(Environment const&) {
        return single_instance_per_thread;
    }

private:
    static D2_THREAD_LOCAL std::set<LockId> single_instance_per_thread;
};
D2_THREAD_LOCAL std::set<LockId> _gatelocks::single_instance_per_thread;
static const _gatelocks gatelocks{};

namespace gatelocks_detail {
    struct updater {
        // Whenever a lock is acquired, we add its lock_id to the gatelocks.
        template <typename Environment>
        void operator()(acquire<dyno::_>, dyno::minimal_env<_lock_id, _gatelocks>, Environment& env) {
            env[gatelocks].insert(env[lock_id]);
        }

        // Whenever a lock is released, we remove its lock_id from the gatelocks.
        template <typename Environment>
        void operator()(release<dyno::_>, dyno::minimal_env<_lock_id, _gatelocks>, Environment& env) {
            env[gatelocks].erase(env[lock_id]);
        }
    };
}
} // end namespace env



//////////////////////////////////////////////////////////////////////////
// recursive_lock_count.hpp
//////////////////////////////////////////////////////////////////////////
namespace env {
struct _recursive_lock_count {
    template <typename Environment>
    static unsigned long& get(Environment& env) {
        return counts_per_thread[env[lock_id]];
    }

private:
    typedef std::map<LockId, unsigned long> map_type;
    static D2_THREAD_LOCAL map_type counts_per_thread;
};
D2_THREAD_LOCAL _recursive_lock_count::map_type _recursive_lock_count::counts_per_thread;
static const _recursive_lock_count recursive_lock_count{};

namespace recursive_lock_count_detail {
    struct updater {
        template <typename Environment>
        void operator()(acquire<recursive>, dyno::minimal_env<_recursive_lock_count>, Environment& env)
        { ++env[recursive_lock_count]; }

        template <typename Environment>
        void operator()(release<recursive>, dyno::minimal_env<_recursive_lock_count>, Environment& env)
        { --env[recursive_lock_count]; }

        template <typename Environment>
        void operator()(dyno::or_<release<non_recursive>, acquire<non_recursive> >, dyno::minimal_env<>, Environment const&) { }
    };
}
} // end namespace env



//////////////////////////////////////////////////////////////////////////
// build_lock_graph.hpp
//////////////////////////////////////////////////////////////////////////
struct build_lock_graph {
    boost::directed_graph<> lock_graph;

public:
    template <typename Environment>
    void operator()(acquire<non_recursive>,
                    dyno::minimal_env<env::_gatelocks, env::_lock_id>,
                    Environment& env)
    {
        std::cout << "exclusive acquire non recursive mutex" << std::endl;
        auto mutex = env[d2::env::lock_id];
        (void)mutex;
        // add_vertex(mutex, lock_graph);
        for (auto gatelock: env[d2::env::gatelocks]) {
            std::cout << "holding: " << gatelock << "\n";
            /*
            // TODO: If the code location of this acquire and that of the
            //       other acquire making `mutex` and `gatelock` adjacent
            //       are different, then we would like to record this acquire
            //       because it is not redundant to do so.
            if (!is_adjacent(mutex, gatelock, lock_graph))
                // we might want to tag the edge with the whole environment
                add_edge(gatelock, mutex, lock_graph);
            */
        }
    }

    template <typename Environment>
    void operator()(release<non_recursive>,
                    dyno::minimal_env<>,
                    Environment&)
    { std::cout << "release non recursive mutex" << std::endl; }

    template <typename Environment>
    void operator()(acquire<recursive>,
                    dyno::minimal_env<env::_recursive_lock_count>,
                    Environment& env)
    {
        // If this is the first time we're locking it, continue.
        if (env[d2::env::recursive_lock_count] == 1)
            std::cout << "exclusive acquire recursive mutex" << std::endl;
    }

    template <typename Environment>
    void operator()(release<recursive>,
                    dyno::minimal_env<env::_recursive_lock_count>,
                    Environment& env)
    {
        // If the mutex is not locked anymore after this unlock, continue.
        if (env[d2::env::recursive_lock_count] == 0)
            std::cout << "release recursive mutex" << std::endl;
    }
};



//////////////////////////////////////////////////////////////////////////
// build_segmentation_graph.hpp
//////////////////////////////////////////////////////////////////////////
struct build_segmentation_graph {
    template <typename Environment>
    void operator()(start<parallelism_level<thread> >, Environment const&) const
    { std::cout << "start<parallelism_level<thread> >" << std::endl; }

    template <typename Environment>
    void operator()(join<parallelism_level<thread> >, Environment const&) const
    { std::cout << "join<parallelism_level<thread> >" << std::endl; }

    template <typename Environment>
    void operator()(detach<parallelism_level<thread> >, Environment const&) const
    { std::cout << "detach<parallelism_level<thread> >" << std::endl; }
};



//////////////////////////////////////////////////////////////////////////
// analysis.hpp
//////////////////////////////////////////////////////////////////////////
struct goodlock_analysis : build_lock_graph, build_segmentation_graph {
    using build_lock_graph::operator();
    using build_segmentation_graph::operator();

    template <typename Environment>
    void operator()(struct end_of_program const&, Environment const&) const {
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
    : dyno::domain<
        dyno::root_domain,
        env::recursive_lock_count_detail::updater,
        env::gatelocks_detail::updater,
        goodlock_analysis
    >
{ };

struct thread_sync_domain
    : dyno::domain<dyno::root_domain, goodlock_analysis>
{ };
} // end namespace d2


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
            dyno::key<d2::env::_lock_id>() = 2u
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
            dyno::key<d2::env::_lock_id>() = 2u
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

// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic ~/code/sandbox/d2_brainstorm.cpp -o d2_brainstorm.out && ./d2_brainstorm.out && rm d2_brainstorm.out
// g++-4.8 -std=c++11 -ftemplate-backtrace-limit=0 -I /usr/local/include -Wall -Wextra -pedantic -I ~/code/dyno/include ~/code/sandbox/d2_brainstorm.cpp -o d2_brainstorm.out && ./d2_brainstorm.out && rm d2_brainstorm.out

int main() {
    WrappedMutex wrap;
    wrap.lock();
    wrap.unlock();
    wrap.try_lock();

    WrappedThread t1([]{});
    t1.join();
}

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


template <typename ...Characteristics> struct mutex_operation;
// characteristics of all operations on mutexes:
template <typename> struct synchronization_object;
template <typename> struct previous_ownership;
template <typename> struct new_ownership;


// We can now define some common operations
namespace operations {
    // BasicLockable
    typedef mutex_operation<
        previous_ownership<none>, new_ownership<exclusive>, synchronization_object<_1>
    > lock;

    typedef mutex_operation<
        previous_ownership<exclusive>, new_ownership<none>, synchronization_object<_1>
    > unlock;

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


// Now let's define what to do when these operations happen
struct build_lock_graph {
    boost::directed_graph<...> lock_graph;

    void operator()(mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<_> >,
                    environment<tags::thread_id, tags::gatelocks, tags::mutex_id, tags::code_segment_id> env) {
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
    }

    void operator()(mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<_> >,
                    environment<tags::thread_id, tags::gatelocks, tags::mutex_id, tags::code_segment_id> env) {
        // nothing to do
    }

    void operator()(mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<mutex<recursiveness<recursive> > > >,
                    environment<tags::thread_id, tags::gatelocks, tags::mutex_id, tags::code_segment_id, tags::recursive_lock_count> env) {
        if (env[tags::recursive_lock_count]++)
            operator()(mutex_operation<previous_ownership<none>, new_ownership<exclusive>, synchronization_object<_> >(), env);
    }

    void operator()(mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<mutex<recursiveness<recursive> > > >,
                    environment<tags::thread_id, tags::gatelocks, tags::mutex_id, tags::code_segment_id, tags::recursive_lock_count> env) {
        if (--env[tags::recursive_lock_count] == 0)
            operator()(mutex_operation<previous_ownership<exclusive>, new_ownership<none>, synchronization_object<_> >(), env);
    }
};




template <typename ...Characteristics> struct code_segment;
// Characteristics of code segments
template <typename> struct parallelism_level;
struct task;
struct thread;
struct process; // don't think about this right now

namespace operations {
    template <typename Level> struct start;   // semantics: current segment must complete before the two new segments can begin (fork semantics)
    template <typename Level> struct join;    // semantics: current segment waits for the two other segments to complete
    template <typename Level> struct detach;  // semantics: segment still running but no one will wait for it to complete before the end of the execution
}

struct build_segmentation_graph {
    boost::directed_graph<...> segmentation_graph;

    void operator()(start<parallelism_level<thread> >,
                    environment<tags::parent_segment, tags::child_segment, tags::new_parent_segment> env) {
        add_vertex(env[tags::new_parent_segment], segmentation_graph);
        add_vertex(env[tags::child_segment], segmentation_graph);
        add_edge(env[tags::parent_segment], env[tags::new_parent_segment], segmentation_graph);
        add_edge(env[tags::parent_segment], env[tags::child_segment], segmentation_graph);
        env[tags::parent_segment] = env[tags::new_parent_segment];
    }

    void operator()(join<parallelism_level<thread> >,
                    environment<tags::parent_segment, tags::child_segment, tags::new_parent_segment> env) {
        add_vertex(env[tags::new_parent_segment], segmentation_graph);
        add_edge(env[tags::parent_segment], env[tags::new_parent_segment], segmentation_graph);
        add_edge(env[tags::child_segment], env[tags::new_parent_segment], segmentation_graph);
    }
};


auto single_threaded_cycle = [](auto cycle) {
    return any_of(cross_product(cycle), _1 != _2 && _1.thread == _2.thread);
};

struct goodlock_analysis : union_of<build_lock_graph, build_segmentation_graph> {
    auto operator()(end_of_program) {
        return boost::graph::all_cycles(lock_graph)
                | boost::adaptors::filtered(single_threaded_cycle)
                | boost::adaptors::filtered(explicitly_ordered_segments)
                | boost::adaptors::filtered(overlapping_gatelocks)
                ;
    }
};

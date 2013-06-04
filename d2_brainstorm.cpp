
#include <boost/config.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/proto/proto.hpp>
#include <dyno/model/easy_map.hpp>
#include <dyno/v2/dyno.hpp>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <utility>


namespace d2 {
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
struct thread_sync_domain
    : dyno::domain<dyno::root_domain, goodlock_analysis>
{ };
} // end namespace d2




// Wrapper over a std::thread (or any other thread with the same interface and semantics)
template <typename Thread>
struct std_thread_wrapper : Thread {
    template <typename F, typename ...Args>
    explicit std_thread_wrapper(F&& f, Args&& ...args)
        : Thread(std::forward<F>(f), std::forward<Args>(args)...)
    { }
};

typedef std_thread_wrapper<std::thread> WrappedThread;


template <typename Event>
struct dyno_event : boost::proto::extends<Event, dyno_event<Event> > { };





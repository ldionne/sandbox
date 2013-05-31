
#include <boost/mpl/bool.hpp>
#include <boost/mpl/empty_base.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/inherit_linearly.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_void.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <utility>


namespace dyno {
#if 0
//////////////////////////////////////////////////////////////////////////////
// Both ends of the pipeline
//////////////////////////////////////////////////////////////////////////////
template <typename Facade, typename Wrapped>
struct basemost_layer : Wrapped {
    template <typename F>
    auto operator()(F&& f) const -> decltype(std::forward<F>(f)()) {
        return std::forward<F>(f)();
    }

protected:
    typedef Facade facade_type;
    facade_type& facade()
    { return static_cast<facade_type&>(*this); }
    facade_type const& facade() const
    { return static_cast<facade_type const&>(*this); }

    typedef Wrapped wrapped_type;
    wrapped_type& wrapped()
    { return static_cast<wrapped_type&>(*this); }
    wrapped_type const& wrapped() const
    { return static_cast<wrapped_type const&>(*this); }
};

template <typename Next>
struct topmost_layer : Next {
private:
    using typename Next::facade_type;
    facade_type& facade() = delete;
    facade_type const& facade() const = delete;

    using typename Next::wrapped_type;
    wrapped_type& wrapped() = delete;
    wrapped_type const& wrapped() const = delete;
};



//////////////////////////////////////////////////////////////////////////////
// Easily create pipelines as wrappers around other objects or as mixins
//////////////////////////////////////////////////////////////////////////////
struct replace_x_by_next {
    template <typename Next, typename Part>
    struct apply;

    template <typename Next, template <typename> class Part>
    struct apply<Next, Part<struct x> > {
        typedef Part<Next> type;
    };
};

template <typename Facade, typename Wrapped, template <typename Next> class ...Parts>
struct make_pipeline_base
    : boost::mpl::inherit_linearly<
        typename boost::mpl::reverse<
            boost::mpl::vector<
                topmost_layer<struct x>, Parts<struct x>...
            >
        >::type,
        replace_x_by_next,
        basemost_layer<Facade, Wrapped>
    >
{ };


template <typename Wrapped, template <typename Next> class ...Parts>
struct pipeline_as_wrapper
    : make_pipeline_base<
        pipeline_as_wrapper<Wrapped, Parts...>, Wrapped, Parts...
    >::type
{ };


template <typename Derived, template <typename Next> class ...Parts>
struct pipeline_as_mixin
    : make_pipeline_base<
        Derived, boost::mpl::empty_base, Parts...
    >::type
{ };

#endif



//////////////////////////////////////////////////////////////////////////////
// Basic components to create pipelines
//////////////////////////////////////////////////////////////////////////////
template <typename Part>
struct pipe_into {
    template <typename ...Args>
    static void call(Args&& ...args) {
        Part::call(std::forward<Args>(args)...);
    }
};

struct bottom {
    template <typename ...Args>
    static void call(Args&& ...) { }
};

namespace pipeline_detail {
    template <template <typename> class ...Parts> struct make_pipeline;
    template <template <typename> class LastPart>
    struct make_pipeline<LastPart> {
        typedef LastPart<bottom> type;
    };

    template <template <typename> class Head, template <typename> class ...Tail>
    struct make_pipeline<Head, Tail...> {
        typedef Head<typename make_pipeline<Tail...>::type> type;
    };
} // end namespace pipeline_detail

template <template <typename Next> class ...Parts>
using pipeline = typename pipeline_detail::make_pipeline<Parts...>::type;



//////////////////////////////////////////////////////////////////////////////
// Super generic pipeline parts
//////////////////////////////////////////////////////////////////////////////
template <typename Next = bottom>
struct benchmark {
    template <typename ...Args>
    static void call(Args&& ...args) {
        std::cout << "start timer\n";
        auto start = std::chrono::high_resolution_clock::now();
        pipe_into<Next>::call(std::forward<Args>(args)...);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "stop timer: " << (end - start).count() << " elapsed\n";
    }
};

template <typename Next = bottom>
struct forward {
    template <typename SemanticTag, typename ...Args>
    static void call(Args&& ...args) {
        std::cout << "do nothing\n";
        pipe_into<Next>::call(std::forward<Args>(args)...);
    }
};

template <typename Next = bottom>
struct invoke {
    template <typename F, typename ...Args>
    static void call(F&& f, Args&& ...args) {
        std::cout << "invoke function\n";
        do_call(typename boost::is_void<
                    decltype(std::forward<F>(f)(std::forward<Args>(args)...))
                >::type(),
                std::forward<F>(f), std::forward<Args>(args)...);
    }

private:
    template <typename F, typename ...Args>
    static void do_call(boost::mpl::true_, F&& f, Args&& ...args) {
        std::forward<F>(f)(std::forward<Args>(args)...);
        pipe_into<Next>::call();
    }

    template <typename F, typename ...Args>
    static void do_call(boost::mpl::false_, F&& f, Args&& ...args) {
        pipe_into<Next>::call(std::forward<F>(f)(std::forward<Args>(args)...));
    }
};

template <typename Next = bottom>
struct generate_right_after {
    template <typename ...Args>
    static void call(Args&& ...args) {
        pipe_into<Next>::call(std::forward<Args>(args)...);
        // generate here
        std::cout << "generate event\n";
    }
};
} // end namespace dyno

#if 0
//////////////////////////////////////////////////////////////////////////////
// Put some interface onto a pipeline and launch it
//////////////////////////////////////////////////////////////////////////////
template <typename Next>
struct mixin_lock_interface : Next {
    void lock() {
        typedef typename Next::facade_type Facade;
        dyno::pipe_into<Next>::call(*this, std::bind(&Facade::lock_impl, std::ref(this->facade())));
    }
};

template <typename Next>
struct wrapper_lock_interface : Next {
    void lock() {
        typedef typename Next::wrapped_type Wrapped;
        dyno::pipe_into<Next>::call(*this, std::bind(&Wrapped::lock, std::ref(this->wrapped())));
    }
};



typedef dyno::pipeline_as_wrapper<std::mutex, wrapper_lock_interface, dyno::generate_right_after> WrappedMutex;
#endif

// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic ~/code/sandbox/pipeline.cpp -o/dev/null
// g++-4.8 -std=c++11 -ftemplate-backtrace-limit=0 -I /usr/local/include -Wall -Wextra -pedantic -I ~/code/dyno/include ~/code/sandbox/pipeline.cpp -o/dev/null

using namespace dyno;
typedef pipeline<benchmark, generate_right_after, invoke> Benchmarker;


void function() {
    for (int i = 0; i < 1000; ++i)
        ;
}


int main() {
    Benchmarker::call(function);
    // WrappedMutex m;
    // m.lock();
    // m.unlock();
}

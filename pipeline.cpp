
#include <boost/mpl/empty_base.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/inherit_linearly.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/vector.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <utility>


namespace dyno {
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

template <typename Part>
struct pipe_into {
    template <typename Action>
    static auto call(Part& part, Action&& action)
   -> decltype(part(std::forward<Action>(action))) {
        return part(std::forward<Action>(action));
    }
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



//////////////////////////////////////////////////////////////////////////////
// Super generic pipeline parts
//////////////////////////////////////////////////////////////////////////////
template <typename Next>
struct benchmark : Next {
    template <typename Action>
    void operator()(Action&& action) {
        auto start = std::chrono::high_resolution_clock::now();
        pipe_into<Next>::call(*this, std::forward<Action>(action));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << (end - start).count() << '\n';
    }
};

template <typename Next>
struct nothing : Next {
    template <typename Action>
    void operator()(Action&& action) {
        pipe_into<Next>::call(*this, std::forward<Action>(action));
    }
};

template <typename Next>
struct generate_right_after : Next {
    template <typename Action>
    void operator()(Action&& action) {
        pipe_into<Next>::call(*this, std::forward<Action>(action));
        // generate here
        std::cout << "I GENERATED\n";
    }
};
} // end namespace dyno


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


// clang++ -I /usr/lib/c++/v1 -ftemplate-backtrace-limit=0 -I /usr/local/include -stdlib=libc++ -std=c++11 -I ~/code/dyno/include -Wall -Wextra -pedantic ~/code/sandbox/pipeline.cpp -o/dev/null

int main() {
    WrappedMutex m;
    m.lock();
    m.unlock();
}

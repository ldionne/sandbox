
// clang++ -std=c++1y -stdlib=libc++ -Wall -Wextra -pedantic ~/desktop/workbench.cpp -I ~/code/libcxx/include -o /dev/null
// g++-4.9 -std=c++11 -Wall -Wextra -pedantic ~/desktop/workbench.cpp -o /dev/null


#include <cassert>
#include <functional>


#if 0
auto y_combinator = [](auto f) {
    return f(f);
};

auto factorial_helper = [](auto facto) {
    return [=](int n) {
        return n == 0 ? 1 : n * facto(facto)(n - 1);
    };
};
#endif

template <typename F, typename A0>
struct curried {
    F f;
    A0 a0;

    template <typename ...An>
    auto operator()(An&& an...) {
        return f(a0, an...);
    }
};


auto y_combinator = [](auto f) {
    // in Haskell, we would use automatic currying and write:
    // f (y_combinator f)
    return [=](auto ...args) {
        return f(y_combinator(f), args...);
    };
};

auto factorial = y_combinator([](auto facto, int n) {
    return n == 0 ? 1 : n * facto(n - 1);
});


int main() {
    assert(factorial(0) == 1);
    assert(factorial(1) == 1);
    assert(factorial(5) == 5 * 4 * 3 * 2 * 1);
}

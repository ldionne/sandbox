
template <typename T, unsigned long long i = 0>
struct really_long
    : really_long<T, i + 1>
{ };

template <typename T>
struct really_long<T, static_cast<unsigned long long>(-1)> {
    using type = T;
};


template <typename T> auto pick(int) -> T;
template <typename T> auto pick(...) -> typename really_long<T>::type;

// Surpisingly, both prototypes will be instantiated, even though only the
// first one can match. This will result in endless compilation.
using xxx = decltype(pick<struct x>(int{}));



int main() { }

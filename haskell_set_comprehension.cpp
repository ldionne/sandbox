
// clang++ -std=c++11 -stdlib=libc++ -Wall -Wextra -pedantic -I/Users/louisdionne/Documents/Ordi/d3_ext_boost


#include <iostream>
#include <vector>


namespace Haskell {



} // end namespace Haskell


struct Placeholder { };

template <typename Collection>
struct IterableExpression {
    Collection collection_;

    explicit IterableExpression(Collection const& val) : collection_(val) { }

    auto begin() const -> decltype(collection_.begin()) {
        return collection_.begin();
    }

    auto begin() -> decltype(collection_.begin()) {
        return collection_.begin();
    }

    auto end() const -> decltype(collection_.end()) {
        return collection_.end();
    }

    auto end() -> decltype(collection_.end()) {
        return collection_.end();
    }
};


template <typename Callable, typename Collection>
class TransformedIterableExpression : public IterableExpression<Collection> {
    Callable f_;
    using Base = IterableExpression<Collection>;

public:
    explicit TransformedIterableExpression(Callable const& f, Collection const& coll)
        : Base(coll), f_(f)
    { }

    boost::transform_iterator<Callable,
};

template <typename Predicate, typename Collection>
struct FilteredExpression : IterableExpression<Collection> {
    using Base = IterableExpression<Collection>;
    Predicate pred_;

    explicit FilteredExpression(Predicate const& pred, Collection const& coll)
        : Base(coll), pred_(pred)
    { }
};

namespace detail {
template <typename Collection>
struct PreAssignWrapper {
    Collection collection_;

    explicit PreAssignWrapper(Collection const& coll) : collection_(coll) { }

    operator Collection() const {
        return collection_;
    }
};
} // end namespace detail



template <typename Callable, typename Collection>
TransformedIterableExpression<Callable, Collection> operator|(Callable f, IterableExpression<Collection> it) {
    return TransformedIterableExpression<Callable, Collection>(f, it.collection_);
}

template <typename Collection, typename Predicate>
FilteredExpression<Predicate, IterableExpression<Collection>> operator,(IterableExpression<Collection> coll, Predicate pred) {
    return FilteredExpression<Predicate, IterableExpression<Collection>>(pred, coll);
}

template <typename Collection>
detail::PreAssignWrapper<Collection> operator-(Collection const& coll) {
    return detail::PreAssignWrapper<Collection>(coll);
}

template <typename Collection>
IterableExpression<Collection> operator<(Placeholder, detail::PreAssignWrapper<Collection> coll) {
    return IterableExpression<Collection>(coll);
}

// goal is to write:
// (x | x < collection, predicate_with_x(x))

inline int add2(int i) { return i + 2; }
inline bool is_odd(int i) { return i % 2 != 0; }


int main() {
    std::vector<int> v{0, 1, 2, 3, 4, 5};
    Placeholder _1;

    auto iterable = _1 <- v;
    auto transformed = add2 | iterable;
    auto filtered = (transformed, is_odd);

    auto filtered2 = (add2 | _1 <- v, is_odd);

    for (auto i: filtered2)
        std::cout << i << " ";
    std::cout << std::endl;
}



template <typename T>
class Array {
    T storage[10];

    void map();
};


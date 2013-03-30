/**
 * This file defines ruby-style integers.
 */

#ifndef RUBY_INTEGER_HPP
#define RUBY_INTEGER_HPP

#include <type_traits>


template <typename Integer>
class ruby_integer {
    using Self = ruby_integer;
    Integer self_;

public:
    ruby_integer() = default;
    ruby_integer(Integer i) : self_(i) { };

    template <typename OtherInteger, typename = typename std::
        enable_if<std::is_convertible<OtherInteger, Integer>::value>::type>
    ruby_integer(ruby<OtherInteger> const& other) : self_(other.self_) { }

    operator Integer() const { return self_; }

    template <typename Block>
    Self times(Block yield) {
        for (auto i = 0; i < self_; ++i)
            yield(Self(i));
        return *this;
    }
};

#endif // !RUBY_INTEGER_HPP

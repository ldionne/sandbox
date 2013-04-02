
#ifndef LDIONNE_SANDBOX_SIBLING_HPP
#define LDIONNE_SANDBOX_SIBLING_HPP

#include <boost/assert.hpp>
#include <boost/move/move.hpp>
#include <boost/utility/swap.hpp>
#include <cstddef>


namespace sandbox {
/*!
 * Class implementing a two-nodes linked list on the stack.
 */
template <typename T>
struct sibling {
    sibling(BOOST_RV_REF(sibling) sib) {
        // first of all, link *this and sib's sibling
        other_ = sib.other_;
        other_->other_ = this;

        // then, kill sib
        sib.other_ = NULL;
    }

    sibling& operator=(BOOST_RV_REF(sibling) sib) {
        if (this == &sib)
            return *this;

        // first of all, kill *this (inform our sibling that we're dead)
        if (has_sibling())
            other_->other_ = NULL;

        // then, link *this and sib's sibling
        if (sib.has_sibling()) {
            other_ = sib.other_;
            other_->other_ = this;

            // finally, kill sib
            sib.other_ = NULL;
        }
        else
            other_ = NULL;

        return *this;
    }

    ~sibling() {
        // if *this has a sibling, inform him that we're dead
        if (has_sibling())
            other_->other_ = NULL;
    }

    bool has_sibling() const {
        return other_ != NULL;
    }

    sibling& other() {
        BOOST_ASSERT(has_sibling());
        return *other_;
    }

    sibling const& other() const {
        BOOST_ASSERT(has_sibling());
        return *other_;
    }

    friend void swap(sibling& x, sibling& y) {
        boost::swap(y.other_->other_, x.other_->other_);
        boost::swap(y.other_, x.other_);
    }

    friend bool are_siblings(sibling const& x, sibling const& y) {
        BOOST_ASSERT((x.other_ == &y) == (y.other_ == &x));
        BOOST_ASSERT(!x.has_sibling() ? x.other_ != &y : true);
        BOOST_ASSERT(!y.has_sibling() ? y.other_ != &x : true);
        return x.other_ == &y;
    }

private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(sibling)

    template <typename U>
    friend struct siblings;

    // These two constructors are required to initialize two siblings.
    sibling() : other_(NULL) { }
    explicit sibling(sibling* other) : other_(other) { }

    sibling* other_;
};

template <typename T>
struct siblings {
    typedef sibling<T> sibling_type;
    typedef sibling_type first_type;
    typedef sibling_type second_type;

    first_type first;
    second_type second;

    siblings()
        : first(&second), second(&first)
    { }

    siblings(BOOST_RV_REF(siblings) others)
        : first(boost::move(others.first)),
          second(boost::move(others.second))
    { }

    siblings& operator=(BOOST_RV_REF(siblings) others) {
        first = boost::move(others.first);
        second = boost::move(others.second);
        return *this;
    }

    friend void swap(siblings& a, siblings& b) {
        boost::swap(a.first, b.first);
        boost::swap(a.second, b.second);
    }

private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(siblings)
};
} // end namespace sandbox

#endif // !LDIONNE_SANDBOX_SIBLING_HPP


#include <boost/assert.hpp>


// g++-4.8 -std=c++11 -Wall -Wextra -pedantic -I /usr/local/include -o sibling sibling.cpp -O0

typedef sandbox::siblings<int> Siblings;
typedef Siblings::sibling_type Sibling;


void simple_construction() {
    Siblings siblings;
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);
    BOOST_ASSERT(are_siblings(a, b));
    BOOST_ASSERT(are_siblings(b, a));
}

void self_assign_is_noop() {
    Siblings siblings;
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);
    a = boost::move(a);
    BOOST_ASSERT(are_siblings(a, b));
    BOOST_ASSERT(are_siblings(b, a));
}

void move_construction_kills_moved_from() {
    Siblings siblings;
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);

    Sibling c = boost::move(b);
    BOOST_ASSERT(!b.has_sibling());
    BOOST_ASSERT(are_siblings(a, c));
    BOOST_ASSERT(are_siblings(c, a));
}

void killing_sibling_empties_both() {
    Siblings siblings;
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);

    a = boost::move(b);
    BOOST_ASSERT(!a.has_sibling());
    BOOST_ASSERT(!b.has_sibling());
}

void swapping_related_siblings_is_noop() {
    Siblings siblings;
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);

    swap(a, b);
    BOOST_ASSERT(are_siblings(a, b));
    BOOST_ASSERT(are_siblings(b, a));
}

void swapping_unrelated_siblings_works_as_expected() {
    Siblings ab, cd;
    Sibling a = boost::move(ab.first);
    Sibling b = boost::move(ab.second);

    Sibling c = boost::move(cd.first);
    Sibling d = boost::move(cd.second);

    swap(a, c);
    BOOST_ASSERT(are_siblings(a, d));
    BOOST_ASSERT(are_siblings(d, a));

    BOOST_ASSERT(are_siblings(b, c));
    BOOST_ASSERT(are_siblings(c, b));
}

int main() {
    simple_construction();
    self_assign_is_noop();
    move_construction_kills_moved_from();
    killing_sibling_empties_both();
    swapping_related_siblings_is_noop();
    swapping_unrelated_siblings_works_as_expected();
}

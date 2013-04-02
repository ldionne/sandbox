
#ifndef LDIONNE_SANDBOX_SIBLING_HPP
#define LDIONNE_SANDBOX_SIBLING_HPP

#include <boost/assert.hpp>
#include <boost/move/move.hpp>
#include <cstddef>
#include <utility>


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

    static std::pair<sibling, sibling> get() {
        sibling a;
        sibling b(&a);
        a.other_ = &b;
        return std::make_pair(boost::move(a), boost::move(b));
    }

    bool has_sibling() const {
        return other_ != NULL;
    }

    bool is_sibling_of(sibling const& sib) const {
        BOOST_ASSERT((other_ == &sib) == (sib.other_ == this));
        BOOST_ASSERT(!has_sibling() ? other_ != &sib : true);
        return other_ == &sib;
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
        using std::swap;
        swap(y.other_->other_, x.other_->other_);
        swap(y.other_, x.other_);
    }

private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(sibling)

    // These two constructors are required to initialize two siblings.
    sibling() : other_(NULL) { }
    explicit sibling(sibling* other) : other_(other) { }

    sibling* other_;
};
} // end namespace sandbox

#endif // !LDIONNE_SANDBOX_SIBLING_HPP


#include <boost/assert.hpp>


// g++-4.8 -std=c++11 -Wall -Wextra -pedantic -I /usr/local/include -o sibling sibling.cpp -O0

typedef sandbox::sibling<int> Sibling;


void simple_construction() {
    std::pair<Sibling, Sibling> siblings = Sibling::get();
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);
    BOOST_ASSERT(a.is_sibling_of(b));
    BOOST_ASSERT(b.is_sibling_of(a));
}

void self_assign_is_noop() {
    std::pair<Sibling, Sibling> siblings = Sibling::get();
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);
    a = boost::move(a);
    BOOST_ASSERT(a.is_sibling_of(b));
    BOOST_ASSERT(b.is_sibling_of(a));
}

void move_construction_kills_moved_from() {
    std::pair<Sibling, Sibling> siblings = Sibling::get();
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);

    Sibling c = boost::move(b);
    BOOST_ASSERT(!b.has_sibling());
    BOOST_ASSERT(a.is_sibling_of(c));
    BOOST_ASSERT(c.is_sibling_of(a));
}

void killing_sibling_empties_both() {
    std::pair<Sibling, Sibling> siblings = Sibling::get();
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);

    a = boost::move(b);
    BOOST_ASSERT(!a.has_sibling());
    BOOST_ASSERT(!b.has_sibling());
}

void swapping_related_siblings_is_noop() {
    std::pair<Sibling, Sibling> siblings = Sibling::get();
    Sibling a = boost::move(siblings.first);
    Sibling b = boost::move(siblings.second);

    swap(a, b);
    BOOST_ASSERT(a.is_sibling_of(b));
    BOOST_ASSERT(b.is_sibling_of(a));
}

void swapping_unrelated_siblings_works_as_expected() {
    std::pair<Sibling, Sibling> ab = Sibling::get();
    Sibling a = boost::move(ab.first);
    Sibling b = boost::move(ab.second);

    std::pair<Sibling, Sibling> cd = Sibling::get();
    Sibling c = boost::move(cd.first);
    Sibling d = boost::move(cd.second);

    swap(a, c);
    BOOST_ASSERT(a.is_sibling_of(d));
    BOOST_ASSERT(d.is_sibling_of(a));

    BOOST_ASSERT(b.is_sibling_of(c));
    BOOST_ASSERT(c.is_sibling_of(b));
}

int main() {
    simple_construction();
    self_assign_is_noop();
    move_construction_kills_moved_from();
    killing_sibling_empties_both();
    swapping_related_siblings_is_noop();
    swapping_unrelated_siblings_works_as_expected();
}

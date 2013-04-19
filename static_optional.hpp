/*!
 * @file
 * This file defines `dyno::detail::static_optional`.
 */

#ifndef DYNO_DETAIL_STATIC_OPTIONAL_HPP
#define DYNO_DETAIL_STATIC_OPTIONAL_HPP

#include <boost/operators.hpp>
#include <boost/spirit/include/classic_safe_bool.hpp>
#include <boost/type_traits/remove_reference.hpp>


namespace dyno {
namespace detail {

namespace static_optional_detail {
template <typename Derived, typename T, bool IsValid>
class static_optional_base
    : public boost::spirit::classic::safe_bool<
        static_optional_base<Derived, T, IsValid>
    >,
    public boost::totally_ordered<static_optional_base<Derived, T, IsValid> >
{
protected:
    typedef typename boost::remove_reference<T>::type value_type;
    typedef value_type* pointer;
    typedef value_type const* const_pointer;
    typedef value_type& reference;
    typedef value_type const& const_reference;

    bool operator_bool() const { return IsValid; }

public:
    // get (references)
    const_reference operator*() const;
    reference operator*();

    const_reference get() const                         { return **this; }
    reference get()                                     { return **this; }
    friend const_reference get(Derived const& self)     { return *self; }
    friend reference get(Derived& self)                 { return *self; }

    // get_value_or, get_optional_value_or
    const_reference get_value_or(const_reference def) const;
    reference get_value_or(reference def);

    friend reference
    get_optional_value_or(Derived& self, reference def)
    { return self.get_value_or(def); }

    friend const_reference
    get_optional_value_or(Derived const& self, const_reference def)
    { return self.get_value_or(def); }

    // get (pointers)
    const_pointer get_ptr() const;
    pointer get_ptr();

    friend const_pointer get(Derived const* self) { return self->get_ptr(); }
    friend pointer get(Derived* self)             { return self->get_ptr(); }
    friend const_pointer
    get_pointer(Derived const* self)              { return self->get_ptr(); }
    friend pointer get_pointer(Derived* self)     { return self->get_ptr(); }

    // comparison operators
    bool operator!() const { return !static_cast<bool>(*this); }

    friend bool operator==(self_type const& x, self_type const& y) {
        return x && y ? *x == *y : !(x || y);
    }

    friend bool operator<(self_type const& x, self_type const& y) {
        return y && (!x || *x < *y);
    }

    // misc
    const_pointer operator->() const;
    pointer operator->();

    friend void swap(self_type& x, self_type& t);
};

// [new in 1.34]
template<class T> inline optional<T> make_optional ( T const& v ) ;

// [new in 1.34]
template<class T> inline optional<T> make_optional ( bool condition, T const& v ) ;
} // end namespace static_optional_detail

/*!
 *
 */
template <typename T>

} // end namespace detail
} // end namespace dyno

#endif // !DYNO_DETAIL_STATIC_OPTIONAL_HPP

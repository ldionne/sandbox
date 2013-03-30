
namespace detail {
    class dynamic_any;

    template <typename T>
    class static_any;
}

class object {
    detail::dynamic_any* self_;

public:
    object();

    template <typename T>
    object(T const& t);

    object(object const& other);

    ~object();

    object& swap(object& other);

    template <typename T>
    object& operator=(T const& t);

    object& operator=(object other);

    template <typename T>
    friend object operator+(object const& obj, T const& other)

    // and so on...

    std::ostream& operator<<(std::ostream& os);
};

namespace detail {
    class dynamic_any {
    public:
        virtual dynamic_any* clone() const = 0;

        virtual object add(void const* other) {
            throw no_method_error();
            return object(); // never reached.
        }

        virtual object sub(void const* other) {
            throw no_method_error();
            return object(); // never reached.
        }

        virtual std::ostream& operator<<(std::ostream& os) {
            throw no_method_error();
            return os; // never reached.
        }

        // and so on...

        virtual ~dynamic_any() {
            // COMBO BREAKER!
        }
    };

    // This is the bridge between compile-time and runtime.
    // We must implement all the operators supported by `T`.
    template <typename T>
    class static_any : public dynamic_any {
    public:
        static_any() { }

        static_any(T const& t) : value_(t) { }

        virtual dynamic_any* clone() const {
            return new static_any(value_);
        }

        virtual object add(void const* other) {
            return object(value_ + *static_cast<T const*>(other));
        }

        virtual object sub(void const* other) {
            return object(value_ - *static_cast<T const*>(other));
        }

        virtual std::ostream& operator<<(std::ostream& os) {
            return value_ << os;
        }

        // and so on...

    private:
        T value_;
    };
} // end namespace detail


object::object()
    : self_(0)
{ }

template <typename T>
object::object(T const& t)
    : self_(new detail::static_any<T>(t))
{ }

object::object(object const& other)
    : self_(other.self_ ? other.self_->clone() : 0)
{ }

object::~object() {
    delete self_;
}

object& object::swap(object& other) {
    std::swap(self_, other.self_);
    return *this;
}

template <typename T>
object& object::operator=(T const& t) {
    object(t).swap(*this);
    return *this;
}

object& object::operator=(object other) {
    other.swap(*this);
    return *this;
}

template <typename T>
object operator+(object const& obj, T const& other) {
    // If we REALLY have a static_any<T>, then either:
    //  - There will be no implemented method and we will fall back to
    //    dynamic_any's implementation, which will throw.
    //
    //  - There will be an implementation in static_any<T> that will
    //    forward to the real implementation by T. The casts from void*
    //    will be alright, because we make sure here that we pass types
    //    for which the casts are legal. While this restricts the types
    //    involved in the operation to what has been decided (T),
    //    it is still better than nothing.
    detail::static_any<T>* self = dynamic_cast<detail::static_any<T>*>(obj.self_);
    if (!self)
        throw bad_any_cast();

    return self->add(static_cast<void const*>(&other));
}

template <typename T>
object operator-(object const& obj, T const& other) {
    detail::static_any<T>* self = dynamic_cast<detail::static_any<T>*>(obj.self_);
    if (!self)
        throw bad_any_cast();

    return self->sub(static_cast<void const*>(&other));
}

template <typename T>
any operator+(any const& self, T const& other) {
    return any(boost::any_cast<U>(self) + other);
}

std::ostream& operator<<(std::ostream& os) {
    os << self_; // regular runtime dispatch.
}




struct my_any : boost::any {
    inline my_any() { }

    template <typename T>
    my_any(T const& t) : boost::any(t) { }

    using boost::any::operator=;

    template <typename T>
    operator T() {
        return boost::any_cast<T>(*this);
    }

    template <typename T>
    operator T() const {
        return boost::any_cast<T>(*this);
    }
};

class AnyEvent {
    my_any value_;

public:
    template <typename Ostream>
    Ostream& operator<<(Ostream& os, AnyEvent const& self) {
        os << value_;
        return os;
    }
};

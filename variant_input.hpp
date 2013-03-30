/**
 * Represents any type of event.
 * @note We can't use a typedef because we can't define our own output stream
 *       operator then.
 */
struct event : detail::event_types {
    inline event() { }
    template <typename T> event(T const& t) : detail::event_types(t) { }

    template <typename T>
    event& operator=(T const& t) {
        detail::event_types::operator=(t);
        return *this;
    }
};

namespace detail {
    namespace mpl = boost::mpl;
    typedef mpl::map<
        mpl::pair<acquire_event, mpl::char_<'a'> >,
        mpl::pair<release_event, mpl::char_<'r'> >,
        mpl::pair<start_event, mpl::char_<'s'> >,
        mpl::pair<join_event, mpl::char_<'j'> >
    > output_event_tags;

    template <typename Ostream>
    struct output_event_visitor : boost::static_visitor<> {
        Ostream& os_;
        explicit output_event_visitor(Ostream& os) : os_(os) { }

        template <typename Event>
        void operator()(Event const& e) {
            os_ << mpl::at<output_event_tags, Event>::type::value << e;
        }
    };

    template <typename First, typename Last>
    struct try_input {
        template <typename Istream>
        static void call(Istream& is, event& e, char tag) {
            typedef typename mpl::deref<First>::type EventTag;
            if (EventTag::second::value == tag) {
                typename EventTag::first tmp;
                is >> tmp;
                e = tmp;
            }
            else {
                typedef typename mpl::next<First>::type Next;
                try_input<Next, Last>::call(is, e, tag);
            }
        }
    };

    template <typename Last>
    struct try_input<Last, Last> {
        template <typename Istream>
        static void call(Istream&, event&, char) { }
    };
} // end namespace detail

template <typename Ostream>
Ostream& operator<<(Ostream& os, event const& e) {
    detail::output_event_visitor<Ostream> visitor(os);
    boost::apply_visitor(visitor, e);
    return os;
}

template <typename Istream>
Istream& operator>>(Istream& is, event& e) {
    char tag;
    is >> tag;
    detail::try_input<
        typename boost::mpl::begin<detail::output_event_tags>::type,
        typename boost::mpl::end<detail::output_event_tags>::type
    >::call(is, e, tag);
    return is;
}
namespace dyno { namespace events {
    struct lock_acquire;
    struct lock_release;
}}


struct mutex {
    void lock() {
        // ...
        dyno::generate<dyno::events::acquire>();
    }
};

namespace dyno {
    // can we automatically populate this vector with some ugly hacks?
    // for example by using something similar to my concept based overloading
    // technique?
    typedef boost::mpl::vector<...> FRAMEWORKS;

    template <typename Tag>
    void generate() {
        boost::mpl::for_each<FRAMEWORKS>(generate<Tag>);
    }
}

namespace d2 {
    // dyno defines all generic stuff that has to do with locks (and other objects).
    // we define the characteristics (a.k.a. how to classify) the objects.
    // we basically define what could be described as concepts in a way that
    // we can pattern match.
    namespace events {
        template <typename Recursive, typename Upgradable, typename ReadWrite>
        struct lock;
    }

    // dyno defines generic wrappers applicable to all locks (and other objects)
    namespace wrappers {
        template <typename Wrapped, typename Recursive, typename Upgradable, typename ReadWrite>
        struct lockable : Wrapped {
            using Wrapped::Wrapped;

            void lock() {
                Wrapped::lock();
                dyno::generate<events::lock<Recursive, Upgradable, ReadWrite> >(state_);
            }

            bool try_lock() {
                if (Wrapped::try_lock()) {
                    dyno::generate<events::try_lock_success<Recursive, Upgradable, ReadWrite> >(state_);
                    return true;
                }
                dyno::generate<events::try_lock_failure<Recursive, Upgradable, ReadWrite> >(state_);
                return false;
            }

            void unlock() {
                Wrapped::unlock();
                dyno::generate<events::release<Recursive, Upgradable, ReadWrite> >(state_);
            }

        private:
            d2_framework::state_for<lockable<Recursive, Upgradable, ReadWrite> > state_;
        };
    }

    // and then frameworks use these events to do stuff. it must also be trivial
    // to extend the set of existing events: dyno itself must use that extension
    // mechanism to define the core events it provides.

    /* ce n'est peut-être pas intéressant de spécifier les transforms
       comme avec proto, mais l'idée du pattern matching est vraiment
       intéressante. à la place d'avoir une forme en when<...> comme
       en haut, on pourrait grouper les grammaires à matcher dans un
       listens_to<...> et puis grouper les actions à faire lors d'un
       match dans une autre structure.
    */

    typedef dyno::framework<
        // use pattern matching (a complete proto grammar?) for determining
        // the events to listen to? we could listen to events with some
        // characteristics while ignoring events with some other characteristics,
        // and perform different actions based on those characteristics.
        //
        // note: events could have a lot of characteristics describing
        //       them. for example, necessary characteristics for lock
        //       acquires would be whether it is an upgrade, a read/write
        //       lock and so on.
        dyno::listens_to<
            dyno::events::lock_acquire<dyno::recursive>,
            dyno::events::lock_release<dyno::_>,
            dyno::events::lock_acquire<std::mutex> // we could go down to per-type granularity
        >,
        dyno::listener<d2_listener>
    > d2_framework;

    // statically registers the framework to the dyno::FRAMEWORKS vector.
#ifdef D2_ENABLED
    DYNO_REGISTER_FRAMEWORK(d2_framework)
#endif

    // another way of doing it would be to only specify what's really needed:
    // a visitor-like interface. using expression SFINAE, we could then detect
    // whether a framework has a method for handling an event (using pattern
    // matching). that would probably be the most concise way of doing things.
    struct d2_framework {
        // define a property that every lock has (maybe?)
        struct properties_of<any_lock> {
            properties_of() { lock_id = get_new_unique_lock_id(); }
            unsigned lock_id;
        };

        d2_framework() {
            // called during static initialization
        }

        template <typename Event>
        void operator()(dyno::events::lock_acquire<dyno::recursive>, Event const& event) const {
            getattr<thread_id>(event);
            getattr<lock_id>(event);
        }

        template <typename Event>
        void operator()(dyno::events::lock_release<dyno::_>, Event const& event) const {

        }
    };
}

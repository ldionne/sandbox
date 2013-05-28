//////////////////////////////////////////////////////////////////////////////
// This is an example of trying to automatize unit testing by using concepts
// and providing an implementation of its intrinsic that does semantic checking.
// I'm not sure the principle works, but this is an interesting idea since it
// would allow us to write unit tests for concepts and check whether implementations
// pass those tests instead of writing a unit test for each implementation.
//////////////////////////////////////////////////////////////////////////////








/*!
 * @file
 * This file contains the definition of the `dyno::UniquelyIdentifiable`
 * concept.
 */

#ifndef DYNO_CONCEPT_UNIQUELY_IDENTIFIABLE_HPP
#define DYNO_CONCEPT_UNIQUELY_IDENTIFIABLE_HPP

#include <dyno/fwd/concept/uniquely_identifiable.hpp>
#include <dyno/intrinsic/unique_id.hpp>

#include <boost/concept/usage.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_unsigned.hpp>


namespace dyno {
    template <typename U, typename UniqueId>
    struct UniquelyIdentifiable {
        typedef typename result_of::unique_id<U>::type Id;

        BOOST_CONCEPT_USAGE(UniquelyIdentifiable) {
            BOOST_MPL_ASSERT((boost::is_same<UniqueId, Id>));
            BOOST_MPL_ASSERT((boost::is_unsigned<Id>));

            (void)static_cast<Id>(unique_id(ref_to<U>()));
            (void)static_cast<Id>(unique_id(ref_to<U const>()));
            (void)static_cast<Id>(unique_id(temp_to<U>()));
        }

    private:
        template <typename T> static T temp_to() {
            return *static_cast<T*>(0);
        }

        template <typename T> static T& ref_to() {
            return *static_cast<T*>(0);
        }
    };

    template <typename U>
    struct UniquelyIdentifiableSemantics {
        typedef typename result_of::unique_id<U>::type N;
        typedef std::map<U, N> Bijection;

        static Bijection bijection;
        static void on_unique_id(U& self) {
            if (bijection.count(self))
                BOOST_ASSERT(bijection[self] == unique_id(self));
            else
                bijection.insert(self, unique_id(self));
        }
    };

    struct uniquely_identifiable : contract_checked<uniquely_identifiable> {
        friend std::size_t unique_id(uniquely_identifiable const& self) {
            return self.id_;
        }
    };

    template <typename T>
    ... unique_id(T& t) {
        typedef typename boost::mpl::if_<
                    is_contract_checked<T>, bypass_normal<T>, normal<T>
                >::type Func;
        return Func::call(t);
    }


    template <typename Checked>
    struct uid_contract_checker : Checked {
        using Checked::Checked; // fwd ctor
        typedef Checked checked_type;
    };

    namespace extension {
        template <>
        struct unique_id_impl<uid_contract_checker_detail::tag> {
            template <typename ContractChecker>
            struct apply {
                typedef typename ContractChecker::checked_type U;
                typedef typename result_of::unique_id<U>::type Id;

                typedef std::map<U, Id> Bijection;
                static Bijection bijection;

                typedef typename result_of::unique_id<U>::type type;

                static type call(U const& self) {
                    type my_unique_id = unique_id(self);

                    // a == b <==> unique_id(a) == unique_id(b)
                    {
                        if (bijection.count(self))
                            BOOST_ASSERT(bijection[self] == my_unique_id);
                        else
                            bijection.insert(self, my_unique_id);
                    }
                    return my_unique_id;
                }
            };
        };

        template <typename E, typename AttributeMap>
        struct checked {

            std::map<E*, AttributeMap> construction_args;

            template <typename Args>
            explicit checked(Args const& args)
                : E(args)
            {
                construction_args[this] = args; // fill as much as possible,
                                                // default construct the rest
            }

        };

        template <>
        struct getattr_impl<checked> {
            template <typename Tag, typename ContractChecker>
            struct apply {
                typedef ContractChecker::checked_type Event;
                typedef typename result_of::getattr<Tag, Event>::type type;
                static type call(Event& self) {
                    type attribute = getattr<Tag>(self);
                    // here, we assume the attribute was not modified in any
                    // way since the event was constructed.
                    BOOST_ASSERT(attribute == at_key<Tag>(construction_args[&self]));
                    return attribute;
                }
            };
        };
    }

    // to prove that U is correct with regard to function f, call f
    // with all u's from its domain while checking its contract.

    template <typename U>
    struct UniquelyIdentifiableSemantics {
        typedef typename result_of::unique_id<U>::type Id;
        static Id const max_ids = ::boost::integer_traits<Id>::const_max;


        static void check() {
            all_have_different_ids();
        }

        static void all_have_different_ids() {
            std::vector<Id> uniques;
            std::generate_n(std::back_inserter(uniques), 10000, new_unique_id);
            std::sort(uniques.begin(), uniques.end());
            ASSERT_TRUE(
                std::adjacent_find(uniques.begin(), uniques.end()) == uniques.end());
        }
    };
}

#endif // !DYNO_CONCEPT_UNIQUELY_IDENTIFIABLE_HPP

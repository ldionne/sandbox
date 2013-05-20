/*!
 * @file
 * This file contains the definition of the `dyno::EvaluationPolicy` concept.
 */

#ifndef DYNO_CONCEPT_EVALUATION_POLICY_HPP
#define DYNO_CONCEPT_EVALUATION_POLICY_HPP

#include <dyno/fwd/concept/evaluation_policy.hpp>

#include <boost/concept/usage.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>


namespace dyno {
    template <typename P>
    struct EvaluationPolicy {
        BOOST_CONCEPT_USAGE(EvaluationPolicy) {
            BOOST_MPL_ASSERT((is_an_evaluation_policy<P>));
        }

    private:
        template <typename T>
        struct is_an_evaluation_policy
            : boost::mpl::false_
        { };

        template <typename T>
        struct is_an_evaluation_policy<T const>
            : is_an_evaluation_policy<T>
        { };

        template <typename T>
        struct is_an_evaluation_policy<evaluation_policy<T> >
            : boost::mpl::true_
        { };
    };
}

#endif // !DYNO_CONCEPT_EVALUATION_POLICY_HPP

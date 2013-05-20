/*!
 * @file
 * This file contains the forward declaration of the `dyno::EvaluationPolicy`
 * concept.
 */

#ifndef DYNO_FWD_CONCEPT_EVALUATION_POLICY_HPP
#define DYNO_FWD_CONCEPT_EVALUATION_POLICY_HPP

#include <dyno/detail/doxygen.hpp>


namespace dyno {
    /*!
     * Dummy template used to create new evaluation policies.
     *
     * In order to create a new policy, one must create a unique tag type
     * that will be used to specialize `evaluation_policy`. If the unique
     * tag type is named `PolicyTag`, then `evaluation_policy<PolicyTag>`
     * is a model of the `EvaluationPolicy` concept.
     *
     * While it is possible to use any type as a policy tag, it is strongly
     * recommended to use a type with a name carrying the semantics of the
     * evaluation policy.
     *
     * It is also recommended to use a private type for policy tags; a
     * typedef to the actual policy should be created and the tag should
     * not be used beyond that point. This is to ensure that policy tags
     * are not mistaken for policies.
     *
     * @note
     * A dummy template is used because this allows partially
     * specializing metafunctions for evaluation policies only
     * with a single specialization, leveraging the awesome
     * pattern matching skills of the C++ compiler.
     */
    template <typename>
    struct evaluation_policy DYNO_DOXYGEN_FWD_DECL;

    /*!
     * Concept specification for the `EvaluationPolicy` concept.
     *
     * A type models the `EvaluationPolicy` concept iff it is a specialization
     * of the `evaluation_policy` template.
     */
    template <typename P>
    struct EvaluationPolicy DYNO_DOXYGEN_FWD_DECL;
}

#endif // !DYNO_FWD_CONCEPT_EVALUATION_POLICY_HPP

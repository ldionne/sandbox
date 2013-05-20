/*!
 * @file
 * This file contains the forward declaration of
 * `dyno::evaluation_policy_archetype`.
 */

#ifndef DYNO_FWD_ARCHETYPE_EVALUATION_POLICY_HPP
#define DYNO_FWD_ARCHETYPE_EVALUATION_POLICY_HPP

#include <dyno/fwd/concept/evaluation_policy.hpp>


namespace dyno {
    namespace evaluation_policy_archetype_detail { struct tag; }
    //! Archetype of the `EvaluationPolicy` concept.
    typedef evaluation_policy<
                evaluation_policy_archetype_detail::tag
            > evaluation_policy_archetype;
}

#endif // !DYNO_FWD_ARCHETYPE_EVALUATION_POLICY_HPP

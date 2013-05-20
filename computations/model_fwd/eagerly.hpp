/*!
 * @file
 * This file contains the forward declaration of the `dyno::eagerly`
 * evaluation policy.
 */

#ifndef DYNO_FWD_MODEL_EAGERLY_HPP
#define DYNO_FWD_MODEL_EAGERLY_HPP

#include <dyno/fwd/concept/evaluation_policy.hpp>


namespace dyno {
    namespace eagerly_detail { struct tag; }

    /*!
     * Evaluation policy for eagerly performed computations.
     *
     * The computation is performed once and as soon as it is created. If any,
     * the result is stored to avoid recomputing it.
     */
    typedef evaluation_policy<eagerly_detail::tag> eagerly;
}

#endif // !DYNO_FWD_MODEL_EAGERLY_HPP

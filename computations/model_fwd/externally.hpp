/*!
 * @file
 * This file contains the forward declaration of the `dyno::externally`
 * evaluation policy.
 */

#ifndef DYNO_FWD_MODEL_EXTERNALLY_HPP
#define DYNO_FWD_MODEL_EXTERNALLY_HPP

#include <dyno/fwd/concept/evaluation_policy.hpp>


namespace dyno {
    namespace externally_detail { struct tag; }

    /*!
     * Evaluation policy for computations whose result is provided externally.
     *
     * The result of the computation __must__ be provided when the computation
     * is created.
     */
    typedef evaluation_policy<externally_detail::tag> externally;
}

#endif // !DYNO_FWD_MODEL_EXTERNALLY_HPP

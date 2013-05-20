/*!
 * @file
 * This file contains the forward declaration of the `dyno::lazily`
 * evaluation policy.
 */

#ifndef DYNO_FWD_MODEL_LAZILY_HPP
#define DYNO_FWD_MODEL_LAZILY_HPP

#include <dyno/fwd/concept/evaluation_policy.hpp>


namespace dyno {
    namespace lazily_detail { struct tag; }

   /*!
    * Evaluation policy for lazily evaluated computations.
    *
    * Performing the computation is delayed until the result of the
    * computation is required. If the result of the computation is
    * never required, the computation is never performed.
    *
    * If the computation is performed, it is performed only once, the result
    * being stored and used on subsequent queries.
    *
    * @todo Support cached and normal computations. It could be more efficient
    *       to perform some trivial computations everytime we need them than
    *       to store them.
    */
    typedef evaluation_policy<lazily_detail::tag> lazily;
}

#endif // !DYNO_FWD_MODEL_LAZILY_HPP

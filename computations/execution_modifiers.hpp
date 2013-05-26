/*!
 * Execute a group of `Computations` when the enclosing `dyno::event` is
 * generated, with the possibility of parallelizing the work.
 *
 * The recommendation to perform work in parallel may be ignored.
 *
 * `Computations` may be specified with `dyno::computes`, `dyno::in_parallel`
 * and `dyno::in_order`. This modifier does not change the behavior of
 * `dyno::computation` with respect to the return value of a computation.
 *
 * @warning The warning expressed in `dyno::in_order` is valid here too.
 *
 * @tparam Computations...
 *         One or more computations to be performed.
 */
template <
    typename AtLeastOne, DYNO_DECLARE_VARIADIC_TEMPLATE_PARAMS(Computations)
>
struct in_parallel DYNO_DOXYGEN_FORWARD_DECL;

/*!
 * Execute a group of `Computations` in order when the enclosing
 * `dyno::event` is generated.
 *
 * Every unit of work in the group is guaranteed to be performed in its order
 * of appearance in the definition of `dyno::in_order`.
 *
 * `Computations` may be specified with `dyno::computes`, `dyno::in_parallel`
 * and `dyno::in_order`. This modifier does not change the behavior of
 * `dyno::computation` with respect to the return value of a computation.
 *
 * @warning It must be noted that the computation policy of a computation will
 *          always be respected. Thus, a lazy computation might __not__ be
 *          performed in its order of appearance in the modifier. Similarly,
 *          it makes no sense for a `dyno::already_computed` computation to
 *          be performed, since it is already computed and its value is
 *          provided when the event is generated. Thus, precomputed
 *          computations are ignored when determining the sequence of
 *          computations to perform.
 *
 * @tparam Computations...
 *         One or more computations to be performed.
 */
template <
    typename AtLeastOne, DYNO_DECLARE_VARIADIC_TEMPLATE_PARAMS(Computations)
>
struct in_order DYNO_DOXYGEN_FORWARD_DECL;

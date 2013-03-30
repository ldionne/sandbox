/**
 * This file defines the @em find_last_subsequence and
 * @em find_last_subsequence_comp algorithms.
 *
 * @note These algorithms are equivalent to the @em find_end algorithms of the
 *       C++ standard library.
 */

#ifndef NSTL_ALGORITHM_FIND_LAST_SUBSEQUENCE_H
#define NSTL_ALGORITHM_FIND_LAST_SUBSEQUENCE_H

#include <nstl/internal.h>


#define NSTL_FIND_LAST_SUBSEQUENCE(ForwardTraversalReadableIterator1,          \
                                   ForwardTraversalReadableIterator2, T)       \
    NSTL_I_FIND_LAST_SUBSEQUENCE(                                              \
        nstl_find_last_subsequence(ForwardTraversalReadableIterator1,          \
                                   ForwardTraversalReadableIterator2),         \
        ForwardTraversalReadableIterator1,                                     \
        ForwardTraversalReadableIterator2,                                     \
        T                                                                      \
    )                                                                          \
/**/

#define NSTL_I_FIND_LAST_SUBSEQUENCE(algo, Iter1, Iter2, T)                    \
NSTL_TYPE(algo,                                                                \
                                                                               \
(defun find_last_subsequence                                                   \
typedef nstl_bool (*nstl_helper(algo, impl_comp))(T, T);                       \
NSTL_GETF(                                                                     \
    NSTL_I_FIND_LAST_SUBSEQUENCE_COMP(                                         \
        nstl_helper(algo, impl),                                               \
        Iter1,                                                                 \
        Iter2,                                                                 \
        nstl_helper(algo, impl_comp)                                           \
    ),                                                                         \
    find_last_subsequence_comp                                                 \
)                                                                              \
                                                                               \
static NSTL_INLINE Iter1 algo(Iter1 first1, Iter1 last1,                       \
                              Iter2 first2, Iter2 last2) {                     \
    return nstl_helper(algo, impl)(first1, last1,                              \
                                   first2, last2, nstl_eq(T, T));              \
}                                                                              \
)                                                                              \
                                                                               \
)                                                                              \
/**/


#define NSTL_FIND_LAST_SUBSEQUENCE_COMP(ForwardTraversalReadableIterator1,     \
                                        ForwardTraversalReadableIterator2,     \
                                        Compare)                               \
    NSTL_I_FIND_LAST_SUBSEQUENCE_COMP(                                         \
        nstl_find_last_subsequence_comp(ForwardTraversalReadableIterator1,     \
                                        ForwardTraversalReadableIterator2,     \
                                        Compare),                              \
        ForwardTraversalReadableIterator1,                                     \
        ForwardTraversalReadableIterator2,                                     \
        Compare                                                                \
    )                                                                          \
/**/


#define NSTL_I_FIND_LAST_SUBSEQUENCE_COMP_FORWARD(algo, Iter1, Iter2, Comp)    \
NSTL_TYPE(algo,                                                                \
                                                                               \
(defun find_last_subsequence_comp                                              \
NSTL_GETF(                                                                     \
    NSTL_I_SEARCH_COMP(                                                        \
        nstl_helper(algo, search_comp),                                        \
        Iter1,                                                                 \
        Iter2,                                                                 \
        Comp                                                                   \
    ),                                                                         \
    search_comp                                                                \
)                                                                              \
                                                                               \
static Iter1 algo(Iter1 first1_, Iter1 last1_,                                 \
                  Iter2 first2, Iter2 last2, Comp comp) {                      \
    Iter1 first1;                                                              \
    Iter1 result;                                                              \
    if (nstl_eq(Iter1, Iter2)(first2, last2)) {                                \
        Iter1 last1;                                                           \
        nstl_copy_ctor(Iter1)(&last1, last1_);                                 \
        return last1;                                                          \
    }                                                                          \
    nstl_copy_ctor(Iter1)(&result, last1_);                                    \
    nstl_copy_ctor(Iter1)(&first1, first1_);                                   \
    while (nstl_true) {                                                        \
        Iter1 new_result = nstl_helper(algo, search_comp)(first1, last1,       \
                                                         first2, last2, comp); \
        if (nstl_eq(Iter1, Iter1)(new_result, last1)) {                        \
            nstl_dtor(Iter1)(&new_result);                                     \
            nstl_dtor(Iter1)(&first1);                                         \
            return result;                                                     \
        }                                                                      \
        nstl_asg(Iter1, Iter1)(&result, new_result);                           \
        nstl_asg(Iter1, Iter1)(&first1, new_result);                           \
        nstl_inc(Iter1)(&first1);                                              \
    }                                                                          \
}                                                                              \
)                                                                              \
                                                                               \
)                                                                              \
/**/


#define NSTL_I_FIND_LAST_SUBSEQUENCE_COMP_BIDIRECTIONAL(algo, Iter1, Iter2,    \
                                                                        Comp)  \
NSTL_TYPE(algo,                                                                \
                                                                               \
(defun find_last_subsequence_comp                                              \
static Iter1 algo(Iter1 first1, Iter1 last1,                                   \
                  Iter2 first2, Iter2 last2, Comp comp) {                      \
    nstl_reverse_iterator1 rfirst1(last1);                                     \
    nstl_reverse_iterator1 rlast1(first1);                                     \
    nstl_reverse_iterator1 rfirst2(last2);                                     \
    nstl_reverse_iterator2 rlast2(first2);                                     \
    nstl_reverse_iterator1 rresult = nstl_search(rfirst1, rlast1 rfirst2, rlast2, comp); \
    if (rresult == rlast1)                                                     \
        return last1;                                                          \
    Iter1 result = rresult.base();                                             \
    advance(result, -distance(first2, last2));                                 \
    return result;                                                             \
}                                                                              \
)                                                                              \
                                                                               \
)                                                                              \
/**/


/* [[[cog

import nstl
nstl.generate(cog,
    'find_last_subsequence(ForwardTraversalReadableIterator1, ' +
                           'ForwardTraversalReadableIterator2)',
    'find_last_subsequence_comp(ForwardTraversalReadableIterator1, ' +
                                'ForwardTraversalReadableIterator2, ' +
                                'Compare)',

    token=True, mangle=True,
)

]]] */
#include <joy/cat.h>
#define NSTL_TOKEN_find_last_subsequence (f i n d _ l a s t _ s u b s e q u e n c e)
#define nstl_find_last_subsequence(ForwardTraversalReadableIterator1,  ForwardTraversalReadableIterator2) JOY_CAT5(nstl_mangled_find_last_subsequence, _, ForwardTraversalReadableIterator1, _,  ForwardTraversalReadableIterator2)
#define NSTL_TOKEN_find_last_subsequence_comp (f i n d _ l a s t _ s u b s e q u e n c e _ c o m p)
#define nstl_find_last_subsequence_comp(ForwardTraversalReadableIterator1,  ForwardTraversalReadableIterator2,  Compare) JOY_CAT7(nstl_mangled_find_last_subsequence_comp, _, ForwardTraversalReadableIterator1, _,  ForwardTraversalReadableIterator2, _,  Compare)
/* [[[end]]] */

#endif /* !NSTL_ALGORITHM_FIND_LAST_SUBSEQUENCE_H */

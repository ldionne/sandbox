/*!
 * @file
 * Implementation of variadic argument passing using keywords.
 */

#ifndef KWARGS_H
#define KWARGS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "../lib/chaos/preprocessor.h"
#include "concat.h"
#include "narg.h"
#include "stringize.h"
#ifdef DEBUG
#	include "eprintf.h"
#endif


/*!	@warning	It is very important to be careful when using kwargs.
 *				Since they are implemented as compound literals, they become
 *				invalid whenever they go out of scope. Don't make the mistake
 *				of extending/using kwargs that are not in scope.
 */

typedef void *kwargs;

struct kwargs_control_ {
	struct kwargs_control_ *next;
	char const *kw;
};

/*!	This is a hack to make sure that we don't have alignment problems.
 *	Since we don't know the type of the content, we make sure that it
 *	will have maximum aligment anyway. This way, we can use the offsetof
 *	macro to pass the arguments in and out of our implementation functions.
 */
#define KWARGS_STRUCT_(...) \
	struct { \
		struct kwargs_control_ control; \
		union { \
			union { \
				char c; \
				short s; \
				long l; \
				long long ll; \
				float f; \
				double d; \
				long double ld; \
				void *p; \
				void (*fp) (); \
			} max_align; \
			struct { \
				/* avoid illegal empty struct */ \
				CHAOS_PP_IF(NARG(__VA_ARGS__)) (__VA_ARGS__, char c;) \
			} content; \
		} content; \
	}

/*!	Get the pointer to the real beginning of the kwargs, including the
 *	control structure.
 */

#define KWARGS_IN_(args) ( (struct kwargs_control_ *) ((char *)(args) - offsetof(KWARGS_STRUCT_(), content.content)) )

/*!	Skip the control structure at the beginning of the kwargs, giving direct
 *	access to the content of the arguments.
 */
#define KWARGS_OUT_(args) ( (kwargs) ((char *)(args) + offsetof(KWARGS_STRUCT_(), content.content)) )


/*!	Pass arguments using keywords.
 *	Usage :
 *		to_kwargs((Keyword1, type1, value1, ..., typeN, valueN),
 *				   ...
 *				  (KeywordN, type1, value1, ..., typeN, valueN))
 * @internal	This only links the nodes together, the kw is hashed
 *					when the compound literal is created.
 */
#define to_kwargs(...) \
	to_kwargs_I_(CHAOS_PP_SEQ_SIZE(__VA_ARGS__), (struct kwargs_control_* []){KWARGS_APPLY_SEQ_(KWARGS_FORMAT_, __VA_ARGS__)})
#define KWARGS_APPLY_SEQ_(macro, seq) \
	CHAOS_PP_EXPR(CHAOS_PP_SEQ_FOR_EACH(macro, seq))
static inline kwargs to_kwargs_I_ (unsigned n, struct kwargs_control_* args[n])
{
	struct kwargs_control_ **i;
	for (i = args; --n; ) {
		struct kwargs_control_ * const tmp = *i;
		tmp->next = *++i;
	}
	(*i)->next = NULL;
	return KWARGS_OUT_(args[0]);
}

/*!	Compare two keywords together. */
static inline bool kwargs_kw_equality_ (char const *s1, char const *s2)
{
	char a;
	while ((a = *s1++) == *s2++)
		if (a == '\0')
			return true;
	return false;
}

/*!	Retrieve arguments passed using to_kwargs(). */
#define from_kwargs(kw, args) from_kwargs_I_(STRINGIZE(kw), args)
static inline kwargs from_kwargs_I_ (char const *restrict kw, kwargs args)
{
	for (struct kwargs_control_ *i = KWARGS_IN_(args); i != NULL; i = i->next)
		if (kwargs_kw_equality_(i->kw, kw))
			return KWARGS_OUT_(i);
#ifdef DEBUG
	eprintf("Invalid keyword : %s", kw);
#endif
	return NULL;
}

/*!	Extend existing kwargs. */
static inline void kwargs_extend (kwargs existing, kwargs extension)
{
	struct kwargs_control_ *i;
	for (i = KWARGS_IN_(existing); i->next != NULL; i = i->next)
		;
	i->next = KWARGS_IN_(extension);
}

/*!	Form the type used to access kwargs in the callee.
 *	Usage :
 *		kwargs(type1 name1, ..., typeN nameN) identifier;
 *		identifier = from_kwargs(Keyword, args);
 */
#define kwargs(...) \
	struct { \
		CHAOS_PP_IF(NARG(__VA_ARGS__)) ( \
			CHAOS_PP_TUPLE_TO_STRING(NARG(__VA_ARGS__), \
				CHAOS_PP_EXPR(CHAOS_PP_TUPLE_TRANSFORM( \
					KWARGS_ADD_SEMICOLON_, (__VA_ARGS__) \
				)) \
			), /* nothing if no args */ \
		) \
	} *
#define KWARGS_ADD_SEMICOLON_(_, elem) elem ;


/*!	Deep formatting implementation of the kwargs. */
#define KWARGS_FORMAT_(_, ...) \
	KWARGS_FORMAT_I_(KWARGS_FORMAT_ARGTYPE_, KWARGS_FORMAT_ARGVALUE_, __VA_ARGS__)
#define KWARGS_FORMAT_GNU_(_, ...) \
	KWARGS_FORMAT_I_(KWARGS_FORMAT_ARGTYPE_GNU_, KWARGS_FORMAT_ARGVALUE_GNU_, __VA_ARGS__)
/*!	Make a tuple from the arguments and extract the keyword from it. */
#define KWARGS_FORMAT_I_(format_type, format_values, ...) \
	KWARGS_FORMAT_II_(format_type, format_values, CHAOS_PP_TUPLE_ELEM_ALT(0, (__VA_ARGS__)), CHAOS_PP_TUPLE_DROP(1, (__VA_ARGS__)))
/*!	Actual formatting.	*/
#define KWARGS_FORMAT_II_(format_type, format_values, kw_, types_values) \
	(&(( KWARGS_STRUCT_(KWARGS_APPLY_TUPLE_(format_type, types_values)) ){ \
			.control = {.kw = STRINGIZE(kw_)}, \
			.content.content = {KWARGS_APPLY_TUPLE_(format_values, types_values)} \
		}).control),
#define KWARGS_APPLY_TUPLE_(macro, tuple) \
		CHAOS_PP_EXPR(CHAOS_PP_TUPLE_FOR_EACH_I(macro, tuple))

/*!	Filter elements at odd indexes : +int+, -1-, +float+, -0.0- */
#define KWARGS_FORMAT_ARGTYPE_(_, index, type) \
	CHAOS_PP_WHEN(KWARGS_NUMBER_ISEVEN_(index)) ( \
		type CONCAT(kwargs_unique, index) ; \
	)
#define KWARGS_NUMBER_ISEVEN_(number) CHAOS_PP_EQUAL(CHAOS_PP_MOD(number, 2), 0)

/*!	Filter elements at even indexes : -int-, +1+, -float-, +0.0+ */
#define KWARGS_FORMAT_ARGVALUE_(_, index, value) \
	CHAOS_PP_WHEN(KWARGS_NUMBER_ISODD_(index)) ( \
		CHAOS_PP_COMMA_IF(CHAOS_PP_DEC(index)) value \
	)
#define KWARGS_NUMBER_ISODD_(number) CHAOS_PP_NOT(KWARGS_NUMBER_ISEVEN_(number))

/*!	Use the typeof() operator to resolve the value's type. */
#define KWARGS_FORMAT_ARGTYPE_GNU_(_, index, value) \
	typeof(value) CONCAT(kwargs_unique, index) ;
#define KWARGS_FORMAT_ARGVALUE_GNU_(_, index, value) \
	CHAOS_PP_COMMA_IF(index) value


#undef KWARGS_IN_
#undef KWARGS_OUT_

#ifdef __cplusplus
}
#endif
#endif /* !KWARGS_H */

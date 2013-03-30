
#ifndef VA_MORE_H
#define VA_MORE_H

#include <stdarg.h>


#define va_pop(list, type, i)                                                  \
do {                                                                           \
	typeof(i) __i;                                                             \
	(void) ((&__i) == (&i));                                                   \
	for (__i = (i); __i > 0; __i--) {                                          \
		(void) va_arg((list), type);                                           \
	}                                                                          \
} while (0)


#define va_sync(syncwith, ...)                                                 \
do {                                                                           \
	va_list __syncwith, __tosync[NARG(__VA_ARGS__)] = {__VA_ARGS__};           \
	va_copy(__syncwith, (syncwith));                                           \
	int __i;                                                                   \
	for (__i = 0; __i < NARG(__VA_ARGS__); __i++) {                            \
																			   \
		va_end(__tosync[__i]);                                                 \
		va_copy(__tosync[__i], __syncwith);                                    \
	}                                                                          \
} while (0)


#endif/* VA_MORE_H */

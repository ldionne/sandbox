
#ifndef ASSERT_CALLED_H
#define ASSERT_CALLED_H


/*!
 * This macro asserts that expr is called. If it is not called, an error is
 * generated.
 * For example :
 * @code
 *      typedef void (*voidfp) (int arg2);
 *      voidfp foo (int arg1);
 *      #define call_foo(arg1) ASSERT_CALLED(	foo(arg1)	)
 *      call_foo(0);
 * @endcode
 * This will generate an error
 * @code
 *      call_foo(0)(0);
 * @endcode
 * but this won't.
 *
 * It can also be used to make sure a function is not called via function pointer :
 * @code
 *      void bar (void);
 *      #define bar ASSERT_CALLED(bar)
 *      void (*pbar) (void) = bar;
 * @endcode
 * This will generate an error
 * @code
 *      bar();
 * @endcode
 * but this won't.
 */
#define ASSERT_CALLED(expr) (expr) MACRO_UTILS_ASSERT_CALLED_FAILED_
#define MACRO_UTILS_ASSERT_CALLED_FAILED_(...) (__VA_ARGS__)

#endif /* !ASSERT_CALLED_H */

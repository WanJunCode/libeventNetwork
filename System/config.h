#ifndef __WJ_CONFIG_H
#define __WJ_CONFIG_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>       // fabs()

#define INVALID_SOCKET -1
typedef unsigned char BYTE;

// 设置未使用的函数参数，防止编译器报错
#define UNUSED(x) (void)(x)

// 宏定义：是否检查pthread函数的返回值
#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail(int errnum,
                                 const char *file,
                                 unsigned int line,
                                 const char *function)
__THROW __attribute__((__noreturn__));
__END_DECLS
#endif

#define CHECK_RETURN_VALUE(ret) ({ __typeof__ (ret) errnum = (ret);         \
if (__builtin_expect(errnum != 0, 0))    \
    __assert_perror_fail(errnum, __FILE__, __LINE__, __func__); })

#else  // CHECK_PTHREAD_RETURN_VALUE

// __typeof__ (ret)  获得当前宏参数 ret 的类型
#define CHECK_RETURN_VALUE(ret) ({ __typeof__ (ret) errnum = (ret);         \
    assert(errnum == 0); (void)errnum; })

#endif // CHECK_PTHREAD_RETURN_VALUE

// 两种判断float浮点数是否为零的宏
const float PRECISION = 0.000001f;
#define ISZERO(ret) (ret >= -PRECISION && ret <= PRECISION)
#define FLOATZERO(flt) ((fabs(flt)<1e-15))

#endif
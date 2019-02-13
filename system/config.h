#ifndef __CF_CONFIG_H
#define __CF_CONFIG_H

#include <stdint.h>
#include <string.h>
#define INVALID_SOCKET -1
typedef unsigned char BYTE;
#include <assert.h>
#include <errno.h>

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

#define CHECK_RETURN_VALUE(ret) ({ __typeof__ (ret) errnum = (ret);         \
    assert(errnum == 0); (void)errnum; })

#endif // CHECK_PTHREAD_RETURN_VALUE

const float PRECISION = 0.000001f;

#define ISZERO(ret) (ret >= -PRECISION && ret <= PRECISION)

#endif
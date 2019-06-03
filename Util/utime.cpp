#include "utime.h"
#include <sys/time.h>

int64_t Util::currentTimeTicks(int64_t ticksPerSec) {
    int64_t result;
    struct timeval now;
    int ret = gettimeofday(&now, NULL);
    assert(ret == 0);
    toTicks(result, now, ticksPerSec);
    return result;
}
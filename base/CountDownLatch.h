#ifndef WJ_COUNT_DOWN_LATCH
#define WJ_COUNT_DOWN_LATCH
#include "noncopyable.h"
#include "Mutex.h"

// 默认继承方式是 private
class CountDownLatch : noncopyable{
private:
    mutable Mutex mutex_;
    Condition condition_;
    int count_;
public:
    explicit CountDownLatch(int count);
    void wait();        // 等待 count 为 0 
    void countDown();   // 减少 1 个 count 值
    int getCount() const;// 获得当前的 count 值
};

#endif // !WJ_COUNT_DOWN_LATCH



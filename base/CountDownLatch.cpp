#include "CountDownLatch.h"

// 使用构造函数创建 mutex_ 然后作为构造参数传入 condition_
CountDownLatch::CountDownLatch(int count)
  : mutex_(),
    condition_(mutex_),
    count_(count)
{
}

// 等待，一直等到 count_ == 0
void CountDownLatch::wait()
{
  MutexGuard lock(mutex_);
  while (count_ > 0)
  {
    condition_.wait();
  }
}

void CountDownLatch::countDown()
{
  MutexGuard lock(mutex_);
  --count_;
  if (count_ == 0)
  {
    condition_.notifyAll();
  }
}

int CountDownLatch::getCount() const
{
  MutexGuard lock(mutex_);
  return count_;
}


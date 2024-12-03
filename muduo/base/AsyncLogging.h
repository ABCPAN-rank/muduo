// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include "muduo/base/BlockingQueue.h"
#include "muduo/base/BoundedBlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/LogStream.h"

#include <atomic>
#include <vector>

namespace muduo
{

/**
 * @brief 异步日志类，提供高性能的日志记录功能
 * 
 * @details 该类使用双缓冲技术实现异步日志记录，通过后台线程写入日志文件,
 *          避免日志记录对主业务逻辑的性能影响。
 */
class AsyncLogging : noncopyable
{
 public:

  /**
   * @brief 构造函数
   * @param basename 日志文件基础名称
   * @param rollSize 单个日志文件的最大大小
   * @param flushInterval 日志刷新间隔（秒）
   */
  AsyncLogging(const string& basename,
               off_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() NO_THREAD_SAFETY_ANALYSIS
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  void threadFunc();

  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  const int flushInterval_;
  std::atomic<bool> running_;
  const string basename_;
  const off_t rollSize_;
  muduo::Thread thread_;
  muduo::CountDownLatch latch_;
  muduo::MutexLock mutex_;
  muduo::Condition cond_ GUARDED_BY(mutex_);
  BufferPtr currentBuffer_ GUARDED_BY(mutex_);
  BufferPtr nextBuffer_ GUARDED_BY(mutex_);
  BufferVector buffers_ GUARDED_BY(mutex_);
};

}  // namespace muduo

#endif  // MUDUO_BASE_ASYNCLOGGING_H

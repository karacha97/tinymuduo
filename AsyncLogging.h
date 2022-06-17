#ifndef ASYNCLOGGING_H
#define ASYNCLOGGING_H

#include "noncopyable.h"
#include "Thread.h"
#include "CountDownLatch.h"
#include "LogStream.h"

#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
namespace tinymuduo
{
    class AsyncLogging : noncopyable
    {
    public:
        AsyncLogging(const std::string &basename,
                     off_t rollSize, int flushInterval = 2);
        ~AsyncLogging()
        {
            if (running_)
            {
                stop();
            }
        }
        void append(const char *logline, int len);
        void start()
        {
            running_ = true;
            thread_.start();
            
            latch_.wait();
            //等待线程真正开启才退出
        
        }
        void stop()
        {
            running_ = false;
            cond_.notify_one();
            thread_.join();
        }

    private:
        using Buffer=detail::FixedBuffer<detail::kLargeBuffer>;
        using BufferVector = std::vector<std::unique_ptr<Buffer>>;
        using BufferPtr = BufferVector::value_type;

        void threadFunc();
        const int flushInterval_; //超时时间
        bool running_;
        const std::string basename_;
        const off_t rollSize_;
        tinymuduo::Thread thread_;
        tinymuduo::CountDownLatch latch_;
        std::mutex mutex_;
        std::condition_variable cond_;
        BufferPtr currentBuffer_;
        BufferPtr nextBuffer_;
        BufferVector buffers_;
    };

} // namespace tinymuduo

#endif // ASYNCLOGGING_H
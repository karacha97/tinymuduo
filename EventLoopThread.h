#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include <mutex>
#include <condition_variable>
#include <memory>

#include "Thread.h"
namespace tinymuduo
{
    namespace net
    {
        class EventLoop;
        class EventLoopThread :noncopyable
        {
        public:
        using ThreadInitCallback=std::function<void(EventLoop*)>;

            EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const std::string& name = std::string());
            ~EventLoopThread();
            EventLoop* startLoop();
            private:
            void threadFunc();
            EventLoop* loop_;
            bool exiting_;
            Thread thread_;
            std::mutex mutex_;
            std::condition_variable cond_;
            ThreadInitCallback callback_;
        };
    } // namespace net

} // namespace tinymuduo

#endif // EVENTLOOPTHREAD_H
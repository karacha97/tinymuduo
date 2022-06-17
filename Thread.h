#ifndef THREAD_H
#define THREAD_H
#include "noncopyable.h"
#include "CountDownLatch.h"
#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

namespace tinymuduo
{
    class Thread : noncopyable
    {

    public:
        using ThreadFunc = std::function<void()>;
        //线程回调函数
        explicit Thread(ThreadFunc, const std::string &name = std::string());
        ~Thread();
        void start();
        void join();
        bool started() const { return started_; }

    private:
        void setDefaultName();
        CountDownLatch latch_;
        bool started_;
        bool joined_;
        //使用C++11线程
        std::shared_ptr<std::thread> thread_;
        //使用sharedptr管理线程
        pid_t tid_;
        //线程真实id
        ThreadFunc func_;
        std::string name_;
        //使用C++11原子操作
        static std::atomic_int numCreated_;
    };

} // namespace tinymuduo

#endif // THREAD_H
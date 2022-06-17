#include "Thread.h"
#include "CurrentThread.h"
using namespace tinymuduo;

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name),
      latch_(1)
{
    setDefaultName();
}
Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach();
    }
}
void Thread::start()
{
    started_ = true;
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                           {
        tid_=CurrentThread::tid();
        CurrentThread::t_threadName=name_.empty()?"muduoThread" : name_.c_str();
        latch_.countDown();
        func_();
        CurrentThread::t_threadName = "finished";
         }));
    //需要等待上面获取新创建的线程的返回值
    latch_.wait();
}
void Thread::join()
{
    joined_ = true;
    thread_->join();
}


void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

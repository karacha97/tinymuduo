#include "EventLoopThread.h"
#include "EventLoop.h"
using namespace tinymuduo;
using namespace tinymuduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb ,
                                 const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb)
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if(loop_!=nullptr){
        loop_->quit();
        thread_.join();
    }
}
EventLoop *EventLoopThread::startLoop()
{
    thread_.start();
    // 启动底层的新线程
    EventLoop *loop=nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_==nullptr)
        {
            cond_.wait(lock);
        }
        loop=loop_;
        
    }
    return loop;
}
void EventLoopThread::threadFunc()
//在单独的新线程里面运行的
{
    EventLoop loop;
    //one loop per thread;
    if(callback_){
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;    //当前新线程中的loop
        cond_.notify_one();
    }
    loop.loop();
    //开始执行loop
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;

}
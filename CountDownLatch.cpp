#include "CountDownLatch.h"
using namespace tinymuduo;
CountDownLatch::CountDownLatch(int count) : mutex_(),
                                            cond_(),
                                            count_(count)
{
}
CountDownLatch::~CountDownLatch(){};
void CountDownLatch::wait(){
    std::unique_lock<std::mutex> lock(mutex_);
    while (count_>0)
    {
        cond_.wait(lock);
    }
    
}
void CountDownLatch::countDown(){
    std::unique_lock<std::mutex> lock(mutex_);
    --count_;
    if(count_==0){
        cond_.notify_all();
    }
}
int CountDownLatch::getCount() const{
    std::unique_lock<std::mutex> lock(mutex_);
    return count_;
}
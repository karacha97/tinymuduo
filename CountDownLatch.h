#ifndef COUNTDOWNLATCH_H
#define COUNTDOWNLATCH_H
#include <mutex>
#include <condition_variable>
#include <memory>
namespace tinymuduo
{
    class CountDownLatch
    {
    public:
        explicit CountDownLatch(int count);
        ~CountDownLatch();
        void wait();
        void countDown();
        int getCount() const;

    private:
        mutable std::mutex mutex_;
        std::condition_variable cond_;
        int count_;
    };

} // namespace tinymuduo

#endif // COUNTDOWNLATCH_H
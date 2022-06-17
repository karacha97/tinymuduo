#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <iostream>
#include <string>
#include "copyable.h"
namespace tinymuduo
{
    class Timestamp : public tinymuduo::copyable
    {
    public:
        Timestamp();
        explicit Timestamp(int64_t microSecondsSinceEpoch);
        static Timestamp now();
        std::string toString() const;
        void swap(Timestamp &that)
        {
            std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
        }

    private:
        int64_t microSecondsSinceEpoch_;
    };
    // 时间类
} // namespace tinymuduo
#endif // TIMESTAMP_H
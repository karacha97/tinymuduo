#include "LogStream.h"
#include <algorithm>
#include <limits>
#include <type_traits>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

using namespace tinymuduo;
using namespace tinymuduo::detail;
namespace tinymuduo
{
    namespace detail
    {
        const char digits[] = "9876543210123456789";
        const char *zero = digits + 9;

        template <typename T>
        size_t convert(char buf[], T value)
        {
            //格式转换:转换成10进制
            T i = value;
            char *p = buf;

            do
            {
                int lsd = static_cast<int>(i % 10);
                i /= 10;
                *p++ = zero[lsd];
            } while (i != 0);

            if (value < 0)
            {
                *p++ = '-';
                //负数
            }
            *p = '\0';
            std::reverse(buf, p);

            return p - buf;
        }
        template class FixedBuffer<kSmallBuffer>;
        template class FixedBuffer<kLargeBuffer>;
    } // namespace detail

    template <typename T>
    void LogStream::formatInteger(T v)
    {
        //转换成字符串
        if (buffer_.avail() >= kMaxNumericSize)
        {
            //如果空间小于size则丢弃
            size_t len = convert(buffer_.current(), v);
            buffer_.add(len);
        }
    }

    LogStream &LogStream::operator<<(short v)
    {
        //整型类型都是先转换成int
        *this << static_cast<int>(v);
        return *this;
    }

    LogStream &LogStream::operator<<(unsigned short v)
    {
        *this << static_cast<unsigned int>(v);
        return *this;
    }

    LogStream &LogStream::operator<<(int v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream &LogStream::operator<<(unsigned int v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream &LogStream::operator<<(long v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream &LogStream::operator<<(unsigned long v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream &LogStream::operator<<(long long v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream &LogStream::operator<<(unsigned long long v)
    {
        formatInteger(v);
        return *this;
    }
    LogStream &LogStream::operator<<(double v)
    {
        if (buffer_.avail() >= kMaxNumericSize)
        {
            int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
            buffer_.add(len);
        }
        return *this;
    }
} // namespace tinymuduo

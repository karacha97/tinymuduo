#pragma once

#include <memory>
#include <functional>
#include "Timestamp.h"
namespace tinymuduo
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    namespace net
    {
        class Buffer;
        class TcpConnection;
       //Timestamp需要include完整的包，因为不是指针

        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
        using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                                   Buffer *,
                                                   Timestamp)>;
        using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
    } // namespace net
} // namespace tinymuduo

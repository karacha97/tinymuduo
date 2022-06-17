#ifndef ACCEPTOR_H
#define ACCEPTOR_H
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

namespace tinymuduo
{
    namespace net
    {
        class EventLoop;
        class InetAddress;

        class Acceptor : noncopyable
        {
        public:
            using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
            Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
            ~Acceptor();

            void setNewConnectionCallback(const NewConnectionCallback &cb)
            {
                //往其他子reactor或者loop中注册新连接
                newConnectionCallback_ = cb;
            }

            bool listenning() const { return listenning_; }
            void listen();

        private:
            void handleRead();

            EventLoop *loop_; // Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
            Socket acceptSocket_;   //监听端口对应的sockfd
            Channel acceptChannel_; //监听端口对应的channel
            NewConnectionCallback newConnectionCallback_;
            bool listenning_;
        };

    } // namespace net

} // namespace tinymuduo

#endif

#ifndef SOCKET_H
#define SOCKET_H

#include "noncopyable.h"
namespace tinymuduo
{
    namespace net
    {
        class InetAddress;
        class Socket : noncopyable
        {

        public:
            explicit Socket(int sockfd) : sockfd_(sockfd) {}
            ~Socket();
            int fd() const { return sockfd_; }
            void bindAddress(const InetAddress &localaddr);
            void listen();
            int accept(InetAddress *peeradder);
            void shutdownWrite();

            void setTcpNoDelay(bool on);//TCP_NODELAY (disable/enable Nagle's algorithm).
            void setReuseAddr(bool on);//SO_REUSEADDR
            void setReusePort(bool on);//SO_REUSEPORT
            void setKeepAlive(bool on);//SO_KEEPALIVE

        private:
            int sockfd_;
        };

    } // namespace net

} // namespace tinymuduo

#endif
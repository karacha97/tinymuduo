#ifndef INETADDRESS_H
#define INETADDRESS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装socket地址类型

namespace tinymuduo
{
    namespace net
    {
        class InetAddress
        {
        public:
            explicit InetAddress(uint16_t port = 8000, std::string ip = "127.0.0.1"); //默认本机
            explicit InetAddress(const sockaddr_in &addr)
                : addr_(addr)
            {
            }

            std::string toIp() const;
            std::string toIpPort() const;
            uint16_t toPort() const;

            const sockaddr_in *getSockAddr() const { return &addr_; }
            void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

        private:
            sockaddr_in addr_; //地址
        };

    } // namespace net

} // namespace tinymuduo
#endif // INETADDRESS_H
#include "Acceptor.h"
#include "Logging.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

using namespace tinymuduo;
using namespace tinymuduo::net;

namespace
{
    static int createNonblocking()
    {
        int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (sockfd < 0)
        {
            LOG_FATAL << "listen socket create err";
        }
        return sockfd;
    }

} // namespace

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop),                              // baseloop_
      acceptSocket_(createNonblocking()),       // sockfd非阻塞的！
      acceptChannel_(loop, acceptSocket_.fd()), //新建一个channel，绑定baseloop和fd
      listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr); // bind
    // TcpServer::start() Acceptor.listen  有新用户的连接，要执行一个回调（connfd=》channel=》subloop）
    // baseLoop => acceptChannel_(listenfd) =>
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this)); //设置回调
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen(); // listen 
    acceptChannel_.enableReading(); // acceptChannel_ => Poller
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;   // 新用户的地址
    int connfd = acceptSocket_.accept(&peerAddr);   
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr); 
            // 轮询找到subLoop，唤醒，分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_SYSERR << "in Acceptor::handleRead";
        if (errno == EMFILE)
        {
            LOG_ERROR<<"sockfd reached limit!";
        }
    }
}
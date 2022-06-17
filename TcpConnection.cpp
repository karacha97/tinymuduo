#include "TcpConnection.h"
#include "Logging.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>

using namespace tinymuduo;
using namespace tinymuduo::net;

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 64M
{
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    socket_->setKeepAlive(true);
}
TcpConnection::~TcpConnection()
{
}

void TcpConnection::send(const std::string &buf)
{
    //向outputbuf中发送
    //一般数据发送时:数据->json ..这里直接改成发string了
    if (state_ == kConnected)
    {
        //前提是已经建立连接了
        if (loop_->isInLoopThread())
        {
            //判断是否是在当前线程执行
            //一般而言是在响应的loop中执行
            //某些情况下可能会在其他线程统一执行发送。
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            //修改了这里，重载函数bind可能会引起二义性？
            void (TcpConnection::*fp)(const void *data, size_t len) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(
                fp,
                this,
                buf.c_str(),
                buf.size()));
        }
    }
}

void TcpConnection::send(Buffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const std::string &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp,
                          this, // FIXME
                          buf->retrieveAllAsString()));
        }
    }
}

/**
 * 发送数据  应用写的快， 而内核发送数据慢，
 *  需要把待发送数据写入缓冲区， 而且设置了水位回调
 */
void TcpConnection::sendInLoop(const std::string &message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false; //是否产生错误
    if (state_ == kDisconnected)
    {
        //断开连接了
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        //没有事件正在写，并且缓冲中没有数据，直接就写了！
        //或者表示channel第一次开始写数据，并且缓冲区没有待发送数据
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            //发送成功
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                //写完了，且有写入完成的回调
                loop_->queueInLoop(std::bind(
                    writeCompleteCallback_,
                    shared_from_this()));
            }
        }
        else
        {
            // nwrote<0
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    //接受到对端重置了，说明有错误发生
                    faultError = true;
                }
            }
        }
    } //已经一次写完了，不用=调用epoll注册handlewrite事件了

    // 说明当前这一次write，并没有把数据全部发送出去，
    // 剩余的数据需要保存到缓冲区当中，然后给channel
    // 注册epollout事件，poller发现tcp的发送缓冲区有空间，
    // 会通知相应的sock-channel，调用writeCallback_回调方法
    // 也就是调用TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完成
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();                      //已经有的待发送数据长度
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ //上一次没有调高水位的回调
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
            // 这里一定要注册channel的写事件，否则poller不会给channel通知epollout

            // 水平触发需要epoll_ctl是因为，
            // 只要send buffer没有满就会一直触发EPOLLOUT，因此需要关闭EPOLLOUT
        }
    }
}
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}
void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明outputBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite(); // 关闭写端
    }                             //如果没有发送完，不执行，仅仅是将state改成kDisconnecting
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); //绑定
    channel_->enableReading();         //新连接建立，向Epoll注册读事件，执行回调

    connectionCallback_(shared_from_this());
}
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        //之前handleclose已经disable过了
        connectionCallback_(shared_from_this());
        //这里其实connectionCallback应该啥都不做（也可以操作，根据自己需求）
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    //如果是ET呢？
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 有可读事件发生，调用回调操作
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        // shared_from_this() 获得一个TcpConnection智能指针
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "TCPConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        //这里可以用ET
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) //发送完了
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    // why 是queueInLoop？
                    // 在执行此方法时，必然是在TcpConnection所在的线程中了
                    // 这样写也没有问题，runInLoop也可以？优先级更高
                    // 唤醒loop_对应的thread线程，执行回调
                }
                if (state_ = kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }
    else
    {
        //要执行handlewrite，但是channel并不是可写
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    setState(kDisconnected);
    channel_->disableAll();
    // 要通知用户的onconnection以及closcallback
    TcpConnectionPtr guardThis(shared_from_this());
    // shared_from_this():功能为返回一个当前类的std::share_ptr,
    if (connectionCallback_)
    {
        connectionCallback_(guardThis);
    }
    //?
    if (closeCallback_)
    {
        closeCallback_(guardThis); //执行的是TcpServer::removeConnection回调方法
    }
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
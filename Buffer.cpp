#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace tinymuduo;
using namespace tinymuduo::net;

const char Buffer::kCRLF[] = "\r\n";

ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; //栈上的内存空间  64K
    struct iovec vec[2];

    const size_t writable = writableBytes();
    // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    //一次至少能读64k
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n < writable) // buffer已经够了
    {
        writerIndex_ += n;
    }
    else
    {
        // extrabuf里面也写入了数据
        writerIndex_ = buffer_.size();
        // writerIndex_开始写 n - writable大小的数据
        append(extrabuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

ssize_t Buffer::readFdET(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; //栈上的内存空间  64K
    struct iovec vec[2];

    size_t writable = writableBytes();
    // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    //一次至少能读64k
    const int iovcnt = 2;
    ssize_t readlen = 0;
    while (true)
    {
        const ssize_t n = ::readv(fd, vec, iovcnt);
        if (n < 0)
        {
            
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                //读完了
                break;
            }
            *saveErrno = errno;
            return -1;
        }
        else if (n == 0)
        {
            //没有读到数据
            return 0;
        }
        else if (n <= writable)
        {
            //读了，但是buffer还没满
            writerIndex_ += n;
        }
        else
        {
            // 已经写满缓冲区, 则需要把剩余的buf写进去
            writerIndex_ = buffer_.size();
            append(extrabuf, n - writable);
        }

        // 写完后需要更新 vec[0] 便于下一次读入
        writable = writableBytes();
        vec[0].iov_base = begin() + writerIndex_;
        vec[0].iov_len = writable;
        readlen += n;
    }
    return readlen;
}

ssize_t Buffer::writeFdET(int fd, int *saveErrno)
{

    ssize_t writelen = 0;

    while (true)
    {
        ssize_t n = ::write(fd, peek(), readableBytes());
        if (n > 0)
        {
            writelen += n;
            retrieve(n); // 更新可读索引
            if (readableBytes() == 0)
            {
                //一次写完了
                return writelen;
            }
        }
        else if (n < 0)
        {
            if (errno == EAGAIN) //系统缓冲区满，非阻塞返回
            {
                break;
            }
            // 暂未考虑其他错误
            else
            {
                return -1;
            }
        }
        else
        {
            // 返回0的情况，查看write的man，可以发现，一般是不会返回0的
            return 0;
        }
    }
    return writelen;
}
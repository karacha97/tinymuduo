#include "RingBuffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace tinymuduo;
using namespace tinymuduo::net;

const char RingBuffer::kCRLF[] = "\r\n";
void RingBuffer::makeSpace(size_t len)
{
    if (writerIndex_ >= readerIndex_)
    {
        buffer_.resize(writerIndex_ + len);
    }
    else
    {
        size_t readable = readableBytes();
        std::vector<char> newbuffer(buffer_.size() + len);
        std::copy(begin() + readerIndex_, begin() + buffer_.size(), &*newbuffer.begin() + kCheapPrepend);
        std::copy(begin(), begin() + writerIndex_, &*newbuffer.begin() + kCheapPrepend + buffer_.size() - readerIndex_);
        buffer_ = std::move(newbuffer);
        // buffer_=newbuffer;
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}

// 从fd上读取数据
ssize_t RingBuffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536]; // 64*1024
    struct iovec vec[3];
    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;

    if (writerIndex_ < readerIndex_)
    {
        /*
        | readableBytes | writableBytes | prepend | readableBytes |

                   writerIndex_               readerIndex_
        */
        vec[0].iov_len = writable;
        // 如果可写数据只在头端，则不使用第二块缓冲区
        vec[1].iov_base = begin();
        vec[1].iov_len = 0;
    }
    else // 如果可写数据横跨缓冲区尾端和头端
    {
        /*
        | writableBytes | prepend | readableBytes | writableBytes |

                            readerIndex_       writerIndex_
        */
        vec[0].iov_len = buffer_.size() - writerIndex_;
        vec[1].iov_base = begin();
        vec[1].iov_len = writable - vec[0].iov_len;
    }
    vec[2].iov_base = extrabuf;
    vec[2].iov_len = sizeof(extrabuf);
    // readv()代表分散读， 即将数据从文件描述符读到分散的内存块中
    const ssize_t n = readv(fd, vec, 3);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        writerIndex_ += n;
        writerIndex_ %= buffer_.size();
    }
    else
    {
        writerIndex_ += writable;
        writerIndex_ %= buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

ssize_t RingBuffer::writeFd(int fd, int *saveErrno)
{
    struct iovec vec[2];
    if (writerIndex_ < readerIndex_)
    {
        vec[0].iov_base = begin() + readerIndex_;
        vec[0].iov_len = buffer_.size() - readerIndex_;
        vec[1].iov_base = begin();
        vec[1].iov_len = writerIndex_;
    }
    else
    {
        vec[0].iov_base = begin() + readerIndex_;
        vec[0].iov_len = writerIndex_ - readerIndex_;
        vec[1].iov_base = begin();
        vec[1].iov_len = 0;
    }
    ssize_t n = ::writev(fd, vec, readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

ssize_t RingBuffer::readFdET(int fd, int *saveErrno)
{
    char extrabuf[65536]; // 64*1024
    struct iovec vec[3];
    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;

    if (writerIndex_ < readerIndex_)
    {
        /*
        | readableBytes | writableBytes | prepend | readableBytes |

                   writerIndex_               readerIndex_
        */
        vec[0].iov_len = writable;
        // 如果可写数据只在头端，则不使用第二块缓冲区
        vec[1].iov_base = begin();
        vec[1].iov_len = 0;
    }
    else // 如果可写数据横跨缓冲区尾端和头端
    {
        /*
        | writableBytes | prepend | readableBytes | writableBytes |

                            readerIndex_       writerIndex_
        */
        vec[0].iov_len = buffer_.size() - writerIndex_;
        vec[1].iov_base = begin();
        vec[1].iov_len = writable - vec[0].iov_len;
    }
    vec[2].iov_base = extrabuf;
    vec[2].iov_len = sizeof(extrabuf);
    // readv()代表分散读， 即将数据从文件描述符读到分散的内存块中

    ssize_t readlen = 0;
    while (true)
    {
        ssize_t n = readv(fd, vec, 3);
        if (n < 0)
        {
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                break;
            }
            *saveErrno = errno;
            return -1;
        }
        else if (n == 0)
        {
            return 0;
        }
        else if (n <= writable)
        {
            //读了，buffer未满
            writerIndex_ += n;
            writerIndex_ %= buffer_.size();
        }
        else
        {
            writerIndex_ += writable;
            writerIndex_ %= buffer_.size();
            append(extrabuf, n - writable);
        }
        readlen += n;
    }
    return readlen;
}
ssize_t RingBuffer::writeFdET(int fd, int *saveErrno)
{
    ssize_t writesum = 0;
  // 从可读位置开始读取
  struct iovec vec[2];
  while(true) {
    if (writerIndex_ < readerIndex_) {
      vec[0].iov_base = begin()+ readerIndex_;
      vec[0].iov_len = buffer_.size() - readerIndex_;
      vec[1].iov_base = begin();
      vec[1].iov_len = writerIndex_;
    } else {
      vec[0].iov_base = begin() + readerIndex_;
      // vec[0].iov_len = readableBytes();
      vec[0].iov_len = (writerIndex_ - readerIndex_);
      vec[1].iov_base = begin();
      vec[1].iov_len = 0;
    }
    //写之前需要不断更新iovec
    ssize_t n = ::writev(fd, vec, 2);
    if (n > 0) {
      writesum += n;
      retrieve(n); // 更新可读索引
      if (readableBytes() == 0) {
        return writesum;
      }
    } else if (n < 0) {
      if (errno == EAGAIN) //系统缓冲区满，非阻塞返回
      {
        break;
      }
      // 暂未考虑其他错误
      else {
        return -1;
      }
    } else {
      // 返回0的情况，查看write的man，可以发现，一般是不会返回0的
      return 0;
    }
  }
  return writesum;
}
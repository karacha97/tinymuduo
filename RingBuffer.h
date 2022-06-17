#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <vector>
#include <string>
#include <algorithm>
namespace tinymuduo
{
    namespace net
    {
        //优化:采用环形缓冲区，避免频繁移动
        class RingBuffer
        {
        public:
            static const size_t kCheapPrepend = 8;
            static const size_t kInitialSize = 1024;

            explicit RingBuffer(size_t initialSize = kInitialSize)
                : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
            {
            }

            size_t readableBytes() const
            {
                if (writerIndex_ >= readerIndex_)
                {
                    //非环状
                    return writerIndex_ - readerIndex_;
                }
                //环
                return buffer_.size() - readerIndex_ + writerIndex_;
            }

            size_t writableBytes() const
            {
                if (writerIndex_ >= readerIndex_)
                {
                    return buffer_.size() - writerIndex_;
                }
                return readerIndex_ - writerIndex_ - kCheapPrepend;
                //可写的buffer长度
            }

            size_t prependableBytes() const
            {
                return readerIndex_;
                //当前读到了哪里，因为可能不会一次读完
            }

            // 返回缓冲区中可读数据的起始地址
            const char *peek() const
            {
                return begin() + readerIndex_;
            }

            const char *findCRLF() const
            {
                if (writerIndex_ >= readerIndex_)
                {
                    const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
                    return crlf == beginWrite() ? NULL : crlf;
                }else{
                    if(buffer_.back()==kCRLF[0]&&buffer_.front()==kCRLF[1]){
                        return begin()+buffer_.size()-1;
                    }
                    const char *crlfR=std::search(peek(),begin()+buffer_.size(),kCRLF, kCRLF + 2);
                    if(crlfR!=begin()+buffer_.size()){
                        return crlfR;
                    }
                    const char* crlfL=std::search(begin(),beginWrite(),kCRLF,kCRLF+2);
                    return crlfL==beginWrite()?NULL:crlfL;
                }
            }

            // onMessage string <- Buffer
            void retrieve(size_t len)
            {
                if (len < readableBytes())
                {
                    readerIndex_ += len;
                    readerIndex_ %= buffer_.size();
                    // 应用只读取了刻度缓冲区数据的一部分，就是len，
                    //还剩下readerIndex_ += len -> writerIndex_
                }
                else // len == readableBytes()
                {
                    retrieveAll();
                }
            }

            void retrieveAll()
            {
                readerIndex_ = writerIndex_ = kCheapPrepend;
                //更新，相当于清零
            }

            // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
            std::string retrieveAllAsString()
            {
                return retrieveAsString(readableBytes()); // 应用可读取数据的长度
            }

            std::string retrieveAsString(size_t len)
            {
                if (writerIndex_ >= readerIndex_)
                {
                    std::string result(peek(), len);
                    //转成string
                    retrieve(len); // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
                    return result;
                }
                //成环的情况
                if (len <= buffer_.size() - readerIndex_)
                {
                    //只读前半段
                    std::string result(peek(), len);
                    retrieve(len);
                    return result;
                }
                std::string result(peek(), buffer_.size() - readerIndex_);
                std::string resbegin(begin(), writerIndex_);
                result += resbegin;
                retrieve(len);
                return result;
            }

            // buffer_.size() - writerIndex_    len
            void ensureWriteableBytes(size_t len)
            {
                if (writableBytes() < len)
                {
                    makeSpace(len); // 扩容函数
                }
            }

            // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
            void append(const char *data, size_t len)
            {
                ensureWriteableBytes(len);
                //如果扩容了，就会变成正常的顺序
                //容量必然足够
                if (writerIndex_ < readerIndex_)
                {
                    //这是没有扩容的
                    /*
                    | readableBytes | writableBytes | prepend | readableBytes |

                                writerIndex_               readerIndex_
                    */
                    std::copy(data, data + len, beginWrite());
                }
                else
                {
                    /*
                    | writableBytes | prepend | readableBytes | writableBytes |

                                        readerIndex_       writerIndex_
                    */
                    //如果尾部空间够
                    if (buffer_.size() - writerIndex_ >= len)
                    {
                        std::copy(data, data + len, beginWrite());
                        writerIndex_ += len;
                    }
                    else
                    {
                        size_t reserve_tail = buffer_.size() - writerIndex_;
                        std::copy(data, data + reserve_tail, beginWrite());
                        writerIndex_ = len - reserve_tail;
                        std::copy(data + reserve_tail, data + len, begin());
                    }
                }
            }

            char *beginWrite()
            {
                return begin() + writerIndex_;
            }

            const char *beginWrite() const
            {
                return begin() + writerIndex_;
            }

            // 从fd上读取数据
            ssize_t readFd(int fd, int *saveErrno);
            // 通过fd发送数据
            ssize_t writeFd(int fd, int *saveErrno);

            ssize_t readFdET(int fd, int *saveErrno);
            ssize_t writeFdET(int fd, int *saveErrno);

        private:
            char *begin()
            {
                // it.operator*()
                return &*buffer_.begin(); // vector底层数组首元素的地址，也就是数组的起始地址
            }
            const char *begin() const
            {
                return &*buffer_.begin();
            }
            void makeSpace(size_t len);
            std::vector<char> buffer_;
            size_t readerIndex_;
            size_t writerIndex_;
            static const char kCRLF[];
        };

    } // namespace net

} // namespace tinymuduo

#endif // RINGBUFFER_H
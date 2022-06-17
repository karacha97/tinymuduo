#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>

namespace tinymuduo
{
    AsyncLogging::AsyncLogging(const std::string &basename,
                               off_t rollSize,
                               int flushInterval) : basename_(basename),
                                                    rollSize_(rollSize),
                                                    flushInterval_(flushInterval),
                                                    thread_(std::bind(&AsyncLogging::threadFunc, this), "LoggingThread"),
                                                    latch_(1),
                                                    mutex_(),
                                                    cond_(),
                                                    currentBuffer_(new Buffer),
                                                    nextBuffer_(new Buffer),
                                                    buffers_()
    {
        currentBuffer_->bzero();
        nextBuffer_->bzero();
        //两块buffer清空
        buffers_.reserve(16);
        // vector备用16个
    }

    void AsyncLogging::append(const char *logline, int len)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (currentBuffer_->avail() > len)
        {
            currentBuffer_->append(logline, len);
        }
        else
        {
            buffers_.push_back(std::move(currentBuffer_));
            //先将当前buffer存入buffers
            if (nextBuffer_)
            {
                //如果有备用buffer,使用它
                currentBuffer_ = std::move(nextBuffer_);
            }
            else
            {
                //否则新建一个buffer
                currentBuffer_.reset(new Buffer);
            }
            currentBuffer_->append(logline, len);
            cond_.notify_one();
        }
    }

    void AsyncLogging::threadFunc(){
        //实际上是4组buffer
        latch_.countDown();
        
        LogFile output(basename_,rollSize_);
        //绑定输出的文件流
        //新建两个buffer，这里实际上是uniqueptr？
        BufferPtr newBuffer1(new Buffer);
        BufferPtr newBuffer2(new Buffer);
        newBuffer1->bzero();
        newBuffer2->bzero();
        
        BufferVector buffersToWrite;
        buffersToWrite.reserve(16);
        //新建一个vector
        while(running_){
            {
                std::unique_lock<std::mutex> lock(mutex_);
                //加锁
                
                if(buffers_.empty()){
                    // 如果 buffer 为空,那么表示没有数据需要写入文件,那么就等待指定的时间
                    //cond_.waitForSeconds(flushInterval_);
                    cond_.wait(lock);
                }
                //无论 cond是因为什么被唤醒，都要将currentBuffer_放到buffers_中
                buffers_.push_back(std::move(currentBuffer_));
                currentBuffer_=std::move(newBuffer1);
                //currentBuffer更新成新的
                buffersToWrite.swap(buffers_);
                //交换!
                if(!nextBuffer_){
                    nextBuffer_=std::move(newBuffer2);
                }
            }
            //两组待输出buffer交换
            if(buffersToWrite.size()>25){
                //如果将要写入文件的buffer个数大于25,那么将多余数据删除,消息堆积
                // 丢掉多余日志， 以腾出内存， 仅保留两块缓冲区
                buffersToWrite.erase(buffersToWrite.begin()+2,buffersToWrite.end());
            }
            for(const auto& buffer:buffersToWrite){
                //把buffer输出到Appendfile中
                //因为LogFile是自己管理缓冲区，因此不用操心啥时候刷入磁盘
                output.append(buffer->data(),buffer->length());
            }

            if(buffersToWrite.size()>2){
                buffersToWrite.resize(2);
                //保留两块buffer
            }
            //重置两块buffer
            //why从vector中取？不用新开辟了，如果new一个再销毁vector中之前存的就有开销
            if(!newBuffer1){
                newBuffer1=std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer1->reset();
            }
            if(!newBuffer2){
                newBuffer2=std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer2->reset();
            }
            buffersToWrite.clear();
            //清除所有元素,实际上不需要了吧？已经清空过了
            output.flush();
            //当然也可以显示地刷入磁盘
            //实际上可能不需要
            
        }
        output.flush();
        //剩余内容刷入文件
    }

} // namespace tinymuduo

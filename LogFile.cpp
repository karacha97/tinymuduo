#include "LogFile.h"
#include "FileUtil.h"
#include <stdio.h>
#include <time.h>
using namespace tinymuduo;
LogFile::LogFile(const std::string &basename, off_t rollSize, int flushEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushEveryN_(flushEveryN),
      count_(0),
      mutex_() // 默认为加锁
{
    time_t now = 0;
    std::string filename = getLogFileName(basename, &now);
    file_.reset(new FileUtil::AppendFile(filename));
    //智能指针初始化
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len)
{
    std::unique_lock<std::mutex> lock(mutex_);
    append_unlocked(logline, len);
}

void LogFile::append_unlocked(const char *logline, int len)
{
    file_->append(logline, len);
    if (file_->writtenBytes() > rollSize_)
    {   //超出文件上限了
        //重新创建一个文件
        rollFile();
    }
    else
    {
        count_++;
        if (count_ > flushEveryN_)
        {
            count_ = 0;
            file_->flush();
        }
    }
}

void LogFile::flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    file_->flush();
}

std::string LogFile::getLogFileName(const std::string &basename, time_t *now)
{
    //根据时间定义Log文件名
    std::string filename;
    filename.reserve(basename.size() + 32);
    filename = basename;
    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now, &tm);
    strftime(timebuf, sizeof(timebuf), "%Y%m%d-%H%M%S", &tm);
    filename += timebuf;
    filename += ".log";
    return filename;
}

bool LogFile::rollFile()
{
    time_t now = 0;
    //创建新的输出文件
    std::string filename = getLogFileName(basename_, &now);
    file_.reset(new FileUtil::AppendFile(filename));
    return true;
}
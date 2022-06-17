#ifndef LOGFILE_H
#define LOGFILE_H
#include <mutex>
#include <memory>
#include "noncopyable.h"

namespace tinymuduo
{
    namespace FileUtil
    {
        class AppendFile;
    }
    class LogFile : noncopyable
    {
    public:
        LogFile(const std::string &basename, 
        off_t rollSize, 
        int flushEveryN=1024);
        ~LogFile();
       void append(const char *logline, int len);
        void flush();
        bool rollFile();

    private:
        void append_unlocked(const char *logline, int len); // 不加锁的 append方式
        std::string getLogFileName(const std::string &basename, time_t *now);
        const std::string basename_; // 日志文件的 basename
        off_t rollSize_;    //刷新的大小
        const int flushEveryN_;
        int count_;
        // 计数器，检测是否需要换新文件
        std::mutex mutex_;
        // 加锁
        std::unique_ptr<FileUtil::AppendFile> file_;
        // 文件智能指针
    };

} // namespace tinymuduo

#endif // LOGFILE_H
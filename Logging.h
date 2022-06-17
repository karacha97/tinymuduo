#ifndef LOGGING_H
#define LOGGING_H

#include "LogStream.h"
//#include "Timestamp.h"
#include <sys/time.h>
#include <string.h>
namespace tinymuduo
{
    class Logger
    {
    public:
        enum LogLevel
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            NUM_LOG_LEVELS,
        };
        //定义Log的等级

        class SourceFile
        {
            //存文件名字的结构
            //实际使用时用__FILE__宏传入，这里会做一些处理
            //只取文件名
            //其实也可以不用这个操作，直接输出绝对路径也可以
        public:
            template <int N>
            SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1)
            {
                const char *slash = strrchr(data_, '/'); //查找最后一个出现'/'的
                //如果传递的是src/filename，那么返回的应该是/filename
                if (slash)
                {
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr); //减去之前的无效的
                }
            }
            explicit SourceFile(const char *filename) : data_(filename)
            {
                const char *slash = strrchr(filename, '/');
                if (slash)
                {
                    data_ = slash + 1;
                }
                size_ = static_cast<int>(strlen(data_));
            }
            const char *data_; //文件名
            int size_;         //文件名长度
        };
        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char *func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();
        LogStream &stream() { return impl_.stream_; }

        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);              //设定日志显示级别
        typedef void (*OutputFunc)(const char *msg, int len); //输出回调函数
        typedef void (*FlushFunc)();                          //刷新回调函数
        static void setOutput(OutputFunc);
        static void setFlush(FlushFunc);
        // static void setTimeZone(const TimeZone &tz);
    private:
        class Impl
        {
        public:
            typedef Logger::LogLevel LogLevel;
            Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
            void formatTime();
            void finish();

            // Timestamp time_;
            LogStream stream_;
            LogLevel level_;
            int line_;
            SourceFile basename_;//当前的文件名，
        };
        //Impl编程技巧：用一个指针代替对象。
        Impl impl_;
    };

    extern Logger::LogLevel g_logLevel;
    inline Logger::LogLevel Logger::logLevel()
    {
        return g_logLevel;
    }
#define LOG_TRACE                                                  \
    if (tinymuduo::Logger::logLevel() <= tinymuduo::Logger::TRACE) \
    tinymuduo::Logger(__FILE__, __LINE__, tinymuduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                                  \
    if (tinymuduo::Logger::logLevel() <= tinymuduo::Logger::DEBUG) \
    tinymuduo::Logger(__FILE__, __LINE__, tinymuduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                                  \
    if (tinymuduo::Logger::logLevel() <= tinymuduo::Logger::INFO) \
    tinymuduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN tinymuduo::Logger(__FILE__, __LINE__, tinymuduo::Logger::WARN).stream()
#define LOG_ERROR tinymuduo::Logger(__FILE__, __LINE__, tinymuduo::Logger::ERROR).stream()
#define LOG_FATAL tinymuduo::Logger(__FILE__, __LINE__, tinymuduo::Logger::FATAL).stream()
#define LOG_SYSERR tinymuduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL tinymuduo::Logger(__FILE__, __LINE__, true).stream()

    const char *strerror_tl(int savedErrno);
#define CHECK_NOTNULL(val) \
    ::tinymuduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

    template <typename T>
    T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
    {
        if (ptr == NULL)
        {
            Logger(file, line, Logger::FATAL).stream() << names;
        }
        return ptr;
    }

} // namespace tinymuduo

#endif // LOGGING_H
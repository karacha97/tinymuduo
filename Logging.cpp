#include "Logging.h"
#include "CurrentThread.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

namespace tinymuduo
{
    thread_local char t_errnobuf[512];
    thread_local char t_time[64];
    thread_local time_t t_lastSecond;

    const char *strerror_tl(int savedErrno)
    {
        return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
        //对于函数strerror_r，第一个参数errnum是错误代码，
        //第二个参数buf是用户提供的存储错误描述的缓存，
        //第三个参数n是缓存的大小。
        // returns a string describing the error code passed in the argument errnum
    }
    Logger::LogLevel g_logLevel = Logger::INFO;
    //设定当前输出的日志等级是INFO
    const char *LogLevelName[Logger::NUM_LOG_LEVELS] = {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
    };

    class T
    {
    public:
        T(const char *str, unsigned len) : str_(str), len_(len) {}
        const char *str_;
        const unsigned len_;
    };
    inline LogStream &operator<<(LogStream &s, T v)
    {
        s.append(v.str_, v.len_);
        return s;
    }
    inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v)
    {
        //对应的当前文件名以及行号
        //Q:为何要定义在这里而不是在LogStream中
        //A:为了使用SourceFile类
        s.append(v.data_, v.size_);
        return s;
    }

    void defaultOutput(const char *msg, int len)
    {
        size_t n = fwrite(msg, 1, len, stdout);
        // FIXME check n
        (void)n;
    }

    void defaultFlush()
    {
        fflush(stdout);
    }

    Logger::OutputFunc g_output = defaultOutput;
    Logger::FlushFunc g_flush = defaultFlush;

    Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile &file, int line)
        : stream_(),
          level_(level),
          line_(line),
          basename_(file)
    {
        formatTime();
        CurrentThread::tid();
        stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
        stream_ << T(LogLevelName[level], 6);
        if (savedErrno != 0)
        {
            stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
        }
    }

    void Logger::Impl::formatTime()
    {
        struct timeval tv;
        time_t time;
        char str_t[26] = {0};
        gettimeofday(&tv, NULL);
        time = tv.tv_sec;
        struct tm *p_time = localtime(&time);
        strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
        stream_ << str_t;
    }

    void Logger::Impl::finish()
    {
        //写入当前文件名以及行号
        stream_ << " - " << basename_ << ':' << line_ << '\n';
    }

    Logger::Logger(SourceFile file, int line)
        : impl_(INFO, 0, file, line)
    {
        //初始化
    }

    Logger::Logger(SourceFile file, int line, LogLevel level, const char *func)
        : impl_(level, 0, file, line)
    {
        impl_.stream_ << func << ' ';
    }

    Logger::Logger(SourceFile file, int line, LogLevel level)
        : impl_(level, 0, file, line)
    {
    }

    Logger::Logger(SourceFile file, int line, bool toAbort)
        : impl_(toAbort ? FATAL : ERROR, errno, file, line)
    {
    }

    Logger::~Logger()
    {
        //logger析构时，会将内容写入LogStream对应的buffer中
        //Logger->impl_.stream_.buf
        //impl_.stream_.buf->fwrite的buffer(标准IO的缓冲区)
        
        //然后调用g_output将buf的数据保存到标准IO的缓冲区中
        //标准库IO会自己控制缓冲区刷入磁盘或者内存的时机
        impl_.finish();
        const LogStream::Buffer &buf(stream().buffer());
        g_output(buf.data(), buf.length());
        if (impl_.level_ == FATAL)
        {
            g_flush();
            abort();
        }
    }

    void Logger::setLogLevel(Logger::LogLevel level)
    {
        g_logLevel = level;
    }

    void Logger::setOutput(OutputFunc out)
    {
        g_output = out;
    }

    void Logger::setFlush(FlushFunc flush)
    {
        g_flush = flush;
    }

} // namespace tinymuduo
#include "FileUtil.h"
#include "Logging.h"
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

using namespace tinymuduo;
using namespace tinymuduo::FileUtil;

AppendFile::AppendFile(std::string filename) : fp_(fopen(filename.c_str(), "ae")),
                                               writtenBytes_(0)
{
    
    ::setbuffer(fp_, buffer_, sizeof(buffer_));
    //设置缓冲区，缓冲类型是全缓冲，buffer满了自动刷入文件。
}
AppendFile::~AppendFile()
{
    ::fclose(fp_);
}
void AppendFile::append(const char *logline, size_t len)
{
    size_t written = 0;

    while (written != len)
    {
        size_t remain = len - written;
        size_t n = write(logline + written, remain);
        if (n != remain) //是不是有点问题。
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
                break;
            }
        }
        written += n;
    }

    writtenBytes_ += written;
}
void FileUtil::AppendFile::flush()
{
    ::fflush(fp_);
}

size_t FileUtil::AppendFile::write(const char *logline, size_t len)
{
    // #undef fwrite_unlocked
    //不带锁的向流中输入数据，因为流自带缓冲区管理，所以实际上可能是先
    //向buffer中缓存，然后buffer满了调用系统调用刷入文件
    //这样效率会更高，避免多次系统调用.
    return ::fwrite_unlocked(logline, 1, len, fp_);
}
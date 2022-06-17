#ifndef FILEUTIL_H
#define FILEUTIL_H
#include "noncopyable.h"
#include <sys/types.h>
#include <string>
namespace tinymuduo
{
    namespace FileUtil
    {
        //定义一个和流绑定的文件，自带64kb缓冲区
        class AppendFile : noncopyable
        {
        public:
            explicit AppendFile(std::string filename);
            ~AppendFile();
            void append(const char *logline, size_t len);
            void flush();
            off_t writtenBytes(){
                return writtenBytes_;
            }

        private:
            size_t write(const char *logline, size_t len);
            FILE *fp_;
            //fp_:对应的流，绑定某个文件
            //自带缓冲区管理，这里指定缓冲区是buffer_大小是64k
            char buffer_[64 * 1024];    //缓冲区
            off_t writtenBytes_;
        };

    } // namespace tinymuduo
}
#endif // FILEUTIL_H
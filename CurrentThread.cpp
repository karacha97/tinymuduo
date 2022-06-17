#include "CurrentThread.h"
#include <stdio.h>

namespace tinymuduo
{
    namespace CurrentThread
    {
        thread_local int t_cachedTid = 0;
        thread_local char t_tidString[32];
        thread_local int t_tidStringLength = 6;
        thread_local const char *t_threadName = "unknown";
        void cacheTid()
        {
            if (t_cachedTid == 0)
            {
                // 通过linux系统调用，获取当前线程的tid值
                t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
                t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
            }
        }

    }
}
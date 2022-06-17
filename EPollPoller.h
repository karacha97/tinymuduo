#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H
#include "EventLoop.h"
#include "Timestamp.h"
#include <sys/epoll.h>
#include <map>
#include <vector>
namespace tinymuduo
{
    namespace net
    {
        class EPollPoller : noncopyable
        {

        public:
            using ChannelList = std::vector<Channel *>;
            EPollPoller(EventLoop *loop);
            ~EPollPoller();
            Timestamp poll(int timeoutMs, ChannelList* activeChannels);
            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);
            bool hasChannel(Channel *channel) const;

        protected:
            using ChannelMap = std::map<int, Channel *>;
            ChannelMap channels_;

        private:
            static const int kInitEventListSize = 16;
            // 填写活跃的连接
            void fillActiveChannels(int numEvents,
                                    ChannelList *activeChannels) const;
            // 更新channel通道
            void update(int operation, Channel *channel);
            using EventList = std::vector<epoll_event>;
        
            int epollfd_;
            EventList events_;
            EventLoop *ownerLoop_;
        };

    } // namespace net

} // namespace tinymuduo
#endif // EPOLLPOLLER_H
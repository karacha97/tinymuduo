#include "EPollPoller.h"
#include "Logging.h"
#include "Channel.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

using namespace tinymuduo;
using namespace tinymuduo::net;

namespace
{
    const int kNew = -1;    //某个channel还没加入Poller
    const int kAdded = 1;   //已经加入Poller
    const int kDeleted = 2; //已经从Poller中删除

} // namespace

EPollPoller::EPollPoller(EventLoop *loop)
    : ownerLoop_(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    //封装了epoll_wait;
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_INFO << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        //会传入一个ChannelList,应该分配足够的大小监听事件。
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
            //扩容
        }
    }
    else if (numEvents == 0)
    {
        LOG_TRACE << "nothing happened";
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::poll()";
        }
    }
    return now;
}
void EPollPoller::updateChannel(Channel *channel)
{

    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd()
              << " events = " << channel->events() << " index = " << index;
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)
        {
            channels_[fd] = channel;
            // channel加入epoll中
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
        //实际调用epollCTL
        //检测此fd
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    int index = channel->index();

    size_t n = channels_.erase(fd);

    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}
bool EPollPoller::hasChannel(Channel *channel) const
{
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}
void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// 更新channel通道
void EPollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    bzero(&event, sizeof(event));
    // memZero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    // event.data.fd = channel->fd();
    int fd = channel->fd();
    // event.data.fd =fd;
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_SYSERR << "epoll_ctl op =" << operation << " fd =" << fd;
        }
        else
        {
            LOG_SYSFATAL << "epoll_ctl op =" << operation << " fd =" << fd;
        }
    }
}
//
// Created by tian on 2020/11/23.
//

#include "poller.h"
#include "channel.h"

//#ifdef OS_LINUX
CPollerBase *createPoller() {
    return new CPollerEpoll();
}

CPollerEpoll::CPollerEpoll() {
    fd_ = epoll_create1(EPOLL_CLOEXEC);
    if(fd_ < 0){
        iFatal("epoll_create error %d %s", errno, strerror(errno));
    }
    iInfo("poller epoll %d created", fd_);
}

CPollerEpoll::~CPollerEpoll() {
    //iInfo("destroying poller %d", fd_);
    while (liveChannels_.size()) {
        (*liveChannels_.begin())->close();
    }
    ::close(fd_);
    iInfo("poller %d destroyed", fd_);
}

void CPollerEpoll::addChannel(CChannel *ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    iTrace("adding channel %lld fd %d events %d epoll %d", (long long) ch->id(), ch->fd(), ev.events, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
    if(r){
        iFatal("epoll_ctl add failed %d %s", errno, strerror(errno));
    }
    liveChannels_.insert(ch);
}

void CPollerEpoll::updateChannel(CChannel *ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    iTrace("modifying channel %lld fd %d events read %d write %d epoll %d", (long long) ch->id(), ch->fd(), ev.events & POLLIN, ev.events & POLLOUT, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    //fatalif(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
    if(r){
        iFatal("epoll_ctl mod failed %d %s", errno, strerror(errno));
    }
}

void CPollerEpoll::removeChannel(CChannel *ch) {
    iTrace("deleting channel %lld fd %d epoll %d", (long long) ch->id(), ch->fd(), fd_);
    liveChannels_.erase(ch);
    for (int i = lastActive_; i >= 0; i--) {
        if (ch == activeEvs_[i].data.ptr) {
            activeEvs_[i].data.ptr = NULL;
            break;
        }
    }
}

void CPollerEpoll::loop_once(int waitMs) {
    int64_t ticks = TimeZoo::timeMilli();
    lastActive_ = epoll_wait(fd_, activeEvs_, kMaxEvents, waitMs);
    int64_t used = TimeZoo::timeMilli() - ticks;
    iTrace("epoll wait %d return %d errno %d used %lld millsecond", waitMs, lastActive_, errno, (long long) used);
    //fatalif(lastActive_ == -1 && errno != EINTR, "epoll return error %d %s", errno, strerror(errno));
    if(lastActive_ == -1 && errno != EINTR){
        iFatal("epoll return error %d %s", errno, strerror(errno));
    }
    while (--lastActive_ >= 0) {
        int i = lastActive_;
        CChannel *ch = (CChannel *) activeEvs_[i].data.ptr;
        int events = activeEvs_[i].events;
        if (ch) {
            if (events & (kReadEvent | POLLERR)) {
                iTrace("channel %lld fd %d handle read", (long long) ch->id(), ch->fd());
                ch->handleRead();

            } else if (events & kWriteEvent) {
                iTrace("channel %lld fd %d handle write", (long long) ch->id(), ch->fd());
                ch->handleWrite();
            } else {
                iFatal("unexpected poller events");
            }
        }
    }
}
//#elif defined(OS_MACOSX)
//CPollerKqueue::PollerKqueue() {
//    fd_ = kqueue();
//    fatalif(fd_ < 0, "kqueue error %d %s", errno, strerror(errno));
//    iInfo("poller kqueue %d created", fd_);
//}
//
//CPollerKqueue::~PollerKqueue() {
//    iInfo("destroying poller %d", fd_);
//    while (liveChannels_.size()) {
//        (*liveChannels_.begin())->close();
//    }
//    ::close(fd_);
//    iInfo("poller %d destroyed", fd_);
//}
//
//void CPollerKqueue::addChannel(CChannel *ch) {
//    struct timespec now;
//    now.tv_nsec = 0;
//    now.tv_sec = 0;
//    struct kevent ev[2];
//    int n = 0;
//    if (ch->readEnabled()) {
//        EV_SET(&ev[n++], ch->fd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, ch);
//    }
//    if (ch->writeEnabled()) {
//        EV_SET(&ev[n++], ch->fd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, ch);
//    }
//    iTrace("adding channel %lld fd %d events read %d write %d  epoll %d", (long long) ch->id(), ch->fd(), ch->events() & POLLIN, ch->events() & POLLOUT, fd_);
//    int r = kevent(fd_, ev, n, NULL, 0, &now);
//    if(r)
//        iFatal("kevent add failed %d %s", errno, strerror(errno));
//
//    liveChannels_.insert(ch);
//}
//
//void CPollerKqueue::updateChannel(CChannel *ch) {
//    struct timespec now;
//    now.tv_nsec = 0;
//    now.tv_sec = 0;
//    struct kevent ev[2];
//    int n = 0;
//    if (ch->readEnabled()) {
//        EV_SET(&ev[n++], ch->fd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, ch);
//    } else {
//        EV_SET(&ev[n++], ch->fd(), EVFILT_READ, EV_DELETE, 0, 0, ch);
//    }
//    if (ch->writeEnabled()) {
//        EV_SET(&ev[n++], ch->fd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, ch);
//    } else {
//        EV_SET(&ev[n++], ch->fd(), EVFILT_WRITE, EV_DELETE, 0, 0, ch);
//    }
//    iTrace("modifying channel %lld fd %d events read %d write %d epoll %d", (long long) ch->id(), ch->fd(), ch->events() & POLLIN, ch->events() & POLLOUT, fd_);
//    int r = kevent(fd_, ev, n, NULL, 0, &now);
//    if(r)
//        iFatal("kevent mod failed %d %s", errno, strerror(errno));
//}
//
//void CPollerKqueue::removeChannel(CChannel *ch) {
//    trace("deleting channel %lld fd %d epoll %d", (long long) ch->id(), ch->fd(), fd_);
//    liveChannels_.erase(ch);
//    // remove channel if in ready stat
//    for (int i = lastActive_; i >= 0; i--) {
//        if (ch == activeEvs_[i].udata) {
//            activeEvs_[i].udata = NULL;
//            break;
//        }
//    }
//}
//
//void CPollerKqueue::loop_once(int waitMs) {
//    struct timespec timeout;
//    timeout.tv_sec = waitMs / 1000;
//    timeout.tv_nsec = (waitMs % 1000) * 1000 * 1000;
//    long ticks = util::timeMilli();
//    lastActive_ = kevent(fd_, NULL, 0, activeEvs_, kMaxEvents, &timeout);
//    iTrace("kevent wait %d return %d errno %d used %lld millsecond", waitMs, lastActive_, errno, util::timeMilli() - ticks);
//    fatalif(lastActive_ == -1 && errno != EINTR, "kevent return error %d %s", errno, strerror(errno));
//    while (--lastActive_ >= 0) {
//        int i = lastActive_;
//        Channel *ch = (Channel *) activeEvs_[i].udata;
//        struct kevent &ke = activeEvs_[i];
//        if (ch) {
//            // only handle write if read and write are enabled
//            if (!(ke.flags & EV_EOF) && ch->writeEnabled()) {
//                iTrace("channel %lld fd %d handle write", (long long) ch->id(), ch->fd());
//                ch->handleWrite();
//            } else if ((ke.flags & EV_EOF) || ch->readEnabled()) {
//                iTrace("channel %lld fd %d handle read", (long long) ch->id(), ch->fd());
//                ch->handleRead();
//            } else {
//                iFatal("unexpected epoll events %d", ch->events());
//            }
//        }
//    }
//}
//#endif

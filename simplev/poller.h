//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_POLLER_H
#define TEST_EVENT_POLLER_H

#include "../common/noncopyable.h"
#include "simplev_imp.h"
#include "channel.h"
#include <poll.h>

//#ifdef OS_LINUX
#include <sys/epoll.h>
//#elif defined(OS_MACOSX)
//#include <sys/event.h>
//#else
//#error "platform unsupported"
//#endif

const int kMaxEvents = 2000;
const int kReadEvent = POLLIN;
const int kWriteEvent = POLLOUT;

using namespace BASE;

class CChannel;

class CPollerBase :BASE::noncopyable{
public:
    CPollerBase() : lastActive_(-1) {
        static std::atomic<int64_t> id(0);
        id_ = ++id;
    }
    virtual void addChannel(CChannel *ch)=0;
    virtual void removeChannel(CChannel *ch) = 0;
    virtual void updateChannel(CChannel *ch) = 0;
    virtual void loop_once(int waitMs) = 0;
    virtual ~CPollerBase(){};
public:
    __int64_t id_;
    int lastActive_;
};

//#ifdef OS_LINUX
struct CPollerEpoll : public CPollerBase{
public:
    CPollerEpoll();
    ~CPollerEpoll();
    void addChannel(CChannel *ch) override ;
    void removeChannel(CChannel *ch) override;
    void updateChannel(CChannel *ch) override;
    void loop_once(int waitMs) override;
public:
    int fd_;
    std::set<CChannel *> liveChannels_;
    struct epoll_event activeEvs_[kMaxEvents];

};
CPollerBase *createPoller();
//#elif defined(OS_MACOSX)
//class CPollerKqueue : public CPollerBase{
//public:
//    CPollerKqueue();
//    ~CPollerKqueue();
//    void addChannel(CChannel *ch) override;
//    void removeChannel(CChannel *ch) override;
//    void updateChannel(CChannel *ch) override;
//    void loop_once(int waitMs) override;
//public:
//    int fd_;
//    std::set<CChannel *> liveChannels_;
//    struct kevent activeEvs_[kMaxEvents];
//};

#endif //TEST_EVENT_POLLER_H

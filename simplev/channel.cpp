//
// Created by tian on 2020/11/23.
//

#include "channel.h"

CChannel::CChannel(CEventLoop *loop, int fd, int events) : loop_(loop), fd_(fd), events_(events) {
    if(net::setNonBlock(fd_) < 0)
        iFatal("channel set non block failed");
    static std::atomic<int64_t> id(0);
    id_ = ++id;
    poller_ = loop_->epoller_;
    poller_->addChannel(this);
}

CChannel::~CChannel() {
    close();
}

void CChannel::enableRead(bool enable) {
    if (enable) {
        events_ |= kReadEvent;
    } else {
        events_ &= ~kReadEvent;
    }
    poller_->updateChannel(this);
}

void CChannel::enableWrite(bool enable) {
    if (enable) {
        events_ |= kWriteEvent;
    } else {
        events_ &= ~kWriteEvent;
    }
    poller_->updateChannel(this);
}

void CChannel::enableReadWrite(bool readable, bool writable) {
    if (readable) {
        events_ |= kReadEvent;
    } else {
        events_ &= ~kReadEvent;
    }
    if (writable) {
        events_ |= kWriteEvent;
    } else {
        events_ &= ~kWriteEvent;
    }
    poller_->updateChannel(this);
}

void CChannel::close() {
    if (fd_ >= 0) {
        iTrace("close channel %ld fd %d", (long) id_, fd_);
        poller_->removeChannel(this);
        ::close(fd_);
        fd_ = -1;
        handleRead();
    }
}


bool CChannel::readEnabled() {
    return events_ & kReadEvent;
}
bool CChannel::writeEnabled() {
    return events_ & kWriteEvent;
}
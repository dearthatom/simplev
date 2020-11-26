//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_CHANNEL_H
#define TEST_EVENT_CHANNEL_H

#include "../common/noncopyable.h"
#include "simplev_imp.h"
//#include "poller.h"
#include "data/net.h"
#include "eventloop.h"
#include <atomic>

class CPollerEpoll;

class CChannel :BASE::noncopyable{
public:
    CChannel(CEventLoop *loop, int fd, int events);
    ~CChannel();
    CEventLoop *getLoop() { return loop_; }
    int fd() { return fd_; }
    //通道id
    int64_t id() { return id_; }
    short events() { return events_; }
    //关闭通道
    void close();

    //挂接事件处理器
    void onRead(const Task &readcb) { readcb_ = readcb; }
    void onWrite(const Task &writecb) { writecb_ = writecb; }
    void onRead(Task &&readcb) { readcb_ = std::move(readcb); }
    void onWrite(Task &&writecb) { writecb_ = std::move(writecb); }

    //启用读写监听
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writable);
    bool readEnabled();
    bool writeEnabled();

    //处理读写事件
    void handleRead() { readcb_(); }
    void handleWrite() { writecb_(); }

private:
    CEventLoop *loop_;
    CPollerEpoll *poller_;
    int fd_;
    short events_;
    int64_t id_;
    std::function<void()> readcb_, writecb_, errorcb_;
};


#endif //TEST_EVENT_CHANNEL_H

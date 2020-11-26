//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_EVENTLOOP_H
#define TEST_EVENT_EVENTLOOP_H

#include "../common/util.h"
#include "../common/safeguard_queue.h"
#include "simplev_imp.h"
#include "poller.h"
//#include "tcpconn.h"

struct TimerRepeatable {
    int64_t at;  // current timer timeout timestamp
    int64_t interval;
    TimerId timerid;
    Task cb;
};

struct IdleNode {
    CTcpConnPtr con_;
    int64_t updated_;
    TcpCallBack cb_;
};

struct IdleIdImp {
    IdleIdImp() {}
    typedef list<IdleNode>::iterator Iter;
    IdleIdImp(list<IdleNode> *lst, Iter iter) : lst_(lst), iter_(iter) {}
    list<IdleNode> *lst_;
    Iter iter_;
};

class CTcpConn;

class CEventLoop {
public:
    // taskCapacity指定任务队列的大小，0无限制
    CEventLoop(int taskCapacity = 0);
    ~CEventLoop();

    void init();
    void callIdles();

//处理已到期的事件,waitMs表示若无当前需要处理的任务，需要等待的时间
    void loop_once(int waitMs);
    //进入事件处理循环
    void loop();
    //取消定时任务，若timer已经过期，则忽略
    bool cancel(TimerId timerid);
    //添加定时任务，interval=0表示一次性任务，否则为重复任务，时间为毫秒
    TimerId runAt(int64_t milli, const Task &task, int64_t interval = 0) { return runAt(milli, Task(task), interval); }
    TimerId runAt(int64_t milli, Task &&task, int64_t interval = 0);
    TimerId runAfter(int64_t milli, const Task &task, int64_t interval = 0) { return runAt(TimeZoo::timeMilli() + milli, Task(task), interval); }
    TimerId runAfter(int64_t milli, Task &&task, int64_t interval = 0) { return runAt(TimeZoo::timeMilli() + milli, std::move(task), interval); }

    //下列函数为线程安全的

    //退出事件循环
    CEventLoop &exit();
    //是否已退出
    bool exited();
    //唤醒事件处理
    void wakeup();
    //添加任务
    void safeCall(Task &&task);
    void safeCall(const Task &task) { safeCall(Task(task)); }

    IdleId registerIdle(int idle, const CTcpConnPtr &con, const TcpCallBack &cb);
    void unregisterIdle(const IdleId &id);
    void updateIdle(const IdleId &id);
private:
    void refreshNearest(const TimerId *tid = NULL);
    void repeatableTimeout(TimerRepeatable *tr);
    void handleTimeouts();


public:
    CPollerEpoll *epoller_;
    std::set<CTcpConnPtr> reconnectConns_;
private:
    CPollerBase *poller_;

    std::atomic<bool> exit_;
    int wakeupFds_[2];
    int nextTimeout_;
    BASE::safeguard_queue<Task> tasks_;

    std::map<TimerId, TimerRepeatable> timerReps_;
    std::map<TimerId, Task> timers_;
    std::atomic<int64_t> timerSeq_;
    // 记录每个idle时间（单位秒）下所有的连接。链表中的所有连接，最新的插入到链表末尾。连接若有活动，会把连接从链表中移到链表尾部，做法参考memcache
    std::map<int, std::list<IdleNode>> idleConns_;

    bool idleEnabled;
};


class CMultiBase{
public:
    CMultiBase(int sz) : id_(0), loops_(sz) {}
    virtual CEventLoop *allocLoop() {
        int c = id_++;
        return &loops_[c % loops_.size()];
    }
    void loop();
    CMultiBase &exit() {
        for (auto &b : loops_) {
            b.exit();
        }
        return *this;
    }

private:
    std::atomic<int> id_;
    std::vector<CEventLoop> loops_;
};


#endif //TEST_EVENT_EVENTLOOP_H

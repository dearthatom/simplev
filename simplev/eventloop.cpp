//
// Created by tian on 2020/11/23.
//

#include "eventloop.h"

void handyUnregisterIdle(CEventLoop *loop, const IdleId &idle) {
    loop->unregisterIdle(idle);
}

void handyUpdateIdle(CEventLoop *loop, const IdleId &idle) {
    loop->updateIdle(idle);
}

CEventLoop::CEventLoop(int taskCapacity) {

}

CEventLoop::~CEventLoop() {

}
void CEventLoop::init() {
    int r = pipe(wakeupFds_);
    fatalif(r, "pipe failed %d %s", errno, strerror(errno));
    r = UtilZoo::addFdFlag(wakeupFds_[0], FD_CLOEXEC);
    fatalif(r, "addFdFlag failed %d %s", errno, strerror(errno));
    r = UtilZoo::addFdFlag(wakeupFds_[1], FD_CLOEXEC);
    fatalif(r, "addFdFlag failed %d %s", errno, strerror(errno));
    iTrace("wakeup pipe created %d %d", wakeupFds_[0], wakeupFds_[1]);
    CChannel *ch = new CChannel(this, wakeupFds_[0], kReadEvent);
    ch->onRead([=] {
        char buf[1024];
        int r = ch->fd() >= 0 ? ::read(ch->fd(), buf, sizeof buf) : 0;
        if (r > 0) {
            Task task;
            while (tasks_.pop(task)) {
                task();
            }
        } else if (r == 0) {
            delete ch;
        } else if (errno == EINTR) {
        } else {
            iFatal("wakeup channel read error %d %d %s", r, errno, strerror(errno));
        }
    });
}

void CEventLoop::callIdles() {
    int64_t now = TimeZoo::timeMilli() / 1000;
    for (auto &l : idleConns_) {
        int idle = l.first;
        auto lst = l.second;
        while (lst.size()) {
            IdleNode &node = lst.front();
            if (node.updated_ + idle > now) {
                break;
            }
            node.updated_ = now;
            lst.splice(lst.end(), lst, lst.begin());
            node.cb_(node.con_);
        }
    }
}

CEventLoop &CEventLoop::exit() {
    exit_ = true;
    wakeup();
    return *this;
}

bool CEventLoop::exited() {
    return exit_;
}

bool CEventLoop::cancel(TimerId timerid) {
    if (timerid.first < 0) {
        auto p = timerReps_.find(timerid);
        auto ptimer = timers_.find(p->second.timerid);
        if (ptimer != timers_.end()) {
            timers_.erase(ptimer);
        }
        timerReps_.erase(p);
        return true;
    } else {
        auto p = timers_.find(timerid);
        if (p != timers_.end()) {
            timers_.erase(p);
            return true;
        }
        return false;
    }
}

void CEventLoop::wakeup() {
    int r = write(wakeupFds_[1], "", 1);
    if(r<= 0)
        iFatal("write error wd %d %d %s", r, errno, strerror(errno));
}

void CEventLoop::safeCall(Task &&task) {
    tasks_.push(move(task));
    wakeup();
}

void CEventLoop::loop_once(int waitMs) {
    epoller_->loop_once(std::min(waitMs, nextTimeout_));
    handleTimeouts();
}

void CEventLoop::loop() {
    while (!exit_)
        loop_once(10000);
    timerReps_.clear();
    timers_.clear();
    idleConns_.clear();
//    for (auto recon : reconnectConns_) {  //重连的连接无法通过channel清理，因此单独清理
//        recon->cleanup(recon);
//    }
    loop_once(0);
}

TimerId CEventLoop::runAt(int64_t milli, Task &&task, int64_t interval) {
    if (exit_) {
        return TimerId();
    }
    if (interval) {
        TimerId tid{-milli, ++timerSeq_};
        TimerRepeatable &rtr = timerReps_[tid];
        rtr = {milli, interval, {milli, ++timerSeq_}, move(task)};
        TimerRepeatable *tr = &rtr;
        timers_[tr->timerid] = [this, tr] { repeatableTimeout(tr); };
        refreshNearest(&tr->timerid);
        return tid;
    } else {
        TimerId tid{milli, ++timerSeq_};
        timers_.insert({tid, move(task)});
        refreshNearest(&tid);
        return tid;
    }
}

IdleId CEventLoop::registerIdle(int idle, const CTcpConnPtr &con, const TcpCallBack &cb) {
    if (!idleEnabled) {
        runAfter(1000, [this] { callIdles(); }, 1000);
        idleEnabled = true;
    }
    auto &lst = idleConns_[idle];
    lst.push_back(IdleNode{con, TimeZoo::timeMilli() / 1000, move(cb)});
    iTrace("register idle");
    return IdleId(new IdleIdImp(&lst, --lst.end()));
}

void CEventLoop::unregisterIdle(const IdleId &id) {
    iTrace("unregister idle");
    id->lst_->erase(id->iter_);
}

void CEventLoop::updateIdle(const IdleId &id) {
    iTrace("update idle");
    id->iter_->updated_ = TimeZoo::timeMilli() / 1000;
    id->lst_->splice(id->lst_->end(), *id->lst_, id->iter_);
}

void CEventLoop::refreshNearest(const TimerId *tid) {
    if (timers_.empty()) {
        nextTimeout_ = 1 << 30;
    } else {
        const TimerId &t = timers_.begin()->first;
        nextTimeout_ = t.first - TimeZoo::timeMilli();
        nextTimeout_ = nextTimeout_ < 0 ? 0 : nextTimeout_;
    }
}
void CEventLoop::handleTimeouts() {
    int64_t now = TimeZoo::timeMilli();
    TimerId tid{now, 1L << 62};
    while (timers_.size() && timers_.begin()->first < tid) {
        Task task = move(timers_.begin()->second);
        timers_.erase(timers_.begin());
        task();
    }
    refreshNearest();
}



void CMultiBase::loop() {
    int sz = loops_.size();
    std::vector<std::thread> ths(sz - 1);
    for (int i = 0; i < sz - 1; i++) {
        std::thread t([this, i] { loops_[i].loop(); });
        ths[i].swap(t);
    }
    loops_.back().loop();
    for (int i = 0; i < sz - 1; i++) {
        ths[i].join();
    }
}
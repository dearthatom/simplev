//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_SIMPLEV_IMP_H
#define TEST_EVENT_SIMPLEV_IMP_H

#include "data/slice.h"
#include "tlog.hpp"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <atomic>
#include <memory>
#include <thread>

class CChannel;
class CTcpConn;
class CTcpServer;
class CEventLoop;
class CPollerBase;
class CPollerEpoll;
class IdleIdImp;

typedef std::shared_ptr<CTcpConn> CTcpConnPtr;
typedef std::shared_ptr<CTcpServer> CTcpServerPtr;
typedef std::pair<int64_t, int64_t> TimerId;
typedef std::function<void()> Task;
typedef std::function<void(const CTcpConnPtr &)> TcpCallBack;
typedef std::function<void(const CTcpConnPtr &, Slice msg)> MsgCallBack;
typedef std::unique_ptr<IdleIdImp> IdleId;
struct AutoContext : noncopyable {
    void *ctx;
    Task ctxDel;
    AutoContext() : ctx(0) {}
    template <class T>
    T &context() {
        if (ctx == NULL) {
            ctx = new T();
            ctxDel = [this] { delete (T *) ctx; };
        }
        return *(T *) ctx;
    }
    ~AutoContext() {
        if (ctx)
            ctxDel();
    }
};

#endif //TEST_EVENT_SIMPLEV_IMP_H

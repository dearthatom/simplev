//
// Created by tian on 2020/11/25.
//

#ifndef TEST_EVENT_CALLBACKS_H
#define TEST_EVENT_CALLBACKS_H

struct CTcpConn;
struct CTcpServer;
struct CEventLoop;
struct CPollerBase;

typedef std::shared_ptr<CTcpConn> CTcpConnPtr;
typedef std::pair<int64_t, int64_t> TimerId;
typedef std::function<void()> Task;
typedef std::function<void(const CTcpConnPtr &)> TcpCallBack;
typedef std::function<void(const CTcpConnPtr &, Slice msg)> MsgCallBack;


#endif //TEST_EVENT_CALLBACKS_H

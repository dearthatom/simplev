//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_TCPSERVER_H
#define TEST_EVENT_TCPSERVER_H

#include "../common/noncopyable.h"
#include "simplev_imp.h"
#include "data/net.h"
#include "eventloop.h"
#include "tcpconn.h"
#include "channel.h"
#include "data/codec.h"

class CEventLoop;

class CTcpServer : private BASE::noncopyable {

public:
    CTcpServer(CEventLoop *loop);
    // return 0 on sucess, errno on error
    int bind(const std::string &host, unsigned short port, bool reusePort = false);
    static CTcpServerPtr startServer(CEventLoop *loop, const std::string &host, unsigned short port, bool reusePort = false);
    ~CTcpServer() { delete listen_channel_; }
    Ipv4Addr getAddr() { return addr_; }
    CEventLoop *getLoop() { return loop_; }
    void onConnCreate(const std::function<CTcpConnPtr()> &cb) { createcb_ = cb; }
    void onConnState(const TcpCallBack &cb) { statecb_ = cb; }
    void onConnRead(const TcpCallBack &cb) {
        readcb_ = cb;
        assert(!msgcb_);
    }
    // 消息处理与Read回调冲突，只能调用一个
    void onConnMsg(CodecBase *codec, const MsgCallBack &cb) {
        codec_.reset(codec);
        msgcb_ = cb;
        assert(!readcb_);
    }

private:
    void handleAccept();
private:
    CEventLoop *loop_;
    //EventBases *bases_;
    Ipv4Addr addr_;
    CChannel *listen_channel_;
    TcpCallBack statecb_, readcb_;
    MsgCallBack msgcb_;
    std::function<CTcpConnPtr()> createcb_;
    std::unique_ptr<CodecBase> codec_;

};


#endif //TEST_EVENT_TCPSERVER_H

//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_TCPCONN_H
#define TEST_EVENT_TCPCONN_H

//#include "../common/noncopyable.h"
#include "simplev_imp.h"
#include "data/net.h"
#include "callbacks.h"
#include "channel.h"
#include "eventloop.h"
#include "data/codec.h"
#include <memory>

class CChannel;
class CEventLoop;

class CTcpConn : public std::enable_shared_from_this<CTcpConn>, BASE::noncopyable {

public:
    // Tcp连接的个状态
    enum State {
        Invalid = 1,
        Handshaking,
        Connected,
        Closed,
        Failed,
    };
    // Tcp构造函数，实际可用的连接应当通过createConnection创建
    CTcpConn();
    virtual ~CTcpConn();
    //可传入连接类型，返回智能指针
    template <class C = CTcpConn>
    static CTcpConnPtr createConnection(CEventLoop *loop, const std::string &host, unsigned short port, int timeout = 0, const std::string &localip = "") {
        CTcpConnPtr con(new C);
        con->connect(loop, host, port, timeout, localip);
        return con;
    }
    template <class C = CTcpConn>
    static CTcpConnPtr createConnection(CEventLoop *loop, int fd, Ipv4Addr local, Ipv4Addr peer) {
        CTcpConnPtr con(new C);
        con->attach(loop, fd, local, peer);
        return con;
    }

    bool isClient() { return destPort_ > 0; }
    // automatically managed context. allocated when first used, deleted when destruct
    template <class T>
    T &context() {
        return ctx_.context<T>();
    }

    CEventLoop *getLoop() { return loop_; }
    State getState() { return state_; }
    // TcpConn的输入输出缓冲区
    Buffer &getInput() { return input_; }
    Buffer &getOutput() { return output_; }

    CChannel *getChannel() { return channel_; }
    bool writable() { return channel_ ? channel_->writeEnabled() : false; }

    //发送数据
    void sendOutput() { send(output_); }
    void send(Buffer &msg);
    void send(const char *buf, size_t len);
    void send(const std::string &s) { send(s.data(), s.size()); }
    void send(const char *s) { send(s, strlen(s)); }

    //数据到达时回调
    void onRead(const TcpCallBack &cb) {
        assert(!readcb_);
        readcb_ = cb;
    };
    //当tcp缓冲区可写时回调
    void onWritable(const TcpCallBack &cb) { writablecb_ = cb; }
    // tcp状态改变时回调
    void onState(const TcpCallBack &cb) { statecb_ = cb; }
    // tcp空闲回调
    void addIdleCB(int idle, const TcpCallBack &cb);

    //消息回调，此回调与onRead回调冲突，只能够调用一个
    // codec所有权交给onMsg
    void onMsg(CodecBase *codec, const MsgCallBack &cb);
    //发送消息
    void sendMsg(Slice msg);

    // conn会在下个事件周期进行处理
    void close();
    //设置重连时间间隔，-1: 不重连，0:立即重连，其它：等待毫秒数，未设置不重连
    void setReconnectInterval(int milli) { reconnectInterval_ = milli; }

    //!慎用。立即关闭连接，清理相关资源，可能导致该连接的引用计数变为0，从而使当前调用者引用的连接被析构
    void closeNow() {
        if (channel_)
            channel_->close();
    }

    //远程地址的字符串
    std::string str() { return peer_.toString(); }

public:
    void handleRead(const CTcpConnPtr &con);
    void handleWrite(const CTcpConnPtr &con);
    ssize_t isend(const char *buf, size_t len);
    void cleanup(const CTcpConnPtr &con);
    void connect(CEventLoop *loop, const std::string &host, unsigned short port, int timeout, const std::string &localip);
    //void reconnect();
    void attach(CEventLoop *loop, int fd, Ipv4Addr local, Ipv4Addr peer);
    virtual int readImp(int fd, void *buf, size_t bytes) { return ::read(fd, buf, bytes); }
    virtual int writeImp(int fd, const void *buf, size_t bytes) { return ::write(fd, buf, bytes); }
    virtual int handleHandshake(const CTcpConnPtr &con);
public:
    CEventLoop *loop_;
    CChannel *channel_;
    Buffer input_, output_;
    Ipv4Addr local_, peer_;
    State state_;
    TcpCallBack readcb_, writablecb_, statecb_;
    std::list<IdleId> idleIds_;
    TimerId timeoutId_;
    AutoContext ctx_, internalCtx_;
    std::string destHost_, localIp_;
    int destPort_, connectTimeout_, reconnectInterval_;
    int64_t connectedTime_;
    std::unique_ptr<CodecBase> codec_;

};


#endif //TEST_EVENT_TCPCONN_H

//
// Created by tian on 2020/11/23.
//

#include "tcpconn.h"

void handyUnregisterIdle(CEventLoop *loop, const IdleId &idle);
void handyUpdateIdle(CEventLoop *loop, const IdleId &idle);

CTcpConn::CTcpConn():
        loop_(NULL),
        channel_(NULL),
        state_(State::Invalid),
        destPort_(-1),
        connectTimeout_(0),
        reconnectInterval_(-1),
        connectedTime_(TimeZoo::timeMilli())
{

}

CTcpConn::~CTcpConn() {
    iTrace("tcp destroyed %s - %s", local_.toString().c_str(), peer_.toString().c_str());
    delete channel_;
}

void CTcpConn::attach(CEventLoop *loop, int fd, Ipv4Addr local, Ipv4Addr peer) {
    fatalif((destPort_ <= 0 && state_ != State::Invalid) || (destPort_ >= 0 && state_ != State::Handshaking),
            "you should use a new TcpConn to attach. state: %d", state_);
    loop_ = loop;
    state_ = State::Handshaking;
    local_ = local;
    peer_ = peer;
    delete channel_;
    channel_ = new CChannel(loop, fd, kWriteEvent | kReadEvent);
    iTrace("tcp constructed %s - %s fd: %d", local_.toString().c_str(), peer_.toString().c_str(), fd);
    CTcpConnPtr con = shared_from_this();
    con->channel_->onRead([=] { con->handleRead(con); });
    con->channel_->onWrite([=] { con->handleWrite(con); });
}

void CTcpConn::connect(CEventLoop *loop, const std::string &host, unsigned short port, int timeout, const std::string &localip) {
    fatalif(state_ != State::Invalid && state_ != State::Closed && state_ != State::Failed, "current state is bad state to connect. state: %d", state_);
    destHost_ = host;
    destPort_ = port;
    connectTimeout_ = timeout;
    connectedTime_ = TimeZoo::timeMilli();
    localIp_ = localip;
    Ipv4Addr addr(host, port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fatalif(fd < 0, "socket failed %d %s", errno, strerror(errno));
    net::setNonBlock(fd);
    int t = UtilZoo::addFdFlag(fd, FD_CLOEXEC);
    fatalif(t, "addFdFlag FD_CLOEXEC failed %d %s", t, strerror(t));
    int r = 0;
    if (localip.size()) {
        Ipv4Addr addr(localip, 0);
        r = ::bind(fd, (struct sockaddr *) &addr.getAddr(), sizeof(struct sockaddr));
        iError("bind to %s failed error %d %s", addr.toString().c_str(), errno, strerror(errno));
    }
    if (r == 0) {
        r = ::connect(fd, (sockaddr *) &addr.getAddr(), sizeof(sockaddr_in));
        if (r != 0 && errno != EINPROGRESS) {
            iError("connect to %s error %d %s", addr.toString().c_str(), errno, strerror(errno));
        }
    }

    sockaddr_in local;
    socklen_t alen = sizeof(local);
    if (r == 0) {
        r = getsockname(fd, (sockaddr *) &local, &alen);
        if (r < 0) {
            iError("getsockname failed %d %s", errno, strerror(errno));
        }
    }
    state_ = State::Handshaking;
    attach(loop, fd, Ipv4Addr(local), addr);
    if (timeout) {
        CTcpConnPtr con = shared_from_this();
        timeoutId_ = loop->runAfter(timeout, [con] {
            if (con->getState() == Handshaking) {
                con->channel_->close();
            }
        });
    }
}

void CTcpConn::close() {
    if (channel_) {
        CTcpConnPtr con = shared_from_this();
        getLoop()->safeCall([con] {
            if (con->channel_)
                con->channel_->close();
        });
    }
}

void CTcpConn::cleanup(const CTcpConnPtr &con) {
    if (readcb_ && input_.size()) {
        readcb_(con);
    }
    if (state_ == State::Handshaking) {
        state_ = State::Failed;
    } else {
        state_ = State::Closed;
    }
    iTrace("tcp closing %s - %s fd %d %d", local_.toString().c_str(), peer_.toString().c_str(), channel_ ? channel_->fd() : -1, errno);
    getLoop()->cancel(timeoutId_);
    if (statecb_) {
        statecb_(con);
    }
//    if (reconnectInterval_ >= 0 && !getLoop()->exited()) {  // reconnect
//        reconnect();
//        return;
//    }
    for (auto &idle : idleIds_) {
        handyUnregisterIdle(getLoop(), idle);
    }
    // channel may have hold TcpConnPtr, set channel_ to NULL before delete
    readcb_ = writablecb_ = statecb_ = nullptr;
    CChannel *ch = channel_;
    channel_ = NULL;
    delete ch;
}

void CTcpConn::handleRead(const CTcpConnPtr &con) {
    if (state_ == State::Handshaking && handleHandshake(con)) {
        return;
    }
    while (state_ == State::Connected) {
        input_.makeRoom();
        int rd = 0;
        if (channel_->fd() >= 0) {
            rd = readImp(channel_->fd(), input_.end(), input_.space());
            iTrace("channel %lld fd %d readed %d bytes", (long long) channel_->id(), channel_->fd(), rd);
        }
        if (rd == -1 && errno == EINTR) {
            continue;
        } else if (rd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            for (auto &idle : idleIds_) {
                handyUpdateIdle(getLoop(), idle);
            }
            if (readcb_ && input_.size()) {
                readcb_(con);
            }
            break;
        } else if (channel_->fd() == -1 || rd == 0 || rd == -1) {
            cleanup(con);
            break;
        } else {  // rd > 0
            input_.addSize(rd);
        }
    }
}

int CTcpConn::handleHandshake(const CTcpConnPtr &con) {
    fatalif(state_ != Handshaking, "handleHandshaking called when state_=%d", state_);
    struct pollfd pfd;
    pfd.fd = channel_->fd();
    pfd.events = POLLOUT | POLLERR;
    int r = poll(&pfd, 1, 0);
    if (r == 1 && pfd.revents == POLLOUT) {
        channel_->enableReadWrite(true, false);
        state_ = State::Connected;
        if (state_ == State::Connected) {
            connectedTime_ = TimeZoo::timeMilli();
            iTrace("tcp connected %s - %s fd %d", local_.toString().c_str(), peer_.toString().c_str(), channel_->fd());
            if (statecb_) {
                statecb_(con);
            }
        }
    } else {
        iTrace("poll fd %d return %d revents %d", channel_->fd(), r, pfd.revents);
        cleanup(con);
        return -1;
    }
    return 0;
}

void CTcpConn::handleWrite(const CTcpConnPtr &con) {
    if (state_ == State::Handshaking) {
        handleHandshake(con);
    } else if (state_ == State::Connected) {
        ssize_t sended = isend(output_.begin(), output_.size());
        output_.consume(sended);
        if (output_.empty() && writablecb_) {
            writablecb_(con);
        }
        if (output_.empty() && channel_->writeEnabled()) {  // writablecb_ may write something
            channel_->enableWrite(false);
        }
    } else {
        iError("handle write unexpected");
    }
}

ssize_t CTcpConn::isend(const char *buf, size_t len) {
    size_t sended = 0;
    while (len > sended) {
        ssize_t wd = writeImp(channel_->fd(), buf + sended, len - sended);
        iTrace("channel %lld fd %d write %ld bytes", (long long) channel_->id(), channel_->fd(), wd);
        if (wd > 0) {
            sended += wd;
            continue;
        } else if (wd == -1 && errno == EINTR) {
            continue;
        } else if (wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!channel_->writeEnabled()) {
                channel_->enableWrite(true);
            }
            break;
        } else {
            iError("write error: channel %lld fd %d wd %ld %d %s", (long long) channel_->id(), channel_->fd(), wd, errno, strerror(errno));
            break;
        }
    }
    return sended;
}

//void CTcpConn::reconnect() {
//    auto con = shared_from_this();
//    getLoop()->reconnectConns_.insert(con);
//    long long interval = reconnectInterval_ - (TimeZoo::timeMilli() - connectedTime_);
//    interval = interval > 0 ? interval : 0;
//    iInfo("reconnect interval: %d will reconnect after %lld ms", reconnectInterval_, interval);
//    getLoop()->runAfter(interval, [this, con]() {
//        getLoop()->reconnectConns_.erase(con);
//        connect(getLoop(), destHost_, (unsigned short) destPort_, connectTimeout_, localIp_);
//    });
//    delete channel_;
//    channel_ = NULL;
//}

void CTcpConn::send(Buffer &buf) {
    if (channel_) {
        if (channel_->writeEnabled()) {  // just full
            output_.absorb(buf);
        }
        if (buf.size()) {
            ssize_t sended = isend(buf.begin(), buf.size());
            buf.consume(sended);
        }
        if (buf.size()) {
            output_.absorb(buf);
            if (!channel_->writeEnabled()) {
                channel_->enableWrite(true);
            }
        }
    } else {
        iWarn("connection %s - %s closed, but still writing %lu bytes", local_.toString().c_str(), peer_.toString().c_str(), buf.size());
    }
}

void CTcpConn::send(const char *buf, size_t len) {
    if (channel_) {
        if (output_.empty()) {
            ssize_t sended = isend(buf, len);
            buf += sended;
            len -= sended;
        }
        if (len) {
            output_.append(buf, len);
        }
    } else {
        iWarn("connection %s - %s closed, but still writing %lu bytes", local_.toString().c_str(), peer_.toString().c_str(), len);
    }
}

void CTcpConn::onMsg(CodecBase *codec, const MsgCallBack &cb) {
    assert(!readcb_);
    codec_.reset(codec);
    onRead([cb](const CTcpConnPtr &con) {
        int r = 1;
        while (r) {
            Slice msg;
            r = con->codec_->tryDecode(con->getInput(), msg);
            if (r < 0) {
                con->channel_->close();
                break;
            } else if (r > 0) {
                iTrace("a msg decoded. origin len %d msg len %ld", r, msg.size());
                cb(con, msg);
                con->getInput().consume(r);
            }
        }
    });
}

void CTcpConn::sendMsg(Slice msg) {
    codec_->encode(msg, getOutput());
    sendOutput();
}
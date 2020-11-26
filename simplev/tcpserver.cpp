//
// Created by tian on 2020/11/23.
//

#include "tcpserver.h"

CTcpServer::CTcpServer(CEventLoop *loop) :
                loop_(loop),
                //base_(bases->allocBase()),
                //bases_(bases),
                listen_channel_(NULL),
                createcb_([] { return CTcpConnPtr(new CTcpConn); }) {

}


CTcpServerPtr CTcpServer::startServer(CEventLoop *loop, const std::string &host, unsigned short port, bool reusePort) {
    CTcpServerPtr p(new CTcpServer(loop));
    int r = p->bind(host, port, reusePort);
    if (r) {
        iError("bind to %s:%d failed %d %s", host.c_str(), port, errno, strerror(errno));
    }
    return r == 0 ? p : NULL;
}

int CTcpServer::bind(const std::string &host, unsigned short port, bool reusePort) {
    addr_ = Ipv4Addr(host, port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int r = net::setReuseAddr(fd);
    fatalif(r, "set socket reuse option failed");
    r = net::setReusePort(fd, reusePort);
    fatalif(r, "set socket reuse port option failed");
    r = UtilZoo::addFdFlag(fd, FD_CLOEXEC);
    fatalif(r, "addFdFlag FD_CLOEXEC failed");
    r = ::bind(fd, (struct sockaddr *) &addr_.getAddr(), sizeof(struct sockaddr));
    if (r) {
        close(fd);
        iError("bind to %s failed %d %s", addr_.toString().c_str(), errno, strerror(errno));
        return errno;
    }
    r = listen(fd, 20);
    fatalif(r, "listen failed %d %s", errno, strerror(errno));
    iInfo("fd %d listening at %s", fd, addr_.toString().c_str());
    listen_channel_ = new CChannel(loop_, fd, kReadEvent);
    listen_channel_->onRead([this] { handleAccept(); });
    return 0;
}

void CTcpServer::handleAccept() {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int lfd = listen_channel_->fd();
    int cfd;
    while (lfd >= 0 && (cfd = accept(lfd, (struct sockaddr *) &raddr, &rsz)) >= 0) {
        sockaddr_in peer, local;
        socklen_t alen = sizeof(peer);
        int r = getpeername(cfd, (sockaddr *) &peer, &alen);
        if (r < 0) {
            iError("get peer name failed %d %s", errno, strerror(errno));
            continue;
        }
        r = getsockname(cfd, (sockaddr *) &local, &alen);
        if (r < 0) {
            iError("getsockname failed %d %s", errno, strerror(errno));
            continue;
        }
        r = UtilZoo::addFdFlag(cfd, FD_CLOEXEC);
        fatalif(r, "addFdFlag FD_CLOEXEC failed");
        //EventBase *b = bases_->allocBase();
        //todo
        CEventLoop *b = new CEventLoop;
        auto addcon = [=] {
            CTcpConnPtr con = createcb_();
            con->attach(b, cfd, local, peer);

            if (statecb_) {
                con->onState(statecb_);
            }
            if (readcb_) {
                con->onRead(readcb_);
            }
            if (msgcb_) {
                con->onMsg(codec_->clone(), msgcb_);
            }
        };
        if (b == loop_) {
            addcon();
        } else {
            b->safeCall(std::move(addcon));

        }
    }
    if (lfd >= 0 && errno != EAGAIN && errno != EINTR) {
        iWarn("accept return %d  %d %s", cfd, errno, strerror(errno));
    }
}

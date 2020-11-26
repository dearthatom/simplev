//
// Created by tian on 2020/11/24.
//

#include "net.h"

using namespace BASE;

struct in_addr getHostByName(const std::string &host){
    struct in_addr addr;
    char buf[1024];
    struct hostent hent;
    struct hostent *he = NULL;
    int herrno = 0;
    memset(&hent, 0, sizeof hent);
    int r = gethostbyname_r(host.c_str(), &hent, buf, sizeof buf, &he, &herrno);
    if (r == 0 && he && he->h_addrtype == AF_INET) {
        addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
    } else {
        addr.s_addr = INADDR_NONE;
    }
    return addr;
}


Ipv4Addr::Ipv4Addr(const std::string &host, unsigned short port){
    memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (host.size()) {
        addr_.sin_addr = port::getHostByName(host);
    } else {
        addr_.sin_addr.s_addr = INADDR_ANY;
    }
    if (addr_.sin_addr.s_addr == INADDR_NONE) {
        iError("cannot resove %s to ip", host.c_str());
    }
}

std::string Ipv4Addr::toString() const {
    uint32_t uip = addr_.sin_addr.s_addr;
    return UtilZoo::format("%d.%d.%d.%d:%d", (uip >> 0) & 0xff, (uip >> 8) & 0xff, (uip >> 16) & 0xff, (uip >> 24) & 0xff, ntohs(addr_.sin_port));
}

char *Buffer::makeRoom(size_t len) {
    if (e_ + len <= cap_) {
    } else if (size() + len < cap_ / 2) {
        moveHead();
    } else {
        expand(len);
    }
    return end();
}

void Buffer::expand(size_t len) {
    size_t ncap = std::max(exp_, std::max(2 * cap_, size() + len));
    char *p = new char[ncap];
    std::copy(begin(), end(), p);
    e_ -= b_;
    b_ = 0;
    delete[] buf_;
    buf_ = p;
    cap_ = ncap;
}

void Buffer::copyFrom(const Buffer &b) {
    memcpy(this, &b, sizeof b);
    if (b.buf_) {
        buf_ = new char[cap_];
        memcpy(data(), b.begin(), b.size());
    }
}

Buffer &Buffer::absorb(Buffer &buf) {
    if (&buf != this) {
        if (size() == 0) {
            char b[sizeof buf];
            memcpy(b, this, sizeof b);
            memcpy(this, &buf, sizeof b);
            memcpy(&buf, b, sizeof b);
            std::swap(exp_, buf.exp_);  // keep the origin exp_
        } else {
            append(buf.begin(), buf.size());
            buf.clear();
        }
    }
    return *this;
}
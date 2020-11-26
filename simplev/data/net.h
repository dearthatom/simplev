//
// Created by tian on 2020/11/24.
//

#ifndef TEST_EVENT_NET_H
#define TEST_EVENT_NET_H


#include "util.h"
#include "slice.h"
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <string>
#include <syscall.h>


namespace port {
    static const int kLittleEndian = LITTLE_ENDIAN;
    inline uint16_t htobe(uint16_t v) {
        if (!kLittleEndian) {
            return v;
        }
        unsigned char *pv = (unsigned char *) &v;
        return uint16_t(pv[0]) << 8 | uint16_t(pv[1]);
    }
    inline uint32_t htobe(uint32_t v) {
        if (!kLittleEndian) {
            return v;
        }
        unsigned char *pv = (unsigned char *) &v;
        return uint32_t(pv[0]) << 24 | uint32_t(pv[1]) << 16 | uint32_t(pv[2]) << 8 | uint32_t(pv[3]);
    }
    inline uint64_t htobe(uint64_t v) {
        if (!kLittleEndian) {
            return v;
        }
        unsigned char *pv = (unsigned char *) &v;
        return uint64_t(pv[0]) << 56 | uint64_t(pv[1]) << 48 | uint64_t(pv[2]) << 40 | uint64_t(pv[3]) << 32 | uint64_t(pv[4]) << 24 | uint64_t(pv[5]) << 16 |
               uint64_t(pv[6]) << 8 | uint64_t(pv[7]);
    }
    inline int16_t htobe(int16_t v) {
        return (int16_t) htobe((uint16_t) v);
    }
    inline int32_t htobe(int32_t v) {
        return (int32_t) htobe((uint32_t) v);
    }
    inline int64_t htobe(int64_t v) {
        return (int64_t) htobe((uint64_t) v);
    }
    struct in_addr getHostByName(const std::string &host);
//{
//        struct in_addr addr;
//        char buf[1024];
//        struct hostent hent;
//        struct hostent *he = NULL;
//        int herrno = 0;
//        memset(&hent, 0, sizeof hent);
//        int r = gethostbyname_r(host.c_str(), &hent, buf, sizeof buf, &he, &herrno);
//        if (r == 0 && he && he->h_addrtype == AF_INET) {
//            addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
//        } else {
//            addr.s_addr = INADDR_NONE;
//        }
//        return addr;
//    }
    inline uint64_t gettid(){return syscall(SYS_gettid);}
}  // namespace port

class net {
public:
    template <class T>
    static T hton(T v) {
        return port::htobe(v);
    }
    template <class T>
    static T ntoh(T v) {
        return port::htobe(v);
    }
    static int setNonBlock(int fd, bool value = true);
    static int setReuseAddr(int fd, bool value = true);
    static int setReusePort(int fd, bool value = true);
    static int setNoDelay(int fd, bool value = true);
};

struct Ipv4Addr {
    Ipv4Addr(const std::string &host, unsigned short port);
    Ipv4Addr(unsigned short port = 0) : Ipv4Addr("", port) {}
    Ipv4Addr(const struct sockaddr_in &addr) : addr_(addr){};
    std::string toString() const;
    std::string ip() const;
    unsigned short port() const;
    unsigned int ipInt() const;
    // if you pass a hostname to constructor, then use this to check error
    bool isIpValid() const;
    struct sockaddr_in &getAddr() {
        return addr_;
    }
    static std::string hostToIp(const std::string &host) {
        Ipv4Addr addr(host, 0);
        return addr.ip();
    }

private:
    struct sockaddr_in addr_;
};

struct Buffer {
    Buffer() : buf_(NULL), b_(0), e_(0), cap_(0), exp_(512) {}
    ~Buffer() { delete[] buf_; }
    void clear() {
        delete[] buf_;
        buf_ = NULL;
        cap_ = 0;
        b_ = e_ = 0;
    }
    size_t size() const { return e_ - b_; }
    bool empty() const { return e_ == b_; }
    char *data() const { return buf_ + b_; }
    char *begin() const { return buf_ + b_; }
    char *end() const { return buf_ + e_; }
    char *makeRoom(size_t len);
    void makeRoom() {
        if (space() < exp_)
            expand(0);
    }
    size_t space() const { return cap_ - e_; }
    void addSize(size_t len) { e_ += len; }
    char *allocRoom(size_t len) {
        char *p = makeRoom(len);
        addSize(len);
        return p;
    }
    Buffer &append(const char *p, size_t len) {
        memcpy(allocRoom(len), p, len);
        return *this;
    }
    Buffer &append(Slice slice) { return append(slice.data(), slice.size()); }
    Buffer &append(const char *p) { return append(p, strlen(p)); }
    template <class T>
    Buffer &appendValue(const T &v) {
        append((const char *) &v, sizeof v);
        return *this;
    }
    Buffer &consume(size_t len) {
        b_ += len;
        if (size() == 0)
            clear();
        return *this;
    }
    Buffer &absorb(Buffer &buf);
    void setSuggestSize(size_t sz) { exp_ = sz; }
    Buffer(const Buffer &b) { copyFrom(b); }
    Buffer &operator=(const Buffer &b) {
        if (this == &b)
            return *this;
        delete[] buf_;
        buf_ = NULL;
        copyFrom(b);
        return *this;
    }
    operator Slice() { return Slice(data(), size()); }

private:
    char *buf_;
    size_t b_, e_, cap_, exp_;
    void moveHead() {
        std::copy(begin(), end(), buf_);
        e_ -= b_;
        b_ = 0;
    }
    void expand(size_t len);
    void copyFrom(const Buffer &b);
};

#endif //TEST_EVENT_NET_H

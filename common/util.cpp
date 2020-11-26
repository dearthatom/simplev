//
// Created by tian on 20-11-26.
//

#include "util.h"

int64_t TimeZoo::timeMicro() {
    std::chrono::time_point<std::chrono::system_clock> p = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count();
}

std::string UtilZoo::format(const char *fmt, ...) {
    char buffer[500];
    std::unique_ptr<char[]> release1;
    char *base;
    for (int iter = 0; iter < 2; iter++) {
        int bufsize;
        if (iter == 0) {
            bufsize = sizeof(buffer);
            base = buffer;
        } else {
            bufsize = 30000;
            base = new char[bufsize];
            release1.reset(base);
        }
        char *p = base;
        char *limit = base + bufsize;
        if (p < limit) {
            va_list ap;
            va_start(ap, fmt);
            p += vsnprintf(p, limit - p, fmt, ap);
            va_end(ap);
        }
        // Truncate to available space if necessary
        if (p >= limit) {
            if (iter == 0) {
                continue;  // Try again with larger buffer
            } else {
                p = limit - 1;
                *p = '\0';
            }
        }
        break;
    }
    return base;
}
int UtilZoo::addFdFlag(int fd, int flag) {
    int ret = fcntl(fd, F_GETFD);
    return fcntl(fd, F_SETFD, ret | flag);
}

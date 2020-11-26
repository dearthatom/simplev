//
// Created by tian on 2020/11/24.
//

#ifndef TEST_EVENT_UTIL_H
#define TEST_EVENT_UTIL_H

//#include "noncopyable.h"
#include "tlog.hpp"
#include <stdarg.h>
#include <algorithm>
#include <chrono>
#include <memory>


class UtilZoo{
public:
    static std::string format(const char *fmt, ...);
    static int addFdFlag(int fd, int flag);
};

class TimeZoo{
public:
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro() / 1000; }

};

#endif //TEST_EVENT_UTIL_H

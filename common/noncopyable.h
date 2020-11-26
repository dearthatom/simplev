//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_NONCOPYABLE_H
#define TEST_EVENT_NONCOPYABLE_H
#pragma once
//#include "tlog.hpp"


//using namespace BASE;

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif //TEST_EVENT_NONCOPYABLE_H

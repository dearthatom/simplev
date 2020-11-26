//
// Created by tian on 2020/11/23.
//

#ifndef TEST_EVENT_COPYABLE_H
#define TEST_EVENT_COPYABLE_H

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable
{
protected:
    copyable() = default;
    ~copyable() = default;
};

#endif //TEST_EVENT_COPYABLE_H

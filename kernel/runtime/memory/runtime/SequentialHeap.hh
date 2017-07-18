/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MIT License -- MyThOS: The Many-Threads Operating System
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/optional.hh"
#include "util/alignments.hh"
#include "util/FirstFitHeap.hh"
#include "cpu/hwthread_pause.hh"
#include "app/mlog.hh"
#include <atomic>


namespace mythos {

class SpinMutex {
public:
    void lock() {
        while (flag.test_and_set()) { mythos::hwthread_pause(50); }
    }

    void unlock() {
        flag.clear();
    }

    template<typename FUNCTOR>
    void operator<<(FUNCTOR fun) {
        lock();
        fun();
        unlock();
    }

private:
    std::atomic_flag flag;
};

/**
 * Used to store the size of an allocated chunk.
 * Maybe additional meta data can be placed here.
 */
struct ObjData {
    size_t size;
};

/**
 * Wrapper for FirstFitHeap intrusive. Allocates additional meta data.
 */
template<typename T, class A = AlignLine>
class SequentialHeap
{
public:
    typedef T addr_t;
    typedef A Alignment;

    SequentialHeap() {}
    virtual ~SequentialHeap() {}

    size_t getAlignment() const { return heap.getAlignment(); }

    optional<addr_t> alloc(size_t length, size_t alignment) {
        optional<addr_t> res;
        mutex << [&]() {
            res = heap.alloc(length + sizeof(ObjData), alignment);
        };
        if (res) {
            ObjData *data = reinterpret_cast<ObjData*>(*res);
            data->size = length;
            MLOG_DETAIL(mlog::app, "Alloc size:", length + sizeof(ObjData), "alignment:", alignment);
            return {*res + sizeof(ObjData)};
        }
        return res;
    }



    void free(addr_t start) {
        ObjData *data = reinterpret_cast<ObjData*>(start - sizeof(ObjData));
        mutex << [&]() {
            MLOG_DETAIL(mlog::app, "Free ", data, "size:", data->size + sizeof(ObjData));
            heap.free(start - sizeof(ObjData), data->size + sizeof(ObjData));
        };
    }

    void addRange(addr_t start, size_t length) {
        mutex << [&]() {
            MLOG_DETAIL(mlog::app, "Add range", DVARhex(start), DVARhex(length));
            heap.addRange(start, length);
        };
    }

private:
    FirstFitHeap<T, A> heap;
    SpinMutex mutex;

};

} // namespace mythos
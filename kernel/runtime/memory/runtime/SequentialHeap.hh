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
        while (flag.test_and_set()) { mythos::hwthread_pause(); }
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
 * Wrapper for FirstFitHeap intrusive. Allocates additional meta data.
 */
template<typename T, class A = AlignLine>
class SequentialHeap
{
public:
    /**
    * Used to store the size of an allocated chunk.
    * Maybe additional meta data can be placed here.
    */
    struct ObjData {
        size_t size;
    };
    
    typedef T addr_t;
    typedef A Alignment;

    static_assert(Alignment::alignment() >= sizeof(ObjData));

    SequentialHeap() {}
    virtual ~SequentialHeap() {}

    size_t getAlignment() const { return heap.getAlignment(); }

    optional<addr_t> alloc(size_t length) {
        optional<addr_t> res;
        auto allocSize = Alignment::round_up(sizeof(ObjData)) + length;
        mutex << [&]() {
            res = heap.alloc(allocSize, heap.getAlignment());
        };
        if (res) {
            auto addr = *res;
            ObjData *data = reinterpret_cast<ObjData*>(addr);
            data->size = length;
            auto retAddr = addr + Alignment::round_up(sizeof(ObjData));
            MLOG_DETAIL(mlog::app, "Alloc size:", allocSize, "alignment:", heap.getAlignment(), DVARhex(retAddr));
            ASSERT(Alignment::is_aligned(addr + Alignment::round_up(sizeof(ObjData))));
            ASSERT(Alignment::is_aligned(data));
            return {addr + Alignment::round_up(sizeof(ObjData))};
        }
        return res;
    }

    void free(addr_t start) {
        ObjData *data = reinterpret_cast<ObjData*>(start - Alignment::round_up(sizeof(ObjData)));
        addr_t to_free = start - Alignment::round_up(sizeof(ObjData));
        size_t size = Alignment::round_up(sizeof(ObjData)) + data->size;
        ASSERT(Alignment::is_aligned(data));
        ASSERT(Alignment::is_aligned(to_free));
        MLOG_DETAIL(mlog::app, "Free ", DVARhex(start), "size:", size);
        mutex << [&]() {
            heap.free(to_free, size);
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
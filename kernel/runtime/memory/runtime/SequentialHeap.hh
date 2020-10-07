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
#include "util/align.hh"
#include "util/FirstFitHeap.hh"
#include "runtime/mlog.hh"
#include "runtime/Mutex.hh"
#include <sys/time.h>


namespace mythos {

/**
 * Wrapper for FirstFitHeap intrusive. Allocates additional meta data.
 */
template<typename T, size_t HA = alignLine>
//template<typename T, size_t HA = align4K>
class SequentialHeap
{
public:
    typedef T addr_t;
    constexpr static size_t heapAlign = HA;
	unsigned long time;

    /**
    * Used to store the size of an allocated chunk.
    * Maybe additional meta data can be placed here.
    */
    struct Head {
        Head() {}
        bool isGood() const { return cannary == 0x08154711; }
        size_t cannary = 0x08154711;
        char* begin; //< the start of the allocated area, not same as "this" because of alignment 
        size_t size; //< the size that was actually allocated starting at begin
        size_t reqSize; //< the size that was requested in alloc()
    };
    

    SequentialHeap() {}
    virtual ~SequentialHeap() {
	    //printTime();
    }

    void init(){
    	static bool isInitialized = false;
	if(!isInitialized){
		new(heap) (FirstFitHeap<T, HA>);
		allocated = 0;	
		time = 0;
		isInitialized = true;
	}
    }

    void printTime(){
	MLOG_ERROR(mlog::app, "Heap time: ", DVAR(time)); 
	}

    size_t getAlignment() const { return getHeap().getAlignment(); }

    optional<addr_t> alloc(size_t length) { return this->alloc(length, heapAlign); }

    optional<addr_t> alloc(size_t length, size_t align) {
        //timeval tstart, tend;
	//asm volatile ("":::"memory");
	//gettimeofday(&tstart, 0);
	//asm volatile ("":::"memory");

        optional<addr_t> res;
        align = round_up(align, heapAlign); // enforce own minimum alignment
        auto headsize = round_up(sizeof(Head), align);
        auto allocSize = round_up(headsize + length, heapAlign);
        //MLOG_DETAIL(mlog::app, "heap: try to allocate", DVAR(length), DVAR(align), 
            //DVAR(allocSize));
        mutex << [&]() {
            res = getHeap().alloc(allocSize, align);
        };
        if (!res){ 
	MLOG_ERROR(mlog::app, "heap: out of memory! allocation request:", DVAR(length), DVAR(align), 
	    DVAR(allocSize));
	MLOG_ERROR(mlog::app, "heap: already allocted: ", DVAR(allocated));
		return res;
	}
	allocated += allocSize;
        auto begin = reinterpret_cast<char*>(*res);
        auto addr = begin + headsize;
        auto head = new(addr - sizeof(Head)) Head();
        head->begin = begin;
        head->size = allocSize;
        head->reqSize = length;
        //MLOG_DETAIL(mlog::app, "allocated", DVARhex(begin), DVARhex(addr),
            //DVAR(allocSize), DVAR(align));
        ASSERT(is_aligned(addr, align));
        ASSERT(is_aligned(addr, heapAlign));
	//asm volatile ("":::"memory");
	//gettimeofday(&tend, 0);
	//asm volatile ("":::"memory");
	//time += (tend.tv_sec - tstart.tv_sec)*1000000;
	//time += tend.tv_usec - tstart.tv_usec;
	return {reinterpret_cast<addr_t>(addr)};
    }

    void free(addr_t start) {
        //timeval tstart, tend;
	//asm volatile ("":::"memory");
	//gettimeofday(&tstart, 0);
	//asm volatile ("":::"memory");

        auto addr = reinterpret_cast<char*>(start);
        auto head = reinterpret_cast<Head*>(addr - sizeof(Head));
        ASSERT(head->isGood());
        auto begin = head->begin;
        auto allocSize = head->size;
        ASSERT(is_aligned(begin, heapAlign));
        ASSERT(is_aligned(allocSize, heapAlign));
        //MLOG_DETAIL(mlog::app, "freeing ", DVARhex(begin), DVARhex(addr), DVARhex(allocSize));
        mutex << [&]() {
            getHeap().free(reinterpret_cast<addr_t>(begin), allocSize);
	    allocated -= allocSize;
        };
	//asm volatile ("":::"memory");
	//gettimeofday(&tend, 0);
	//asm volatile ("":::"memory");
	//time += (tend.tv_sec - tstart.tv_sec)*1000000;
	//time += tend.tv_usec - tstart.tv_usec;
    }

    void addRange(addr_t start, size_t length) {
        mutex << [&]() {
        MLOG_DETAIL(mlog::app, "Add range", DVARhex(start), DVARhex(length));
            getHeap().addRange(start, length);
        };
    }
    
    size_t getSize(void* ptr) {
        auto addr = reinterpret_cast<char*>(ptr);
        auto head = reinterpret_cast<Head*>(addr - sizeof(Head));
        ASSERT(head->isGood());
        return head->reqSize;
    }

private:
    FirstFitHeap<T, HA>& getHeap(){ return *reinterpret_cast<FirstFitHeap<T, HA>* >(&heap[0]); }
    alignas(FirstFitHeap<T, HA>) char heap[sizeof(FirstFitHeap<T, HA>)];
    Mutex mutex;

    size_t allocated;
};

} // namespace mythos

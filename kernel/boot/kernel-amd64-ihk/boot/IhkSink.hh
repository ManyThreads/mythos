/* -*- mode:C++; -*- */
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
 * Copyright 2019 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/ISink.hh"
#include "util/TidexMutex.hh"
#include "cpu/hwthreadid.hh"
#include "boot/ihk-entry.hh"
#include "util/FixedStreamBuf.hh"

namespace mythos {

class IhkSink
  : public mlog::ISink
{
public:
  void write(char const* buf, size_t len);
  void flush() {} // @todo signal Linux side via IPI
protected:
  TidexMutex<KernelMutexContext> mutex;
};

inline void IhkSink::write(char const* buf, size_t len)
{
  if (boot::kmsg_buf == NULL) return;
  auto id = cpu::getThreadID();
  mutex << [this,buf,len,id]() {
    // acquire spinlock for the queue
    // @todo is this lock actually acquired from the Linux side? Otherwise we don't need this because of our own mutex here.
    while(__sync_val_compare_and_swap(&boot::kmsg_buf->lock, 0, 1) != 0) asm volatile("pause" ::: "memory");

    FixedStreamBuf<10> idbuf;
    ostream idstream(&idbuf);
    idstream << id << ": ";
    boot::memcpy_ringbuf(idbuf.c_str(), idbuf.size());
    boot::memcpy_ringbuf(buf, len);
    if (buf[len-1] != '\n') boot::memcpy_ringbuf("\n", 1);

    // @todo this looks weird. what is the meaning of head, tail and len on the Linux side?
    if (len+1 >= DEBUG_KMSG_MARGIN) { // overflow
      boot::kmsg_buf->head = (boot::kmsg_buf->tail + 1) % boot::kmsg_buf->len;
    }

    boot::kmsg_buf->lock = 0; // release the lock on the queue, no barrier needed because of total store order
  };
}

} // namespace mythos

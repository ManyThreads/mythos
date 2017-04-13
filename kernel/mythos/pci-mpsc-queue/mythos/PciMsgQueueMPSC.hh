/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MyThOS: The Many-Threads Operating System
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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include "util/optional.hh"
#include "cpu/hwthread_pause.hh"
#include "cpu/clflush.hh"

namespace mythos {

  /** Multiple Producer Single Consumer Message Queue in shared
   *  memory. The implementation is safe to use over PCIe2.
   *
   * PCIe2 does not support atomic fetch&increment operations and
   * silently drops them.
   */
  template<size_t S, unsigned N>
  class PCIeRingChannel
  {
  public:
    typedef uint16_t handle_t;
    constexpr static handle_t SLOTS=N;
    constexpr static size_t SIZE=S;
    constexpr static size_t DATASIZE = (SIZE-sizeof(std::atomic<handle_t>));

    struct Slot{
      std::atomic<handle_t> seq;
      char data[DATASIZE];
    };

    /** initialise the sequence counters in the slotz */
    void init() { for (handle_t i=0; i<SLOTS; i++) slotz[i].seq=handle_t(i<<1); }
    Slot& getSlot(handle_t handle) { return slotz[handle%SLOTS]; }
    template<class T>
    T* getData(handle_t handle) { return reinterpret_cast<T*>(getSlot(handle).data); }
    bool isWriteable(handle_t handle) { return getSlot(handle).seq==handle_t(handle<<1); }
    bool isReadable(handle_t handle) { return getSlot(handle).seq==handle_t((handle<<1)|1); }
    void setReadable(handle_t handle) { getSlot(handle).seq=handle_t((handle<<1)|1); }
    void setWriteable(handle_t handle) { getSlot(handle).seq=handle_t((handle+SLOTS)<<1); }
    void flush(handle_t handle, size_t bytes=SIZE) {
      auto help=reinterpret_cast<char*>(&getSlot(handle));
      for (uintptr_t i=0; i<bytes; i+=cpu::CACHELINESIZE) cpu::clflush(help+i);
    }
  private:
    // this array is named slotz because QT has a macro named slots. Thank you!
    alignas(64) Slot slotz[SLOTS];
  };

  template<class CHANNEL>
  class PCIeRingConsumer
  {
  public:
    typedef typename CHANNEL::handle_t handle_t;
    PCIeRingConsumer() {}
    PCIeRingConsumer(CHANNEL* channel) : channel(channel) {}

    void setChannel(CHANNEL* channel) { this->channel = channel; }
    bool hasMessages() { return channel->isReadable(readPos); }

    optional<handle_t> tryAcquireRecv() {
      handle_t temp(readPos);
      if(channel->isReadable(temp)){
        if(readPos.compare_exchange_strong(temp,readPos+1))
          return temp;
      }
      cpu::clflush(&channel->getSlot(temp));
      return optional<handle_t>(Error::INHIBIT);
    }

    handle_t acquireRecv() {
      handle_t rp = readPos.fetch_add(1);
      while (!channel->isReadable(rp)) {
        cpu::clflush(&channel->getSlot(rp));
        hwthread_pause(400);
      }
      return rp;
    }

    template<class T>
    T& get(handle_t handle) { return *channel->template getData<T>(handle); }
    char const* getPtr(handle_t handle) const { return channel->template getData<char>(handle); }

    void finishRecv(handle_t handle) {
      channel->setWriteable(handle);
      channel->flush(handle);
    }

    template<class T>
    optional<T> tryRecv() {
      auto h = tryAcquireRecv();
      if (!h) return optional<T>(Error::UNSET);
      T msg(get<T>(*h));
      finishRecv(*h);
      return msg;
    }

    template<class T>
    T recv() {
      auto h = acquireRecv();
      T msg(get<T>(h));
      finishRecv(h);
      return msg;
    }

  protected:
    CHANNEL* channel = nullptr;
    std::atomic<handle_t> readPos = {0};
  };

  template<class CHANNEL>
  class PCIeRingProducer
  {
  public:
    typedef typename CHANNEL::handle_t handle_t;
    PCIeRingProducer() {}
    PCIeRingProducer(CHANNEL* channel) : channel(channel) {}

    void setChannel(CHANNEL* channel) { this->channel = channel; }

    optional<handle_t> tryAcquireSend() {
      handle_t temp=writePos;
      if(channel->isWriteable(temp)){
        if(writePos->compare_exchange_strong(temp,writePos+1))
          return temp;
      }
      cpu::clflush(&channel->getSlot(temp));
      return optional<handle_t>(Error::INHIBIT);
    }

    handle_t acquireSend() {
      handle_t wp = writePos.fetch_add(1);
      while (!channel->isWriteable(wp)) {
        cpu::clflush(&channel->getSlot(wp));
        hwthread_pause(400);
      }
      return wp;
    }

    template<class T>
    T& get(handle_t handle) { return *channel->template getData<T>(handle); }
    char* getPtr(handle_t handle) { return channel->template getData<char>(handle); }

    void finishSend(handle_t handle, size_t bytes=CHANNEL::DATASIZE) {
      ASSERT(bytes <= CHANNEL::DATASIZE);
      channel->setReadable(handle);
      channel->flush(handle, bytes);
    }

    template<class T>
    bool trySend(T const& msg) {
      auto h = tryAcquireSend();
      if (!h) return false;
      new (&get<T>(*h)) T(msg);
      finishSend(*h, sizeof(T));
      return true;
    }

    template<class T>
    void send(T const& msg) {
      auto h = acquireSend();
      new (&get<T>(h)) T(msg);
      finishSend(h, sizeof(T));
    }

  protected:
    CHANNEL* channel = nullptr;
    std::atomic<handle_t> writePos = {0};
  };

} // namespace mythos

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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <atomic>
#include "util/assert.hh"
#include "util/ThreadMutex.hh"

namespace mythos {

  /** intrusive linked FIFO queue with enqueue, dequeue and delete of
   * elements */
  template<class T>
  class LinkedList
  {
  public:
    typedef T value_t;
    LinkedList() : head(nullptr), tail(&head) {}

    /** item implementation for list members. It is not intended as
     * base class such that future alternative implementations have an
     * easier time with dummy elements for the list management. */
    class Queueable
    {
    public:
      Queueable(value_t const& value) : value(value) {}
      //bool isEnqueued() const;
      value_t& get() { return value; }
      value_t& operator-> () { return value; }
      value_t const& operator-> () const { return value; }
      value_t& operator* () { return value; }
      value_t const& operator* () const { return value; }
    private:
      friend class LinkedList<value_t>;
      std::atomic<Queueable*> next;
      value_t value;
    };

    /** Tries to see if the queue is empty.
     * @warning because of remove() a later pull() may fail anyway.
     */
    bool empty() const { return head == nullptr; }

    void push(Queueable* item) { mutex << [this,item]() { this->_push(item); }; }

    Queueable* pull()
    {
      Queueable* res;
      mutex << [this,&res]() { res = this->_pull(); };
      return res;
    }

    Queueable* rotate()
    {
      Queueable* res;
      mutex << [this,&res]() {
        res = this->_pull();
        if (res) this->_push(res);
      };
      return res;
    }

    Queueable* peek() { return head; }

    bool remove(Queueable* item) {
      bool res;
      mutex << [this,&res,item]() {
        if (head == item) { // remove head
          head = item->Queueable::next.load();
          if (empty()) tail = &head;
          res = true; return;
        }
        // find the previous item in the chain
        for (auto cur = head.load(); cur != nullptr; cur = cur->Queueable::next.load()) {
          if (cur->Queueable::next == item) { // remove the next node
            cur->Queueable::next = item->Queueable::next.load();
            if (tail == &item->Queueable::next) tail = &cur->Queueable::next;
            res = true; return;
          }
        }
        res = false;
      };
      return res;
    }

  private:

    Queueable* _pull()
    {
      if (head != nullptr) {
        auto result = head.load();
        head = result->Queueable::next.load();
        if (empty()) tail = &head;
        return result;
      } else return nullptr;
    }

    void _push(Queueable* item)
    {
      ASSERT(item);
      item->Queueable::next = nullptr;
      *tail = item;
      tail = &item->Queueable::next;
    }

    ThreadMutex mutex; //< simple mutex to protect the queue updates
    std::atomic<Queueable*> head;
    std::atomic<Queueable*>* tail;
  };

} // namespace mythos

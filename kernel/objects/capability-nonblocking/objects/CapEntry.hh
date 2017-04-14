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
 * Copyright 2016 Robert Kuban, Randolf Rotta, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "objects/Cap.hh"
#include "objects/CapLink.hh"
#include <cstdint>
#include <atomic>

/* non-blocking doubly linked list based on
 * SundellTsigas2004 - Lock-Free and Practical Deques using Single-Word Compare-And-Swap
 * SundellTsigas2005 - Lock-Free and Practical Doubly Linked List-Based Deques Using Single-Word Compare-and-Swap
 * SundellTsigas2008 - Lock-free Deques and Doubly Linked Lists
 *
 * line annotations based on SundellTsigas2008
 */

namespace mythos {

  class CapEntry
  {
  public:

    Cap get() const { return Cap(cap.load()); }
    void set(Cap c) { cap.store(c.val()); }

    void insertBefore(CapEntry* newNode) {
      CapEntry* cursor = this;
      CapEntry* prevEntry = cursor->prev.deref(); // IB3
      CapEntry* nextEntry;

      while (true) {
        while (cursor->next.isMarked()) { // IB5
          cursor = nextNode(cursor);
          prevEntry = correctPrev(prevEntry, cursor); // IB7
        }

        nextEntry = cursor; // IB8
        newNode->prev.set(prevEntry, false);
        newNode->next.set(nextEntry, false);

        if (prevEntry->next.cas(cursor,newNode)) break; // IB11
        prevEntry = correctPrev(prevEntry, cursor); // IB12

        // TODO backoff
      }

      // IB14 just returns the pointer to the new node via the cursor
      correctPrev(prevEntry, nextEntry); // IB15 (is it needed?)
    }

    CapEntry* getNext() { return nextNode(this); }

  private:

    CapEntry* nextNode(CapEntry* cursor)
    {
      while (true) { // NT1
        CapEntry* nextEntry = cursor->next.deref(); // NT3
        bool delMark = nextEntry->next.isMarked(); // NT4
        if (delMark && !cursor->next.equals(nextEntry, true)) { // NT5
          nextEntry->prev.setMark(); // NT6
          cursor->next.cas(nextEntry, nextEntry->next.deref()); // NT7
          continue; // NT9
        }
        cursor = nextEntry; // NT11
        if (!delMark) return cursor;  // NT12
      }
    }

    CapEntry* correctPrev(CapEntry* prevEntry, CapEntry* node)
    {
      CapEntry* lastlink = nullptr; // CP1
      while (true) { // CP2
        CapLink link1 = node->prev; // CP3
        if (link1.isMarked()) break; // CP3
        CapLink prev2 = prevEntry->next; // CP4
        if (prev2.isMarked()) { // CP5
          if (lastlink != nullptr) { // CP6
            prevEntry->prev.setMark(); // CP7
            lastlink->next.cas(prevEntry, prev2.deref()); // CP8
            prevEntry = lastlink; // CP10
            lastlink = nullptr;
            continue;
          }
          prevEntry = prevEntry->prev.deref(); // CP13
          continue;
        }

        if (prev2.deref() != node) { // CP16
          lastlink = prevEntry; // CP18
          prevEntry =  prev2.deref(); // CP19
          continue;
        }

        if (node->prev.cas(link1, prevEntry)) { // CP22
          if (prevEntry->prev.isMarked()) continue; // CP23
          break;
        }
      }
      return prevEntry;
    }


  private:
    CapLink prev;
    CapLink next;
    std::atomic<uint64_t> cap;
  };

} // namespace mythos

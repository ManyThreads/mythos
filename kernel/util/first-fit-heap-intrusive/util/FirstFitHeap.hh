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

#include "util/assert.hh"
#include "util/optional.hh"
#include "util/align.hh"

namespace mythos {

  /** Utility class for intrusive First-Fit free list.
   *
   * \author Randolf Rotta, Robert Kuban
   */
  template<typename T, size_t HA=alignLine>
  class FirstFitHeap
  {
  public:
    typedef T addr_t;
    constexpr static size_t heapAlign = HA;

    FirstFitHeap() : first_free(0) {}
    virtual ~FirstFitHeap() {}

    size_t getAlignment() const { return heapAlign; }

    optional<addr_t> alloc(size_t length, size_t alignment) {
      length = round_up(length, heapAlign);
      auto r = salloc(length, alignment);
      //tracer.allocated(r, length, alignment);
      // if(r) tracer.allocated(size_t(*r), length, alignment);
      // else tracer.allocateFailed(length, alignment);
      return r;
    }

    void free(addr_t start, size_t length) {
      length = round_up(length, heapAlign);
      sfree(size_t(start), length);
      //tracer.freed(uintptr_t(start), length);
    }

    void addRange(addr_t start, size_t length) {
      start = round_up(start, heapAlign);
      length = round_down(length, heapAlign);
      sfree(size_t(start), length);
      //tracer.rangeAdded(size_t(start), length);
    }

  protected:

    struct Range {

      Range(uintptr_t length)
        : length(length), next(0) {}

      uintptr_t start() { return reinterpret_cast<uintptr_t>(this); }
      uintptr_t start(size_t alignment) { return round_up(start(), alignment); }
      uintptr_t offset(size_t alignment) { return start(alignment) - start(); }

      uintptr_t aligned_length(size_t alignment) {
        if (length > offset(alignment)) {
          return length - offset(alignment);
        } else return 0;
      }

      uintptr_t fit(size_t size, size_t alignment) {
        return size <= aligned_length(alignment);
      }

      size_t length;
      Range* next;
    };

    static_assert(sizeof(Range) <= heapAlign, "Firstfit range descriptor must fit into alignment.");
    static_assert(sizeof(uintptr_t) == sizeof(addr_t), "Firstfit range type size.");

    optional<addr_t> salloc(size_t size, size_t alignment);

    void sfree(uintptr_t start, size_t length);

  protected:
    Range* first_free;
  };

  template<typename T, size_t HA>
  auto FirstFitHeap<T,HA>::salloc(size_t size, size_t alignment) -> optional<addr_t>
  {
    ASSERT(is_aligned(size, heapAlign));
    ASSERT(is_aligned(alignment, heapAlign));
    // first fit search
    Range* r = first_free;
    Range** prev = &first_free;
    while (r != 0) {
      if (r->fit(size, alignment)) {
        // it fits
        if (!is_aligned(r,alignment)) {
          // split into "offset" and aligned range n
          auto n = reinterpret_cast<Range*>(r->start(alignment));
          n->length = r->aligned_length(alignment);
          n->next = r->next;
          r->length = r->offset(alignment);
          r->next = n;
          // then skip current range
          prev = &(r->next);
          r = n;
        }
        // now we are aligned
        if (r->length > size) { // there is a remaining range
          auto n = reinterpret_cast<Range*>(r->start() + size);
          n->length = r->length - size; // the remaining length
          n->next = r->next; // point to the next range
          *prev = n; // unlink r
        } else {
          *prev = r->next; // unlink r
        }
        // r is the range we reqested
        return optional<addr_t>(addr_t(r));
      } else { // no fit
        prev = &(r->next);
        r = r->next;
      }
    }
    return optional<addr_t>();
  }

  template<typename T, size_t HA>
  void FirstFitHeap<T,HA>::sfree(size_t start, size_t length)
  {
    ASSERT(is_aligned(start, heapAlign));
    ASSERT(is_aligned(length, heapAlign));
    Range* r = first_free;
    Range** prev = &first_free;
    while (r != 0) {
      if (r->start() == start + length) {
        // prepend to range
        auto n = reinterpret_cast<Range*>(start);
        n->length = length + r->length;
        n->next = r->next;
        *prev = n; // unlink r
        // can't link with predessor
        // if it had been possible, freed range would been added to predessor before
        return;
      } else if (r->start() + r->length == start) {
        // append to range
        r->length += length;
        // now r might be merged with successor
        if (r->next != 0 && r->start() + r->length == r->next->start()) {
          r->length += r->next->length;
          r->next = r->next->next;
        }
        return;
      } else if (start < r->start()) {
        // insert new range before r
        Range* n = reinterpret_cast<Range*>(start);
        n->length = length;
        n->next = r;
        *prev = n;
        return;
      } else {
        ASSERT(r->start() + r->length < start);
        // go to next range
        prev = &(r->next);
        r = r->next;
      }
    }
    // no free range left, append new range
    Range* n = reinterpret_cast<Range*>(start);
    n->length = length;
    n->next = 0;
    *prev = n;
  }

} // namespace mythos

/* -*- mode:C++; -*- */
#pragma once

#include "util/assert.hh"
#include "util/optional.hh"
#include "util/alignments.hh"

namespace mythos {

  /** Utility class for intrusive First-Fit free list.
   *
   * \author Randolf Rotta, Robert Kuban
   */
  template<typename T, class A=AlignLine>
  class FirstFitHeap
  {
  public:
    typedef T addr_t;
    typedef A Alignment;

    FirstFitHeap() : first_free(0) {}
    virtual ~FirstFitHeap() {}

    size_t getAlignment() const { return A::alignment(); }

    optional<addr_t> alloc(size_t length, size_t alignment) {
      length = Alignment::round_up(length);
      auto r = salloc(length, AlignmentObject(alignment));
      //tracer.allocated(r, length, alignment);
      // if(r) tracer.allocated(size_t(*r), length, alignment);
      // else tracer.allocateFailed(length, alignment);
      return r;
    }

    void free(addr_t start, size_t length) {
      length = Alignment::round_up(length);
      sfree(size_t(start), length);
      //tracer.freed(uintptr_t(start), length);
    }

    void addRange(addr_t start, size_t length) {
      start = Alignment::round_up(start);
      length = Alignment::round_down(length);
      sfree(size_t(start), length);
      //tracer.rangeAdded(size_t(start), length);
    }

  protected:

    struct Range {

      Range(uintptr_t length)
        : length(length), next(0) {}

      uintptr_t start() { return reinterpret_cast<uintptr_t>(this); }

      template<class Al>
      uintptr_t start(const Al& alignment) { return alignment.round_up(start()); }

      template<class Al>
      uintptr_t offset(const Al& alignment) { return start(alignment) - start(); }

      template<class Al>
      uintptr_t aligned_length(const Al& alignment) {
        if (length > offset(alignment)) {
          return length - offset(alignment);
        } else return 0;
      }

      template<class Al>
      uintptr_t fit(size_t size, const Al& alignment) {
        return size <= aligned_length(alignment);
      }

      size_t length;
      Range* next;
    };

    static_assert(sizeof(Range) <= Alignment::alignment(), "Firstfit range descriptor must fit into alignment.");
    static_assert(sizeof(uintptr_t) == sizeof(addr_t), "Firstfit range type size.");

    template<class Al>
    optional<addr_t> salloc(size_t size, const Al& alignment);

    void sfree(uintptr_t start, size_t length);

  protected:
    Range* first_free;
  };

  template<typename T, class A>
  template<class Al>
  auto FirstFitHeap<T,A>::salloc(size_t size, const Al& alignment) -> optional<addr_t>
  {
    ASSERT(Alignment::is_aligned(size));
    ASSERT(Alignment::is_aligned(alignment.alignment()));
    // first fit search
    Range* r = first_free;
    Range** prev = &first_free;
    while (r != 0) {
      if (r->fit(size, alignment)) {
        // it fits
        if (!alignment.is_aligned(r)) {
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

  template<typename T, class A>
  void FirstFitHeap<T,A>::sfree(size_t start, size_t length)
  {
    ASSERT(Alignment::is_aligned(start));
    ASSERT(Alignment::is_aligned(length));
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

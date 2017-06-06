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

#include "mythos/caps.hh"
#include "util/bitfield.hh"
#include <cstdint>
#include <new>
#include <atomic>

namespace mythos{

  class InvocationBase
  {
  public:
    constexpr static uint16_t maxBytes = 480;
    constexpr static uint8_t maxLength = maxBytes/4;

    BITFIELD_DEF(uint32_t, Tag)
    UIntField<value_t, base_t, 0, 16> label;
    UIntField<value_t, base_t, 16, 7> length;
    UIntField<value_t, base_t, 23, 3> extra_caps;
    UIntField<value_t, base_t, 26, 6> unwrapped_mask;
    BITFIELD_END

    InvocationBase() : tag(0) {}
    InvocationBase(uint16_t label) : tag(0) { tag.label = label; }
    InvocationBase(uint16_t label, uint8_t length) : tag(0) { tag = tag.label(label).length(length); }

    Tag tag;           // 1x 4byte
    CapPtr dstPtr = null_cap;     // 1x 4byte
    CapPtr capPtrs[6]; // 6x 4byte => 32 byte (half cache line)

    uint16_t getBytes() const { return uint16_t(tag.length*4); }

    void addExtraCap(CapPtr cptr) { capPtrs[tag.extra_caps++] = cptr; }

    template<class MSG>
    uint8_t getLength(MSG const*) const { return uint8_t((sizeof(MSG)-sizeof(InvocationBase)+3)/4); }

    template<class MSG>
    void setLength(MSG const* m) { tag.length = getLength(m); }

    template<class MSG>
    MSG const* cast() const { return static_cast<MSG const*>(this); }

    template<class MSG>
    MSG* cast() { return static_cast<MSG*>(this); }
  };

  class InvocationBuf
    : public InvocationBase
  {
  public:
    char message[maxBytes];

    template<class MSG, class... ARGS>
    MSG* write(ARGS const&... args) {
      static_assert(sizeof(MSG) <=sizeof(InvocationBuf), "message too large");
      return new(this) MSG(args...);
    }

    template<class MSG>
    MSG read() const {
      // struct Barrier {
      //   ~Barrier() { std::atomic_thread_fence(std::memory_order_seq_cst); }
      // } barrier;
      static_assert(sizeof(MSG) <=sizeof(InvocationBuf), "message too large");
      return *this->cast<MSG>();
    }
  };

  static_assert(sizeof(InvocationBuf) == 512, "invocation buffer size does not match specification");

} // namespace mythos

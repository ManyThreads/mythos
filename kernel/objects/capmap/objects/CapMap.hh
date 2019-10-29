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

#include <cstddef>
#include <cstdint>
#include <atomic>
#include "util/alignments.hh"
#include "mythos/protocol/CapMap.hh"
#include "mythos/protocol/KernelObject.hh"
#include "async/NestedMonitorDelegating.hh"
#include "objects/CapEntry.hh"
#include "objects/IKernelObject.hh"
#include "objects/IAllocator.hh"
#include "objects/ICapMap.hh"
#include "objects/IFactory.hh"
#include "objects/ops.hh"

namespace mythos {

class CapMap final
  : public IKernelObject
  , public ICapMap
{
public:
  CapMap(IAsyncFree* memory, CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard);
  CapMap(const CapMap&) = delete;
  virtual ~CapMap() {}

  /** helper function for the construction of the initial capability space */
  CapEntry* get(size_t idx) { return caps()+idx; }

  static size_t offset() { return AlignWord::round_up(sizeof(CapMap)); }
  static size_t size(uint8_t bits) { return offset() + cap_count(bits)*sizeof(CapEntry); }
  static size_t cap_count(uint8_t bits) { ASSERT(bits <= 32); return 1 << (bits); }

  CapEntry& getRoot() { return ownRoot; }

public: // ICapMap interface
  void acquireEntryRef() override { monitor.acquireRef(); }
  void releaseEntryRef() override { monitor.releaseRef(); }

public: // IKernelObject interface
  optional<Cap> mint(CapEntry&, Cap self, CapRequest request, bool derive) override;
  optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
  void deleteObject(Tasklet* t, IResult<void>* r) override {
    monitor.doDelete(t, [=](Tasklet* t) { memory->free(t, r, this, CapMap::size(indexbits)); });
  }
  optional<CapEntryRef> lookup(Cap self, CapPtr ptr, CapPtrDepth depth=32, bool writeable=true) override;
  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
  optional<void const*> vcast(TypeId id) const override;

  Error invokeReference(Tasklet*, Cap, IInvocation*);
  Error invokeDerive(Tasklet*, Cap, IInvocation*);
  Error invokeMove(Tasklet*, Cap, IInvocation*);
  Error invokeDelete(Tasklet*, Cap, IInvocation*);
  Error invokeRevoke(Tasklet*, Cap, IInvocation*);
  Error getDebugInfo(Cap, IInvocation*);

private:

  optional<CapEntryRef> lookupDst(Cap self, IInvocation* msg, protocol::CapMap::BinaryOp& data);
  CapEntry* caps() { return reinterpret_cast<CapEntry*>(uintptr_t(this)+offset()); }

private:
  const CapPtrDepth indexbits;
  CapPtrDepth guardbits;
  CapPtr guard;
  static constexpr size_t ptrbits = sizeof(CapPtr)*8;

  /** root capability entry for itself, everybody else just has derived and reference caps */
  CapEntry ownRoot;

  LinkedList<IKernelObject*>::Queueable del_handle = {this};
  IAsyncFree* memory;
  async::NestedMonitorDelegating monitor;
};

class CapMapFactory : public FactoryBase
{
public:
  static optional<CapMap*>
  initial(CapEntry* memEntry, Cap memCap, IAllocator* mem,
            CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard);

  static optional<CapMap*>
  factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
          CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard);

  Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                IAllocator* mem, IInvocation* ib) const override {
    auto data = ib->getMessage()->cast<protocol::CapMap::Create>();
    return factory(dstEntry, memEntry, memCap, mem, data->indexbits, data->guardbits, data->guard).state();
  }
};

}  // namespace mythos

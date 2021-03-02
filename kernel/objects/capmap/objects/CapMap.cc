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

#include <new>
#include "objects/CapMap.hh"
#include "objects/TypedCap.hh"
#include "objects/ops.hh"
#include "objects/DebugMessage.hh"

namespace mythos {

  BITFIELD_DEF(CapData, CapMapData)
  BoolField<value_t,base_t,0> writable;
  CapMapData() : value(0) {}
  BITFIELD_END

  CapMap::CapMap(IAsyncFree* memory, CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard)
    : indexbits(indexbits), guardbits(guardbits), guard(guard)
    , ownRoot(Cap::asAllocated()), memory(memory)
  {
    ASSERT(indexbits+guardbits <= ptrbits);
    for (size_t i=0; i<cap_count(indexbits); i++) new(caps()+i) CapEntry();
  }

  optional<CapMap*>
  CapMapFactory::initial(CapEntry* memEntry, Cap memCap, IAllocator* mem,
                        CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard)
  {
    /// @todo check bits
    auto ptr = mem->alloc(CapMap::size(indexbits), 64);
    if (!ptr) RETHROW(ptr);
    auto obj = new(*ptr) CapMap(mem, indexbits, guardbits, guard);
    auto cap = Cap(obj).withData(CapMapData().writable(true));
    auto res = cap::inherit(*memEntry, memCap, obj->getRoot(), cap);
    if (!res) {
      mem->free(*ptr, CapMap::size(indexbits));
      RETHROW(res);
    }
    return obj;
  }

  optional<CapMap*>
  CapMapFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
                        CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard)
  {
    auto ptr = mem->alloc(CapMap::size(indexbits), 64);
    if (!ptr) RETHROW(ptr);
    auto obj = new(*ptr) CapMap(mem, indexbits, guardbits, guard);
    auto cap = Cap(obj).withData(CapMapData().writable(true));
    auto res = cap::inherit(*memEntry, memCap, *dstEntry, cap);
    if (!res) {
      mem->free(*ptr, CapMap::size(indexbits));
      RETHROW(res);
    }
    return obj;
  }

  optional<Cap> CapMap::mint(CapEntry&, Cap self, CapRequest request, bool)
  {
    // intersection between own and requested flags
    return self.withData(self.data() & CapData(request));
  }

  optional<void> CapMap::deleteCap(CapEntry&, Cap self, IDeleter& del)
  {
    MLOG_INFO(mlog::cap, "CapMap::deleteCap");
    if (self.isOriginal()) {
      MLOG_DETAIL(mlog::cap, "delete original");
      // delete all entries, leaves them in a locked state (allocated)
      // in order to prevent concurrent insertion of new caps.
      for (size_t i=0; i<cap_count(indexbits); i++) {
        auto res = del.deleteEntry(caps()[i]);
        if (!res) return res;
      }
      // only delete if all subentries are deleted
      del.deleteObject(del_handle);
    }
    RETURN(Error::SUCCESS);
  }

  optional<CapEntryRef> CapMap::lookup(Cap self, CapPtr ptr, CapPtrDepth depth, bool writable)
  {
    if (writable && !CapMapData(self.data()).writable) { THROW(Error::NO_LOOKUP); }
    CapPtr guard = this->guard;
    CapPtrDepth guardbits = this->guardbits;
    if (depth < guardbits+indexbits)
      THROW(Error::INVALID_CAPABILITY); // ptr is too small
    if (ptr >> (ptrbits-guardbits) != guard)
      THROW(Error::INVALID_CAPABILITY); // guard not matching
    depth = CapPtrDepth(depth - guardbits);
    ptr <<= guardbits; // strip guard from address
    CapPtr index = ptr >> (ptrbits-indexbits); // get index
    depth = CapPtrDepth(depth - indexbits);
    ptr <<= indexbits; // strip index from address
    CapEntry* entry = caps()+index;
    if (depth == 0) return CapEntryRef(entry, this); // reached end of address => return entry
    TypedCap<ICapMap> sub(entry->cap());
    if (!sub) RETHROW(sub.state());
    return sub.lookup(ptr, depth, writable); // recursive lookup
  }

  void CapMap::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    /// @todo Monitor might not be neccessary for derive, reference, move
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
         case protocol::KernelObject::proto:
           err = protocol::KernelObject::dispatchRequest(this, msg->getMethod(), self, msg);
           break;
        case protocol::CapMap::proto:
          err = protocol::CapMap::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  optional<CapEntryRef> CapMap::lookupDst(Cap self, IInvocation* msg, protocol::CapMap::BinaryOp& data)
  {
    auto cspace = data.capPtrs[0];
    if (cspace != null_cap) {
      // The destination cspace must be looked up from the ECs cspace
      // because the source and destination capmaps can be completely unrelated.
      TypedCap<ICapMap> dstMap(msg->lookupEntry(cspace));
      if (!dstMap) RETHROW(dstMap);
      RETURN(dstMap.lookup(data.dstPtr(), data.dstDepth, true));
    } else {
      // source = dest
      RETURN(this->lookup(self, data.dstPtr(), data.dstDepth, true));
    }
  }

  Error CapMap::getDebugInfo(Cap self, IInvocation* msg)
  {
    return writeDebugInfo("CapMap", self, msg);
  }

  Error CapMap::invokeDerive(Tasklet*, Cap cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::CapMap::Derive>();
    // retrieve dst cap entry
    auto dstRef = lookupDst(cap, msg, data);
    if (!dstRef) return dstRef.state();
    // retrieve source cap entry
    auto srcRef = this->lookup(cap, data.srcPtr(), data.srcDepth, false);
    if (!srcRef) return srcRef.state();
    // derive
    return cap::derive(*srcRef->entry, *dstRef->entry, srcRef->entry->cap(), data.request).state();
  }

  Error CapMap::invokeReference(Tasklet*, Cap cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::CapMap::Reference>();
    // retrieve dst cap entry
    auto dstRef = lookupDst(cap, msg, data);
    if (!dstRef) return dstRef.state();
    // retrieve source cap entry
    auto srcRef = this->lookup(cap, data.srcPtr(), data.srcDepth, false);
    if (!srcRef) return srcRef.state();
    // reference
    return cap::reference(*srcRef->entry, *dstRef->entry, srcRef->entry->cap(), data.request).state();
  }

  Error CapMap::invokeMove(Tasklet*, Cap cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::CapMap::Move>();
    // retrieve dst cap entry
    auto dstRef = lookupDst(cap, msg, data);
    if (!dstRef) return dstRef.state();
    // retrieve source cap entry
    auto srcRef = this->lookup(cap, data.srcPtr(), data.srcDepth, true);
    if (!srcRef) return srcRef.state();
    // move
    auto res = dstRef->entry->acquire();
    if (!res) { return res.state(); }
    return srcRef->entry->moveTo(*dstRef->entry).state();
  }

  Error CapMap::invokeDelete(Tasklet*, Cap cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::CapMap::Delete>();
    auto ref = this->lookup(cap, data.srcPtr(), data.srcDepth, true);
    if (!ref) return ref.state();
    MLOG_INFO(mlog::cap, "invokeDelete", DVAR(data.srcPtr()), DVAR(ref->entry->cap()));
    msg->deletionResponse(ref->entry, true); // delete
    monitor.requestDone();
    return Error::INHIBIT;
  }

  Error CapMap::invokeRevoke(Tasklet*, Cap cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::CapMap::Delete>();
    auto ref = this->lookup(cap, data.srcPtr(), data.srcDepth, true);
    if (!ref) return ref.state();
    MLOG_INFO(mlog::cap, "invokeRevoke", DVAR(data.srcPtr()), DVAR(ref->entry->cap()));
    msg->deletionResponse(ref->entry, false); // revoke
    monitor.requestDone();
    return Error::INHIBIT;
  }

  optional<void const*> CapMap::vcast(TypeId id) const
  {
    if (id == typeId<ICapMap>()) { return static_cast<const ICapMap*>(this); }
    THROW(Error::TYPE_MISMATCH);
  }
}  // namespace mythos

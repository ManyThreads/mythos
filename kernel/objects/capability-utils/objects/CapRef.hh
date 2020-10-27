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

#include "objects/mlog.hh"
#include "objects/Cap.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "util/PhysPtr.hh"

namespace mythos {

  /**
   *
   * Subject is the owner of the CapRef instance.
   */ 
  class CapRefBase
    : protected IKernelObject
  {
  public:

    CapRefBase() : orig(0) {}

    /** removes the entry from the tree before deleting the entry's memory. */
    virtual ~CapRefBase();

    /** removes the entry from the tree, calls revoked() if successfull. */
    void reset();

    /** checks if the reference is currently usable. warning: may change concurrently. */
    bool isUsable() const { return Cap(this->orig.load()).isUsable(); }

    Cap cap() const { return Cap(this->orig.load()); }

  protected:
    optional<void> set(void* subject, CapEntry& src, Cap srcCap);

    /** called during successful write to the reference.
     *
     * Users should override this method in order to set cached values
     * and state flags. Mutual exclusion is ensured by the @c CapRef.
     */
    virtual void binding(void* subject, Cap obj) = 0;

    /** called during the revocation or reset of the reference.
     *
     * Users should override this method in order to reset cached
     * values and state flags. Mutual exclusion is ensured by the @c
     * CapRef.
     */
    virtual void unbinding(void* subject, Cap obj) = 0;

  protected: // IKernelObject interface
    Range<uintptr_t> addressRange(CapEntry& entry, Cap self) override;
    optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;

  protected:
    CapEntry entry;
    std::atomic<CapValue> orig;
  };

  class CapRefBind
  {
  public:
    template<class Subject, class Object>
    static void binding(Subject *s, Object const& o) { s->bind(o); }

    template<class Subject, class Object>
    static void unbinding(Subject *s, Object const& o) { s->unbind(o); }
  };

  template<class Subject, class Object, class Revoker=CapRefBind>
  class CapRef
    : public CapRefBase
  {
  public:
    typedef Subject subject_type;
    typedef Object object_type;

    virtual ~CapRef() {}

    optional<void> set(Subject* subject, CapEntry* src, Cap srcCap)
    { return CapRefBase::set(subject, *src, srcCap); }

    optional<void> set(Subject* subject, CapEntry& src, Cap srcCap)
    { return CapRefBase::set(subject, src, srcCap); }

    optional<Object*> get() const {
      Cap obj = Cap(this->orig.load());
      if (obj.isUsable()) return obj.getPtr()->cast<Object>();
      else THROW(Error::GENERIC_ERROR);
    }

  protected:
    void binding(void* subject, Cap orig) override {
      ASSERT(orig.isUsable());
      Revoker::binding(static_cast<Subject*>(subject), orig.getPtr()->cast<Object>());
    }

    void unbinding(void* subject, Cap orig) override {
      ASSERT(orig.isUsable());
      auto obj = orig.getPtr()->cast<Object>();
      Revoker::unbinding(static_cast<Subject*>(subject), obj);
      MLOG_INFO(mlog::cap,"before return");
    }
  };

} // namespace mythos

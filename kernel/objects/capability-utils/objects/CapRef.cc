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

#include "objects/CapRef.hh"
#include "objects/ops.hh"

namespace mythos {

  CapRefBase::~CapRefBase()
  {
    cap::resetReference(this->entry);
  }

  optional<void> CapRefBase::set(void* subject, CapEntry& src, Cap srcCap)
  {
    this->reset();
    RETURN(cap::setReference(
        this->entry,
        srcCap.asReference(this, kernel2phys(subject)),
        src, srcCap,
        [=](){
          this->orig.store(srcCap.asReference().value());
          this->binding(subject, srcCap.asReference());
        }));
  }

  void CapRefBase::reset()
  {
    // assumption: no need to inform the original object about the revoked reference
    cap::resetReference(this->entry,
      [=](){
        this->unbinding(offset2kernel<void>(this->entry.cap().data()), Cap(this->orig.load()));
        this->orig.store(Cap().value());
      });
  }

  Range<uintptr_t> CapRefBase::addressRange(CapEntry& entry, Cap) {
    Cap obj = Cap(this->orig.load());
    ASSERT(obj.isUsable());
    return obj.getPtr()->addressRange(entry, obj);
  }

  optional<void> CapRefBase::deleteCap(CapEntry& /*entry*/, Cap self, IDeleter& /*del*/)
  {
    // assumption: no need to inform the original object about the revoked reference
    // orig.getPtr()->deleteCap(entry, orig, del);
    this->unbinding(offset2kernel<void>(self.data()), Cap(orig.load()));
    this->orig.store(Cap().value());
    RETURN(Error::SUCCESS);
  }

} // namespace mythos

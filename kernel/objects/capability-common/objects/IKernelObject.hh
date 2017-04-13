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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "objects/IAllocator.hh"
#include "objects/IDeleter.hh"
#include "objects/IInvocation.hh"
#include "objects/Cap.hh"
#include "mythos/caps.hh"
#include "util/ICastable.hh"
#include "util/Range.hh"
#include "util/optional.hh"
#include "async/IResult.hh"

namespace mythos {

  class CapEntry;

  class IKernelObject
    : public ICastable
  {
  public:
    virtual ~IKernelObject() {}

    /** returns a address range that is managed by this object, that
     * is which contains its derived objects.  The range is used for
     * inheritance tests in the resource tree.
     * The range must be a subset of its parents range.
     * Default implementation returns this.
     *
     * @param self the capability that was used to access this
     *             object. The contained meta-data can be used for
     *             computing the range.
     */
    virtual Range<uintptr_t> addressRange(Cap self);

    /** create a minted capability if the originial capability
     * provides enough rights for the requested change.
     * Default implementation returns Error::NOT_IMPLEMENTED
     * for everything but NO_REQUEST, which returns the thisCap.
     *
     * @param self     the capability that was used to access this object.
     * @param request  abstract description of the requested change, which is
     *                 interpreted by the object. Can contain, for example,
     *                 a badge value or a bitmask for restricted access rights.
     * @param derive   a derive operation was requested, otherwise it's a reference operation.
     * @return the modified capability value, which then will be stored in a capability entry.
     */
    virtual optional<Cap> mint(Cap self, CapRequest request, bool derive);

    /** notifies the object about the revocation of a reference.  This
     * is called after the affected capability entry was removed from
     * the resource tree. The contained capability is in the zombie
     * state already, that is the transition bit is set but all other
     * meta-data is still usable.
     *
     * Some capabilities, for example the MappedFrame, point to the
     * capability's subject instead of the capability's object. In
     * that case, the receiver of revokeNotification is a capability
     * container that contains the revoked capability (self). This
     * mechanism is used to update, for example, Page Maps.  Such
     * containers shall encode the position of the entry inside the
     * capability's meta-data.
     *
     * If the the object's original capability (the root of the object
     * in the resource tree) was revoked, it is guaranteed that there
     * are no derived capabilities (aka weak references) left before
     * the call to revokeNotification. In this case, the object shall
     * add itself to the passed deletion list and recursively clear
     * all of its contained capability entries.
     *
     * @param self  the value of the capability that was revoked.
     * @param del   a list of kernel objects that are ready for deletion.
     */
    virtual optional<void> deleteCap(Cap self, IDeleter& del) = 0;

    /** initiates the asynchronous final deletion of this kernel
     * object.  The object's monitor delays the deletion until all
     * references were released and all pending tasks processed. The
     * object returns all of its used memory back to the single
     * Untyped Memory from where it was allocated.
     *
     * @param t  the tasklet needed for asynchronous execution.
     * @param r  the receiver of the asynchronous result.
     * @return always replies with Error::SUCCESS.
     */
    virtual void deleteObject(Tasklet* t, IResult<void>* r);

    /** asynchronously processes the invocation message.
     * Default implementation replies with Error::NOT_IMPLEMENTED.
     *
     * @param t     the tasklet needed for asynchronous execution.
     * @param self  the capability entry that was used to access this object.
     *              Some objects need the entry than just the value in order to
     *              know where to append new child entries. These shall consult
     *              the msg object.
     * @param msg   pointer to an object that contains the invocation buffer,
     *              affected capability entries, intermediate portal and so on.
     * @return should reply with Error::SUCCESS with the results or error message
     *         written into the invocation msg.
     */
    virtual void invoke(Tasklet* t, Cap self, IInvocation* msg);
  };

  inline Range<uintptr_t> IKernelObject::addressRange(Cap) {
    return Range<uintptr_t>::bySize(PhysPtr<void>::fromKernel(this).physint(), 1);
  }

  inline optional<Cap> IKernelObject::mint(Cap self, CapRequest, bool) {
    return self;
  }

  inline void IKernelObject::invoke(Tasklet*, Cap, IInvocation* msg) {
    msg->replyResponse(Error::NOT_IMPLEMENTED);
  }

  inline void IKernelObject::deleteObject(Tasklet*, IResult<void>*) {
    PANIC_MSG(false, "this object is not deletable");
  }

} // namespace mythos

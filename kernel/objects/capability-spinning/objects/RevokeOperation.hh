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

#include <cstdint>
#include "util/assert.hh"
#include "util/LinkedList.hh"
#include "async/IResult.hh"
#include "async/NestedMonitorDelegating.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "objects/DeleteBroadcast.hh"
#include "objects/mlog.hh"

#include "objects/ops.hh"

namespace mythos {

class RevokeOperation
  : public IResult<void>
  , protected IDeleter
{
public:

  typedef IResult<void> result_t;
  typedef LinkedList<IKernelObject*> queue_t;

  RevokeOperation(async::NestedMonitorDelegating& m) : monitor(m), _res(nullptr) {}
  RevokeOperation(const RevokeOperation&) = delete;
  virtual ~RevokeOperation() {}

  void deleteObject(LinkedList<IKernelObject*>::Queueable& obj) override
  {
    _deleteQueue.push(&obj);
  }

  optional<void> deleteEntry(CapEntry& entry) override;

  // @todo use pointer?
  void revokeCap(Tasklet* t, result_t* res, CapEntry& entry, IKernelObject* guarded)
  {
    monitor.request(t, [=, &entry](Tasklet* t) {
      MLOG_ERROR(mlog::cap, "revoke cap called");
      _revoke(t, res, entry, guarded);
    });
  }

  void deleteCap(Tasklet* t, result_t* res, CapEntry& entry, IKernelObject* guarded)
  {
    monitor.request(t, [=, &entry](Tasklet* t) {
      _delete(t, res, entry, guarded);
    });
  }

  virtual void response(Tasklet* t, optional<void> res) override
  {
    ASSERT(res);
    monitor.response(t, [=](Tasklet* t) {
        MLOG_DETAIL(mlog::cap, "received broadcast or delete response");
        _deleteObject(t);
      });
  }

  bool acquire() {
    auto result = !_lock.test_and_set();
    MLOG_ERROR(mlog::cap, DVAR(this), "acquire ops =>", result);
    return result;
  }

  void release() {
    MLOG_ERROR(mlog::cap, DVAR(this), "release ops");
    _lock.clear();
  };

private:

  void _revoke(Tasklet* t, result_t* res, CapEntry& entry, IKernelObject* guarded);
  void _delete(Tasklet* t, result_t* res, CapEntry& entry, IKernelObject* guarded);
  void _deleteObject(Tasklet* t);

  optional<void> _delete(CapEntry* root, Cap rootCap);
  bool _startTraversal(CapEntry* root, Cap rootCap);
  CapEntry* _findLockedLeaf(CapEntry* root);

  void _startAsyncDelete(Tasklet* t);

private:
  async::NestedMonitorDelegating& monitor;
  /// FIFO ... this way, every object apears before its parents
  /// and can be deleted (avoids intraprocess/object snychronization)
  /// children might be in another list, but these can be waited for
  /// as another operation objects will delete them.
  queue_t _deleteQueue;
  result_t* _res;
  IKernelObject* _guarded;
  Error _result;

  std::atomic_flag _lock = ATOMIC_FLAG_INIT;

};

} // namespace mythos

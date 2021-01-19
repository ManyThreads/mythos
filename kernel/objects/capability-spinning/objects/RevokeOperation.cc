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

#include "objects/RevokeOperation.hh"

namespace mythos {

  extern CapEntry* getRootCapEntry();

  optional<void> RevokeOperation::deleteEntry(CapEntry& entry)
  {
    if (entry.isDeleted()) {
      RETURN(Error::SUCCESS);
    }
    while(true) {
      ASSERT(!entry.isDeleted());
      if (entry.tryAcquire()) {
        entry.setDeleted(); // let everybody know that this is deleted
        RETURN(Error::SUCCESS);
      }
      // try to kill, then retry acquire
      if (entry.kill()) {
        auto result = _delete(&entry, entry.cap());
        if (!result) RETHROW(result); // we cannot recover from this
      } else {
        hwthread_pause(); // not acquirable nor killable: just wait a little
      }
    }
  }


  void RevokeOperation::_delete(Tasklet* t, result_t* res, CapEntry& entry, IKernelObject* guarded)
  {
    MLOG_INFO(mlog::cap, "_delete", DVAR(t), DVAR(res), DVAR(entry.cap()), DVAR(guarded));
    _res = res;
    _guarded = guarded;
    if (!acquire()) {
      // aquire might fail due to a concurrent deletion of the containing Portal
      MLOG_DETAIL(mlog::cap, "RevokeOperation can not be acquired.");
      res->response(t, Error::LOST_RACE);
      monitor.requestDone();
      return;
    }
    if (!entry.kill()) {
      MLOG_DETAIL(mlog::cap, "Can not kill entry", &entry, entry);
      res->response(t, Error::LOST_RACE);
      release();
      monitor.requestDone();
      return;
    }
    _result = _delete(&entry, entry.cap()).state();
    _startAsyncDelete(t);
  }

  void RevokeOperation::_revoke(Tasklet* t, result_t* res, CapEntry& entry, IKernelObject* guarded)
  {
    _res = res;
    _guarded = guarded;
    if (!acquire()) {
      MLOG_DETAIL(mlog::cap, "RevokeOperation can not be acquired.");
      res->response(t, Error::LOST_RACE);
      monitor.requestDone();
      return;
    }
    entry.lock();
    auto rootCap = entry.cap();
    if (!rootCap.isUsable()) {
      // this is not the cap you are locking for ...
      entry.unlock();
      res->response(t, Error::LOST_RACE);
      release();
      monitor.requestDone();
      return;
    }
    // just set the revoke flag to pin it
    // if some other revoke or delete clears the flag or changes the cap values
    // all children have been deleted in the mean time and we are done
    entry.setRevoking();
    entry.unlock();
    _result = _delete(&entry, rootCap).state();
    _startAsyncDelete(t);
  }

  optional<void> RevokeOperation::_delete(CapEntry* root, Cap rootCap)
  {
    CapEntry* leaf;
    do {
      if (_startTraversal(root, rootCap)) {
        leaf = _findLockedLeaf(root);
        MLOG_DETAIL(mlog::cap, "_findLockedLeaf returned", DVAR(*leaf), DVAR(rootCap));
        if (leaf == root && !rootCap.isZombie()) {
          // this is a revoke, do not delete the root. no more children -> we are done
          root->finishRevoke();
          root->unlock();
          root->unlock_prev();
          RETURN(Error::SUCCESS);
        }
        auto leafCap = leaf->cap();
        ASSERT(leafCap.isZombie());
        if (leafCap.getPtr() == _guarded) {
          leaf->unlock();
          leaf->unlock_prev();
          // attempted to delete guarded object
          THROW(Error::CYCLIC_DEPENDENCY);
        }
        auto delRes = leafCap.getPtr()->deleteCap(*leaf, leafCap, *this);
        if (delRes) {
          leaf->unlink();
          leaf->reset();
        } else {
          // Either tried to delete a portal that is currently deleting
          // or tried to to delete _guarded via a recursive call.
          leaf->unlock();
          leaf->unlock_prev();
          RETHROW(delRes);
        }
      } else RETURN(Error::SUCCESS); // could not restart, must be done
    } while (leaf != root);
    // deleted root
    RETURN(Error::SUCCESS);
  }

  bool RevokeOperation::_startTraversal(CapEntry* root, Cap rootCap)
  {
    if (!root->lock_prev()) {
      // start is currently unlinked
      // must be the work of another deleter ... success!
      return false;
    }
    if (root->prev() == root) {
      // this is the actual root of the tree
      // and has no children
      // avoid deadlocks and finish the operation
      root->unlock_prev();
      return false;
    }
    root->lock();
    // compare only value, not the zombie state
    if (root->cap().asZombie() != rootCap.asZombie()) {
      // start has a new value
      // must be the work of another deleter ... success!
      root->unlock();
      root->unlock_prev();
      return false;
    }
    return true;
  }

  CapEntry* RevokeOperation::_findLockedLeaf(CapEntry* root)
  {
    auto curEntry = root;
    while (true) {
      auto curCap = curEntry->cap();
      auto nextEntry = curEntry->next();
      // wait for potencially allocated cap to become usable/zombie
      Cap nextCap;
      for (nextCap = nextEntry->cap();
          nextCap.isAllocated();
          nextCap = nextEntry->cap()) {
        ASSERT(!nextEntry->isDeleted());
        hwthread_pause();
      }
      if (cap::isParentOf(*curEntry, curCap, *nextEntry, nextCap)) {
        // go to next child
        curEntry->unlock_prev();
        nextEntry->kill();
        nextEntry->lock();
        curEntry = nextEntry;
        continue;
      } else return curEntry;
    }
  }

  void RevokeOperation::_startAsyncDelete(Tasklet* t)
  {
    if (_deleteQueue.empty()) {
      // no async phase
      release();
      _res->response(t, _result);
      monitor.requestDone();
    } else {
      // start async phase
      DeleteBroadcast::run(t, this);
    }
  }

  void RevokeOperation::_deleteObject(Tasklet* t)
  {
    if (_deleteQueue.empty()) {
      _res->response(t, _result);
      release();
      monitor.requestDone();
    } else {
      auto obj = _deleteQueue.pull();
      obj->get()->deleteObject(t, this);
    }
    monitor.responseDone();
  }

} // mythos


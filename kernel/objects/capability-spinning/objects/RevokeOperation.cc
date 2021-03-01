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
    entry.lock_cap();
    auto rootCap = entry.cap();
    if (!rootCap.isUsable()) {
      // this is not the cap you are locking for ...
      entry.unlock_cap();
      res->response(t, Error::LOST_RACE);
      release();
      monitor.requestDone();
      return;
    }
    // just set the revoke flag to pin it
    // if some other revoke or delete clears the flag or changes the cap values
    // all children have been deleted in the mean time and we are done
    entry.setRevoking();
    entry.unlock_cap();
    _result = _delete(&entry, rootCap).state();
    _startAsyncDelete(t);
  }

  optional<void> RevokeOperation::_delete(CapEntry* root, Cap rootCap)
  {
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(root), DVAR(rootCap));
    CapEntry* leaf;
    Cap leafCap;
    do {
      // compare only value, not the zombie state
      if (root->cap().asZombie() != rootCap.asZombie()) {
        // start has a new value
        // must be the work of another deleter ... success!
        RETURN(Error::SUCCESS); // could not restart, must be done
      }
      if (!_findLeaf(root, rootCap, leaf, leafCap)) continue;
      MLOG_ERROR(mlog::cap, "_findLockedLeaf returned", DVAR(*leaf), DVAR(rootCap));
      if (leaf == root && !rootCap.isZombie()) {
        // this is a revoke, do not delete the root. no more children -> we are done
        root->finishRevoke();
        RETURN(Error::SUCCESS);
      }
      ASSERT(leafCap.isZombie());
      if (leafCap.getPtr() == _guarded) {
        leaf->unlock_cap();
        // attempted to delete guarded object
        THROW(Error::CYCLIC_DEPENDENCY);
      }
      leaf->lock_cap();
      if (leaf->cap() != leafCap) {
        MLOG_DETAIL(mlog::cap, "leaf cap changed concurrently");
        leaf->unlock_cap();
        continue;
      }
      auto delRes = leafCap.getPtr()->deleteCap(*leaf, leafCap, *this);
      auto prevres = leaf->lock_prev();
      ASSERT_MSG(prevres, "somebody unlinked CapEntry currently in unlinking process");
      leaf->lock_next();
      if (delRes) {
        leaf->unlinkAndUnlockLinks();
        leaf->reset();
      } else {
        // deletion failed
        // Either tried to delete a portal that is currently deleting
        // or tried to to delete _guarded via a recursive call.
        leaf->unlock_cap();
        RETHROW(delRes);
      }
    } while (leaf != root);
    // deleted root
    RETURN(Error::SUCCESS);
  }

  // not sure if we even need that locking
  bool RevokeOperation::_findLeaf(CapEntry* const root, Cap const rootCap, CapEntry*& leafEntry, Cap& leafCap)
  {
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(root) );
    leafEntry = root;
    leafCap = rootCap;
    while (true) {
      auto nextEntry = leafEntry->next();
      if (nextEntry) {
        Cap nextCap;
        // wait for potencially allocated cap to become usable/zombie
        for (nextCap = nextEntry->cap();
            nextCap.isAllocated();
            nextCap = nextEntry->cap()) {
          ASSERT(!nextEntry->isDeleted());
          hwthread_pause();
        }
        if (cap::isParentOf(*leafEntry, leafCap, *nextEntry, nextCap)) {
          if (!nextEntry->kill(nextCap)) {
            MLOG_DETAIL(mlog::cap, "cap to be killed changed concurrently");
            return false; 
          }
          // go to next child
          leafEntry = nextEntry;
          leafCap = nextCap.asZombie();
          continue;
        } else return true;
      } else {
        MLOG_DETAIL(mlog::cap, "found dead end scanning for leaf");
        return false; // restart at root
      }
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


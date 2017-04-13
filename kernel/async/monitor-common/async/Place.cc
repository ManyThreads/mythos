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
#include "async/Place.hh"

#include "cpu/ctrlregs.hh"

namespace mythos {
namespace async {

  Place places[BOOT_MAX_THREADS];
  CoreLocal<Place*> localPlace_ KERNEL_CLM_HOT; // for DeployHWThread

  void Place::init(size_t apicID)
  {
    MLOG_INFO(mlog::async, "init Place", DVAR(this), DVAR(apicID));
    this->apicID = apicID;
    this->nestingMonitor = true;
    this->_cr3 = PhysPtr<void>(cpu::getPageTable());
    this->queue.tryAcquire();
  }

  void Place::processTasks()
  {
    auto msg = queue.pull();
    while (true) {
      if (msg != nullptr) {
        MLOG_DETAIL(mlog::async, this, "run tasklet", msg);
        msg->run();
      } else {
        if (queue.tryRelease()) break;
        hwthread_pause();
      }
      msg = queue.pull();
    }
    // this assertion races with concurrent push operations
    //OOPS(!queue.isLocked());
    nestingMonitor.store(false); // release?
  }

  void Place::setCR3(PhysPtr<void> value)
  {
    ASSERT(isLocal());
    if (_cr3 != value) {
      _cr3 = value;
      cpu::loadPageTable(value.physint());
    }
  }

  PhysPtr<void> Place::getCR3()
  {
    ASSERT(implies(isLocal(), cpu::getPageTable() == _cr3.physint()));
    return _cr3;
  }

} // async
} // mythos

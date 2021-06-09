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
 * Copyright 2021 Philipp Gypser, BTU Cottbus-Senftenberg
 */

#include "objects/ProcessorTopology.hh"

namespace mythos {
namespace topology {
  CapEntry* ISocket::getFirstSC() { return getTile(0)->getFirstSC(); }
  CapEntry* ITile::getFirstSC() { return getCore(0)->getFirstSC(); }
  CapEntry* ICore::getFirstSC() { return getThread(0)->getFirstSC(); }
  CapEntry* IThread::getFirstSC() { return getSC(); }

  void ISocket::wake() {
    for(size_t i = 0; i < getNumTiles(); i++){
      getTile(i)->wake();
    }
  }
  
  void ITile::wake() {
    for(size_t i = 0; i < getNumCores(); i++){
      getCore(i)->wake();
    }
  }

  void ICore::wake() {
    for(size_t i = 0; i < getNumThreads(); i++){
      getThread(i)->wake();
    }
  }

  void IThread::wake() {
    MLOG_INFO(mlog::pm, __func__, DVAR(getThreadID()));
    auto sce = getSC();
    TypedCap<SchedulingContext> sc(sce);
    ASSERT(sc);
    sc->wake();
  }

  SystemTopo systemTopo;
} //topology
} // namespace mythos
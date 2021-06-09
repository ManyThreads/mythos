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
#pragma once


namespace mythos {

namespace topology {
  struct ISocket;
  struct ITile;
  struct ICore;
  struct IThread;
  struct Resource;
}

  struct IProcessorPool {
    virtual void pushSocket(topology::ISocket* s); 
    virtual void pushTile(topology::ITile* t);
    virtual void pushCore(topology::ICore* c);
    virtual void pushThread(topology::IThread* t);
    virtual size_t numThreads();
    virtual topology::ISocket* popSocket();
    virtual topology::ITile* popTile();
    virtual topology::ICore* popCore(); 
    virtual topology::IThread* popThread();
    virtual topology::Resource* tryGetCoarseChunk();
    virtual topology::ICore* getCore();
    virtual topology::IThread* getThread();
  };

} // namespace mythos
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

#include "objects/IProcessorTopology.hh"
#include "cpu/hwthreadid.hh"

namespace mythos {
namespace topology {
  
  constexpr size_t THREADS_PER_CORE = 2;
  constexpr size_t CORES_PER_TILE = 2;
  constexpr size_t TILES_PER_SOCKET = 2;
  constexpr size_t SOCKETS = 2;

  constexpr size_t MAX_CORES = SOCKETS * TILES_PER_SOCKET * CORES_PER_TILE * THREADS_PER_CORE;
  constexpr size_t MAX_THREADS = MAX_CORES * THREADS_PER_CORE;

  struct SystemTopo 
    : public FixedSystem<SOCKETS, TILES_PER_SOCKET, CORES_PER_TILE, THREADS_PER_CORE>
  {
    void init(){
      cpu::ThreadID id = 0;
      for(size_t s = 0; s < SOCKETS; s++){
        for(size_t t = 0; t < TILES_PER_SOCKET; t++){
          for(size_t c = 0; c < CORES_PER_TILE; c++){
            for(size_t ht = 0; ht < THREADS_PER_CORE; ht++){
              sockets[s].tiles[t].cores[c].threads[ht].init(id);
              id++;
            }
          }
        }
      }
    }
  };

  extern SystemTopo systemTopo;
} //topology
} // namespace mythos

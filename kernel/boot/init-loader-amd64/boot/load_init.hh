/* -*- mode:C++; -*- */
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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/elf64.hh"
#include "util/Range.hh"
#include "util/align.hh"
#include "util/VectorMax.hh"
#include "objects/CapEntry.hh"
#include "objects/IPageMap.hh"
#include "objects/ExecutionContext.hh"
#include "util/events.hh"
#include "mythos/InfoFrame.hh"
#include "boot/MemMapper.hh"
#include "boot/CapAlloc.hh"

#include "boot/CapAlloc.hh"
#include "boot/MemMapper.hh"

namespace mythos {

  class KernelMemory;
  class CapMap;
  class Portal;
  class IFrame;

  namespace boot {

    class InitLoader
    {
    public:
      InitLoader(char* image);
      ~InitLoader();
      optional<void> load();

      optional<void> initCSpace();
      optional<void> createPortal(CapPtr infoFrame, CapPtr dstPortal);
      optional<uintptr_t> loadImage();
      optional<void> createEC(uintptr_t ipc_vaddr);
      optional<CapPtr> createInfoFrame(uintptr_t ipc_vaddr);

      optional<void> loadProgramHeader(
        const elf64::PHeader* ph, CapPtr frameCap, size_t offset);

      template<class Object, class Factory, class... ARGS>
      optional<Object*> create(optional<CapEntry*> dstEntry, ARGS const&...args);

      optional<void> csSet(CapPtr dst, CapEntry& root);
      optional<void> csSet(CapPtr dst, IKernelObject& obj);

      mythos::elf64::Elf64Image _img;

      KernelMemory* _mem;
      CapEntry* _memEntry;

      /** allocates capability entries in CAP_ALLOC_START..CAP_ALLOC_END */
      CapAlloc capAlloc;
      MemMapper memMapper;

      Portal* _portal;

      /* to be manipulated by optional processor allocator */
      bool mapSchedulingContexts;
    };

  } // namespace boot

  namespace event {
    extern Event<boot::InitLoader&> initLoader;
    extern Event<boot::InitLoader&> initLoaderEarly;
    extern Event<ExecutionContext*> initEC;
    extern Event<InfoFrame*> initInfoFrame;
  }

} // namespace mythos

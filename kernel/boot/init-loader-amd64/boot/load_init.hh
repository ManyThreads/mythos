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
#include "util/alignments.hh"
#include "objects/CapEntry.hh"
#include "objects/IPageMap.hh"
#include "plugins/events.hh"

namespace mythos {

  class KernelMemory;
  class CapMap;
  class Portal;
  class IFrame;

  namespace boot {


    class InitLoader {
    public:
      InitLoader(char* image);
      ~InitLoader();
      optional<void> load();

      typedef Align2M PageAlign;

      optional<void> initCSpace();
      optional<void> createDirectories();
      optional<void> createMemoryRegion();
      optional<void> createMsgFrame();
      optional<void> loadImage();
      optional<void> createEC();

      optional<void> load(const elf64::PHeader* header);

      template<class Object, class Factory, class... ARGS>
      optional<Object*> create(CapEntry* dstEntry, ARGS const&...args);

      optional<void> csSet(CapPtr dst, CapEntry& root);
      optional<void> csSet(CapPtr dst, IKernelObject& obj);

      mythos::elf64::Elf64Image _img;
      CapMap* _cspace;

      CapPtr _caps;
      PhysPtr<void> _regionStart;
      size_t _frames;
      size_t _maxFrames;
      KernelMemory* _mem;
      CapEntry* _memEntry;

      Portal* _portal;
      uintptr_t ipc_vaddr;

      typedef Range<uintptr_t> range_type;
      static range_type toPageRange(const elf64::PHeader* header);
      static range_type toFrameRange(const elf64::PHeader* header);

      size_t countPages();

      optional<void> mapDirectory(size_t target, size_t entry, size_t index);
      optional<size_t> mapFrame(size_t pageNum, bool writeable, bool executable);

      CapPtr peekCap();
      /// allocates a capability entry in CAP_ALLOC_START..CAP_ALLOC_END
      CapPtr allocCap();

      /// allocates a consecutive frame in the dynamic memory region
      size_t allocFrame();
    };

    HookRegistry<InitLoader> initLoaderEvent;

    /** sets up the initial application based on the embedded elf
     * image. Returns true if the loading was successful.
     */
    optional<void> load_init();

    /** create the root task EC on the first hardware thread */
    class InitLoaderPlugin
      : public Plugin
    {
    public:
      virtual ~InitLoaderPlugin() {}
      void initThread(size_t apicID) {
        if (apicID == cpu::enumerateHwThreadID(0))
          OOPS(mythos::boot::load_init()); // start the first application
      }
    };

  } // namespace boot
} // namespace mythos

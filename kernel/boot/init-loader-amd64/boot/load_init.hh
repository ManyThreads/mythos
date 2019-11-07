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
#include "util/VectorMax.hh"
#include "objects/CapEntry.hh"
#include "objects/IPageMap.hh"
#include "plugins/Plugin.hh"
#include "plugins/events.hh"
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

      typedef Align2M PageAlign;

      optional<void> initCSpace();
      optional<void> createPortal(uintptr_t ipc_vaddr, CapPtr dstPortal);
      optional<void> loadImage();
      optional<void> createEC(uintptr_t ipc_vaddr);

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
    };

    extern HookRegistry<InitLoader> initLoaderEvent;

    extern char app_image_start SYMBOL("app_image_start");

    /** create the root task EC on the first hardware thread */
    class InitLoaderPlugin
      : public Plugin
    {
    public:
      virtual ~InitLoaderPlugin() {}
      void initThread(cpu::ThreadID threadID) {
        if (threadID == 0) {
            OOPS(InitLoader(&app_image_start).load());
        }
      }
    };

  } // namespace boot
} // namespace mythos

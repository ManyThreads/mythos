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
 * Copyright 2017 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#include "plugin/Plugin.hh"
#include "util/PhysPtr.hh"
#include "util/MultiBoot.hh"

namespace mythos {

  class PluginDumpMultiboot
    : public Plugin
  {
  public:
    void initGlobal() override;
  };

  PluginDumpMultiboot plugin_dump_multiboot;

  extern uint64_t _mboot_magic SYMBOL("_mboot_magic");
  extern uint64_t _mboot_table SYMBOL("_mboot_table");

  void PluginDumpMultiboot::initGlobal()
  {
    auto mboot_magic = *physPtr(&_mboot_magic);
    auto mboot_table = *physPtr(&_mboot_table);

    MLOG_INFO(log, DVARhex(mboot_magic), DVARhex(mboot_table));

    //MultiBootInfo mbi(mboot_table);
    //MLOG_INFO(log, DVARhex(mbi->flags));

  }

} // namespace mythos


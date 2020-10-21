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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "runtime/cgaAttr.hh"
#include "runtime/cgaChar.hh"
#include "cpu/ctrlregs.hh"
#include "util/ISink.hh"
#include "util/TextMsg.hh"

namespace mythos {

class CgaScreen : public mlog::ISink {
private:   
  enum Ports {
    INDEX_PORT = 0x3D4,
    DATA_PORT  = 0x3D5
  };

  enum Cursor {
    HIGH = 14,
    LOW  = 15
  };

public:
  enum Video {
    VIDEO_RAM = 0xB8000
  };

  enum Screen {
    ROWS    = 25,
    COLUMNS = 80
  };

  CgaScreen(uintptr_t vaddr);

  CgaScreen();

  CgaScreen(CgaAttr attr);

  void clear();

  void scroll();

  void setAttr(CgaAttr& attr){
    this->attr = attr;
  }

  void getAttr(CgaAttr attr){
    attr = this->attr;
  }

  void setCursor(unsigned row, unsigned column);
  
  void getCursor(unsigned& row, unsigned& column);

  void show(char ch, const CgaAttr& attr);

  void show(char ch){
    show(ch, attr);
  }

  void write(char const* msg, size_t length) override;

  void flush() override {};

template<typename... ARGS>
  void log(ARGS&&... args){
    mlog::TextMsg<400> msg("log:", "", std::forward<ARGS>(args)...);
    write(msg.getData(), msg.getSize());
  }
protected:


  //void mapMemory();
  
  CgaAttr attr;
  IOPort8 index, data;
  CgaChar* screen;
  unsigned xpos, ypos;
};

} // namespace mythos

/* -*- mode:C++; -*- */
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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>

namespace mythos {

  /** simple interface to the UART8250 serial line.
   *
   * This implementation aims at the virtual serial port of qemu.
   * Based on http://wiki.osdev.org/Serial_ports
   */
  class SerialPort {
  public:
    SerialPort() {}
    SerialPort(uint16_t port) { setPort(port); }

    void setPort(uint16_t port) {
      this->port = port;
      setInterrupt(false);
      outb(uint16_t(port + 3), 0x03);    // 8 bits, no parity, one stop bit
    }

    void setInterrupt(bool active=true) {
      if (active) {
	// TODO
      } else {
	outb(uint16_t(port + 1), 0x00);
      }
    }

    bool isWritable() const { return (port != 0) && (inb(uint16_t(port + 5)) & 0x20); }
    bool isReadable() const { return (port != 0) && (inb(uint16_t(port + 5)) & 1); }

    void read(char& c) {
      c = readChar();
    }

    template<typename T>
    void read(T& v) { read(&v, sizeof(v)); }

    void read(void* p, size_t size) {
      char* c = reinterpret_cast<char*>(p);
      for (size_t i=0; i<size; i++) c[i] = readChar();
    }

    void write(char c) {
      writeChar(c);
    }

    template<typename T>
    void write(T const& v) { write(&v, sizeof(v)); }

    void write(void const* p, size_t size) {
      char const* c = reinterpret_cast<char const*>(p);
      for (size_t i=0; i<size; i++) writeChar(c[i]);
    }

    char readChar() {
      while (!isReadable());
      return inb(port);
    }

    void writeChar(char c) {
      while (!isWritable());
      outb(port, c);
    }

    void writeChars(char const* c, size_t len) {
      for (size_t i=0; i<len; i++) writeChar(c[i]);
    }

  protected:
    static char inb(uint16_t port) {
      char ret;
      asm volatile ("inb %%dx,%%al":"=a" (ret):"d" (port));
      return ret;
    }

    static void outb(uint16_t port, char value) {
      asm volatile ("outb %%al,%%dx": :"d" (port), "a" (value));
    }

  protected:
    uint16_t port;
  };

} // namespace mythos

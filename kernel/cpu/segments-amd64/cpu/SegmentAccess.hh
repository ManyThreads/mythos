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

#include "util/compiler.hh"
#include <cstdint>

namespace mythos {

  class SegmentDS
  {
  public:
    ALWAYS_INLINE static uint16_t getSelector() {
    	uint16_t seg;
    	asm("movw %%ds,%0" : "=rm" (seg));
    	return seg;
    }
    
    ALWAYS_INLINE static void loadSelector(uint16_t seg) {
      asm volatile("movw %0,%%ds" : : "rm" (seg));
    }
  };

  class SegmentFS
  {
  public:
    ALWAYS_INLINE static uint16_t getSelector() {
    	uint16_t seg;
    	asm("movw %%fs,%0" : "=rm" (seg));
    	return seg;
    }
    
    ALWAYS_INLINE static void loadSelector(uint16_t seg) {
      asm volatile("movw %0,%%fs" : : "rm" (seg));
    }

    ALWAYS_INLINE static uint8_t get8(uint8_t const* addr) {
      uint8_t value;
      asm("movb %%fs:%1,%0" : "=q" (value) : "m" (*addr));
      return value;
    }

    ALWAYS_INLINE static uint16_t get16(uint16_t const* addr) {
      uint16_t value;
      asm("movw %%fs:%1,%0" : "=r" (value) : "m" (*addr));
      return value;
    }
    
    ALWAYS_INLINE static uint32_t get32(uint32_t const* addr) {
      uint32_t value;
      asm("movl %%fs:%1,%0" : "=r" (value) : "m" (*addr));
      return value;
    }
    
    ALWAYS_INLINE static uint64_t get64(uint64_t const* addr) {
      uint64_t value;
      asm("movq %%fs:%1,%0" : "=r" (value) : "m" (*addr));
      return value;
    }
    
    ALWAYS_INLINE static void set8(uint8_t const* addr, uint8_t value) {
      asm("movb %1,%%fs:%0" :  : "m" (*addr), "qi" (value));
    }

    ALWAYS_INLINE static void set16(uint16_t const* addr, uint16_t value) {
      asm("movw %1,%%fs:%0" : : "m" (*addr) , "ri" (value));
    }
    
    ALWAYS_INLINE static void set32(uint32_t const* addr, uint32_t value) {
      asm("movl %1,%%fs:%0" : : "m" (*addr) , "ri" (value));
    }
    
    ALWAYS_INLINE static void set64(uint64_t const* addr, uint64_t value) {
      asm("movq %1,%%fs:%0" : : "m" (*addr), "ri" (value));
    }

    static void setWord(uint64_t* addr, uint64_t value) { set64(addr, value); }
    static uint64_t setWord(uint64_t* addr) { return get64(addr); }
  };


  class SegmentGS
  {
  public:
    ALWAYS_INLINE static uint16_t getSelector() {
    	uint16_t seg;
    	asm("movw %%gs,%0" : "=rm" (seg));
    	return seg;
    }
    
    ALWAYS_INLINE static void loadSelector(uint16_t seg) {
      asm volatile("movw %0,%%gs" : : "rm" (seg));
    }

    ALWAYS_INLINE static uint8_t get8(uint8_t const* addr) {
      uint8_t value;
      asm("movb %%gs:%1,%0" : "=q" (value) : "m" (*addr));
      return value;
    }

    ALWAYS_INLINE static uint16_t get16(uint16_t const* addr) {
    	uint16_t value;
    	asm("movw %%gs:%1,%0" : "=r" (value) : "m" (*addr));
    	return value;
    }
    
    ALWAYS_INLINE static uint32_t get32(uint32_t const* addr) {
      uint32_t value;
      asm("movl %%gs:%1,%0" : "=r" (value) : "m" (*addr));
      return value;
    }
    
    ALWAYS_INLINE static uint64_t get64(uint64_t const* addr) {
      uint64_t value;
      asm("movq %%gs:%1,%0" : "=r" (value) : "m" (*addr));
      return value;
    }

    ALWAYS_INLINE static void set8(uint8_t* addr, uint8_t value) {
      asm("movb %1,%%gs:%0" : : "m" (*addr), "qi" (value));
    }

    ALWAYS_INLINE static void set16(uint16_t* addr, uint16_t value) {
      asm("movw %1,%%gs:%0" : : "m" (*addr), "ri" (value));
    }
    
    ALWAYS_INLINE static void set32(uint32_t* addr, uint32_t value) {
      asm("movl %1,%%gs:%0" : : "m" (*addr), "ri" (value));
    }
    
    ALWAYS_INLINE static void set64(uint64_t* addr, uint64_t value) {
      asm("movq %1,%%gs:%0" : : "m" (*addr), "ri" (value));
    }

    static void setWord(uint64_t* addr, uint64_t value) { set64(addr, value); }
    static uint64_t getWord(uint64_t const* addr) { return get64(addr); }
  };

} // namespace mythos

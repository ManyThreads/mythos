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
 * Copyright 2019 Randolf Rotta and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstddef> // for size_t
#include <cstdint> // for uint8_t and so on

namespace mythos {

namespace internal {
    template<typename T, typename A>
    constexpr bool is_aligned(T v, A a) { return (v % a) == 0; }

    template<typename T, typename A>
    constexpr T round_down(T v, A a) { return T((v/a)*a); }

    template<typename T, typename A>
    constexpr T round_up(T v, A a) { return T(((v+a-1)/a)*a); }
} // namespace internal

constexpr bool is_aligned(uint8_t v, size_t a) { return internal::is_aligned(v,a); }
constexpr bool is_aligned(uint16_t v, size_t a) { return internal::is_aligned(v,a); }
constexpr bool is_aligned(uint32_t v, size_t a) { return internal::is_aligned(v,a); }
constexpr bool is_aligned(uint64_t v, size_t a) { return internal::is_aligned(v,a); }
template<typename T>
constexpr bool is_aligned(T* addr, size_t a) { return is_aligned(uintptr_t(addr), a); }

constexpr uint8_t round_down(uint8_t v, size_t a) { return internal::round_down(v,a); }
constexpr uint16_t round_down(uint16_t v, size_t a) { return internal::round_down(v,a); }
constexpr uint32_t round_down(uint32_t v, size_t a) { return internal::round_down(v,a); }
constexpr uint64_t round_down(uint64_t v, size_t a) { return internal::round_down(v,a); }
template<typename T>
constexpr T* round_down(T* addr, size_t a) { 
    return reinterpret_cast<T*>(round_down(uintptr_t(addr), a)); 
}

constexpr uint8_t round_up(uint8_t v, size_t a) { return internal::round_up(v,a); }
constexpr uint16_t round_up(uint16_t v, size_t a) { return internal::round_up(v,a); }
constexpr uint32_t round_up(uint32_t v, size_t a) { return internal::round_up(v,a); }
constexpr uint64_t round_up(uint64_t v, size_t a) { return internal::round_up(v,a); }
template<typename T>
constexpr T* round_up(T* addr, size_t a) { 
    return reinterpret_cast<T*>(round_up(uintptr_t(addr), a)); 
}

constexpr size_t alignByte = 1ull;
constexpr size_t alignWord = sizeof(size_t);
constexpr size_t alignLine = sizeof(size_t)*8;
constexpr size_t align4K = 1ull << 12;
constexpr size_t align2M = 1ull << 21;
constexpr size_t align4M = 1ull << 22;
constexpr size_t align1G = 1ull << 30;
constexpr size_t align512G = 1ull << 39;
    
} // namespace mythos

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
 * Copyright 2019 Randolf Rotta and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/VectorMax.hh"
#include "objects/CapMap.hh"
#include "mythos/caps.hh"
#include <cstdint>

namespace mythos {


class CapAlloc
{
public:
    CapAlloc(CapPtr begin, uint32_t capcount, CapMap* cspace)
        : begin(begin), count(capcount), cspace(cspace)
    {}

    optional<CapPtr> alloc() {
        if (count==0) THROW(Error::INSUFFICIENT_RESOURCES);
        count--;
        return begin++;
    }

    // TODO give access to the cspace
    optional<CapEntry*> get(CapPtr ptr) const {
        return cspace->get(ptr);
    }

protected:
    CapPtr begin;
    uint32_t count;
    CapMap* cspace;
};

} // namespace mythos

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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/error-trace.hh"

namespace mythos {

  /** helper type to produce unique type-ids during compile time without requiring RTTI */
  class TypeId
  {
  public:
    TypeId(void const* id) : _id(id) {}
    void const* debug() const { return _id; }
    bool operator==(TypeId const& other) { return _id == other._id; }
    bool operator!=(TypeId const& other) { return _id != other._id; }
  private:
    void const* _id;
  };

  template<class T>
  TypeId typeId() { 
    static char const foo{}; // uses uniqueness of static variables to create an identity
    return TypeId(&foo);
  }

  class ICastable
  {
  public:
    virtual ~ICastable() {}

    /** actual cast function that has to be implemented by the final object.
     *
     * @param id  identifies the target type of the requested cast.
     * @returns an untyped pointer to the correctly casted object or Error::TYPE_MISMATCH.
     */
    virtual optional<void const*> vcast(TypeId) const { THROW(Error::TYPE_MISMATCH); }

    template<class T>
    optional<T*> cast() {
      auto o = this->vcast(typeId<T>());
      if (o) return const_cast<T*>(reinterpret_cast<T const*>(*o));
      else RETHROW(o);
    }

    template<class T>
    optional<T const*> cast() const {
      auto o = this->vcast(typeId<T>());
      if (o) return reinterpret_cast<T const*>(*o);
      RETHROW(o);
    }
  };

} // namespace mythos

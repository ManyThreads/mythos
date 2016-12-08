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

#include "util/optional.hh"

namespace mythos {

  /** helper type to produce unique type-ids during compile time without requiring RTTI */
  class TypeId
  {
  public:
    template<class T> static TypeId id();
    void* debug() const { return (void*)_id; }
    bool operator==(TypeId const& other) { return _id == other._id; }
    bool operator!=(TypeId const& other) { return _id != other._id; }
  private:
    typedef TypeId (*FPtr)();
    TypeId(FPtr id) : _id(id) {}
    FPtr _id;
  };

  template<class T>
  TypeId TypeId::id() {
    return TypeId(&TypeId::id<T>); // uses uniqueness of function addresses to create an identity
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
    virtual optional<void const*> vcast(TypeId id) const;

    template<class T>
    optional<T*> cast() {
      auto o = this->vcast(TypeId::id<T>());
      if (o) return const_cast<T*>(reinterpret_cast<T const*>(*o));
      else return o.state();
    }

    template<class T>
    optional<T const*> cast() const {
      auto o = this->vcast(TypeId::id<T>());
      if (o) return reinterpret_cast<T const*>(*o);
      return o.state();
    }
  };

  inline optional<void const*> ICastable::vcast(TypeId) const {
    return Error::TYPE_MISMATCH;
  }
  
} // namespace mythos

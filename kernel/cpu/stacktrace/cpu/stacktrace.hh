#pragma once

#include <cstdint>
#include <cstddef>

namespace mythos {

  struct StackFrame {
    StackFrame* bp;
    void* ret;
  };

  class stack_iterator {
  public:

    typedef StackFrame* reg_t;
    typedef StackFrame value_type;
    typedef value_type& reference_type;

    stack_iterator(reg_t bp) : _bp(bp) {}

    stack_iterator& operator++()
    {
      if (_bp->ret) {
        // no return address: end of stack
        _bp = _bp->bp;
      } else {
        _bp = nullptr;
      }
      return *this;
    }

    stack_iterator operator++(int)
    {
      stack_iterator tmp(*this);
      ++tmp;
      return tmp;
    }

    reference_type operator*()
    {
      return *(_bp);
    }

    bool operator==(const stack_iterator& other) const
    {
      return other._bp == _bp;
    }
    bool operator!=(const stack_iterator& other) const { return !(*this == other); }

  private:
    reg_t _bp;
  };

  class StackTrace {
  public:

    typedef stack_iterator::reg_t reg_t;

    StackTrace() { asm volatile ("mov %%rbp, %0\n\t" : "=r" (_rbp)); }
    StackTrace(reg_t bp) : _rbp(bp) {} 
    StackTrace(void* bp) : _rbp(reinterpret_cast<reg_t>(bp)) {} 
    StackTrace(uintptr_t bp) : _rbp(reinterpret_cast<reg_t>(bp)) {} 

    StackTrace(const StackTrace&) = delete;

    stack_iterator begin() const
    {
      return stack_iterator(_rbp);
    }

    stack_iterator end() const
    {
      return stack_iterator(nullptr);
    }

  private:

    reg_t _rbp;
  };

}

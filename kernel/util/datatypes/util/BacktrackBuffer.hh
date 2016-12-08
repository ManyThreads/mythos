#pragma once

#include <cstddef>

namespace mythos {

template<class T, size_t CAPACITY>
class BacktrackBuffer
{
public:

  BacktrackBuffer() : _head(0), _tail(0), _overflow(false) {}

  bool empty() { return _head == _tail; }
  bool overflown() { return _overflow; }

  T& back() { return _buf[_tail]; }

  void push_back(const T& val)
  {
    _buf[inc(_tail)] = val;
    if (_head == _tail) {
      _overflow = true;
      inc(_head);
    }
  }

  T pop_back()
  {
    ASSERT(!empty());
    T result;
    dec(_tail);
  }

private:

  size_t& inc(size_t& index)
  {
    ++index;
    index %= CAPACITY;
    return index;
  }

  size_t& dec(size_t& index)
  {
    if (!index) {
      index = CAPACITY-1;
    } else {
      --index;
    }
    return index;
  }

  size_t _head;
  size_t _tail;
  bool _overflow;
  T _buf[CAPACITY];

};

}

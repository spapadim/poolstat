#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <cstddef>
#include <array>

// Very simplified circular buffer container implementation
template <typename T, std::size_t N>
class ring_buffer {
public:
  explicit ring_buffer() :
    _buf(), _first(0), _size(0)
  { }

  inline bool empty() const {
    return size() == 0;
  }
  inline std::size_t size() const {
    return _size;
  }

  // Behavior of ::front() is undefined if buffer is empty
  inline T& front() {
    return _buf[_first];
  }
  inline const T& front() const {
    return const_cast<const T&>(front());  // XXX - Is const_cast necessary?
  }

  // Behavior of ::front() is undefined if buffer is empty
  inline T& back() {
    return _buf[(_first + _size - 1) % N];
  }
  inline const T& back() const {
    return const_cast<const T&>(back());  // XXX - Is const_cast necessary?
  }

  void push(T&& val) {
    std::size_t i_val = (_first + _size) % N;  // Array index val will be assigned to
    // XXX - We have to explicitly call the destructor of overwritten array element
    //   (it's not dynamically allocated, and it's not going out of scope, so...)
    //   Otherwise, memory leaks may occur!  Aargh, C++ ...
    (_buf[i_val]).~T();

    _buf[i_val] = std::forward<T>(val);

    if (_size == N) {
      _first = (_first + 1) % N;  // Overwrote oldest element
    } else {
      _size++;  // Size grew
    }
  }

  // Behavior is undefined if buffer is empty
  // XXX - DANGER DANGER: Does *not* invoke destructor of popped object!
  //   That will be done when underlying array element is overwritten by another push!!
  void pop() {
    if (_size > 0) {
      _first = (_first + 1) % N;
      _size--;
    }
  }

  // Avoids temporary creation (which will be move-assigned, but still an overhead)
  // XXX - Just found out about "new placement"; is this too hacky/discouraged?
  template <class... Args> 
  void emplace(Args&&... args) {
    std::size_t i_val = (_first + _size) % N;  // Array index val will be assigned to

    (_buf[i_val]).~T();  // XXX - See comments in ::push()

    // Use "new placement" ...
    new (&(_buf[i_val])) T(std::forward<Args>(args)...);

    // XXX - Copy-pasted from ::push(); refactor these at some point 
    //  (but, seems it will have to be done via template tricks, so... eff it for now)
    if (_size == N) {
      _first = (_first + 1) % N;  // Overwrote oldest element
    } else {
      _size++;  // Size grew
    }
  }

  // no copy/move constructors, no swap() method; rarely used (esp. in MCU target)..

  // no operators (comparison etc); rarely used..

private:
  std::array<T, N> _buf;
  std::size_t _first;  // Index of next element that ::pop() should remove
  std::size_t _size;   // Number of elements in buffer (must be <= N)
  // Note: we do not use a _first/_last representation, so we can easily make full use of the array...
};

#endif  /* __RING_BUFFER_H__ */
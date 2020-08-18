#ifndef __SIGSLOT_H__
#define __SIGSLOT_H__

#include <vector>
#include <utility>
#include <functional>

// "Poor man's" signal/slot implementation
// Just a vector of std::function callbacks, basically (in an MCU)
// we don't need contexts, threads, etc etc...).
// Also, we do not need scoped connections (always single-threaded,
// cooperative tasks at most).
//
// Because we want to keep space complexity low, we avoid
// creating a separate connection type, and instead use an
// integer ID to identify connections. However, this ID
// has to stay the same, even if other connections are deleted.
// Again for space efficiency, we also avoid the use
// of std::list (overhead of two pointers per inserted value)
// or std::unordered_map (which is even worse, as it uses a
// linked list *per bucket*).  
// There are (at least) two possibilities.  In both of these,
// disconnections are no longer constant-time; this is acceptable,
// as they should be relatively rare.  
// The first possibilitu is a vector with <id, function> pairs 
// and a separate auto-incrementing counter for new ids.  Deletions 
// are handled via vector::erase() after a linear search.
// The second possibility is a vector with <bool valid, function>
// pairs where deletion is done by setting valid to false.  In this
// way, we can use vector indices as IDs.  This has additional downsides:
// (i) iteration (by ::emit()) wastes time by visiting "useless" elements,
// and (ii) it's possible to accidentally delete the wrong connection
// (if a stale ID is kept around long enough)
//
// Furthermore, only callbacks (aka "slots") with void return type are
// supported.  I do not understand why/where a return value is useful.  Furthermore, 
// it leads to various conundrums. Some of these can be resolved semi-arbitrarily 
// (e.g., if there are more than one slots connected, which return value should be returned? 
// could be last, could be a vector of all, but that's a lot of overhead for something 
// that will be rarely used).  Others, not so much (e.g., what should emit return 
// if no slots are connected yet? we could perhaps add some sort of optional<Ret> wrapper,
// but.. why??)
//
// Finally, we also support single-shot connections, i.e., connections
// that will automatically disconnect after the next emission of the
// signal.  We use the MSB of the connection ID as a flag, to save
// a byte of overhead (we expect a very small number of connections anyway).
//
// NB. Guaranteed-unique IDs are 7bits, so it's possible that we
// have silent failures after 127 slots are connected (specifically,
// disconnections will behave incorrectly).  We don't even bother to
// handle this, as we should never have more than a handful of 
// connections anyway (and, typically, just one).

namespace ch {

template<typename... Args>
class signal {
public:
  using argument_t = std::tuple<Args...>;

  using slot_t = std::function<void(Args...)>;

  // XXX - We should make internals opaque, eventually..?
  struct connection_t {
    bool one_shot : 1;
    uint8_t id : 7;
  };

  signal() :
    _slots(), _nextConnectionId(0)
  { }
  ~signal() { }

  inline std::size_t size() const {
    return _slots.size();
  }

  // XXX - May silently fail after 127 connections...
  connection_t connect(const slot_t&& slot, bool one_shot = false) {
    connection_t conn = { one_shot, _nextConnectionId };
    _slots.emplace_back(conn, slot);
    _nextConnectionId++;
    return conn;
  }

  bool disconnect(connection_t conn) {
    for (auto it = _slots.begin();  it != _slots.end();  it++) {
      // Note it's ok to compare entire connection_t, nust just ID field --
      //   should eliminate need for bitwise masking.
      // XXX - Hopefully, compiler does indeed do this in one instruction?
      //   WTH, can't be that dumb, but check at some point anyways...
      //if ((uint8_t)(it->first) == (uint8_t)conn) {
      if (it->first.id == conn.id) {   // TODO - FIXME!!!
        _slots.erase(it);
        return true;
      }
    }
    return false;
  }

  // XXX - Blargh.. see, e.g.:
  //   https://stackoverflow.com/questions/29638627/
  // Makes sense, in retrospect, as class template args are bound based on datatype,
  // and we want ::emit to work with any combination of reference types, but... ugh!
  template <typename... EArgs>
  void emit(EArgs&&... args) {
    for (auto it = _slots.begin(); it != _slots.end();) {
      if (std::next(it) == _slots.end()) {  // Last callback, forward args
        it->second(std::forward<EArgs>(args)...);
      } else {
        it->second(args...);  // Args will be needed for other callbacks
      }
      // Handle single-shot connections
      if (it->first.one_shot) {
        it = _slots.erase(it);
      } else {
        it++;
      }
    }
  }
  template <typename... OpArgs>
  inline void operator() (OpArgs&&... args) {
    return emit(std::forward<OpArgs>(args)...);
  }

private:
  std::vector<std::pair<connection_t, slot_t> > _slots;
  uint8_t _nextConnectionId;
};


}  // namespace ch

#endif  /* __SIGSLOT_H__ */
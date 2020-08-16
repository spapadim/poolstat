#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__

#include <Arduino.h>
#include <unordered_set>

// Auxiliary static factory constructor to efficiently initialize a String
//   from the concatenation of one or more C-style strings
#if __cplusplus >= 201703L   // C++17
template<typename... Ts>
inline String concat_into(const char* first, Ts... rest) {
  int total_len = (strlen(first) + ... + strlen(rest));
  String s((const char *)nullptr);  // nullptr so buffer is not allocated until .reserve()
  s.reserve(total_len);
  s += first;
  (s += ... += rest);
  return s;
}
#else  // if C++17
inline int __total_strlen(const char* first) {
  return strlen(first);
}
template<typename... Ts>
inline int __total_strlen(const char* first, Ts... rest) {
  return strlen(first) + __total_strlen(rest...);
}
inline void __concat_into_tailrec(String& s, const char* first) {
  s += first;
}
template<typename... Ts>
inline void __concat_into_tailrec(String& s, const char* first, Ts... rest) {
  s += first;
  __concat_into_tailrec(s, rest...);
}
template<typename... Ts>
inline String concat_into(const char* first, Ts... rest) {
  int total_len = __total_strlen(first, rest...);
  String s((const char *)nullptr);  // nullptr so buffer is not allocated until .reserve()
  s.reserve(total_len);
  __concat_into_tailrec(s, first, rest...);
  return s;
}
#endif  // C++17

// Define specialization of std::hash<T> for Arduino String type
// Uses "djb2" from https://stackoverflow.com/a/7666577/13321349
namespace std {
  template<> struct hash<String> {
    std::size_t operator()(String const& s) const noexcept {
      std::size_t hash = 5381;
      for (const char* c = s.begin();  c != s.end();  c++) {
        hash = ((hash << 5) + hash) + (*c);  /* hash * 33 + *c */
      }
      return hash;
    }
  };
}

#endif  /* __STRING_UTIL_H__ */
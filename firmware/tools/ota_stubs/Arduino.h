#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <algorithm>
#define PROGMEM
using std::min;

class String : public std::string {
 public:
  using std::string::string;
  String(const std::string &source) : std::string(source) {}
};

struct MockEsp {
  uint32_t flashSize = 16u * 1024u * 1024u;
  uint32_t freePsram = 8u * 1024u * 1024u;
  uint32_t getFlashChipSize() const { return flashSize; }
  uint32_t getFreePsram() const { return freePsram; }
};
extern MockEsp ESP;

struct MockSerial {
  template <class... Args> int printf(const char *fmt, Args... args) {
    return std::printf(fmt, args...);
  }
  void println(const char *message) { std::printf("%s\n", message); }
};
extern MockSerial Serial;

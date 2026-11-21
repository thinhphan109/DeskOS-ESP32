#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

// Allocation-free SemVer and flash coordination, usable in native host tests.
namespace MimiUpdate {
inline bool digit(char c) {return c>='0' && c<='9';}
struct Version { uint32_t n[3]{}; char pre[32]{}; };
inline bool identifier(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '-';
}
inline bool identifiers(const char *s, bool numericRules) {
  if (!s || !*s) return false;
  while (*s) {
    const char *begin = s; bool numeric = true;
    while (*s && *s != '.') { if (!identifier(*s)) return false; if (!digit(*s)) numeric = false; ++s; }
    if (s == begin || (numericRules && numeric && s-begin > 1 && *begin == '0')) return false;
    if (*s == '.' && !*++s) return false;
  }
  return true;
}
inline bool parseVersion(const char *s, Version &out) {
  out = Version{};
  if (!s || strlen(s) > 63) return false;
  for (unsigned i=0; i<3; ++i) {
    if (!digit(*s) || (*s == '0' && digit(s[1]))) return false;
    uint64_t value = 0;
    while (digit(*s)) { value = value*10 + (*s++-'0'); if (value > UINT32_MAX) return false; }
    out.n[i] = static_cast<uint32_t>(value);
    if (i < 2) {if(*s!='.')return false;++s;}
  }
  if (*s == '-') {
    const char *begin = ++s;
    while (*s && *s != '+') ++s;
    const size_t length = s-begin;
    if (!length || length >= sizeof(out.pre)) return false;
    memcpy(out.pre, begin, length); out.pre[length] = 0;
    if (!identifiers(out.pre, true)) return false;
  }
  if (*s == '+') return identifiers(s+1, false);
  return *s == 0;
}
inline int comparePre(const char *a, const char *b) {
  if (!*a || !*b) return !*a ? (!*b ? 0 : 1) : -1;
  while (*a && *b) {
    const char *ea = a; const char *eb = b; bool na = true, nb = true;
    while (*ea && *ea != '.') { if (!digit(*ea)) na = false; ++ea; }
    while (*eb && *eb != '.') { if (!digit(*eb)) nb = false; ++eb; }
    if (na != nb) return na ? -1 : 1;
    const size_t la=ea-a, lb=eb-b;
    if (na && la != lb) return la < lb ? -1 : 1;
    int cmp = strncmp(a,b,la < lb ? la : lb);
    if (cmp) return cmp < 0 ? -1 : 1;
    if (la != lb) return la < lb ? -1 : 1;
    a = *ea ? ea+1 : ea; b = *eb ? eb+1 : eb;
  }
  return *a ? 1 : (*b ? -1 : 0);
}
inline bool compareVersions(const char *a, const char *b, int &result) {
  Version av, bv; if (!parseVersion(a,av) || !parseVersion(b,bv)) return false;
  for (unsigned i=0;i<3;++i) if (av.n[i] != bv.n[i]) {
    result = av.n[i] < bv.n[i] ? -1 : 1; return true;
  }
  result = comparePre(av.pre,bv.pre); return true;
}
enum class Owner : uint8_t { None, Firmware, CatPack };
class Coordinator {
 public:
  bool acquire(Owner requested) {
    if (requested == Owner::None || owner_ != Owner::None) return false;
    owner_ = requested; return true;
  }
  void release(Owner requested) { if (owner_ == requested) owner_ = Owner::None; }
  Owner owner() const { return owner_; }
 private: Owner owner_ = Owner::None;
};
inline Coordinator &coordinator() { static Coordinator value; return value; }
// Copy the latest manifest into one pending slot. Never mutate a pinned active image.
inline bool newerSequence(uint32_t incoming, uint32_t current) { return incoming > current; }
}

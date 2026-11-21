#include "../src/core/utf8_text.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main() {
  assert(validMediaText(u8"Đừng làm trái tim anh đau · Đỗ Mỹ Linh"));
  assert(validMediaText(u8"Ắ Ằ Ẳ Ẵ Ặ Ấ Ầ Ẩ Ẫ Ậ Ế Ề Ể Ễ Ệ Ớ Ờ Ở Ỡ Ợ Ứ Ừ Ử Ữ Ự"));
  assert(!validMediaText("bad\ncaption"));
  assert(!validMediaText("\xc0\xaf")); // Overlong slash.
  assert(!validMediaText("\xed\xa0\x80")); // Surrogate.
  assert(!validMediaText("\xf4\x90\x80\x80")); // Above Unicode max.
  assert(!validMediaText("\xe1\xbb")); // Truncated Vietnamese code point.
  assert(!validMediaText("\x80"));
  assert(!validMediaText("\xc2\x85")); // C1 newline.
  const char *text=u8"ĐữộA"; uint32_t code=0;
  assert(nextUtf8(text,code)&&code==0x110);
  assert(nextUtf8(text,code)&&code==0x1eef);
  assert(nextUtf8(text,code)&&code==0x1ed9);
  assert(nextUtf8(text,code)&&code=='A');
  assert(!*text);
  // Decoder must always advance even on corrupt input (bounded fallback).
  for(unsigned i=1;i<256;++i) {
    char invalid[]={static_cast<char>(i),0}; const char *cursor=invalid;
    nextUtf8(cursor,code); assert(cursor>invalid && cursor<=invalid+1);
  }
  std::cout<<"OK: music UTF-8 scalar validation and Vietnamese clipping boundaries\n";
}

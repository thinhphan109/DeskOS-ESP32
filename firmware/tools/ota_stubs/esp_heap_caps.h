#pragma once
#include <cstddef>
#include <cstdlib>
constexpr unsigned MALLOC_CAP_SPIRAM = 1;
constexpr unsigned MALLOC_CAP_8BIT = 2;
inline void *heap_caps_malloc(size_t bytes, unsigned) {return std::malloc(bytes);}
inline void heap_caps_free(void *pointer) {std::free(pointer);}

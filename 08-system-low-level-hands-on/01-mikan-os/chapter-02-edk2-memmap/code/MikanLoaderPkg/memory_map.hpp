#pragma once

#include <stdint.h>

// 与 kernel/memory_map.hpp 同源 — Ch2 Loader 本地副本，避免 ../kernel 相对路径
struct MemoryMap {
  unsigned long long buffer_size;
  void*              buffer;
  unsigned long long map_size;
  unsigned long long map_key;
  unsigned long long descriptor_size;
  uint32_t           descriptor_version;
};

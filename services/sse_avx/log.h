#pragma once
#include <stdint.h>

void Log(const char* formatStr, ...);
char* GetPage(uint32_t pageIdx, uint32_t& pageSize);
char* GetFullLog(uint32_t& size);

#pragma once
#include <stdint.h>

static const uint32_t kWaveLength = 8;

void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint64_t s0qInitialValue);
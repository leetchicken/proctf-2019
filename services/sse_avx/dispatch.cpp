#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <algorithm>
#include <time.h>
#include "dispatch.h"


void Kernel(void* helpMask, uint32_t execInitialValue, uint64_t s0qInitialValue, __m256i groupThreadIdX, __m256i groupThreadIdY);


__m256i GHelpMask;
__m256i GThreadIdXInitial = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
__m256i GThreadIdXInc = _mm256_set1_epi32(kWaveLength);
__m256i GThreadIdYInc = _mm256_set1_epi32(1);


void InitHelpMask()
{
	uint64_t* helpMask = (uint64_t*)&GHelpMask;
	helpMask[0] = 0x0000000200000001;
	helpMask[1] = 0x0000000800000004;
	helpMask[2] = 0x0000002000000010;
	helpMask[3] = 0x0000008000000040;
}


uint64_t GetTime()
{
    timespec tp;
    memset(&tp, 0, sizeof(tp));
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec + tp.tv_nsec;
}


uint64_t Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint64_t s0qInitialValue)
{
	/*unsigned junk;
	unsigned long long time1 = __rdtscp(&junk);*/
	uint64_t time1 = GetTime();

    InitHelpMask();

	uint32_t groupDimX = 8;
	uint32_t groupDimY = 1;
	uint32_t wavesPerX = (groupDimX + kWaveLength - 1) / kWaveLength;

	__m256i threadIdX, offsetX, threadIdY;
	for(uint32_t ty = 0; ty < groupCountY; ty++)
	{
		threadIdX = GThreadIdXInitial;
		threadIdY = _mm256_set1_epi32(ty);
		for(uint32_t tx = 0; tx < groupCountX; tx++)
		{
			uint32_t activeLanes = std::min(groupDimX * groupCountX - tx * kWaveLength, kWaveLength);
			uint32_t activeLanesMask = (1 << activeLanes) - 1;
			Kernel(&GHelpMask, activeLanesMask, s0qInitialValue, threadIdX, threadIdY);
			threadIdX = _mm256_add_epi32(threadIdX, GThreadIdXInc);
		}
	}

	/*unsigned long long time2 = __rdtscp(&junk);
	printf("%llu\n", time2 - time1);*/
	uint64_t time2 = GetTime();
	//printf("%llu\n", time2 - time1);
	return time2 - time1;
}
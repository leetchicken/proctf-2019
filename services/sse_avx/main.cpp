#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <algorithm>

typedef void* (*TKernel)(void* helpMask, uint32_t execInitialValue, uint64_t s0qInitialValue, __m256i groupThreadIdX, __m256i groupThreadIdY);


struct Kernel
{
	TKernel code = nullptr;
	uint32_t groupDimX = 0;
	uint32_t groupDimY = 0;
};


static const uint32_t kWaveLength = 8;


void* ReadFile(const char* fileName, uint32_t& size)
{
    FILE* f = fopen(fileName, "r");
    if (!f)
    {
        size = 0;
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* fileData = malloc(size);
    fread(fileData, 1, size, f);
    fclose(f);

    return fileData;
}


Kernel LoadKernel(const char* filename)
{
	Kernel kernel;

	uint32_t fileSize;
	void* file = ReadFile(filename, fileSize);
	if(!file)
		return kernel;

	uint8_t* ptr = (uint8_t*)file;
	memcpy(&kernel.groupDimX, ptr, sizeof(uint32_t));
	ptr += sizeof(uint32_t);
	memcpy(&kernel.groupDimY, ptr, sizeof(uint32_t));
	ptr += sizeof(uint32_t);
	uint32_t codeSize = fileSize - 2 * sizeof(uint32_t);

#if DEBUG
	const uint32_t kPageSize = 4096;
	void* kernelCode = mmap(0, 2 * kPageSize, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	kernelCode = (void*)((uint8_t*)kernelCode + kPageSize);
	memcpy(kernelCode, ptr, codeSize);
#else
	void* kernelCode = mmap(0, codeSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	memcpy(kernelCode, ptr, codeSize);
	mprotect(kernelCode, codeSize, PROT_READ|PROT_EXEC);
#endif
	free(file);

	kernel.code = (TKernel)kernelCode;
	return kernel;
}


struct float2x2
{
	float _00; float _01;
	float _10; float _11;

	float2x2 operator*(const float2x2& b)
	{
		float2x2 ret;
		ret._00 = _00 * b._00 + _01 * b._10;
		ret._01 = _00 * b._01 + _01 * b._11;

		ret._10 = _10 * b._00 + _11 * b._10;
		ret._11 = _10 * b._01 + _11 * b._11;
		return ret;
	}
};


int main(int argc, char* argv[])
{
	if(argc != 4)
	{
		printf("./me <path to kernel> <group count X> <group count Y>\n");
		return -1;
	}

	const char* kernelFilename = argv[1];
	uint32_t groupCountX = strtoul(argv[2], nullptr, 10);
	uint32_t groupCountY = strtoul(argv[3], nullptr, 10);

	Kernel kernel = LoadKernel(kernelFilename);
	if(!kernel.code)
	{
		printf("Failed to load kernel\n");
		return -1;
	}

	unsigned junk;
	unsigned long long time1 = __rdtscp(&junk);

	uint64_t* helpMask = (uint64_t*)aligned_alloc(32, kWaveLength * sizeof(uint32_t));
	helpMask[0] = 0x0000000200000001;
	helpMask[1] = 0x0000000800000004;
	helpMask[2] = 0x0000002000000010;
	helpMask[3] = 0x0000008000000040;

	uint32_t groupsNum = groupCountX * groupCountY;
	uint32_t oneGroupSize = sizeof(float) * 12 * kWaveLength;

	auto* matrices = (float*)aligned_alloc(32, oneGroupSize * groupsNum);
	memset(matrices, 0xdc, oneGroupSize * groupsNum);

	decltype(matrices) ptr = matrices;
	uint32_t counter = 0;
	for(uint32_t g = 0; g < groupsNum; g++)
	{
		for(uint32_t i = 0; i < kWaveLength; i++)
		{
			for(uint32_t j = 0; j < 8; j++)
				ptr[j] = (float)(counter++);
			ptr += 12;
		}
	}

	__m256i groupThreadIDXInit, groupThreadIDXInc;
	for(uint32_t i = 0; i < kWaveLength; i++)
	{
		uint32_t* ptr = (uint32_t*)&groupThreadIDXInit;
		ptr[i] = i;
	}
	groupThreadIDXInc = _mm256_set1_epi32(kWaveLength);

	__m256i groupThreadIDX;
	uint32_t wavesPerX = (kernel.groupDimX + kWaveLength - 1) / kWaveLength;
	for(uint32_t y = 0; y < kernel.groupDimY * groupCountY; y++)
	{
		groupThreadIDX = groupThreadIDXInit;
		__m256i groupThreadIdY = _mm256_set1_epi32(y);
		for(uint32_t x = 0; x < wavesPerX * groupCountX; x++)
		{
			uint32_t activeLanes = std::min(kernel.groupDimX * groupCountX - x * kWaveLength, kWaveLength);
			uint32_t activeLanesMask = (1 << activeLanes) - 1;
			uint64_t s0q = (uint64_t)matrices;
			kernel.code(helpMask, activeLanesMask, s0q, groupThreadIDX, groupThreadIdY);
			groupThreadIDX = _mm256_add_epi32(groupThreadIDX, groupThreadIDXInc);
		}
	}

	unsigned long long time2 = __rdtscp(&junk);
	printf("%llu\n", time2 - time1);

	uint32_t failedCount = 0;
	float2x2* ptr0 = (float2x2*)matrices;
	for(uint32_t g = 0; g < groupsNum; g++)
	{
		for (uint32_t i = 0; i < kWaveLength; i++)
		{
			float2x2& a = ptr0[0];
			float2x2& b = ptr0[1];
			float2x2& c = ptr0[2];
			float2x2 cExp = a * b;
			if (memcmp(&c, &cExp, sizeof(float2x2)) != 0)
				failedCount++;
			ptr0 += 3;
		}
	}

	printf("Num failed: %u\n", failedCount);

    return 0;
}

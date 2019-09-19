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


static const uint32_t kWaveLength = 8;
uint64_t* GHelpMask = nullptr;


void InitHelpMask()
{
	GHelpMask = (uint64_t*)aligned_alloc(32, kWaveLength * sizeof(uint32_t));
	GHelpMask[0] = 0x0000000200000001;
	GHelpMask[1] = 0x0000000800000004;
	GHelpMask[2] = 0x0000002000000010;
	GHelpMask[3] = 0x0000008000000040;
}


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


void Dispatch(const Kernel& kernel, uint32_t groupCountX, uint32_t groupCountY, uint64_t s0qInitialValue)
{
	unsigned junk;
	unsigned long long time1 = __rdtscp(&junk);

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
			kernel.code(GHelpMask, activeLanesMask, s0qInitialValue, groupThreadIDX, groupThreadIdY);
			groupThreadIDX = _mm256_add_epi32(groupThreadIDX, groupThreadIDXInc);
		}
	}

	unsigned long long time2 = __rdtscp(&junk);
	printf("%llu\n", time2 - time1);
}


void MatrixTest(uint32_t groupCountX)
{
	printf("Matrix test\n");
	Kernel kernel = LoadKernel("vec_mul.bin");
	if(!kernel.code)
	{
		printf("Failed to load kernel\n");
		return;
	}

	uint32_t groupCountY = 1;

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

	Dispatch(kernel, groupCountX, groupCountY, (uint64_t)matrices);

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
	free(matrices);
}


void SaveRestoreTest(uint32_t groupCountY)
{
	printf("Save-restore test\n");
	Kernel kernel = LoadKernel("save_restore.bin");
	if(!kernel.code)
	{
		printf("Failed to load kernel\n");
		return;
	}

	uint32_t oneGroupSize = sizeof(uint32_t) * kWaveLength;
	uint32_t* storage = (uint32_t*)aligned_alloc(32, oneGroupSize * groupCountY);
	memset(storage, 0xcd, oneGroupSize * groupCountY);

	Dispatch(kernel, 1, groupCountY, (uint64_t)storage);

	const uint32_t ref[8] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
	uint32_t failedCount = 0;
	for(uint32_t gi = 0; gi < groupCountY; gi++)
	{
		uint32_t* ptr = storage + gi * kWaveLength;
		if(memcmp(ptr, ref, oneGroupSize) !=0)
			failedCount++;
	}

	printf("Num failed: %u\n", failedCount);
	free(storage);
}


void TransitionTest(uint32_t groupCountY)
{
	printf("Transition test\n");
	Kernel kernel = LoadKernel("transition.bin");
	if(!kernel.code)
	{
		printf("Failed to load kernel\n");
		return;
	}

	uint32_t oneGroupSize = sizeof(float) * kWaveLength;
	auto* storage = (float*)aligned_alloc(32, oneGroupSize * (groupCountY + 1));
	storage[0] = storage[1] = storage[2] = storage[3] = -1.0f;
	storage[4] = storage[5] = storage[6] = storage[7] = 1.0f;
	memset(storage + kWaveLength, 0xdc, oneGroupSize * groupCountY);

	Dispatch(kernel, 1, groupCountY, (uint64_t)storage);

	const float ref[8] = {-2.0f, -2.0f, -2.0f, -2.0f, 2.0f, 2.0f, 2.0f, 2.0f};
	uint32_t failedCount = 0;
	for(uint32_t gi = 0; gi < groupCountY; gi++)
	{
		float* ptr = storage + (gi + 1) * kWaveLength;
		if(memcmp(ptr, ref, oneGroupSize) !=0)
			failedCount++;
	}

	printf("Num failed: %u\n", failedCount);
	free(storage);
}


int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("./me <group count X> <group count Y>\n");
		return -1;
	}

	InitHelpMask();

	uint32_t groupCountX = strtoul(argv[1], nullptr, 10);
	uint32_t groupCountY = strtoul(argv[2], nullptr, 10);

	_mm256_zeroupper();
	MatrixTest(groupCountX);
	_mm256_zeroupper();
	SaveRestoreTest(groupCountY);
	_mm256_zeroupper();
	TransitionTest(groupCountY);	

    return 0;
}

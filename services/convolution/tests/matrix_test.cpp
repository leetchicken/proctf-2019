#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../dispatch.h"


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


void MatrixTest(uint32_t groupCountX)
{
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

	Dispatch(groupCountX, groupCountY, (uint64_t)matrices);

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


int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("./me <group count X> <group count Y>\n");
		return -1;
	}

	uint32_t groupCountX = strtoul(argv[1], nullptr, 10);
	uint32_t groupCountY = strtoul(argv[2], nullptr, 10);

	MatrixTest(groupCountX);	

    return 0;
}

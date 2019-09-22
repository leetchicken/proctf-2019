#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dispatch.h"


void TransitionTest(uint32_t groupCountY)
{
	uint32_t oneGroupSize = sizeof(float) * kWaveLength;
	auto* storage = (float*)aligned_alloc(32, oneGroupSize * (groupCountY + 1));
	storage[0] = storage[1] = storage[2] = storage[3] = -1.0f;
	storage[4] = storage[5] = storage[6] = storage[7] = 1.0f;
	memset(storage + kWaveLength, 0xdc, oneGroupSize * groupCountY);

	Dispatch(1, groupCountY, (uint64_t)storage);

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

	uint32_t groupCountX = strtoul(argv[1], nullptr, 10);
	uint32_t groupCountY = strtoul(argv[2], nullptr, 10);

	TransitionTest(groupCountY);	

    return 0;
}

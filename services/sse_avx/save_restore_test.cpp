#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dispatch.h"


void SaveRestoreTest(uint32_t groupCountY)
{
	uint32_t oneGroupSize = sizeof(uint32_t) * kWaveLength;
	uint32_t* storage = (uint32_t*)aligned_alloc(32, oneGroupSize * groupCountY);
	memset(storage, 0xcd, oneGroupSize * groupCountY);

	Dispatch(1, groupCountY, (uint64_t)storage);

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


int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("./me <group count X> <group count Y>\n");
		return -1;
	}

	uint32_t groupCountX = strtoul(argv[1], nullptr, 10);
	uint32_t groupCountY = strtoul(argv[2], nullptr, 10);

	SaveRestoreTest(groupCountY);	

    return 0;
}

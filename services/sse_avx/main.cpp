#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <x86intrin.h>


typedef void* (*TKernel)(void* sgrp, void* vgpr);


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


int main(int argc, char* argv[])
{
    uint32_t fileSize;
    void* file = ReadFile(argv[1], fileSize);
    if(!file)
        return -1;

#if DEBUG
	const uint32_t kPageSize = 4096;
	void* kernelCode = mmap(0, 2 * kPageSize, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	kernelCode = (void*)((uint8_t*)kernelCode + kPageSize);
	memcpy(kernelCode, file, fileSize);
#else
	void* kernelCode = mmap(0, fileSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	memcpy(kernelCode, file, fileSize);
	mprotect(kernelCode, fileSize, PROT_READ|PROT_EXEC);
#endif

	unsigned junk;
	unsigned long long time1 = __rdtscp(&junk);

	TKernel kernel = (TKernel)kernelCode;
    void* vgpr = &fileSize;
	void* sgpr = argv[1];
	// rdi, rsi
	//for (unsigned j = 0; j < 1000000; ++j)
		kernel(sgpr, vgpr);

	unsigned long long time2 = __rdtscp(&junk);
	printf("%llu\n", time2 - time1);

    return 0;
}

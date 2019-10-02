#include "kernels.h"
#include <mutex>
#include <map>
#include <stdio.h>
#include <string.h>
#include "log.h"

static std::mutex GMutex;
static std::map<std::string, std::string> GConvolutionKernels;
static std::vector<std::string> GGConvolutionKernelsId;
static FILE* GDatabase = nullptr;
static uint32_t GDatabaseOffset = 0;


static void LoadDatabase()
{
    const char* kFilename = "kernels.dat";
    bool error = false;
    GDatabase = fopen(kFilename, "r+");
	if(GDatabase)
	{
		fseek(GDatabase, 0, SEEK_END);
		size_t fileSize = ftell(GDatabase);
		fseek(GDatabase, 0, SEEK_SET); 

        char* data = (char*)malloc(fileSize);
        char* ptrToFree = data;
        if(fread(data, 1, fileSize, GDatabase) != fileSize)
        {
            error = true;
            Log("  Failed to read kernels database\n");
        }
        else
        {
            uint32_t remain = fileSize;
            while(remain)
            {
                uint32_t idLen = strlen(data);
                char* id = data;
                char* kernel = data + idLen + 1;
                GConvolutionKernels[id] = kernel;
                GGConvolutionKernelsId.push_back(id);
                uint32_t read = idLen + kConvolutionKernelSize + 2;
                data += read;
                remain -= read;
            }

            GDatabaseOffset = fileSize;
            Log("Kernels database has been read succefully\n"); 
        }

        free(ptrToFree);
    }
    else
    {
        Log("Kernels database does not exists\n");
        error = true;
    } 

    if(error)
	{
        if(GDatabase)
            fclose(GDatabase);
		FILE* c = fopen(kFilename, "w");
		fclose(c);
		GDatabase = fopen(kFilename, "r+");
        GDatabaseOffset = 0;
	} 
}


void InitKernels()
{
    std::lock_guard<std::mutex> guard(GMutex); 
    LoadDatabase();
}


EKernelsError AddConvolutionKernel(const char* id, const char* kernel)
{
    if(strlen(id) == 0 || strlen(id) > kConvolutionKernelIdMaxSize)
        return kKernelInvalidId;

    if(strlen(kernel) != kConvolutionKernelSize)
        return kKernelInvalidLength;

	std::lock_guard<std::mutex> guard(GMutex); 
	if(GConvolutionKernels.find(id) != GConvolutionKernels.end())
        return kKernelAlreadyExists;

    GConvolutionKernels[id] = kernel;
    GGConvolutionKernelsId.push_back(id);
    
    fseek(GDatabase, GDatabaseOffset, SEEK_SET);
    fwrite(id, strlen(id) + 1, 1, GDatabase);
    fwrite(kernel, kConvolutionKernelSize + 1, 1, GDatabase);
    fflush(GDatabase); 
    GDatabaseOffset += strlen(id) + kConvolutionKernelSize + 2;

    return kKernelOk;
}


EKernelsError GetConvolutionKernel(const std::string& id, std::string& kernel)
{
    if(id.length() == 0)
        return kKernelInvalidId;

    std::lock_guard<std::mutex> guard(GMutex); 
    auto iter = GConvolutionKernels.find(id);
	if(iter == GConvolutionKernels.end())
        return kKernelDoesNotExists;

    kernel = iter->second;
    return kKernelOk;    
}


uint32_t GetConvolutionKernelsIdList(std::vector<std::string>& ids)
{
    std::lock_guard<std::mutex> guard(GMutex);
    ids = GGConvolutionKernelsId;
    return 0;
}
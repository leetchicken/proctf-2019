#pragma once
#include <stdint.h>
#include <string>
#include <vector>

static const uint32_t kConvolutionKernelIdMaxSize = 32;
static const uint32_t kConvolutionKernelSize = 32;

enum EKernelsError
{
    kKernelInvalidId = 0,
    kKernelInvalidLength,
    kKernelAlreadyExists,
    kKernelDoesNotExists,
    kKernelOk
};

void InitKernels();
EKernelsError AddConvolutionKernel(const char* id, const char* kernel);
EKernelsError GetConvolutionKernel(const std::string& id, std::string& kernel);
uint32_t GetConvolutionKernelsIdList(std::vector<std::string>& ids);
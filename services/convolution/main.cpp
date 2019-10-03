#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <map>
#include "log.h"
#include "httpserver.h"
#include "png.h"
#include "misc.h"
#include "dispatch.h"
#include "kernels.h"
#include "signature_verifier.h"
#include <x86intrin.h>


const uint32_t kChannelsNum = 4;


class RequestHandler : public HttpRequestHandler
{
public:
	RequestHandler() = default;
	virtual ~RequestHandler() = default;

	HttpResponse HandleGet(HttpRequest request);
	HttpResponse HandlePost(HttpRequest request, HttpPostProcessor** postProcessor);

private:
	HttpResponse GetKernelsList(HttpRequest request);
	HttpResponse GetKernel(HttpRequest request);
	HttpResponse GetProcessedImage(HttpRequest request);
    HttpResponse PostProcess(HttpRequest request, HttpPostProcessor** postProcessor);
	HttpResponse PostConvolutionKernel(HttpRequest request, HttpPostProcessor** postProcessor);
};


class PostProcessProcessor : public HttpPostProcessor
{
public:
    PostProcessProcessor(const HttpRequest& request, uint32_t contentLength, const std::string& kernel, bool authenticated);
    virtual ~PostProcessProcessor();

    int IteratePostData(MHD_ValueKind kind, const char *key, const char *filename, const char *contentType, const char *transferEncoding, const char *data, uint64_t offset, size_t size);
	void IteratePostData(const char* uploadData, size_t uploadDataSize);

	struct InputImage
	{
		char* data;
		uint32_t size;
	};

    uint32_t m_contentLength = 0;
    char* m_content = nullptr;
	char* m_curContentPtr = nullptr;
	std::map<std::string, InputImage> m_inputImages;
	std::string m_kernel;
	bool m_authenticated = false;

protected:
    virtual void FinalizeRequest();
};


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
    if (ParseUrl(request.url, 1, "list"))
        return GetKernelsList(request);
	else if (ParseUrl(request.url, 1, "get-kernel"))
        return GetKernel(request);
	else if (ParseUrl(request.url, 1, "get-image"))
        return GetProcessedImage(request);

	Log("  unknown method\n");
    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::HandlePost(HttpRequest request, HttpPostProcessor** postProcessor)
{
    if (ParseUrl(request.url, 1, "process"))
        return PostProcess(request, postProcessor);
	else if (ParseUrl(request.url, 1, "add-kernel"))
        return PostConvolutionKernel(request, postProcessor);

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::GetKernelsList(HttpRequest request)
{
	Timer timer;

	std::vector<std::string> ids;
	GetConvolutionKernelsIdList(ids);
	uint32_t size = 0;
	for(auto& id : ids)
		size += id.length() + 1;
	
	HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "text/html"});
    response.content = (char*)malloc(size);
	response.contentLength = size;

	char* ptr = response.content;
	for(auto& id : ids)
	{
		strcpy(ptr, id.c_str());
		ptr += id.length();
		*ptr++ = '\n';
	}

	Log("  ok\n");
	Log("  Timer: %fms\n", timer.GetDurationMillisec());

    return response; 
}


HttpResponse RequestHandler::GetKernel(HttpRequest request)
{
	Timer timer;

	static const std::string kKernelId("kernel-id");
	static const std::string kSignature("signature");
	std::string kernelId;
	FindInMap(request.queryString, kKernelId, kernelId);

	Log("  id: %s\n", kernelId.c_str());

	const char* signatureBase64 = FindInMap(request.headers, kSignature);
	if(!signatureBase64)
	{
		Log("  Missing 'signature' parameter\n");
		return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	if(!VerifySignature(kernelId.c_str(), signatureBase64))
	{
		Log("  Authentication failed\n");
		return HttpResponse(MHD_HTTP_FORBIDDEN);
	}

	std::string kernel;
	if(GetConvolutionKernel(kernelId, kernel) != kKernelOk)
	{
		Log("  does not exists\n");
		return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	HttpResponse response;
	response.code = MHD_HTTP_OK;
	response.headers.insert({"Content-Type", "text/html"});
	
	response.content = (char*)malloc(kConvolutionKernelSize);
	memcpy(response.content, kernel.c_str(), kConvolutionKernelSize);
	response.contentLength = kConvolutionKernelSize;

	Log("  ok\n");
	Log("  Timer: %fms\n", timer.GetDurationMillisec());

	return response;
}


HttpResponse RequestHandler::GetProcessedImage(HttpRequest request)
{
	Timer timer;

	static const std::string kName("name");
	static const std::string kSignature("signature");
	std::string name;
	FindInMap(request.queryString, kName, name);

	Log("  name: %s\n", name.c_str());

	const char* signatureBase64 = FindInMap(request.headers, kSignature);
	if(!signatureBase64)
	{
		Log("  Missing 'signature' parameter\n");
		return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	if(!VerifySignature(name.c_str(), signatureBase64))
	{
		Log("  Authentication failed\n");
		return HttpResponse(MHD_HTTP_FORBIDDEN);
	}

	char filename[256];
	snprintf(filename, sizeof(filename), "data/%s.png", name.c_str());
	uint32_t fileSize = 0;
	void* file = ReadFile(filename, fileSize);
	if(!file)
	{
		Log("  file not found\n");
		return HttpResponse(MHD_HTTP_NOT_FOUND);
	}

	//remove(filename);

	HttpResponse response;
	response.code = MHD_HTTP_OK;
	response.headers.insert({"Content-Type", "image/png"});
	response.content = (char*)file;
	response.contentLength = fileSize;

	Log("  ok\n");
	Log("  Timer: %fms\n", timer.GetDurationMillisec());

	return response;
}


HttpResponse RequestHandler::PostProcess(HttpRequest request, HttpPostProcessor** postProcessor)
{
	uint32_t contentLength = 0;
	std::string kernelId;
    static const std::string kContentLength("content-length");
	static const std::string kKernelId("kernel-id");
	static const std::string kSignature("signature");
    FindInMap(request.headers, kContentLength, contentLength);
	FindInMap(request.queryString, kKernelId, kernelId);
    if(contentLength == 0)
	{
		Log("  Bad request: empty content\n");
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	std::string kernel;
	if(GetConvolutionKernel(kernelId, kernel) != kKernelOk)
	{
		Log("  Unknown kernel id: %s\n", kernelId.c_str());
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	bool authenticated = false;
	const char* signatureBase64 = FindInMap(request.headers, kSignature);
	if(signatureBase64)
	{
		if(!VerifySignature(kernelId.c_str(), signatureBase64))
		{
			Log("  Authentication failed\n");
			return HttpResponse(MHD_HTTP_FORBIDDEN);
		}
		authenticated = true;
	}

	Log("  id: %s\n", kernelId.c_str());
	Log("  content length: %u\n", contentLength);

	*postProcessor = new PostProcessProcessor(request, contentLength, kernel, authenticated);
    return HttpResponse();
}


HttpResponse RequestHandler::PostConvolutionKernel(HttpRequest request, HttpPostProcessor** postProcessor)
{
	Timer timer;

	static const std::string kKernelId("kernel-id");
	static const std::string kKernel("kernel");
	static const std::string kSignature("signature");

	const char* id = FindInMap(request.queryString, kKernelId);
	if(!id)
	{
		Log("  Missing 'kernel-id' parameter\n");
		return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	const char* kernel = FindInMap(request.queryString, kKernel);
	if(!kernel)
	{
		Log("  Missing 'kernel' parameter\n");
		return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	const char* signatureBase64 = FindInMap(request.headers, kSignature);
	if(!signatureBase64)
	{
		Log("  Missing 'signature' parameter\n");
		return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	Log("  id: %s\n", id);
	Log("  kernel: %s\n", kernel);

	if(!VerifySignature(id, signatureBase64))
	{
		Log("  Authentication failed\n");
		return HttpResponse(MHD_HTTP_FORBIDDEN);
	}

	auto errCode = AddConvolutionKernel(id, kernel);
	if(errCode != kKernelOk)
	{
		if(errCode == kKernelInvalidId)
			Log("  Invalid kernel id\n");
		else if(errCode == kKernelInvalidLength)
			Log("  Invalid kernel length\n");
		else if(errCode == kKernelAlreadyExists)
			Log("  Kernel already exists\n");
		else
			Log("  Unknown error\n");

		return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	Log("  ok\n", id, kernel);
	Log("  Timer: %fms\n", timer.GetDurationMillisec());

	return HttpResponse(MHD_HTTP_OK);
}


PostProcessProcessor::PostProcessProcessor(const HttpRequest& request, uint32_t contentLength, const std::string& kernel, bool authenticated)
    : HttpPostProcessor(request), m_contentLength(contentLength), m_kernel(kernel), m_authenticated(authenticated)
{
    if(m_contentLength)
	{
        m_content = (char*)malloc(m_contentLength);
		m_curContentPtr = m_content;
	}
}


PostProcessProcessor::~PostProcessProcessor() 
{
    if(m_content)
        free(m_content);
}


void PostProcessProcessor::FinalizeRequest() 
{
    if(!m_contentLength)
    {
        Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
		Log("  Bad request\n");
        return;
    }

	Timer totalTimer;

	std::string output = "{ ";
	uint32_t counter = 0;

	for(auto& iter : m_inputImages)
	{
		Log("  Process image '%s'\n", iter.first.c_str());
		Timer timer;

		Image srcImage;
		if(!read_png_from_memory(iter.second.data, iter.second.size, srcImage))
		{
			Log("  Bad png\n");
			Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
			return;
		}
		Log("  Size: %ux%u\n", srcImage.width, srcImage.height);

		Image srcImages[kChannelsNum], dstImages[kChannelsNum];
		for(uint32_t i = 0; i < kChannelsNum; i++)
		{
			srcImages[i].width = ((srcImage.width + 23) / 24) * 24;
			srcImages[i].height = ((srcImage.height + 2) / 3) * 3;
			srcImages[i].pixels = (ABGR*)aligned_alloc(16, srcImages[i].GetSize());
			memset(srcImages[i].pixels, 0, srcImages[i].GetSize());
			for(uint32_t y = 0; y < srcImage.height; y++)
			{
				for(uint32_t x = 0; x < srcImage.width; x++)
				{
					ABGR& src = srcImage.Pixel(x, y);
					ABGR& dst = srcImages[i].Pixel(x, y);
					dst.abgr = (src.abgr >> (i * 8)) & 0xFF;
				}
			}

			dstImages[i].width = srcImages[i].width / 3;
			dstImages[i].height = srcImages[i].height / 3;
			dstImages[i].pixels = (ABGR*)aligned_alloc(16, dstImages[i].GetSize());
			memset(dstImages[i].pixels, 0, dstImages[i].GetSize());
		}

		uint64_t timings[kChannelsNum];
		for(uint32_t i = 0; i < kChannelsNum; i++)
		{
			struct UserData
			{
				uint64_t srcImage;
				uint32_t srcImageWidth;
				uint32_t srcImageHeight;
				uint64_t kernel;
				uint64_t dstImage;
				uint32_t dstImageWidth;
				uint32_t dstImageHeight;
			};
			UserData userData;
			userData.srcImage = (uint64_t)srcImages[i].pixels;
			userData.srcImageWidth = srcImages[i].width;
			userData.srcImageHeight = srcImages[i].height;
			userData.kernel = (uint64_t)(m_kernel.c_str() + i * 8);
			userData.dstImage = (uint64_t)dstImages[i].pixels;
			userData.dstImageWidth = dstImages[i].width;
			userData.dstImageHeight = dstImages[i].height;
			timings[i] = Dispatch(srcImages[i].width / 24, srcImages[i].height / 3, (uint64_t)&userData);
		}

		if(m_authenticated)
		{
			Image finalImage(dstImages[0].width, dstImages[0].height);
			memset(finalImage.pixels, 0, finalImage.GetSize());
			for(uint32_t i = 0; i < kChannelsNum; i++)
			{
				for(uint32_t y = 0; y < finalImage.height; y++)
				{
					for(uint32_t x = 0; x < finalImage.width; x++)
					{
						uint32_t p = dstImages[i].Pixel(x,y).abgr;
						p = p << (i * 8);
						finalImage.Pixel(x,y).abgr |= p;
					}
				}
			}
			
			char filename[256];
			snprintf(filename, sizeof(filename), "data/%s.png", iter.first.c_str());
			if(!save_png(filename, finalImage))
				Log("  failed to save image to file\n");

#if 1
			for(uint32_t i = 0; i < 4; i++)
			{
				uint8_t* k = (uint8_t*)(m_kernel.c_str() + i * 8);
				for(uint32_t y = 0; y < finalImage.height; y++)
				{
					for(uint32_t x = 0; x < finalImage.width; x++)
					{
						uint32_t x0 = x * 3;
						uint32_t y0 = y * 3;
						uint32_t v[8];
						v[0] = srcImages[i].Pixel(x0 + 0, y0 + 0).abgr;
						v[1] = srcImages[i].Pixel(x0 + 1, y0 + 0).abgr;
						v[2] = srcImages[i].Pixel(x0 + 2, y0 + 0).abgr;
						v[3] = srcImages[i].Pixel(x0 + 0, y0 + 1).abgr;
						v[4] = srcImages[i].Pixel(x0 + 2, y0 + 1).abgr;
						v[5] = srcImages[i].Pixel(x0 + 0, y0 + 2).abgr;
						v[6] = srcImages[i].Pixel(x0 + 1, y0 + 2).abgr;
						v[7] = srcImages[i].Pixel(x0 + 2, y0 + 2).abgr;
						
						float counter = 0.0f;
						uint32_t sum = 0;
						for(uint32_t j = 0; j < 8; j++)
						{
							if(v[j] >= k[j])
							{
								sum += v[j];
								counter += 1.0f;
							}
						}

						uint32_t r = 0;
						if(counter > 0)
						{
							__m128i vsum = _mm_set1_epi32(sum);
							__m128 vfsum = _mm_cvtepi32_ps(vsum);
							__m128 vcounter = _mm_set1_ps(counter);
							__m128 vfr = _mm_div_ps(vfsum, vcounter);
							r = _mm_cvt_ss2si(vfr);
						}

						uint32_t testVal = (finalImage.Pixel(x, y).abgr >> (i * 8)) & 0xFF;
						if(testVal != r)
						{
							printf("dont match\n");
							exit(1);
						}
					}
				}
			}
#endif
		}

		output.append("\"");
		output.append(iter.first);
		output.append("\": ");
		char buf[512];
		snprintf(buf, sizeof(buf), "{ \"red_channel\" : %lu, \"green_channel\" : %lu, \"blue_channel\" : %lu, \"alpha_channel\" : %lu }", timings[0], timings[1], timings[2], timings[3]);
		output.append(buf);
		if(counter < m_inputImages.size() - 1)
			output.append(", ");
		counter++;

		Log("  Process time: %fms\n", timer.GetDurationMillisec());
	}
	output.append("}");

	HttpResponse response;
	response.code = MHD_HTTP_OK;
	response.headers.insert({"Content-Type", "application/json"});
	
	response.content = (char*)malloc(output.length());
	memcpy(response.content, output.c_str(), output.length());
	response.contentLength = output.length();

	Log("  Total process time: %fms\n", totalTimer.GetDurationMillisec());

	Complete(response);
}


int PostProcessProcessor::IteratePostData(MHD_ValueKind kind, const char *key, const char *filename, const char *contentType,
                                            const char *transferEncoding, const char *data, uint64_t offset, size_t size) 
{
	if(m_inputImages.find(key) == m_inputImages.end())
	{
		InputImage img;
		img.data = m_curContentPtr;
		img.size = 0;
		m_inputImages[key] = img;
	}

	auto& img = m_inputImages.find(key)->second;
	char* ptr = img.data + offset;
	char* endPtr = m_content + m_contentLength;
	if(ptr < endPtr)
	{
		memcpy(ptr, data, size);
		img.size += size;
		m_curContentPtr += size;
	}

    return MHD_YES;
}


void PostProcessProcessor::IteratePostData(const char* uploadData, size_t uploadDataSize)
{
}


void Test(const char* flag, Image& srcImage, uint64_t timings[4])
{
	Image srcImages[kChannelsNum], dstImages[kChannelsNum];
	for(uint32_t i = 0; i < kChannelsNum; i++)
	{
		srcImages[i].width = ((srcImage.width + 23) / 24) * 24;
		srcImages[i].height = ((srcImage.height + 2) / 3) * 3;
		srcImages[i].pixels = (ABGR*)aligned_alloc(16, srcImages[i].GetSize());
		memset(srcImages[i].pixels, 0, srcImages[i].GetSize());
		for(uint32_t y = 0; y < srcImage.height; y++)
		{
			for(uint32_t x = 0; x < srcImage.width; x++)
			{
				ABGR& src = srcImage.Pixel(x, y);
				ABGR& dst = srcImages[i].Pixel(x, y);
				dst.abgr = (src.abgr >> (i * 8)) & 0xFF;
			}
		}

		dstImages[i].width = srcImages[i].width / 3;
		dstImages[i].height = srcImages[i].height / 3;
		dstImages[i].pixels = (ABGR*)aligned_alloc(16, dstImages[i].GetSize());
		memset(dstImages[i].pixels, 0, dstImages[i].GetSize());
	}

	for(uint32_t i = 0; i < kChannelsNum; i++)
	{
		struct UserData
		{
			uint64_t srcImage;
			uint32_t srcImageWidth;
			uint32_t srcImageHeight;
			uint64_t kernel;
			uint64_t dstImage;
			uint32_t dstImageWidth;
			uint32_t dstImageHeight;
		};
		UserData userData;
		userData.srcImage = (uint64_t)srcImages[i].pixels;
		userData.srcImageWidth = srcImages[i].width;
		userData.srcImageHeight = srcImages[i].height;
		userData.kernel = (uint64_t)(flag + i * 8);
		userData.dstImage = (uint64_t)dstImages[i].pixels;
		userData.dstImageWidth = dstImages[i].width;
		userData.dstImageHeight = dstImages[i].height;
		timings[i] = Dispatch(srcImages[i].width / 24, srcImages[i].height / 3, (uint64_t)&userData);
	}

	Image finalImage(dstImages[0].width, dstImages[0].height);
	for(uint32_t i = 0; i < kChannelsNum; i++)
	{
		for(uint32_t y = 0; y < finalImage.height; y++)
		{
			for(uint32_t x = 0; x < finalImage.width; x++)
			{
				uint32_t p = dstImages[i].Pixel(x,y).abgr;
				p = p << (i * 8);
				finalImage.Pixel(x,y).abgr |= p;
			}
		}
	}

	for(uint32_t i = 0; i < 4; i++)
	{
		auto& image = dstImages[i];
		uint8_t* k = (uint8_t*)(flag + i * 8);
		for(uint32_t y = 0; y < image.height; y++)
		{
			for(uint32_t x = 0; x < image.width; x++)
			{
				uint32_t x0 = x * 3;
				uint32_t y0 = y * 3;
				uint32_t v[8];
				v[0] = srcImages[i].Pixel(x0 + 0, y0 + 0).abgr;
				v[1] = srcImages[i].Pixel(x0 + 1, y0 + 0).abgr;
				v[2] = srcImages[i].Pixel(x0 + 2, y0 + 0).abgr;
				v[3] = srcImages[i].Pixel(x0 + 0, y0 + 1).abgr;
				v[4] = srcImages[i].Pixel(x0 + 2, y0 + 1).abgr;
				v[5] = srcImages[i].Pixel(x0 + 0, y0 + 2).abgr;
				v[6] = srcImages[i].Pixel(x0 + 1, y0 + 2).abgr;
				v[7] = srcImages[i].Pixel(x0 + 2, y0 + 2).abgr;
				
				float counter = 0.0f;
				uint32_t sum = 0;
				for(uint32_t j = 0; j < 8; j++)
				{
					if(v[j] >= k[j])
					{
						sum += v[j];
						counter += 1.0f;
					}
				}

				uint32_t r = 0;
				if(counter > 0)
				{
					__m128i vsum = _mm_set1_epi32(sum);
					__m128 vfsum = _mm_cvtepi32_ps(vsum);
					__m128 vcounter = _mm_set1_ps(counter);
					__m128 vfr = _mm_div_ps(vfsum, vcounter);
					r = _mm_cvt_ss2si(vfr);
				}

				uint32_t testVal = image.Pixel(x, y).abgr;
				if(testVal != r)
				{
					printf("dont match\n");
					exit(1);
				}
			}
		}
	}
}


int main(int argc, char* argv[])
{
	srand(time(nullptr));

#if 0
	struct BytePos
	{
		uint32_t x;
		uint32_t y;
		uint32_t component;
	};

	BytePos poses[32] ={{0,0,0}, 
						{1,0,0},
						{2,0,0},
						{0,1,0},
						{2,1,0},
						{0,2,0}, 
						{1,2,0},
						{2,2,0},
						//
						{0,0,1}, 
						{1,0,1},
						{2,0,1},
						{0,1,1},
						{2,1,1},
						{0,2,1}, 
						{1,2,1},
						{2,2,1},
						//
						{0,0,2}, 
						{1,0,2},
						{2,0,2},
						{0,1,2},
						{2,1,2},
						{0,2,2}, 
						{1,2,2},
						{2,2,2},
						//
						{0,0,3}, 
						{1,0,3},
						{2,0,3},
						{0,1,3},
						{2,1,3},
						{0,2,3}, 
						{1,2,3},
						{2,2,3}};

	const char kAlphabet[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '=', 'A', 'B', 'C', 
                              'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
                              'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

	const char* flag = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

	uint32_t width = 384;
	uint32_t height = 384;

	Image image(width, height);
	memset(image.pixels, 0xff, image.GetSize());
	uint64_t timings[4];
	Test(flag, image, timings);
	for(uint32_t c = 0; c < kChannelsNum; c++)
		printf("%8lu ", timings[c]);
	printf("\n");

	for(uint32_t byteIdx = 0; byteIdx < 32; byteIdx++)
	{
		auto bytePos = poses[byteIdx];
		printf("Byte %u %c:\n", byteIdx, flag[byteIdx]);
		for(uint32_t symbolIdx = 0; symbolIdx < sizeof(kAlphabet); symbolIdx++)
			printf("   %c    ", kAlphabet[symbolIdx]);
		printf("\n");

		for(uint32_t symbolIdx = 0; symbolIdx < sizeof(kAlphabet); symbolIdx++)
		{
			char symbol = kAlphabet[symbolIdx];

			Image image(width, height);
			memset(image.pixels, 0xff, image.GetSize());

			for(uint32_t y = 0; y < image.height / 3; y++)
			{
				uint32_t blockIdx = 0;
				for(uint32_t x = 0; x < image.width / 3; x++)
				{
					uint32_t x0 = x * 3;
					uint32_t y0 = y * 3;

					for(uint32_t yi = 0; yi < 3; yi++)
						for(uint32_t xi = 0; xi < 3; xi++)
							image.Pixel(x0 + xi, y0 + yi).abgr = 0xffffffff;

					uint32_t& pixel = image.Pixel(x0 + bytePos.x, y0 + bytePos.y).abgr;
                    uint32_t mask = 0xff << (bytePos.component * 8);
					pixel = pixel & ~mask;
					if(blockIdx == 4)
						pixel |= symbol << (bytePos.component * 8);

					blockIdx = (blockIdx + 1) % 8;
				}
			}

			uint64_t timings[4];
			Test(flag, image, timings);
			printf("%8lu", timings[bytePos.component]);
		}
		printf("\n");
	}
	return 0;
#endif

	InitKernels();
	InitSignatureVerifier();
    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(8081);

    while (1)
        sleep(1);

    server.Stop();
	ShutdownSignatureVerifier();

    return 0;
}

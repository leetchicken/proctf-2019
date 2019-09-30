#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "log.h"
#include "httpserver.h"
#include "png.h"
#include "dispatch.h"
#include "kernels.h"
#include <x86intrin.h>


class RequestHandler : public HttpRequestHandler
{
public:
	RequestHandler() = default;
	virtual ~RequestHandler() = default;

	HttpResponse HandleGet(HttpRequest request);
	HttpResponse HandlePost(HttpRequest request, HttpPostProcessor** postProcessor);

private:
	HttpResponse GetKernelsList(HttpRequest request);
    HttpResponse PostImage(HttpRequest request, HttpPostProcessor** postProcessor);
	HttpResponse PostConvolutionKernel(HttpRequest request, HttpPostProcessor** postProcessor);
};


class PostImageProcessor : public HttpPostProcessor
{
public:
    PostImageProcessor(const HttpRequest& request, uint32_t contentLength, const std::string& kernel);
    virtual ~PostImageProcessor();

    int IteratePostData(MHD_ValueKind kind, const char *key, const char *filename, const char *contentType, const char *transferEncoding, const char *data, uint64_t offset, size_t size);
	void IteratePostData(const char* uploadData, size_t uploadDataSize);

    uint32_t m_contentLength = 0;
    char* m_content = nullptr;
	std::string m_kernel;

protected:
    virtual void FinalizeRequest();
};


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
    if (ParseUrl(request.url, 1, "list"))
        return GetKernelsList(request);

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::HandlePost(HttpRequest request, HttpPostProcessor** postProcessor)
{
    if (ParseUrl(request.url, 1, "image"))
        return PostImage(request, postProcessor);
	else if (ParseUrl(request.url, 1, "kernel"))
        return PostConvolutionKernel(request, postProcessor);	

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::GetKernelsList(HttpRequest request)
{
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
    return response; 
}


HttpResponse RequestHandler::PostImage(HttpRequest request, HttpPostProcessor** postProcessor)
{
	uint32_t contentLength = 0;
	std::string kernelId;
    static const std::string kContentLength("content-length");
	static const std::string kKernelId("kernel-id");
    FindInMap(request.headers, kContentLength, contentLength);
	FindInMap(request.headers, kKernelId, kernelId);
    if(contentLength == 0)
	{
		Log("  Bad request: empty content\n");
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	std::string kernel;
	if(GetConvolutionKernel(kernelId, kernel) != kKernelOk)
	{
		Log("  Unknown kernel id: %s\n", kKernelId.c_str());
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
	}

	*postProcessor = new PostImageProcessor(request, contentLength, kernel);
    return HttpResponse();
}


HttpResponse RequestHandler::PostConvolutionKernel(HttpRequest request, HttpPostProcessor** postProcessor)
{
	static const std::string kKernelId("kernel-id");
	static const std::string kKernel("kernel");

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

	Log("  id: %s\n", id);
	Log("  kernel: %s\n", kernel);

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

	Log("  success\n", id, kernel);

	return HttpResponse(MHD_HTTP_OK);
}


PostImageProcessor::PostImageProcessor(const HttpRequest& request, uint32_t contentLength, const std::string& kernel)
    : HttpPostProcessor(request), m_contentLength(contentLength), m_kernel(kernel)
{
    if(m_contentLength)
        m_content = (char*)malloc(m_contentLength);
}


PostImageProcessor::~PostImageProcessor() 
{
    if(m_content)
        free(m_content);
}


void png_to_mem(void *context, void *data, int size)
{
	HttpResponse* response = (HttpResponse*)context;
	response->content = (char*)malloc(size);
	memcpy(response->content, data, size);
	response->contentLength = size;
}


void PostImageProcessor::FinalizeRequest() 
{
    if(!m_contentLength)
    {
        Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
		Log("  Bad request\n");
        return;
    }

	const uint32_t kChannelsNum = 4;

    Image srcImage;
	if(!read_png_from_memory(m_content, m_contentLength, srcImage))
	{
		Log("  Bad png\n");
		Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
		return;
	}

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

	HttpResponse response;
	response.code = MHD_HTTP_OK;
	response.headers.insert({"Content-Type", "application/json"});
	char buf[512];
	snprintf(buf, sizeof(buf), "{ \"Red channel processing time\" : %lu, \"Blue channel processing time\" : %lu, \"Green channel processing time\" : %lu, \"Alpha channel processing time\" : %lu }", timings[0], timings[1], timings[2], timings[3]);
	response.content = (char*)malloc(strlen(buf) + 1);
	strcpy(response.content, buf);
	response.contentLength = strlen(buf);

	Complete(response);
}


int PostImageProcessor::IteratePostData(MHD_ValueKind kind, const char *key, const char *filename, const char *contentType,
                                            const char *transferEncoding, const char *data, uint64_t offset, size_t size) 
{
    if(m_content)
        memcpy(m_content + offset, data, size);
    return MHD_YES;
}


void PostImageProcessor::IteratePostData(const char* uploadData, size_t uploadDataSize)
{
}


void Test(const char* flag, Image& srcImage, uint64_t timings[4])
{
	/*char flag[33];
	for(uint32_t i = 0; i < 32; i++)
		flag[i] = (rand() % 42) + 40;
	flag[32] = 0;*/

	//printf("%s\n", flag);

	/*Image srcImage(24 * 512, 3, 4);
	memset(srcImage.pixels, 0, srcImage.width * srcImage.height * 4);
	for(uint32_t y = 0; y < srcImage.height; y++)
	{
		for(uint32_t x = 0; x < srcImage.width; x++)
		{
			uint32_t p = rand() % 256;
			p |= (rand() % 256) << 8;
			p |= (rand() % 256) << 16;
			p |= (rand() % 256) << 24;
			ABGR& dst = srcImage.Pixel(x, y);
			dst.abgr = p;
		}
	}*/

	Image srcImages[4], dstImages[4];
	for(uint32_t i = 0; i < 4; i++)
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

	for(uint32_t i = 0; i < 4; i++)
	{
		struct UserData
		{
			uint64_t srcImage;
			uint32_t srcImageWidth;
			uint32_t srcImageHeight;
			uint64_t flag;
			uint64_t dstImage;
			uint32_t dstImageWidth;
			uint32_t dstImageHeight;
		};
		UserData userData;
		userData.srcImage = (uint64_t)srcImages[i].pixels;
		userData.srcImageWidth = srcImages[i].width;
		userData.srcImageHeight = srcImages[i].height;
		userData.flag = (uint64_t)(flag + i * 8);
		userData.dstImage = (uint64_t)dstImages[i].pixels;
		userData.dstImageWidth = dstImages[i].width;
		userData.dstImageHeight = dstImages[i].height;
		timings[i] = Dispatch(srcImages[i].width / 24, srcImages[i].height / 3, (uint64_t)&userData);
	}

	Image finalImage(dstImages[0].width, dstImages[0].height);
	for(uint32_t i = 0; i < 4; i++)
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

	/*struct BytePos
	{
		uint32_t x;
		uint32_t y;
		uint32_t c;
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

	const char* flag = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

	for(uint32_t b = 0; b < 32; b++)
	{
		auto p = poses[b];
		printf("Byte %u %c:\n", b, flag[b]);
		for(uint32_t c = 48; c <= 90; c++)
			printf("   %c    ", (char)c);
		printf("\n");

		for(uint32_t c = 48; c <= 90; c++)
		{
			Image srcImage(24 * 512, 3);
			memset(srcImage.pixels, 0, srcImage.width * srcImage.height * 4);
			for(uint32_t y = 0; y < srcImage.height / 3; y++)
			{
				uint32_t bc = 0;
				for(uint32_t x = 0; x < srcImage.width / 3; x++)
				{
					uint32_t x0 = x * 3;
					uint32_t y0 = y * 3;

					for(uint32_t yi = 0; yi < 3; yi++)
						for(uint32_t xi = 0; xi < 3; xi++)
							srcImage.Pixel(x0 + xi, y0 + yi).abgr = 0xffffffff;

					uint32_t mask = 0xff << (p.c * 8);
					uint32_t& t = srcImage.Pixel(x0 + p.x, y0 + p.y).abgr;
					t = t & ~mask;
					if(bc == 4)
						t |= c << (p.c * 8);

					bc = (bc + 1) % 8;
				}
			}

			uint64_t timings[4];
			Test(flag, srcImage, timings);
			printf("%8lu", timings[p.c]);
		}
		printf("\n");
	}
	return 0;*/

	InitKernels();
    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(8081);

    while (1)
        sleep(1);

    server.Stop();

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "log.h"
#include "httpserver.h"
#include "png.h"
#include "dispatch.h"
#include <x86intrin.h>


class RequestHandler : public HttpRequestHandler
{
public:
	RequestHandler() = default;
	virtual ~RequestHandler() = default;

	HttpResponse HandleGet(HttpRequest request);
	HttpResponse HandlePost(HttpRequest request, HttpPostProcessor** postProcessor);

private:
    HttpResponse PostImage(HttpRequest request, HttpPostProcessor** postProcessor);
};


class PostImageProcessor : public HttpPostProcessor
{
public:
    PostImageProcessor(const HttpRequest& request);
    virtual ~PostImageProcessor();

    int IteratePostData(MHD_ValueKind kind, const char *key, const char *filename, const char *contentType, const char *transferEncoding, const char *data, uint64_t offset, size_t size);
	void IteratePostData(const char* uploadData, size_t uploadDataSize);

    uint32_t m_contentLength = 0;
    char* m_content = nullptr;

protected:
    virtual void FinalizeRequest();
};


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
    /*if (ParseUrl(request.url, 1, ""))
        return GetMainPage(request);*/

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::HandlePost(HttpRequest request, HttpPostProcessor** postProcessor)
{
    if (ParseUrl(request.url, 1, "image"))
        return PostImage(request, postProcessor);

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::PostImage(HttpRequest request, HttpPostProcessor** postProcessor)
{
	*postProcessor = new PostImageProcessor(request);
    return HttpResponse();
}


PostImageProcessor::PostImageProcessor(const HttpRequest& request)
    : HttpPostProcessor(request)
{
    static const std::string kContentLength("content-length");
    static const std::string kAuth("auth");
    FindInMap(request.headers, kContentLength, m_contentLength);

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

    /*Image image;
	if(!read_png_from_memory(m_content, m_contentLength, image))
	{
		Log("  bad png\n");
		Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
		return;
	}*/

	const char* flag = "AC539Y5FAJ7A8VV8WHZDM3C15A22A55=";

	/*Image image(24, 6, 4);
	uint32_t counter = 0x30;
	for(uint32_t y = 0; y < image.height; y++)
	{
		for(uint32_t x = 0; x < image.width; x++)
		{
			uint32_t p = counter & 0xff;
			p |= (counter & 0xff) << 8;
			p |= (counter & 0xff) << 16;
			p |= (counter & 0xff) << 24;
			uint32_t* dst = (uint32_t*)image.pixels + y * image.width + x;
			*dst = p;
			counter++;
		}
	}

	Image images[4], dstImages[4];
	for(uint32_t i = 0; i < image.componentsNum; i++)
	{
		images[i].width = ((image.width + 23) / 24) * 24;
		images[i].height = ((image.height + 2) / 3) * 3;
		images[i].componentsNum = 4;
		images[i].pixels = aligned_alloc(16, image.width * image.height * 4);
		for(uint32_t y = 0; y < image.height; y++)
		{
			for(uint32_t x = 0; x < image.width; x++)
			{
				uint32_t* src = (uint32_t*)image.pixels + y * image.width + x;
				uint32_t* dst = (uint32_t*)images[i].pixels + y * images[i].width + x;
				*dst = (*src >> (i * 8)) & 0xFF;
			}
		}

		dstImages[i].width = images[i].width / 3;
		dstImages[i].height = images[i].height / 3;
		dstImages[i].componentsNum = 4;
		dstImages[i].pixels = aligned_alloc(16, image.width * image.height * 4);

	}

	for(uint32_t i = 0; i < image.componentsNum; i++)
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
		userData.srcImage = (uint64_t)images[i].pixels;
		userData.srcImageWidth = images[i].width;
		userData.srcImageHeight = images[i].height;
		userData.flag = (uint64_t)(flag + i * 8);
		userData.dstImage = (uint64_t)dstImages[i].pixels;
		userData.dstImageWidth = dstImages[i].width;
		userData.dstImageHeight = dstImages[i].height;
		Dispatch(images[i].width / 24, images[i].height / 3, (uint64_t)&userData);
	}

	Headers responseHeaders;
	responseHeaders.insert( { "Content-Type", "image/png" } );

	HttpResponse response;
	response.code = MHD_HTTP_OK;
	response.headers = responseHeaders;

	stbi_write_png_to_func(png_to_mem, &response, image.width, image.height, 4, image.pixels, image.width * image.componentsNum);

	Complete(response);*/
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

	Image images[4], dstImages[4];
	for(uint32_t i = 0; i < 4; i++)
	{
		images[i].width = ((srcImage.width + 23) / 24) * 24;
		images[i].height = ((srcImage.height + 2) / 3) * 3;
		images[i].pixels = (ABGR*)aligned_alloc(16, images[i].GetSize());
		memset(images[i].pixels, 0, images[i].GetSize());
		for(uint32_t y = 0; y < srcImage.height; y++)
		{
			for(uint32_t x = 0; x < srcImage.width; x++)
			{
				ABGR& src = srcImage.Pixel(x, y);
				ABGR& dst = images[i].Pixel(x, y);
				dst.abgr = (src.abgr >> (i * 8)) & 0xFF;
			}
		}

		dstImages[i].width = images[i].width / 3;
		dstImages[i].height = images[i].height / 3;
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
		userData.srcImage = (uint64_t)images[i].pixels;
		userData.srcImageWidth = images[i].width;
		userData.srcImageHeight = images[i].height;
		userData.flag = (uint64_t)(flag + i * 8);
		userData.dstImage = (uint64_t)dstImages[i].pixels;
		userData.dstImageWidth = dstImages[i].width;
		userData.dstImageHeight = dstImages[i].height;
		timings[i] = Dispatch(images[i].width / 24, images[i].height / 3, (uint64_t)&userData);
	}

	Image finalImage(dstImages[0].width, dstImages[0].height, 4);
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
				v[0] = images[i].Pixel(x0 + 0, y0 + 0).abgr;
				v[1] = images[i].Pixel(x0 + 1, y0 + 0).abgr;
				v[2] = images[i].Pixel(x0 + 2, y0 + 0).abgr;
				v[3] = images[i].Pixel(x0 + 0, y0 + 1).abgr;
				v[4] = images[i].Pixel(x0 + 2, y0 + 1).abgr;
				v[5] = images[i].Pixel(x0 + 0, y0 + 2).abgr;
				v[6] = images[i].Pixel(x0 + 1, y0 + 2).abgr;
				v[7] = images[i].Pixel(x0 + 2, y0 + 2).abgr;
				
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

	/*uint32_t ti = 0;
	while(1)
	{
		Test(ti++);
	}*/

	struct BytePos
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
			Image srcImage(24 * 512, 3, 4);
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
	return 0;

    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(8080);

    while (1)
        sleep(1);

    server.Stop();

    return 0;
}

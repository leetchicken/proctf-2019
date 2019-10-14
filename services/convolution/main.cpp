#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>
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
		std::string name;
		char* data;
		uint32_t size;
	};

    uint32_t m_contentLength = 0;
    char* m_content = nullptr;
	char* m_curContentPtr = nullptr;
	std::vector<InputImage> m_inputImages;
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

	if(m_inputImages.size() > 32)
	{
		Log("  Too many images\n");
		Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
		return;
	}

	Timer totalTimer;

	std::string output = "{ ";
	uint32_t counter = 0;

	for(auto& img : m_inputImages)
	{
		Log("  Process image '%s'\n", img.name.c_str());
		Timer timer;

		Image srcImage;
		if(!read_png_from_memory(img.data, img.size, srcImage))
		{
			Log("  Bad png\n");
			Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
			return;
		}
		Log("  Size: %ux%u\n", srcImage.width, srcImage.height);
		
		if(srcImage.width > 600 || srcImage.height > 600)
		{
			Log("  Image is too big\n");
			Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
			return;
		}

		Image srcImagePadded, dstImage;
		srcImagePadded.width = ((srcImage.width + 23) / 24) * 24;
		srcImagePadded.height = ((srcImage.height + 2) / 3) * 3;
		srcImagePadded.pixels = (ABGR*)aligned_alloc(16, srcImagePadded.GetSize());
		dstImage.width = srcImagePadded.width / 3;
		dstImage.height = srcImagePadded.height / 3;
		dstImage.pixels = (ABGR*)aligned_alloc(16, dstImage.GetSize());

		ABGR zero;
		memset(&zero, 0, sizeof(zero));
		for(uint32_t y = 0; y < srcImagePadded.height; y++)
		{
			for(uint32_t x = 0; x < srcImagePadded.width; x++)
			{
				bool withinSource = x < srcImage.width && y < srcImage.height;
				ABGR& src = withinSource ? srcImage.Pixel(x, y) : zero;
				srcImagePadded.Pixel(x, y) = src;
			}
		}

		uint64_t timings[kChannelsNum];
		for(uint32_t i = 0; i < kChannelsNum; i++)
		{
			struct UserData
			{
				uint64_t srcImage;
				uint32_t srcImageWidth;
				uint32_t channelIdx;
				uint64_t kernel;
				uint64_t dstImage;
				uint32_t dstImageWidth;
				uint32_t dstImageHeight;
			};
			UserData userData;
			userData.srcImage = (uint64_t)srcImagePadded.pixels;
			userData.srcImageWidth = srcImagePadded.width;
			userData.channelIdx = i;
			userData.kernel = (uint64_t)(m_kernel.c_str() + i * 8);
			userData.dstImage = (uint64_t)dstImage.pixels;
			userData.dstImageWidth = dstImage.width;
			userData.dstImageHeight = dstImage.height;
			timings[i] = Dispatch(srcImagePadded.width / 24, srcImagePadded.height / 3, (uint64_t)&userData);
		}

		if(m_authenticated)
		{			
			char filename[256];
			snprintf(filename, sizeof(filename), "data/%s.png", img.name.c_str());
			if(!save_png(filename, dstImage))
				Log("  failed to save image to file\n");
		}

		output.append("\"");
		output.append(img.name);
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
	if(m_inputImages.empty() || m_inputImages.back().name != key)
	{
		InputImage img;
		img.name = key;
		img.data = m_curContentPtr;
		img.size = 0;
		m_inputImages.push_back(img);
	}

	auto& img = m_inputImages.back();
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


int main(int argc, char* argv[])
{
	srand(time(nullptr));

	InitKernels();
	InitSignatureVerifier();
    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(80);

    while (1)
        sleep(1);

    server.Stop();
	ShutdownSignatureVerifier();

    return 0;
}

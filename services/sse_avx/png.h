#pragma once
#include <stdint.h>
#include <malloc.h>
#define STBI_ONLY_PNG 1
#include "stb_image.h"
#include "stb_image_write.h"


union ABGR
{
    struct
    {
        uint32_t r : 8;
        uint32_t g : 8;
        uint32_t b : 8;
        uint32_t a : 8;
    };
    uint32_t abgr;
};


struct Image
{
	ABGR* pixels;
    uint32_t width;
	uint32_t height;

    Image()
		: pixels(nullptr), width(0), height(0)
    {

    }

    Image(uint32_t w, uint32_t h)
		: pixels(nullptr), width(w), height(h)
    {
		uint32_t size = w * h * sizeof(ABGR);
        size = (size + 15) & ~15;
        pixels = (ABGR*)memalign(16, size);
    }

    Image(const Image&) = delete;
    Image(const Image&&) = delete;
    Image& operator=(const Image&) = delete;
    Image& operator=(const Image&&) = delete;

    ~Image()
    {
        if(pixels)
            free(pixels);
    }

	ABGR& Pixel(uint32_t x, uint32_t y)
	{
		return pixels[y * width + x];
    }

	uint32_t GetSize() const
	{
		return width * height * sizeof(ABGR);
	}
};


bool read_png(const char* file_name, Image& image);
bool read_png_from_memory(const void* mem, uint32_t memSize, Image& image);
bool save_png(const char* file_name, const Image& image);
bool save_png(const char* file_name, const void* pixels, uint32_t width, uint32_t height);

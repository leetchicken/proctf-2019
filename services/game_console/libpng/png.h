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


union ARGB
{
    struct
    {
        uint32_t b : 8;
        uint32_t g : 8;
        uint32_t r : 8;
        uint32_t a : 8;
    };
    uint32_t argb;
};


struct Image
{
    ABGR* abgr;
    uint32_t width;
    uint32_t height;

    Image()
    	: abgr(nullptr), width(0), height(0)
    {

    }

    Image(uint32_t w, uint32_t h)
    	: abgr(nullptr), width(w), height(h)
    {
        uint32_t size = w * h * sizeof(ABGR);
        size = (size + 15) & ~15;
        abgr = (ABGR*)memalign(16, size);
    }

    Image(const Image&) = delete;
    Image(const Image&&) = delete;
    Image& operator=(const Image&) = delete;
    Image& operator=(const Image&&) = delete;

    ~Image()
    {
        if(abgr)
            free(abgr);
    }
};


bool read_png(const char* file_name, Image& image);
bool save_png(const char* file_name, const Image& image);
bool save_png(const char* file_name, const ABGR* abgr, uint32_t width, uint32_t height);
ARGB* ABGR_to_ARGB_inplace(ABGR* abgr, uint32_t width, uint32_t height);
void ABGR_to_ARGB(ABGR* abgr, ARGB* argb, uint32_t width, uint32_t height);
#include "png.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


bool read_png(const char* file_name, Image& image)
{
    int x, y, comp;
    stbi_uc* data = stbi_load(file_name, &x, &y, &comp, 4);
    if(!data)
        return false;

    image.width = x;
    image.height = y;
    image.abgr = (ABGR*)memalign(16, image.width * image.height * sizeof(ABGR));
    memcpy(image.abgr, data, x * y * sizeof(uint32_t));
    stbi_image_free(data);
    return true;
}


bool save_png(const char* file_name, const Image& image)
{
    return save_png(file_name, image.abgr, image.width, image.height);
}


bool save_png(const char* file_name, const ABGR* abgr, uint32_t width, uint32_t height)
{
    return stbi_write_png(file_name, width, height, 4, abgr, width * sizeof(uint32_t)) != 0;
}


ARGB* ABGR_to_ARGB_inplace(ABGR* abgr, uint32_t width, uint32_t height)
{
    uint32_t len = width * height;
    for(uint32_t i = 0; i < len; i++)
    {
        ARGB pixel;
        pixel.r = abgr[i].r;
        pixel.g = abgr[i].g;
        pixel.b = abgr[i].b;
        pixel.a = abgr[i].a;
        memcpy(&abgr[i], &pixel, sizeof(pixel));
    }
    return (ARGB*)abgr;
}


void ABGR_to_ARGB(ABGR* abgr, ARGB* argb, uint32_t width, uint32_t height)
{
    uint32_t len = width * height;
    for(uint32_t i = 0; i < len; i++)
    {
        argb[i].r = abgr[i].r;
        argb[i].g = abgr[i].g;
        argb[i].b = abgr[i].b;
        argb[i].a = abgr[i].a;
    }
}
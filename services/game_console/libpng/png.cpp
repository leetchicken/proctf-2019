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
    image.rgba = (RGBA*)memalign(16, image.width * image.height * sizeof(RGBA));
    memcpy(image.rgba, data, x * y * sizeof(uint32_t));
    stbi_image_free(data);
    return true;
}


bool save_png(const char* file_name, const Image& image)
{
    return save_png(file_name, image.rgba, image.width, image.height);
}


bool save_png(const char* file_name, const RGBA* rgba, uint32_t width, uint32_t height)
{
    return stbi_write_png(file_name, width, height, 4, rgba, width * sizeof(uint32_t)) != 0;
}

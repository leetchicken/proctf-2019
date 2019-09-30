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
	uint32_t size = image.width * image.height * sizeof(ABGR);
    image.pixels = (ABGR*)memalign(16, size);
    memcpy(image.pixels, data, size);
    stbi_image_free(data);
    return true;
}


bool read_png_from_memory(const void* mem, uint32_t memSize, Image& image)
{
    int x, y, comp;
    stbi_uc* data = stbi_load_from_memory((const stbi_uc*)mem, memSize, &x, &y, &comp, 4);
    if(!data)
        return false;

    image.width = x;
	image.height = y;
	uint32_t size = image.width * image.height * sizeof(ABGR);
    image.pixels = (ABGR*)memalign(16, size);
    memcpy(image.pixels, data, size);
    stbi_image_free(data);
    return true;
}


bool save_png(const char* file_name, const Image& image)
{
	return save_png(file_name, image.pixels, image.width, image.height);
}


bool save_png(const char* file_name, const void* pixels, uint32_t width, uint32_t height)
{
	return stbi_write_png(file_name, width, height, 4, pixels, width * sizeof(ABGR)) != 0;
}

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "png.h"


int main(int argc, char** argv)
{
    if(argc != 3)
    {
        printf("./gen_pngs <image width> <image height>\n");
        return 1;
    }

    uint32_t width = atoi(argv[1]);
    uint32_t height = atoi(argv[2]);
    if(width % 24 != 0)
    {
        printf("Width must be multiple of 24\n");
        return 1;
    }
    if(height % 3 != 0)
    {
        printf("Height must be multiple of 3\n");
        return 1;
    }

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

    const char kAlphabet[] = {'.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
							  '=', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 
							  'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a'};

	for(uint32_t byteIdx = 0; byteIdx < 32; byteIdx++)
	{
		auto bytePos = poses[byteIdx];
		for(uint32_t symbolIdx = 0; symbolIdx < sizeof(kAlphabet); symbolIdx++)
		{
            char symbol = kAlphabet[symbolIdx];

			Image image(width, height);
			memset(image.pixels, 0, image.GetSize());

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

			char filename[256];
            sprintf(filename, "pngs/%u_%c.png", byteIdx, symbol);
            save_png(filename, image);
		}
	}

    Image image(width, height);
	memset(image.pixels, 0xff, image.GetSize());
    save_png("pngs/white.png", image);

    memset(image.pixels, 0, image.GetSize());
    for(uint32_t y = 0; y < image.height / 3; y++)
    {
        for(uint32_t x = 0; x < image.width / 3; x++)
        {
            uint32_t x0 = x * 3;
            uint32_t y0 = y * 3;

            for(uint32_t yi = 0; yi < 3; yi++)
                for(uint32_t xi = 0; xi < 3; xi++)
                    image.Pixel(x0 + xi, y0 + yi).abgr = 0xffffffff;

            image.Pixel(x0, y0).abgr = 0;
        }
    }
    save_png("pngs/black0.png", image);

    return 0;
}
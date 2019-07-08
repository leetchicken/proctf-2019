#include <string.h>
#include <stdio.h>
#include "png.h"


int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        printf("./build_bmp <input png> <output>\n");
        return 1;
    }

    Image image;
    if(!read_png(argv[1], image))
    {
        printf("Failed to read png: %s\n", argv[1]);
        return 1;
    }

    FILE* f = fopen(argv[2], "w");
    if(!f)
    {
        printf("Failed to open file: %s\n", argv[2]);
        return 1;
    }

    fwrite(image.abgr, 1, image.width * image.height * 4, f);
    fclose(f);

    return 0;
}

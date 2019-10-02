#include "misc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


char* ReadFile(const char* fileName, uint32_t& size)
{
    FILE* f = fopen(fileName, "r");
    if (!f)
    {
        size = 0;
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* fileData = (char*)malloc(size);
    fread(fileData, 1, size, f);
    fclose(f);

    return fileData;
}
#ifndef CUSTOM_MALLOC_H
#define CUSTOM_MALLOC_H

#include "geocacher.pb-c.h"

void *my_malloc(size_t size);
void my_free(void *ptr);
void init_custom_malloc();

extern ProtobufCAllocator protobuf_custom__allocator;

#endif

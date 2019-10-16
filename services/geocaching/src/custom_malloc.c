#include <stdlib.h>
#include "debug.h"
#include "geocacher.pb-c.h"

void *my_memory = 0;
size_t my_memory_size = 10240;
size_t my_memory_pos = 0;

void init_custom_malloc()
{
    if (!my_memory)
    {
        my_memory = malloc(my_memory_size);
    }
    my_memory_pos = 0;
}

void *my_malloc(size_t size)
{
    if (my_memory_size > my_memory_pos + size) {
        void *result = my_memory + my_memory_pos;
        my_memory_pos += size;
        return result;
    }
    else
    {
        LOG("Allocated in system memory");
        return malloc(size);
    }
}

void my_free(void *ptr)
{
    if (ptr >= my_memory && ptr < my_memory + my_memory_size) {
        return;
    }
    free(ptr);
}

void *custom_alloc(void *allocator_data, size_t size)
{
    return my_malloc(size);
}

void custom_free(void *allocator_data, void *data)
{
    my_free(data);
}

ProtobufCAllocator protobuf_custom__allocator = {
    .alloc = &custom_alloc,
    .free = &custom_free,
    .allocator_data = NULL,
};
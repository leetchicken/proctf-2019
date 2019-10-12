/* Prototypes for __malloc_hook, __free_hook */
#include <malloc.h>

static void *(* volatile old_malloc_hook)(size_t, const void*);
static void (* volatile old_free_hook)(void*, const void*);

#define app_debug(...) { fprintf(stderr, __VA_ARGS__); }

void my_free_hook(void*, const void *);

void *my_malloc_hook(size_t size, const void *caller)
{
    void *result;
    /* Restore all old hooks */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    /* Call recursively */
    result = malloc (size);
    /* Save underlying hooks */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    /* printf might call malloc, so protect it too. */
    app_debug("malloc (%u) called from (%p) returns %p\n", (unsigned int) size, caller, result);
    /* Restore our own hooks */
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
    return result;
}

void my_free_hook(void *ptr, const void *caller)
{
    /* Restore all old hooks */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    /* Call recursively */
    free (ptr);
    /* Save underlying hooks */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    /* printf might call free, so protect it too. */
    app_debug("%p freed pointer %p\n", caller, ptr);
    /* Restore our own hooks */
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
}

void setup_malloc_hook()
{
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
}

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#include <stdio.h>
#define LOG(...) fprintf(stderr,__VA_ARGS__);
#else
#define LOG(...) {}
#endif

#endif

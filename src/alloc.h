//
// Created by Anshul Gupta on 4/9/25.
//

#ifndef ALLOC_H
#define ALLOC_H

#ifdef TEST_ALLOC
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#define gmalloc(size) test_malloc(size)
#define gcalloc(num, size) test_calloc(num, size)
#define grealloc(ptr, size) test_realloc(ptr, size)
#define gfree(ptr) test_free(ptr)
#else
#include <stdlib.h>
#define gmalloc(size) malloc(size)
#define gcalloc(num, size) calloc(num, size)
#define grealloc(ptr, size) realloc(ptr, size)
#define gfree(ptr) free(ptr)
#endif

#endif // ALLOC_H

#include <stdlib.h>
#include <stdint.h>
#include "nu_misc.h"

void *nvt_malloc_align(uint32_t size, uint32_t align)
{
    void *ptr;
    uint32_t align_size;

    align = NVT_ALIGN(align, sizeof(void *));

    align_size = NVT_ALIGN(size, sizeof(void *)) + align;

    ptr = malloc(align_size);

    if (ptr != NULL)
    {
        void *align_ptr;

        if (((uint32_t)ptr & (align - 1)) == 0)
        {
            align_ptr = (void *)((uint32_t)ptr + align);
        }
        else
        {
            align_ptr = (void *)NVT_ALIGN((uint32_t)ptr, align);
        }

        *((uint32_t *)((uint32_t)align_ptr - sizeof(void *))) = (uint32_t)ptr;

        ptr = align_ptr;
    }

    return ptr;
}

void nvt_free_align(void *ptr)
{
    if (ptr == NULL) return;

    free((void *) * ((uint32_t *)((uint32_t)ptr - sizeof(void *))));
}

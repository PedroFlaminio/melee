#ifndef MELEE_HOST_MEMORY_H
#define MELEE_HOST_MEMORY_H

#include <melee_host/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void* melee_host_aligned_alloc(size_t size, size_t alignment);
void melee_host_aligned_free(void* pointer, size_t alignment);

#ifdef __cplusplus
}
#endif

#endif

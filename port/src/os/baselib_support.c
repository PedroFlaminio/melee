#include <melee_host/memory.h>

#include <dolphin/ar.h>
#include <dolphin/os/OSCache.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void* HSD_AudioMalloc(size_t size)
{
    return melee_host_aligned_alloc(size, 32);
}

void HSD_AudioFree(void* pointer)
{
    melee_host_aligned_free(pointer, 32);
}

void DCInvalidateRange(void* address, u32 length)
{
    (void) address;
    (void) length;
}

void DCStoreRange(void* address, u32 length)
{
    (void) address;
    (void) length;
}

void DCStoreRangeNoSync(void* address, u32 length)
{
    (void) address;
    (void) length;
}

void ARQPostRequest(ARQRequest* request, u32 owner, u32 type, u32 priority,
                    ARQAddress source, ARQAddress dest, u32 length,
                    ARQCallback callback)
{
    request->next = NULL;
    request->owner = owner;
    request->type = type;
    request->priority = priority;
    request->source = source;
    request->dest = dest;
    request->length = length;
    request->callback = callback;
    if (callback != NULL) {
        callback(request);
    }
}

void OSReport(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

void __assert(const char* file, u32 line, const char* condition)
{
    fprintf(stderr, "HSD assertion failed at %s:%u: %s\n", file, line,
            condition);
    abort();
}

void HSD_Panic(char* file, u32 line, char* message)
{
    fprintf(stderr, "HSD panic at %s:%u: %s\n", file, line, message);
    abort();
}

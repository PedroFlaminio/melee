#include <melee_host/memory.h>

#include <new>

extern "C" void* melee_host_aligned_alloc(size_t size, size_t alignment)
{
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return nullptr;
    }
    return ::operator new(size, std::align_val_t{ alignment }, std::nothrow);
}

extern "C" void melee_host_aligned_free(void* pointer, size_t alignment)
{
    if (pointer == nullptr) {
        return;
    }
    ::operator delete(pointer, std::align_val_t{ alignment });
}

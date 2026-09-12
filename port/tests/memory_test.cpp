#include "test.hpp"

extern "C" {
#include <sysdolphin/baselib/memory.h>
}

#include <cstdint>

TEST_CASE("original HSD memory API uses a 32-byte aligned host allocation")
{
    REQUIRE(HSD_MemAlloc(0) == nullptr);
    void* allocation = HSD_MemAlloc(37);
    REQUIRE(allocation != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(allocation) % 32 == 0);
    HSD_Free(allocation);
    HSD_Free(nullptr);
}

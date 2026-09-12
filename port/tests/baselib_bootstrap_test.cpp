#include "test.hpp"

#include <melee_host/baselib.h>

extern "C" {
#include <sysdolphin/baselib/mtx.h>
}

#include <cstdint>

TEST_CASE("headless baselib bootstrap initializes native vector and matrix pools")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);
    void* vector = HSD_VecAlloc();
    void* matrix = HSD_MtxAlloc();
    REQUIRE(vector != nullptr);
    REQUIRE(matrix != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(vector) % 4 == 0);
    REQUIRE(reinterpret_cast<std::uintptr_t>(matrix) % 4 == 0);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == 1);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_MtxGetAllocData()) == 1);
    HSD_VecFree(vector);
    HSD_MtxFree(matrix);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == 0);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_MtxGetAllocData()) == 0);
}

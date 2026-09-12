#include "test.hpp"

#include <melee_host/baselib.h>

extern "C" {
#include <sysdolphin/baselib/id.h>
}

TEST_CASE("native HSD ID table inserts replaces and removes colliding IDs")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);

    int first = 10;
    int second = 20;
    int replacement = 30;
    s32 found = 0;

    // 1 and 102 deliberately share the same bucket in the original table.
    HSD_IDInsertToTable(nullptr, 1, &first);
    HSD_IDInsertToTable(nullptr, 102, &second);
    REQUIRE(HSD_IDGetData(1, &found) == &first);
    REQUIRE(found == 1);
    REQUIRE(HSD_IDGetData(102, &found) == &second);
    REQUIRE(found == 1);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_IDGetAllocData()) == 2);

    HSD_IDInsertToTable(nullptr, 1, &replacement);
    REQUIRE(HSD_IDGetData(1, &found) == &replacement);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_IDGetAllocData()) == 2);

    HSD_IDRemoveByIDFromTable(nullptr, 1);
    HSD_IDRemoveByIDFromTable(nullptr, 102);
    REQUIRE(HSD_IDGetData(1, &found) == nullptr);
    REQUIRE(found == 0);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_IDGetAllocData()) == 0);
}

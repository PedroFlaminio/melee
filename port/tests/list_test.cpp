#include "test.hpp"

extern "C" {
#include <sysdolphin/baselib/list.h>
}

TEST_CASE("original HSD single-linked list runs on the native object allocator")
{
    HSD_ListInitAllocData();
    int first_value = 11;
    int second_value = 22;
    HSD_SList* list = HSD_SListAllocAndPrepend(nullptr, &first_value);
    list = HSD_SListAllocAndAppend(list, &second_value);

    REQUIRE(list != nullptr);
    REQUIRE(list->data == &first_value);
    REQUIRE(list->next != nullptr);
    REQUIRE(list->next->data == &second_value);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_SListGetAllocData()) == 2);

    list = HSD_SListRemove(list);
    REQUIRE(list != nullptr);
    REQUIRE(list->data == &second_value);
    list = HSD_SListRemove(list);
    REQUIRE(list == nullptr);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_SListGetAllocData()) == 0);
    REQUIRE(HSD_ObjAllocGetFreed(HSD_SListGetAllocData()) == 2);
}

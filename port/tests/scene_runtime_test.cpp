#include "test.hpp"

#include "hsd_include.hpp"

#include <melee_host/scene_runtime.h>

MELEE_HOST_TEST_HSD_BEGIN
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
MELEE_HOST_TEST_HSD_END

#include <string>
#include <vector>

namespace {

struct ProcCounter {
    int calls = 0;
};

void count_call(void* user_data)
{
    static_cast<ProcCounter*>(user_data)->calls += 1;
}

std::vector<std::string>& order_log()
{
    static std::vector<std::string> log;
    return log;
}

void log_call(void* user_data)
{
    order_log().push_back(static_cast<const char*>(user_data));
}

/* The original library exposes no "current gobj" accessor, so the removal
 * tests keep the handle next to the counter. */
struct SelfRemover {
    MeleeHostSceneObject object = 0;
    int calls = 0;
};

void remove_self(void* user_data)
{
    auto* state = static_cast<SelfRemover*>(user_data);
    state->calls += 1;
    if (state->calls == 1) {
        REQUIRE(melee_host_scene_runtime_remove_object(state->object) ==
                MELEE_HOST_OK);
    }
}

} // namespace

TEST_CASE("native HSD GObj runtime boots with Melee's priority ceilings")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    REQUIRE(HSD_GObjLibInitData.gproc_pri_max ==
            MELEE_HOST_SCENE_PROC_PRIORITY_MAX);
    REQUIRE(HSD_GObjLibInitData.p_link_max == MELEE_HOST_SCENE_P_LINK_MAX);
    REQUIRE(HSD_GObjLibInitData.gx_link_max == MELEE_HOST_SCENE_GX_LINK_MAX);
    REQUIRE(HSD_GObjLibInitData.unk_2 != nullptr);

    // HSD_GObj_80391260 registers camera, light, joint and fog in that order.
    REQUIRE(HSD_GObj_CameraKind == 0);
    REQUIRE(HSD_GObj_LightKind == 1);
    REQUIRE(HSD_GObj_JObjKind == 2);
    REQUIRE(HSD_GObj_FogKind == 3);

    MeleeHostSceneRuntimeStats stats{};
    REQUIRE(melee_host_scene_runtime_stats(&stats) == MELEE_HOST_OK);
    const mh_u64 before = stats.frame_count;
    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_stats(&stats) == MELEE_HOST_OK);
    REQUIRE(stats.frame_count == before + 1);
}

TEST_CASE("host scene objects run one process per frame")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    MeleeHostSceneRuntimeStats before{};
    REQUIRE(melee_host_scene_runtime_stats(&before) == MELEE_HOST_OK);

    ProcCounter counter;
    MeleeHostSceneObject object = 0;
    REQUIRE(melee_host_scene_runtime_add_object(0x100, 0, 0, 0, count_call,
                                                &counter,
                                                &object) == MELEE_HOST_OK);
    REQUIRE(object != 0);

    MeleeHostSceneRuntimeStats after{};
    REQUIRE(melee_host_scene_runtime_stats(&after) == MELEE_HOST_OK);
    REQUIRE(after.objects_live == before.objects_live + 1);
    REQUIRE(after.procs_live == before.procs_live + 1);

    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(counter.calls == 1);
    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(counter.calls == 3);

    bool live = false;
    REQUIRE(melee_host_scene_runtime_object_is_live(object, &live) ==
            MELEE_HOST_OK);
    REQUIRE(live);

    REQUIRE(melee_host_scene_runtime_remove_object(object) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_object_is_live(object, &live) ==
            MELEE_HOST_OK);
    REQUIRE(!live);
    // The handle is stale now, so a second removal must be rejected.
    REQUIRE(melee_host_scene_runtime_remove_object(object) ==
            MELEE_HOST_INVALID_ARGUMENT);

    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(counter.calls == 3);

    REQUIRE(melee_host_scene_runtime_stats(&after) == MELEE_HOST_OK);
    REQUIRE(after.objects_live == before.objects_live);
    REQUIRE(after.procs_live == before.procs_live);
}

TEST_CASE("process priority orders the frame ahead of creation order")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);
    order_log().clear();

    MeleeHostSceneObject late = 0;
    MeleeHostSceneObject early = 0;
    MeleeHostSceneObject middle = 0;
    // Created last-to-first by priority to prove the queue sorts by s_link.
    REQUIRE(melee_host_scene_runtime_add_object(
                0x200, 0, 0, 5, log_call, const_cast<char*>("late"), &late) ==
            MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_add_object(
                0x201, 0, 0, 0, log_call, const_cast<char*>("early"),
                &early) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_add_object(
                0x202, 0, 0, 2, log_call, const_cast<char*>("middle"),
                &middle) == MELEE_HOST_OK);

    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(order_log().size() == 3);
    REQUIRE(order_log()[0] == "early");
    REQUIRE(order_log()[1] == "middle");
    REQUIRE(order_log()[2] == "late");

    REQUIRE(melee_host_scene_runtime_remove_object(late) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_remove_object(early) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_remove_object(middle) == MELEE_HOST_OK);
}

TEST_CASE("objects sharing a process priority keep creation order")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);
    order_log().clear();

    MeleeHostSceneObject first = 0;
    MeleeHostSceneObject second = 0;
    REQUIRE(melee_host_scene_runtime_add_object(
                0x210, 0, 0, 4, log_call, const_cast<char*>("first"),
                &first) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_add_object(
                0x211, 0, 0, 4, log_call, const_cast<char*>("second"),
                &second) == MELEE_HOST_OK);

    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(order_log().size() == 2);
    REQUIRE(order_log()[0] == "first");
    REQUIRE(order_log()[1] == "second");

    REQUIRE(melee_host_scene_runtime_remove_object(first) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_remove_object(second) == MELEE_HOST_OK);
}

TEST_CASE("paused p_links suspend their processes without removing them")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    ProcCounter running;
    ProcCounter paused;
    MeleeHostSceneObject running_object = 0;
    MeleeHostSceneObject paused_object = 0;
    REQUIRE(melee_host_scene_runtime_add_object(
                0x300, 1, 0, 0, count_call, &running,
                &running_object) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_add_object(
                0x301, 2, 0, 0, count_call, &paused,
                &paused_object) == MELEE_HOST_OK);

    REQUIRE(melee_host_scene_runtime_set_paused_links(1ULL << 2) ==
            MELEE_HOST_OK);
    mh_u64 mask = 0;
    REQUIRE(melee_host_scene_runtime_get_paused_links(&mask) ==
            MELEE_HOST_OK);
    REQUIRE(mask == (1ULL << 2));

    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(running.calls == 1);
    REQUIRE(paused.calls == 0);

    REQUIRE(melee_host_scene_runtime_set_paused_links(0) == MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(running.calls == 2);
    REQUIRE(paused.calls == 1);

    REQUIRE(melee_host_scene_runtime_remove_object(running_object) ==
            MELEE_HOST_OK);
    REQUIRE(melee_host_scene_runtime_remove_object(paused_object) ==
            MELEE_HOST_OK);
}

TEST_CASE("a process that frees its own object is released after it returns")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    MeleeHostSceneRuntimeStats before{};
    REQUIRE(melee_host_scene_runtime_stats(&before) == MELEE_HOST_OK);

    SelfRemover state;
    ProcCounter witness;
    MeleeHostSceneObject witness_object = 0;
    REQUIRE(melee_host_scene_runtime_add_object(0x400, 0, 0, 0, remove_self,
                                                &state, &state.object) ==
            MELEE_HOST_OK);
    // Queued behind the self-removing object to prove the frame keeps walking.
    REQUIRE(melee_host_scene_runtime_add_object(
                0x401, 0, 0, 1, count_call, &witness,
                &witness_object) == MELEE_HOST_OK);

    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(state.calls == 1);
    REQUIRE(witness.calls == 1);

    bool live = true;
    REQUIRE(melee_host_scene_runtime_object_is_live(state.object, &live) ==
            MELEE_HOST_OK);
    REQUIRE(!live);

    REQUIRE(melee_host_scene_runtime_run_frame() == MELEE_HOST_OK);
    REQUIRE(state.calls == 1);
    REQUIRE(witness.calls == 2);

    REQUIRE(melee_host_scene_runtime_remove_object(witness_object) ==
            MELEE_HOST_OK);
    MeleeHostSceneRuntimeStats after{};
    REQUIRE(melee_host_scene_runtime_stats(&after) == MELEE_HOST_OK);
    REQUIRE(after.objects_live == before.objects_live);
    REQUIRE(after.procs_live == before.procs_live);
}

TEST_CASE("the runtime rejects links and priorities above Melee's ceilings")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    ProcCounter counter;
    MeleeHostSceneObject object = 0;
    REQUIRE(melee_host_scene_runtime_add_object(
                0, MELEE_HOST_SCENE_P_LINK_MAX + 1, 0, 0, count_call,
                &counter, &object) == MELEE_HOST_INVALID_ARGUMENT);
    REQUIRE(melee_host_scene_runtime_add_object(
                0, 0, 0, MELEE_HOST_SCENE_PROC_PRIORITY_MAX + 1, count_call,
                &counter, &object) == MELEE_HOST_INVALID_ARGUMENT);
    REQUIRE(melee_host_scene_runtime_add_object(0, 0, 0, 0, nullptr, &counter,
                                                &object) ==
            MELEE_HOST_INVALID_ARGUMENT);
    REQUIRE(melee_host_scene_runtime_add_object(0, 0, 0, 0, count_call,
                                                &counter, nullptr) ==
            MELEE_HOST_INVALID_ARGUMENT);
    REQUIRE(object == 0);
}

TEST_CASE("original GX link insertion keeps render priority order")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    HSD_GObj* low = GObj_Create(0x500, 3, 0);
    HSD_GObj* high = GObj_Create(0x501, 3, 0);
    HSD_GObj* mid = GObj_Create(0x502, 3, 0);
    REQUIRE(low != nullptr);
    REQUIRE(high != nullptr);
    REQUIRE(mid != nullptr);

    GObj_SetupGXLink(low, nullptr, 5, 1);
    GObj_SetupGXLink(high, nullptr, 5, 9);
    GObj_SetupGXLink(mid, nullptr, 5, 4);

    REQUIRE(HSD_GObjGXLinkHead[5] == low);
    REQUIRE(low->next_gx == mid);
    REQUIRE(mid->next_gx == high);
    REQUIRE(high->next_gx == nullptr);
    REQUIRE(high->prev_gx == mid);
    REQUIRE(mid->prev_gx == low);
    REQUIRE(low->prev_gx == nullptr);

    HSD_GObjFree(mid);
    REQUIRE(low->next_gx == high);
    REQUIRE(high->prev_gx == low);

    HSD_GObjFree(low);
    HSD_GObjFree(high);
    REQUIRE(HSD_GObjGXLinkHead[5] == nullptr);
}

TEST_CASE("original p_link insertion orders objects by priority")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    HSD_GObj* first = GObj_Create(0x600, 7, 2);
    HSD_GObj* second = GObj_Create(0x601, 7, 8);
    HSD_GObj* between = GObj_Create(0x602, 7, 5);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(between != nullptr);

    REQUIRE(HSD_GObjPLinkHead[7] == first);
    REQUIRE(first->next == between);
    REQUIRE(between->next == second);
    REQUIRE(second->next == nullptr);

    REQUIRE(HSD_GObjObject_80390A3C(0x602, 7) == between);
    REQUIRE(HSD_GObjObject_80390A3C(0x603, 7) == nullptr);

    HSD_GObjFree(first);
    HSD_GObjFree(between);
    HSD_GObjFree(second);
    REQUIRE(HSD_GObjPLinkHead[7] == nullptr);
}

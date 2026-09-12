#include "hsd_include.hpp"
#include "test.hpp"

#include <melee_host/os_heap.h>

/* dolphin/os.h is not C++ clean, so the test pulls in only the allocator
 * header and declares the two memory-size queries it checks. */
MELEE_HOST_TEST_HSD_BEGIN
#include <dolphin/os/OSAlloc.h>
u32 OSGetPhysicalMemSize(void);
u32 OSGetConsoleSimulatedMemSize(void);
MELEE_HOST_TEST_HSD_END

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

constexpr std::size_t kTestArena = 1 << 20;

bool aligned32(const void* pointer)
{
    return (reinterpret_cast<std::uintptr_t>(pointer) & 31U) == 0;
}

} // namespace

TEST_CASE("the original OSAlloc heap runs on a host arena")
{
    REQUIRE(melee_host_os_heap_init(kTestArena, 4) == MELEE_HOST_OK);

    MeleeHostOsHeapStats stats{};
    REQUIRE(melee_host_os_heap_stats(&stats) == MELEE_HOST_OK);
    REQUIRE(stats.initialized);
    REQUIRE(stats.arena_bytes == kTestArena);
    REQUIRE(stats.heap_count == 4);
    REQUIRE(stats.current_heap >= 0);
    REQUIRE(stats.free_bytes > 0);

    // The arena is spent once the heap owns it.
    REQUIRE(stats.arena_exhausted);

    REQUIRE(melee_host_os_heap_shutdown() == MELEE_HOST_OK);
    REQUIRE(melee_host_os_heap_shutdown() == MELEE_HOST_NOT_READY);
}

TEST_CASE("heap allocations are 32-byte aligned and survive a 64-bit address")
{
    REQUIRE(melee_host_os_heap_init(kTestArena, 2) == MELEE_HOST_OK);

    void* first = OSAllocFromHeap(__OSCurrHeap, 100);
    void* second = OSAllocFromHeap(__OSCurrHeap, 4096);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first != second);
    REQUIRE(aligned32(first));
    REQUIRE(aligned32(second));

    // A host address usually sits above 4 GB, which the original 32-bit
    // pointer arithmetic would have truncated.
    REQUIRE(OSReferentSize(first) >= 100);
    REQUIRE(OSReferentSize(second) >= 4096);

    // Writing the whole block must not disturb its neighbour.
    auto* bytes = static_cast<unsigned char*>(second);
    for (std::size_t i = 0; i < 4096; ++i) {
        bytes[i] = static_cast<unsigned char>(i & 0xFF);
    }
    REQUIRE(OSReferentSize(first) >= 100);

    OSFreeToHeap(__OSCurrHeap, first);
    OSFreeToHeap(__OSCurrHeap, second);
    REQUIRE(melee_host_os_heap_shutdown() == MELEE_HOST_OK);
}

TEST_CASE("freeing returns every byte the heap started with")
{
    REQUIRE(melee_host_os_heap_init(kTestArena, 1) == MELEE_HOST_OK);

    MeleeHostOsHeapStats before{};
    REQUIRE(melee_host_os_heap_stats(&before) == MELEE_HOST_OK);

    std::vector<void*> blocks;
    for (int i = 0; i < 64; ++i) {
        void* block = OSAllocFromHeap(__OSCurrHeap, 1024);
        REQUIRE(block != nullptr);
        blocks.push_back(block);
    }

    MeleeHostOsHeapStats busy{};
    REQUIRE(melee_host_os_heap_stats(&busy) == MELEE_HOST_OK);
    REQUIRE(busy.free_bytes < before.free_bytes);

    for (void* block : blocks) {
        OSFreeToHeap(__OSCurrHeap, block);
    }

    MeleeHostOsHeapStats after{};
    REQUIRE(melee_host_os_heap_stats(&after) == MELEE_HOST_OK);
    // OSCheckHeap walks both lists, so an equal total means the free list was
    // coalesced back into the single original cell.
    REQUIRE(after.free_bytes == before.free_bytes);

    REQUIRE(melee_host_os_heap_shutdown() == MELEE_HOST_OK);
}

TEST_CASE("an over-subscribed heap refuses instead of overrunning the arena")
{
    REQUIRE(melee_host_os_heap_init(kTestArena, 1) == MELEE_HOST_OK);

    // Larger than the whole arena, so there is no cell that can hold it.
    REQUIRE(OSAllocFromHeap(__OSCurrHeap, kTestArena * 2) == nullptr);

    MeleeHostOsHeapStats stats{};
    REQUIRE(melee_host_os_heap_stats(&stats) == MELEE_HOST_OK);
    REQUIRE(stats.free_bytes > 0);

    REQUIRE(melee_host_os_heap_shutdown() == MELEE_HOST_OK);
}

TEST_CASE("the arena facade validates its own arguments")
{
    REQUIRE(melee_host_os_heap_init(kTestArena, 0) ==
            MELEE_HOST_INVALID_ARGUMENT);
    // Too small to hold the descriptor array plus a usable heap.
    REQUIRE(melee_host_os_heap_init(16, 4) == MELEE_HOST_INVALID_ARGUMENT);
    REQUIRE(melee_host_os_heap_stats(nullptr) == MELEE_HOST_INVALID_ARGUMENT);

    MeleeHostOsHeapStats stats{};
    REQUIRE(melee_host_os_heap_stats(&stats) == MELEE_HOST_OK);
    REQUIRE(!stats.initialized);
}

TEST_CASE("memory size queries report the console the host stands in for")
{
    REQUIRE(OSGetPhysicalMemSize() ==
            MELEE_HOST_OS_PHYSICAL_MEMORY_BYTES);
    REQUIRE(OSGetConsoleSimulatedMemSize() ==
            MELEE_HOST_OS_PHYSICAL_MEMORY_BYTES);
}

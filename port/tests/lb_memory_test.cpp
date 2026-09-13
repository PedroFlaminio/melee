#include "test.hpp"

#include "hsd_include.hpp"

#include <melee_host/memory.h>

MELEE_HOST_TEST_HSD_BEGIN
#include <dolphin/ar.h>
#include <melee/lb/lbmemory.h>
MELEE_HOST_TEST_HSD_END

#include <array>
#include <cstddef>
#include <cstdint>

TEST_CASE("host ARAM is a stack, as ARAlloc and ARFree are in the SDK")
{
    const u32 first = ARAlloc(0x40);
    const u32 second = ARAlloc(0x20);
    REQUIRE(second == first + 0x40);
    u32 length = 0;
    REQUIRE(ARFree(&length) == second);
    REQUIRE(length == 0x20);
    REQUIRE(ARFree(nullptr) == first);
    REQUIRE(ARGetSize() == 16U * 1024U * 1024U);
}

TEST_CASE("lbMemory places blocks at full host addresses")
{
    lbMemory_8001564C();
    constexpr std::size_t kSize = std::size_t{ 1 } << 20U;
    auto* const block =
        static_cast<char*>(melee_host_aligned_alloc(kSize, 32));
    REQUIRE(block != nullptr);
    Handle* const heap = lbMemory_80014E24(block, block + kSize);
    REQUIRE(heap != nullptr);
    REQUIRE(lbMemory_80014F7C(heap) == kSize);

    // Sizes round up to 32 bytes, and a block goes where it leaves least.
    Handle* const first = lbMemory_80014FC8(heap, 100);
    REQUIRE(first->x4_lo == block);
    REQUIRE(reinterpret_cast<std::uintptr_t>(first->x8_hi) == 0x80);
    Handle* const second = lbMemory_80014FC8(heap, 0x40);
    REQUIRE(second->x4_lo == block + 0x80);
    lbMemFreeToHeap(heap, first->x4_lo);

    // The hole at the front leaves least for a small block...
    Handle* const small = lbMemory_80014FC8(heap, 0x20);
    REQUIRE(small->x4_lo == block);
    // ...and a block too big for it lands after the second one, at an address
    // computed from that block's own.  Truncating addresses to 32 bits is
    // what would move it.
    Handle* const large = lbMemory_80014FC8(heap, 0x100);
    REQUIRE(large->x4_lo == block + 0xC0);
    REQUIRE(lbMemory_80014F7C(heap) == kSize - 0x20 - 0x40 - 0x100);

    lbMemory_80014EEC(heap);
    melee_host_aligned_free(block, 32);
}

TEST_CASE("lbMemory links its heap handle pool without PowerPC offsets")
{
    lbMemory_8001564C();
    // The pool has six handles, and the initialiser takes one for ARAM.
    std::array<char, 64> arena{};
    std::array<Handle*, 5> heaps{};
    for (Handle*& heap : heaps) {
        heap = lbMemory_80014E24(arena.data(), arena.data() + arena.size());
        REQUIRE(heap != nullptr);
        REQUIRE(heap->x4_lo == arena.data());
    }
    for (std::size_t index = 1; index < heaps.size(); ++index) {
        REQUIRE(heaps[index] != heaps[index - 1]);
    }
    for (Handle* const heap : heaps) {
        lbMemory_80014EEC(heap);
    }
}

#include "test.hpp"

#include <array>
#include <cstddef>
#include <cstdio>

extern "C" {
int melee_host_command_layout_failures(char* message, std::size_t size);
int melee_host_command_layout_cases(void);
}

TEST_CASE("command stream fields read on the host from the bits MWCC gives them")
{
    // Every named field of the commands in lb/types.h and of the color overlay
    // union, packed into words where MWCC lays it out and read back through
    // port/src/game/host_command_layout.h (command_layout_check.c, generated
    // with it).
    std::array<char, 256> message{};
    REQUIRE(melee_host_command_layout_cases() > 0);
    const int failures =
        melee_host_command_layout_failures(message.data(), message.size());
    if (failures != 0) {
        std::fprintf(stderr, "%d command fields misread; first: %s\n",
                     failures, message.data());
    }
    REQUIRE(failures == 0);
}

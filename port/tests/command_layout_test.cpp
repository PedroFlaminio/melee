#include "test.hpp"

#include <array>
#include <cstddef>
#include <cstdio>

extern "C" {
int melee_host_command_layout_failures(char* message, std::size_t size);
int melee_host_command_layout_cases(void);
void melee_host_test_read_script_event(unsigned word, unsigned* opcode,
                                       unsigned* value1);
}

TEST_CASE("fighter script opcode read from the word's top six bits")
{
    // ftAction_80073240 dispatches on gmScriptEventDefault (ft/types.h), which
    // MWCC lays out with the opcode in the top six bits.  Read from the low
    // bits instead, a four-frame timer (0x04000004) runs as an Execute Loop
    // with nothing pushed.
    unsigned opcode = 0;
    unsigned value1 = 0;
    melee_host_test_read_script_event((4u << 26) | 0x2ABCDEu, &opcode, &value1);
    REQUIRE(opcode == 4);
    REQUIRE(value1 == 0x2ABCDEu);
    melee_host_test_read_script_event(0xFC000001u, &opcode, &value1);
    REQUIRE(opcode == 63);
    REQUIRE(value1 == 1);
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

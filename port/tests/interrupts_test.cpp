#include "test.hpp"

extern "C" {
#include <melee_host/dolphin_os.h>
}

TEST_CASE("host interrupt facade preserves nested restore semantics")
{
    static_cast<void>(OSEnableInterrupts());
    const BOOL outer = OSDisableInterrupts();
    const BOOL inner = OSDisableInterrupts();
    REQUIRE(outer == TRUE);
    REQUIRE(inner == FALSE);
    REQUIRE(OSRestoreInterrupts(inner) == FALSE);
    REQUIRE(OSRestoreInterrupts(outer) == FALSE);
    REQUIRE(OSDisableInterrupts() == TRUE);
    static_cast<void>(OSEnableInterrupts());
}


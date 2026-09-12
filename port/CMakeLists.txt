add_library(melee_host STATIC
    src/assets/hsd_archive.cpp
    src/assets/hsd_runtime_archive.cpp
    src/assets/schemas/db_common.cpp
    src/assets/schemas/scene_graphics.cpp
    src/assets/virtual_disc.cpp
    src/gx/command_recorder.cpp
    src/host.cpp
    src/os/clock.cpp
    src/os/baselib_support.c
    src/os/dolphin_mtx.c
    src/os/dolphin_time.cpp
    src/os/hsd_spline.c
    src/os/interrupts.c
    src/os/memory.cpp
    src/os/pad.cpp
    src/os/dvd.cpp
)

target_include_directories(melee_host
    PUBLIC include
    PRIVATE
        src
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/extern/dolphin/include
)

target_compile_definitions(melee_host PUBLIC MELEE_HOST=1)

function(melee_host_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(MELEE_HOST_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wconversion
            -Wpedantic
            -Wshadow
        )
        if(MELEE_HOST_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(melee_host_set_legacy_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W3)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wno-deprecated-non-prototype
            -Wno-sign-compare
            -Wno-sign-conversion
        )
    endif()
endfunction()

function(melee_host_enable_sanitizers target)
    if(MELEE_HOST_ENABLE_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        else()
            message(FATAL_ERROR "Sanitizers are not configured for this compiler")
        endif()
    endif()
endfunction()

melee_host_set_warnings(melee_host)
melee_host_enable_sanitizers(melee_host)

# The first native-compiled slice of the original game. Sources are added here
# only after they compile without changing their PowerPC matching behavior.
add_library(melee_game_core STATIC
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbtime.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbvector.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/aobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/controller.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/devcom.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/fobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/id.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/memory.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/objalloc.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/list.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/mtx.c
    src/game/baselib_bootstrap.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/random.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/pad/PadClamp.c
)
target_compile_definitions(melee_game_core PUBLIC MELEE_HOST=1)
target_include_directories(melee_game_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/extern/dolphin/include
)
melee_host_set_legacy_warnings(melee_game_core)
melee_host_enable_sanitizers(melee_game_core)
target_link_libraries(melee_game_core PUBLIC melee_host)
if(NOT MSVC)
    target_compile_options(melee_game_core PRIVATE -ffunction-sections -fdata-sections)
endif()

add_executable(melee-pc src/main.cpp)
target_link_libraries(melee-pc PRIVATE melee_game_core)
target_include_directories(melee-pc PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/extern/dolphin/include
)
melee_host_set_warnings(melee-pc)
melee_host_enable_sanitizers(melee-pc)

if(MELEE_HOST_BUILD_TESTS)
    enable_testing()

    add_executable(melee-host-tests
        tests/game_core_test.cpp
        tests/aobj_test.cpp
        tests/gx_command_recorder_test.cpp
        tests/hsd_archive_test.cpp
        tests/hsd_schema_test.cpp
        tests/interrupts_test.cpp
        tests/virtual_disc_test.cpp
        tests/dvd_test.cpp
        tests/host_test.cpp
        tests/input_test.cpp
        tests/id_test.cpp
        tests/memory_test.cpp
        tests/dolphin_mtx_test.cpp
        tests/fobj_test.cpp
        tests/list_test.cpp
        tests/baselib_bootstrap_test.cpp
        tests/pad_test.cpp
        tests/test_main.cpp
    )
    target_link_libraries(melee-host-tests PRIVATE melee_game_core)
    target_include_directories(melee-host-tests PRIVATE src tests)
    melee_host_set_warnings(melee-host-tests)
    melee_host_enable_sanitizers(melee-host-tests)
    if(UNIX AND NOT APPLE)
        target_link_options(melee-host-tests PRIVATE -Wl,--gc-sections)
    endif()

    add_test(NAME melee-host-tests COMMAND melee-host-tests)
    if(MELEE_HOST_ENABLE_SANITIZERS)
        # LeakSanitizer cannot run under the traced execution environment used
        # by some CI runners. AddressSanitizer and UBSan remain enabled.
        set_tests_properties(melee-host-tests PROPERTIES
            ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
        )
    endif()
endif()

add_library(melee_host STATIC
    src/assets/dol_image.cpp
    src/assets/hsd_archive.cpp
    src/assets/hsd_host_archive.cpp
    src/assets/hsd_materialize.cpp
    src/assets/gx_texture.cpp
    src/assets/hsd_runtime_archive.cpp
    src/assets/schemas/db_common.cpp
    src/assets/schemas/scene_graphics.cpp
    src/assets/virtual_disc.cpp
    src/gx/command_recorder.cpp
    src/gx/state_recorder.cpp
    src/gx/tev.cpp
    src/gx/view.cpp
    src/video/vi.cpp
    src/host.cpp
    src/os/clock.cpp
    src/os/absent_devices.c
    src/os/alarm.c
    src/os/baselib_support.c
    src/os/dolphin_mtx.c
    src/os/dolphin_time.cpp
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
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbanim.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbarchive.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbfile.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbheap.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lblanguage.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbmemory.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmmain.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmmain_lib.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmscene.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gm_1A36.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmvs.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmvsmelee.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmvsmode.c
    ${CMAKE_SOURCE_DIR}/src/melee/mn/mnmain.c
    ${CMAKE_SOURCE_DIR}/src/melee/gr/ground.c
    ${CMAKE_SOURCE_DIR}/src/melee/pl/player.c
    ${CMAKE_SOURCE_DIR}/src/melee/pl/plattack.c
    ${CMAKE_SOURCE_DIR}/src/melee/pl/plbonus.c
    ${CMAKE_SOURCE_DIR}/src/melee/pl/plstale.c
    ${CMAKE_SOURCE_DIR}/src/melee/pl/pl_040D.c
    ${CMAKE_SOURCE_DIR}/src/melee/ft/fighter.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/aobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/controller.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/devcom.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/fobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/class.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/object.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hash.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/cobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/displayfunc.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/dobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/fog.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/jobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/lobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/mobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/pobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/robj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/tobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/wobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/shadow.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/state.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/tev.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/texp.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/texpdag.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/bytecode.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/initialize.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/perf.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/util.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/video.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/gobj.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/gobjinit.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/gobjproc.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/gobjplink.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/gobjgxlink.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/gobjobject.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/gobjuserdata.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/id.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/memory.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/objalloc.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/list.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/mtx.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/rumble.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/spline.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/synth.c
    src/game/baselib_bootstrap.c
    src/game/local_match.c
    src/game/match_rules.c
    src/game/hsd_audio_stubs.c
    src/game/menu_native.c
    src/game/scene_runtime.c
    src/game/scene_graphics.cpp
    src/game/archive_probe.cpp
    src/game/boot_memory.c
    src/game/title_scene.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmtitle.c
    ${CMAKE_SOURCE_DIR}/src/melee/db/dbinit.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gm_1601.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gm_1A3F.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmopening.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmopeningmode.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbaudio_ax.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbdvd.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbmthp.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbspdisplay.c
    ${CMAKE_SOURCE_DIR}/src/melee/mn/mn_22EC.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/sislib.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3A64.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3A76.c
    src/game/dol_data.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/axdriver.c
    ${CMAKE_SOURCE_DIR}/src/melee/gr/stage.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmcameramode.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbsnap.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lb_0195.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lb_013B.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lb_00B0.c
    ${CMAKE_SOURCE_DIR}/src/melee/ty/toy.c
    ${CMAKE_SOURCE_DIR}/src/melee/mn/mnname.c
    ${CMAKE_SOURCE_DIR}/src/melee/mn/mnnamenew.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gm_16F1.c
    ${CMAKE_SOURCE_DIR}/src/melee/if/if_2FF2.c
    ${CMAKE_SOURCE_DIR}/src/melee/if/textdraw.c
    ${CMAKE_SOURCE_DIR}/src/melee/if/textlib_1.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3915.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/sobjlib.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3924.c
    # The title's state table, and what the scene frame loop (gm_801A4D34) and
    # its pad alarm call every frame.
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gmtitlemode.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_392C.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3933.c
    ${CMAKE_SOURCE_DIR}/src/melee/db/dbscreenshot.c
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbcardgame.c
    # The end of a game mode's routing reaches the memory card layer, and the
    # title state's exit reaches gm_1736.c.  The host's mode and scene tables
    # stand in for gmscdata.c.
    ${CMAKE_SOURCE_DIR}/src/melee/lb/lbcardnew.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3A94.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3B27.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3B2B.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_3B2E.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/hsd_4D11.c
    ${CMAKE_SOURCE_DIR}/src/melee/gm/gm_1736.c
    ${CMAKE_SOURCE_DIR}/src/melee/ty/tydisplay.c
    src/game/game_tables.c
    src/game/unported.c
    ${CMAKE_SOURCE_DIR}/src/sysdolphin/baselib/random.c
    src/os/os_heap.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/os/OSAlloc.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/os/OSArena.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/pad/PadClamp.c
    # The SDK's audio effects are arithmetic on sample buffers and run as
    # they are.
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/axfx/axfx.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/axfx/chorus.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/axfx/delay.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/axfx/reverb_hi.c
    ${CMAKE_SOURCE_DIR}/extern/dolphin/src/dolphin/axfx/reverb_std.c
)
target_compile_definitions(melee_game_core PUBLIC MELEE_HOST=1)
target_include_directories(melee_game_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/extern/dolphin/include
    PRIVATE
        src
)
melee_host_set_legacy_warnings(melee_game_core)
# Port code keeps the strict warning set even where it shares a target with
# decompiled translation units, which need the legacy relaxations.
set_source_files_properties(
    src/game/scene_graphics.cpp
    src/game/archive_probe.cpp
    PROPERTIES COMPILE_OPTIONS "-Wconversion;-Wpedantic;-Wshadow"
)
melee_host_enable_sanitizers(melee_game_core)
# The player bootstrap is a legacy translation unit with references to the
# unported fighter and stage graph.  Sanitizer global metadata retains that
# graph even when --gc-sections discards the functions.  Keep the host-facing
# boundary instrumented while this island remains independently linked.
if(MELEE_HOST_ENABLE_SANITIZERS)
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/src/melee/pl/player.c
        ${CMAKE_SOURCE_DIR}/src/melee/pl/plattack.c
        ${CMAKE_SOURCE_DIR}/src/melee/pl/plbonus.c
        ${CMAKE_SOURCE_DIR}/src/melee/pl/plstale.c
        ${CMAKE_SOURCE_DIR}/src/melee/pl/pl_040D.c
        ${CMAKE_SOURCE_DIR}/src/melee/gr/ground.c
        # Both refer to the unported preload cache in lbdvd.c.
        ${CMAKE_SOURCE_DIR}/src/melee/lb/lbarchive.c
        ${CMAKE_SOURCE_DIR}/src/melee/lb/lbfile.c
        # The title screen's path reaches these, and each also names menus,
        # stages or card code the host does not build yet.
        ${CMAKE_SOURCE_DIR}/src/melee/gm/gm_1601.c
        ${CMAKE_SOURCE_DIR}/src/melee/gm/gmcameramode.c
        ${CMAKE_SOURCE_DIR}/src/melee/gm/gmmain_lib.c
        ${CMAKE_SOURCE_DIR}/src/melee/gm/gmopeningmode.c
        ${CMAKE_SOURCE_DIR}/src/melee/lb/lbaudio_ax.c
        ${CMAKE_SOURCE_DIR}/src/melee/mn/mnmain.c
        ${CMAKE_SOURCE_DIR}/src/melee/mn/mnname.c
        ${CMAKE_SOURCE_DIR}/src/melee/mn/mnnamenew.c
        PROPERTIES COMPILE_OPTIONS "-fno-sanitize=address,undefined"
    )
endif()
# ground.c's stage table names every stage.  Stages outside the build are weak
# references, so their entries are NULL, which the game reads as a stage
# without data.  Appended so the sanitizer options above are kept.
set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/melee/gr/ground.c APPEND PROPERTY
    COMPILE_OPTIONS "-include;${CMAKE_CURRENT_SOURCE_DIR}/src/game/host_weak_stages.h"
)
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
if(UNIX AND NOT APPLE)
    target_link_options(melee-pc PRIVATE -Wl,--gc-sections)
endif()

option(MELEE_HOST_BUILD_SDL_RENDERER "Build the SDL/OpenGL geometry preview" ON)
if(MELEE_HOST_BUILD_SDL_RENDERER)
    find_package(SDL3 CONFIG QUIET)
    find_package(OpenGL QUIET)
    if(SDL3_FOUND AND OpenGL_FOUND)
        target_sources(melee-pc PRIVATE src/render/sdl_gl_renderer.cpp)
        target_link_libraries(melee-pc PRIVATE SDL3::SDL3 OpenGL::GL)
        target_compile_definitions(melee-pc PRIVATE MELEE_HOST_SDL_RENDERER=1)
    else()
        message(STATUS "SDL3/OpenGL not found; geometry preview is disabled")
    endif()
endif()

if(MELEE_HOST_BUILD_TESTS)
    enable_testing()

    add_executable(melee-host-tests
        tests/game_core_test.cpp
        tests/aobj_test.cpp
        tests/gx_command_recorder_test.cpp
        tests/hsd_archive_test.cpp
        tests/gx_texture_test.cpp
        tests/hsd_schema_test.cpp
        tests/interrupts_test.cpp
        tests/os_clock_test.cpp
        tests/virtual_disc_test.cpp
        tests/dvd_test.cpp
        tests/host_test.cpp
        tests/input_test.cpp
        tests/id_test.cpp
        tests/memory_test.cpp
        tests/dolphin_mtx_test.cpp
        tests/fobj_test.cpp
        tests/list_test.cpp
        tests/local_match_test.cpp
        tests/scene_runtime_test.cpp
        tests/spline_test.cpp
        tests/synth_test.cpp
        tests/gx_state_test.cpp
        tests/gx_tev_test.cpp
        tests/gx_view_test.cpp
        tests/video_test.cpp
        tests/os_heap_test.cpp
        tests/hsd_graphics_test.cpp
        tests/hsd_materialize_test.cpp
        tests/hsd_host_archive_test.cpp
        tests/lb_memory_test.cpp
        tests/dol_test.cpp
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
    # Local game assets are intentionally untracked.  When available, retain
    # an end-to-end regression for the TLUT material path without making CI
    # depend on proprietary data.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/GmRegClr.dat")
        add_test(
            NAME melee-host-tlut-asset
            COMMAND melee-pc --inspect-pobj
                    ${CMAKE_SOURCE_DIR}/assets-local/GmRegClr.dat
                    ScGamRegClear_scene_data
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_tests_properties(melee-host-tlut-asset PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    # The same data, loaded through the original object layer instead of the
    # read-only schema.  One menu scene and one character model, which is the
    # deeper tree and the one carrying envelopes and shape sets.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/GmPause.dat")
        add_test(
            NAME melee-host-scene-load-asset
            COMMAND melee-pc --render-scene
                    ${CMAKE_SOURCE_DIR}/assets-local/GmPause.dat
                    ScGamPause_scene_data
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_tests_properties(melee-host-scene-load-asset PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/PlMrNr.dat")
        add_test(
            NAME melee-host-model-load-asset
            COMMAND melee-pc --render-joint
                    ${CMAKE_SOURCE_DIR}/assets-local/PlMrNr.dat
                    PlyMario5K_Share_joint
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_tests_properties(melee-host-model-load-asset PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    # The exact symbol list gmTitle_801A1AC0 hands lbArchive_LoadSymbols, which
    # makes every one of them fatal if missing.  The US file is the one the
    # game opens for an English setting.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/GmTtAll.usd")
        add_test(
            NAME melee-host-title-archive-asset
            COMMAND melee-pc --load-archive
                    ${CMAKE_SOURCE_DIR}/assets-local/GmTtAll.usd
                    TtlMoji_Top_joint TtlMoji_Top_animjoint
                    TtlMoji_Top_matanim_joint TtlMoji_Top_shapeanim_joint
                    ScTitle_cam_int1_camera ScTitle_scene_lights ScTitle_fog
                    TtlBg_Top_joint TtlBg_Top_animjoint TtlBg_Top_matanim_joint
                    TtlBg_Top_shapeanim_joint TitleMark_sobjdesc
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_tests_properties(melee-host-title-archive-asset PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    # The same archive through the game's own loader: gmMain's memory sequence,
    # lbArchive_LoadSymbols, and lbFile over devcom and the host DVD.  It runs
    # in its own process because it moves the OS arena.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/dvd-index.bin")
        add_test(
            NAME melee-host-boot-title-archive-asset
            COMMAND melee-pc --boot-title-archive
                    ${CMAKE_SOURCE_DIR}/assets-local
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_tests_properties(melee-host-boot-title-archive-asset PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    # The title screen entered by its own code: the scene manager's setup
    # (gm_801A4BD4) and gm_Scene_Title_OnEnter, after gmMain's boot.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/sys/main.dol")
        add_test(
            NAME melee-host-title-scene-asset
            COMMAND melee-pc --boot-title-scene
                    ${CMAKE_SOURCE_DIR}/assets-local
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_tests_properties(melee-host-title-scene-asset PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
        # The title screen's own frame loop, gm_801A4D34, on a frozen OS clock:
        # the scene has to draw its frames and leave by itself when its
        # countdown and 600 frames run out.
        add_test(
            NAME melee-host-title-frames-asset
            COMMAND melee-pc --run-title-scene
                    ${CMAKE_SOURCE_DIR}/assets-local
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_tests_properties(melee-host-title-frames-asset PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    # A model and its animation from the same archive, advanced far enough that
    # a joint actually travels.  This is the end-to-end check that keyframe
    # streams survived translation and the original interpreter read them.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/GmTtAll.dat")
        add_test(
            NAME melee-host-animation-asset
            COMMAND melee-pc --animate-joint
                    ${CMAKE_SOURCE_DIR}/assets-local/GmTtAll.dat
                    TtlMoji_Top_joint
                    ${CMAKE_SOURCE_DIR}/assets-local/GmTtAll.dat
                    TtlMoji_Top_animjoint TtlMoji_Top_matanim_joint 200
        )
        set_tests_properties(melee-host-animation-asset PROPERTIES
            PASS_REGULAR_EXPRESSION "AObj frame: 199 of 1600"
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_property(TEST melee-host-animation-asset APPEND PROPERTY
                ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    # A character's own walk cycle, out of the file that holds one archive per
    # action.  This is the whole chain: the concatenated container, the
    # FigaTree format, and the original applier driving a real skeleton.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets-local/PlMrAJ.dat")
        add_test(
            NAME melee-host-character-animation-asset
            COMMAND melee-pc --animate-named
                    ${CMAKE_SOURCE_DIR}/assets-local/PlMrNr.dat
                    PlyMario5K_Share_joint
                    ${CMAKE_SOURCE_DIR}/assets-local/PlMrAJ.dat
                    PlyMario5K_Share_ACTION_WalkMiddle_figatree 30
        )
        set_tests_properties(melee-host-character-animation-asset PROPERTIES
            PASS_REGULAR_EXPRESSION "AObj frame 29 of 45"
        )
        if(MELEE_HOST_ENABLE_SANITIZERS)
            set_property(TEST melee-host-character-animation-asset APPEND
                PROPERTY ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
            )
        endif()
    endif()
    if(MELEE_HOST_ENABLE_SANITIZERS)
        # LeakSanitizer cannot run under the traced execution environment used
        # by some CI runners. AddressSanitizer and UBSan remain enabled.
        set_tests_properties(melee-host-tests PROPERTIES
            ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
        )
    endif()
endif()

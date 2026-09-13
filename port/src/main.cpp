#include "assets/hsd_archive.hpp"
#include "gx/tev.hpp"
#include "assets/gx_texture.hpp"
#include "assets/hsd_runtime_archive.hpp"
#include "assets/schemas/db_common.hpp"
#include "assets/schemas/scene_graphics.hpp"
#include "assets/virtual_disc.hpp"

#if defined(MELEE_HOST_SDL_RENDERER)
#include "render/sdl_gl_renderer.hpp"
#endif

#include <dolphin/dvd.h>
#include <dolphin/gx/GXDispList.h>
#include <dolphin/gx/GXStruct.h>
#include <dolphin/gx/GXTev.h>
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXManage.h>
#include <dolphin/pad.h>
#include <dolphin/vi.h>
#include <melee_host/archive_probe.h>
#include <melee_host/baselib.h>
#include <melee_host/boot.h>
#include <melee_host/hsd_archive.h>
#include <melee_host/gx.h>
#include <melee_host/host.h>
#include <melee_host/input.h>
#include <melee_host/local_match.h>
#include <melee_host/match_rules.h>
#include <melee_host/menu_native.h>
#include <melee_host/scene_graphics.h>
#include <melee_host/scene_runtime.h>
#include <melee_host/video.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace {

struct DiagnosticDvdRead {
    bool completed = false;
    s32 result = DVD_RESULT_FATAL_ERROR;
};

void diagnostic_dvd_read_complete(s32 result, DVDFileInfo* file_info)
{
    auto* state = static_cast<DiagnosticDvdRead*>(file_info->cb.userData);
    state->completed = true;
    state->result = result;
}

std::vector<std::byte> read_file(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("unable to open " + path.string());
    }

    const std::vector<char> chars((std::istreambuf_iterator<char>(stream)),
                                  std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes;
    bytes.reserve(chars.size());
    for (const char value : chars) {
        bytes.push_back(static_cast<std::byte>(
            static_cast<unsigned char>(value)));
    }
    return bytes;
}

void print_usage(const char* executable)
{
    std::cerr << "usage:\n"
              << "  " << executable << " --diagnose\n"
              << "  " << executable << " --inspect-match-rules\n"
              << "  " << executable << " --reset-match-rules\n"
              << "  " << executable << " --diagnose-native-menu\n"
              << "  " << executable << " --diagnose-local-match\n"
              << "  " << executable << " --diagnose-scene-runtime [FRAMES]\n"
              << "  " << executable << " --inspect-hsd FILE\n"
              << "  " << executable << " --load-archive FILE [SYMBOL...]\n"
              << "  " << executable << " --sweep-archives DIRECTORY\n"
              << "  " << executable << " --boot-title-archive DIRECTORY\n"
              << "  " << executable << " --inspect-pobj FILE SYMBOL\n"
              << "  " << executable
              << " --load-scene FILE SYMBOL [MODEL_INDEX]\n"
              << "  " << executable << " --load-joint FILE SYMBOL\n"
              << "  " << executable
              << " --render-scene FILE SYMBOL [MODEL_INDEX]\n"
              << "  " << executable << " --render-joint FILE SYMBOL\n"
              << "  " << executable
              << " --animate-joint FILE SYMBOL ANIMFILE ANIMSYM MATSYM"
                 " [FRAMES]\n"
              << "  " << executable << " --list-animations FILE\n"
              << "  " << executable
              << " --animate-named FILE SYMBOL ANIMFILE ANIMSYM [FRAMES]\n"
#if defined(MELEE_HOST_SDL_RENDERER)
              << "  " << executable
              << " --view-animation FILE SYMBOL ANIMFILE ANIMSYM\n"
#endif
              << ""
#if defined(MELEE_HOST_SDL_RENDERER)
              << "  " << executable << " --view-pobj FILE SYMBOL\n"
              << "  " << executable
              << " --view-scene FILE SYMBOL [MODEL_INDEX]\n"
              << "  " << executable << " --view-joint FILE SYMBOL\n"
              << "  " << executable
              << " --tev-conformance-scene FILE SYMBOL [MODEL_INDEX]\n"
              << "  " << executable << " --tev-conformance-joint FILE SYMBOL\n"
#endif
              << "  " << executable << " --inspect-resources DIRECTORY\n"
              << "  " << executable << " --read-resource DIRECTORY PATH\n";
}

int diagnose()
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    const MeleeHostStatus create_status = melee_host_create(&config, &context);
    if (create_status != MELEE_HOST_OK) {
        std::cerr << "host create failed: "
                  << melee_host_status_string(create_status) << '\n';
        return 1;
    }

    const mh_u64 first = melee_host_monotonic_nanoseconds();
    const mh_u64 second = melee_host_monotonic_nanoseconds();
    const MeleeHostStatus baselib_status = melee_host_baselib_bootstrap();
    const MeleeHostStatus step_status = melee_host_step(context);
    melee_host_destroy(context);

    std::cout << "melee native host skeleton\n"
              << "  pointer bits: " << sizeof(void*) * 8 << '\n'
              << "  mh_u32 bytes: " << sizeof(mh_u32) << '\n'
              << "  monotonic clock: " << (second >= first ? "ok" : "failed")
              << '\n'
              << "  baselib bootstrap: "
              << melee_host_status_string(baselib_status) << '\n'
              << "  game step: " << melee_host_status_string(step_status)
              << '\n';
    return second >= first && baselib_status == MELEE_HOST_OK ? 0 : 1;
}

int inspect_match_rules()
{
    MeleeHostMatchRules rules{};
    const MeleeHostStatus status = melee_host_match_rules_get(&rules);
    if (status != MELEE_HOST_OK) {
        std::cerr << "match rules unavailable: "
                  << melee_host_status_string(status) << '\n';
        return 1;
    }
    std::cout << "native VS rules store\n"
              << "  mode: " << static_cast<unsigned>(rules.mode) << '\n'
              << "  time limit: "
              << static_cast<unsigned>(rules.time_limit) << '\n'
              << "  stock count: "
              << static_cast<unsigned>(rules.stock_count) << '\n'
              << "  handicap: " << static_cast<unsigned>(rules.handicap)
              << '\n'
              << "  damage ratio: "
              << static_cast<unsigned>(rules.damage_ratio) << '\n'
              << "  friendly fire: " << (rules.friendly_fire ? "on" : "off")
              << '\n'
              << "  pause: " << (rules.pause ? "on" : "off") << '\n';
    return 0;
}

int reset_match_rules()
{
    const MeleeHostStatus status = melee_host_match_rules_reset_defaults();
    if (status != MELEE_HOST_OK) {
        std::cerr << "match rule reset failed: "
                  << melee_host_status_string(status) << '\n';
        return 1;
    }
    return inspect_match_rules();
}

int diagnose_native_menu()
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    if (melee_host_create(&config, &context) != MELEE_HOST_OK ||
        melee_host_activate_pad_backend(context) != MELEE_HOST_OK ||
        melee_host_native_menu_init() != MELEE_HOST_OK)
    {
        std::cerr << "could not initialize native menu input\n";
        melee_host_destroy(context);
        return 1;
    }
    const MeleeHostPadState input{
        .buttons = PAD_BUTTON_A,
        .stick_x = 0,
        .stick_y = 0,
        .c_stick_x = 0,
        .c_stick_y = 0,
        .trigger_left = 0,
        .trigger_right = 0,
        .connected = true,
    };
    mh_u32 events = 0;
    const MeleeHostStatus submit =
        melee_host_submit_pad_state(context, 0, &input);
    const MeleeHostStatus step = melee_host_step(context);
    const MeleeHostStatus update = melee_host_native_menu_update(
        MELEE_HOST_MAX_CONTROLLERS, &events);
    melee_host_destroy(context);
    if (submit != MELEE_HOST_OK || step != MELEE_HOST_NOT_READY ||
        update != MELEE_HOST_OK)
    {
        std::cerr << "native menu input update failed\n";
        return 1;
    }
    std::cout << "native menu input\n"
              << "  A event: "
              << (((events & MELEE_HOST_NATIVE_MENU_A) != 0) ? "ok" : "failed")
              << '\n'
              << "  confirm event: "
              << (((events & MELEE_HOST_NATIVE_MENU_CONFIRM) != 0) ? "ok"
                                                                  : "failed")
              << '\n';
    return (events & (MELEE_HOST_NATIVE_MENU_A |
                      MELEE_HOST_NATIVE_MENU_CONFIRM)) ==
                   (MELEE_HOST_NATIVE_MENU_A | MELEE_HOST_NATIVE_MENU_CONFIRM)
               ? 0
               : 1;
}

int diagnose_local_match()
{
    if (melee_host_match_rules_reset_defaults() != MELEE_HOST_OK) {
        std::cerr << "could not initialize native VS rules\n";
        return 1;
    }
    const MeleeHostStatus prepare =
        melee_host_prepare_local_two_player_match(2, 8, 3);
    MeleeHostPreparedMatch match{};
    const MeleeHostStatus inspect = melee_host_prepared_match_get(&match);
    const MeleeHostStatus initialize =
        melee_host_initialize_prepared_player_state();
    MeleeHostPlayerState first{};
    MeleeHostPlayerState second{};
    const MeleeHostStatus first_status = melee_host_player_state_get(0, &first);
    const MeleeHostStatus second_status =
        melee_host_player_state_get(1, &second);
    if (prepare != MELEE_HOST_OK || inspect != MELEE_HOST_OK ||
        initialize != MELEE_HOST_OK || first_status != MELEE_HOST_OK ||
        second_status != MELEE_HOST_OK)
    {
        std::cerr << "could not prepare native VS start data\n";
        return 1;
    }
    std::cout << "native local VS start data\n"
              << "  players: " << static_cast<unsigned>(match.player_count)
              << "\n  characters: " << static_cast<int>(match.characters[0])
              << ", " << static_cast<int>(match.characters[1])
              << "\n  stage: " << match.stage_kind
              << "\n  match kind: " << static_cast<unsigned>(match.match_kind)
              << "\n  timer seconds: " << match.time_limit_seconds
              << "\n  materialized slots: " << static_cast<int>(first.character)
              << ", " << static_cast<int>(second.character) << '\n';
    return 0;
}

struct SceneRuntimeProbe {
    unsigned frames = 0;
    unsigned p_link = 0;
};

void scene_runtime_probe_tick(void* user_data)
{
    static_cast<SceneRuntimeProbe*>(user_data)->frames += 1;
}

unsigned scene_runtime_draw_done_callbacks = 0;

void scene_runtime_draw_done_callback(void)
{
    scene_runtime_draw_done_callbacks += 1;
}

// Drives the original HSD process scheduler for a fixed number of frames.
// One probe object runs unpaused while a second sits on a paused p_link, so
// the report shows both that processes fire and that the pause mask is
// honoured by the original HSD_GObj_RunProcs.
int diagnose_scene_runtime(unsigned frames)
{
    if (melee_host_scene_runtime_init() != MELEE_HOST_OK) {
        std::cerr << "could not initialize the native HSD object runtime\n";
        return 1;
    }

    /* Exercise the same frame boundary the host game loop will use: a GX
     * fence completes after process work, then VI presents one field. */
    VIInit();
    melee_host_gx_state_reset();
    scene_runtime_draw_done_callbacks = 0;
    GXSetDrawDoneCallback(scene_runtime_draw_done_callback);

    SceneRuntimeProbe running{ .frames = 0, .p_link = 0 };
    SceneRuntimeProbe paused{ .frames = 0, .p_link = 1 };
    MeleeHostSceneObject running_object = 0;
    MeleeHostSceneObject paused_object = 0;
    if (melee_host_scene_runtime_add_object(
            1, static_cast<mh_u8>(running.p_link), 0, 0,
            scene_runtime_probe_tick, &running, &running_object) !=
            MELEE_HOST_OK ||
        melee_host_scene_runtime_add_object(
            2, static_cast<mh_u8>(paused.p_link), 0, 1,
            scene_runtime_probe_tick, &paused, &paused_object) !=
            MELEE_HOST_OK)
    {
        std::cerr << "could not create native scene objects\n";
        return 1;
    }

    if (melee_host_scene_runtime_set_paused_links(
            1ULL << paused.p_link) != MELEE_HOST_OK)
    {
        std::cerr << "could not apply the native pause mask\n";
        return 1;
    }

    MeleeHostSceneRuntimeStats start{};
    if (melee_host_scene_runtime_stats(&start) != MELEE_HOST_OK) {
        std::cerr << "could not read native scene runtime statistics\n";
        return 1;
    }
    for (unsigned frame = 0; frame < frames; ++frame) {
        GXSetDrawDone();
        if (melee_host_scene_runtime_run_frame() != MELEE_HOST_OK) {
            std::cerr << "native scene frame " << frame << " failed\n";
            return 1;
        }
    }

    MeleeHostSceneRuntimeStats end{};
    if (melee_host_scene_runtime_stats(&end) != MELEE_HOST_OK) {
        std::cerr << "could not read native scene runtime statistics\n";
        return 1;
    }
    MeleeHostVideoState video{};
    if (melee_host_video_state(&video) != MELEE_HOST_OK) {
        std::cerr << "could not read native video statistics\n";
        return 1;
    }

    std::cout << "native HSD scene runtime\n"
              << "  frames run: " << (end.frame_count - start.frame_count)
              << "\n  live objects: " << end.objects_live
              << "\n  live processes: " << end.procs_live
              << "\n  pooled objects: " << end.objects_pooled
              << "\n  VI retraces: " << video.retrace_count
              << "\n  draw-done callbacks: " << scene_runtime_draw_done_callbacks
              << "\n  unpaused process calls: " << running.frames
              << "\n  paused process calls: " << paused.frames << '\n';

    const bool ok = running.frames == frames && paused.frames == 0 &&
                    end.frame_count - start.frame_count == frames &&
                    video.retrace_count == frames &&
                    scene_runtime_draw_done_callbacks == frames;
    GXSetDrawDoneCallback(nullptr);
    if (melee_host_scene_runtime_remove_object(running_object) !=
            MELEE_HOST_OK ||
        melee_host_scene_runtime_remove_object(paused_object) !=
            MELEE_HOST_OK)
    {
        std::cerr << "could not release native scene objects\n";
        return 1;
    }
    return ok ? 0 : 1;
}

int inspect_hsd(const std::filesystem::path& path)
{
    const std::vector<std::byte> bytes = read_file(path);
    const melee::assets::HsdRuntimeArchive runtime(bytes);
    const melee::assets::HsdArchiveView& archive = runtime.disk_view();
    const auto& header = archive.header();

    std::cout << "HSD archive: " << path << '\n'
              << "  file size: " << header.file_size << '\n'
              << "  data size: " << header.data_size << '\n'
              << "  relocations: " << header.relocation_count << '\n'
              << "  public symbols: " << header.public_count << '\n'
              << "  external symbols: " << header.extern_count << '\n';
    std::cout << "  runtime internal references: "
              << runtime.internal_references().size() << '\n';
    for (const auto& symbol : archive.public_symbols()) {
        std::cout << "    " << symbol.name << " @ data+0x" << std::hex
                  << symbol.data_offset << std::dec << '\n';
        if (symbol.name == "dbLoadCommonData") {
            const auto schema =
                melee::assets::schemas::decode_db_load_common_data(runtime);
            std::cout << "      bonus names @ data+0x" << std::hex
                      << schema.bonus_names.data_offset
                      << ", motion states @ data+0x"
                      << schema.motion_state_names.data_offset
                      << ", submotions @ data+0x"
                      << schema.submotion_names.data_offset << std::dec << '\n';
            const auto first_bonus =
                runtime.reference_at(schema.bonus_names, 0);
            const auto first_motion =
                runtime.reference_at(schema.motion_state_names, 0);
            const auto first_submotion =
                runtime.reference_at(schema.submotion_names, 0);
            std::cout << "      first entries: "
                      << runtime.read_c_string(first_bonus) << ", "
                      << runtime.read_c_string(first_motion) << ", "
                      << runtime.read_c_string(first_submotion) << '\n';
            const auto tables =
                melee::assets::schemas::decode_db_common_name_tables(runtime);
            std::cout << "      table sizes: " << tables.bonus_names.size()
                      << " bonus, " << tables.motion_state_names.size()
                      << " motion, " << tables.submotion_names.size()
                      << " submotion\n";
        }
    }
    return 0;
}

int inspect_resources(const std::filesystem::path& path)
{
    const melee::assets::VirtualDisc disc(path);
    std::cout << "native DVD resources: " << disc.root() << '\n'
              << "  indexed FST files: " << disc.entry_count() << '\n';
    return 0;
}

int inspect_pobj(const std::filesystem::path& path, std::string_view symbol,
                 bool preview = false)
{
    const std::vector<std::byte> bytes = read_file(path);
    const melee::assets::HsdRuntimeArchive runtime(bytes);
    const auto geometries =
        melee::assets::schemas::find_scene_pobjs(runtime, symbol);
    const std::size_t textured_geometries = static_cast<std::size_t>(
        std::count_if(geometries.begin(), geometries.end(),
                      [](const auto& geometry) {
                          return geometry.material.has_texture;
                      }));
    const std::size_t translucent_geometries = static_cast<std::size_t>(
        std::count_if(geometries.begin(), geometries.end(),
                      [](const auto& geometry) {
                          return (geometry.material.render_mode & (1U << 30U)) != 0;
                      }));
    std::map<std::uint32_t, std::size_t> texture_formats;
    std::map<std::uint64_t, mh_u32> texture_ids;
#if defined(MELEE_HOST_SDL_RENDERER)
    std::vector<melee::render::TextureImage> renderer_textures;
#endif
    std::size_t decoded_textures = 0;
    for (const auto& geometry : geometries) {
        if (geometry.material.image_data.has_value()) {
            ++texture_formats[geometry.material.texture_format];
            const bool indexed = geometry.material.texture_format == 8 ||
                                 geometry.material.texture_format == 9 ||
                                 geometry.material.texture_format == 10;
            if (geometry.material.texture_format == 0 ||
                geometry.material.texture_format == 1 ||
                geometry.material.texture_format == 2 ||
                geometry.material.texture_format == 3 ||
                geometry.material.texture_format == 4 ||
                geometry.material.texture_format == 5 ||
                geometry.material.texture_format == 6 || indexed ||
                geometry.material.texture_format == 14)
            {
                const std::size_t byte_count = melee::assets::gx_texture_data_size(
                    geometry.material.texture_width,
                    geometry.material.texture_height,
                    geometry.material.texture_format);
                const auto image = runtime.bytes_at(*geometry.material.image_data,
                                                    byte_count);
                melee::assets::DecodedTexture decoded{};
                std::uint64_t texture_key = geometry.material.image_data->data_offset;
                if (indexed) {
                    if (!geometry.material.tlut_data.has_value() ||
                        geometry.material.tlut_entries == 0)
                    {
                        continue;
                    }
                    const std::size_t tlut_size =
                        static_cast<std::size_t>(geometry.material.tlut_entries) * 2;
                    const auto tlut = runtime.bytes_at(*geometry.material.tlut_data,
                                                       tlut_size);
                    decoded = melee::assets::decode_gx_texture_with_tlut(
                        image, geometry.material.texture_width,
                        geometry.material.texture_height,
                        geometry.material.texture_format, tlut,
                        geometry.material.tlut_format);
                    texture_key |= static_cast<std::uint64_t>(
                        geometry.material.tlut_data->data_offset) << 32U;
                } else {
                    decoded = melee::assets::decode_gx_texture(
                        image, geometry.material.texture_width,
                        geometry.material.texture_height,
                        geometry.material.texture_format);
                }
#if defined(MELEE_HOST_SDL_RENDERER)
                const auto insertion = texture_ids.emplace(
                    texture_key,
                    static_cast<mh_u32>(renderer_textures.size()));
                if (insertion.second) {
                    renderer_textures.push_back({
                        decoded.width, decoded.height,
                        geometry.material.texture_wrap_s,
                        geometry.material.texture_wrap_t, decoded.rgba, false
                    });
                }
#else
                static_cast<void>(decoded);
#endif
                ++decoded_textures;
            }
        }
    }

    /* The schema path binds no GX texture and lights nothing: the material it
     * decoded reaches the vertices through melee_host_gx_apply_material.  A
     * modulate stage is the program that combines the two. */
    melee_host_gx_state_reset();
    GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    melee_host_gx_reset_command_log();
    std::size_t display_bytes = 0;
    for (const auto& geometry : geometries) {
        GXClearVtxDesc();
        for (const auto& descriptor : geometry.vertices) {
            if (descriptor.attribute >= GX_VA_MAX_ATTR ||
                descriptor.attribute_type > GX_INDEX16 ||
                descriptor.component_count > GX_NRM_NBT3 ||
                descriptor.component_type > GX_RGBA8 ||
                descriptor.stride > 0xFF)
            {
                throw melee::assets::HsdArchiveError(
                    "PObj contains an unsupported vertex descriptor");
            }
            const auto attribute = static_cast<GXAttr>(descriptor.attribute);
            const auto attribute_type =
                static_cast<GXAttrType>(descriptor.attribute_type);
            GXSetVtxDesc(attribute, attribute_type);
            GXSetVtxAttrFmt(GX_VTXFMT0, attribute,
                            static_cast<GXCompCnt>(descriptor.component_count),
                            static_cast<GXCompType>(descriptor.component_type),
                            descriptor.fractional_bits);
            if (descriptor.array.has_value()) {
                const std::size_t remaining =
                    runtime.disk_view().header().data_size -
                    descriptor.array->data_offset;
                const auto array =
                    runtime.bytes_at(*descriptor.array, remaining);
                melee_host_gx_set_array_bounded(
                    descriptor.attribute, array.data(), array.size(),
                    static_cast<mh_u8>(descriptor.stride));
            }
        }

        const std::size_t display_size =
            static_cast<std::size_t>(geometry.display_list_blocks) * 32;
        const auto display =
            runtime.bytes_at(geometry.display_list, display_size);
        const std::size_t first_vertex = melee_host_gx_captured_vertex_count();
        GXCallDisplayList(const_cast<std::byte*>(display.data()),
                          static_cast<u32>(display.size()));
        MeleeHostGxAffineTransform transform{};
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                transform.values[row][column] =
                    geometry.transform.values[row][column];
            }
        }
        melee_host_gx_transform_vertices(
            first_vertex, melee_host_gx_captured_vertex_count() - first_vertex,
            &transform);
        melee_host_gx_apply_material(
            first_vertex, melee_host_gx_captured_vertex_count() - first_vertex,
            geometry.material.diffuse.data(),
            geometry.material.image_data.has_value() && texture_ids.contains(
                static_cast<std::uint64_t>(geometry.material.image_data->data_offset) |
                (geometry.material.tlut_data.has_value()
                     ? static_cast<std::uint64_t>(
                           geometry.material.tlut_data->data_offset) << 32U
                     : 0U))
                ? texture_ids.at(static_cast<std::uint64_t>(
                                     geometry.material.image_data->data_offset) |
                                 (geometry.material.tlut_data.has_value()
                                      ? static_cast<std::uint64_t>(
                                            geometry.material.tlut_data->data_offset)
                                            << 32U
                                      : 0U))
                : MELEE_HOST_GX_NO_TEXTURE,
            geometry.material.render_mode);
        display_bytes += display.size();
    }

    std::cout << "HSD scene geometry: " << path << " :: " << symbol << '\n'
              << "  drawable PObjs: " << geometries.size() << '\n'
              << "  PObjs with TObj: " << textured_geometries << '\n'
              << "  translucent PObjs: " << translucent_geometries << '\n'
              << "  display list bytes: " << display_bytes << '\n'
              << "  decoded vertices: "
              << melee_host_gx_captured_vertex_count() << '\n'
              << "  decoded triangles: " << melee_host_gx_triangle_count()
              << '\n'
              << "  display list errors: "
              << melee_host_gx_display_list_error_count() << '\n';
    if (!texture_formats.empty()) {
        std::cout << "  image formats:";
        for (const auto& [format, count] : texture_formats) {
            std::cout << " GX_" << format << '=' << count;
        }
        std::cout << '\n';
    }
    if (decoded_textures != 0) {
        std::cout << "  decoded supported textures: " << decoded_textures << '\n';
    }
    if (melee_host_gx_display_list_error_count() != 0) {
        return 1;
    }
#if defined(MELEE_HOST_SDL_RENDERER)
    if (preview) {
        std::string error;
        melee::render::set_texture_images(std::move(renderer_textures));
        MeleeHostContext* context = nullptr;
        const MeleeHostConfig config{ .resource_root = nullptr, .headless = false };
        if (melee_host_create(&config, &context) != MELEE_HOST_OK ||
            melee_host_activate_pad_backend(context) != MELEE_HOST_OK)
        {
            melee_host_destroy(context);
            std::cerr << "could not initialize SDL input context\n";
            return 1;
        }
        const bool rendered = melee::render::show_captured_geometry(context, &error);
        melee_host_destroy(context);
        if (!rendered) {
            std::cerr << "geometry preview failed: " << error << '\n';
            return 1;
        }
    }
#else
    static_cast<void>(preview);
#endif
    return 0;
}

int read_resource(const std::filesystem::path& root, std::string path)
{
    MeleeHostContext* context = nullptr;
    const std::string root_string = root.string();
    const MeleeHostConfig config{ .resource_root = root_string.c_str(),
                                  .headless = true };
    if (melee_host_create(&config, &context) != MELEE_HOST_OK ||
        melee_host_activate_dvd_backend(context) != MELEE_HOST_OK) {
        std::cerr << "could not initialize the virtual DVD\n";
        melee_host_destroy(context);
        return 1;
    }
    std::vector<char> mutable_path(path.begin(), path.end());
    mutable_path.push_back('\0');
    DVDFileInfo file{};
    if (!DVDOpen(mutable_path.data(), &file)) {
        std::cerr << "DVD resource was not found: " << path << '\n';
        melee_host_destroy(context);
        return 1;
    }

    const std::size_t sample_size = std::min<std::size_t>(file.length, 16);
    std::array<std::byte, 16> sample{};
    DiagnosticDvdRead read{};
    file.cb.userData = &read;
    const BOOL submitted = DVDReadAsyncPrio(
        &file, sample.data(), static_cast<s32>(sample_size), 0,
        diagnostic_dvd_read_complete, 2);
    if (submitted) {
        static_cast<void>(melee_host_step(context));
    }
    DVDClose(&file);
    melee_host_destroy(context);
    if (!submitted || !read.completed ||
        read.result != static_cast<s32>(sample_size)) {
        std::cerr << "DVD resource read failed\n";
        return 1;
    }

    std::cout << "DVD resource: " << path << '\n'
              << "  size: " << file.length << '\n'
              << "  asynchronous tick read: ok\n"
              << "  first " << sample_size << " bytes:";
    for (std::size_t index = 0; index < sample_size; ++index) {
        std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0')
                  << std::to_integer<unsigned int>(sample[index]);
    }
    std::cout << std::dec << '\n';
    return 0;
}

/* Loads a scene from disk through the original object layer, rather than
 * through the read-only schema the geometry preview uses. */
#if defined(MELEE_HOST_SDL_RENDERER)
/* Shows what the original display path drew, rather than the geometry the
 * read-only schema decodes separately.  The capture is asked for in world
 * space so the viewer can move its own camera around the model. */
/* Decodes the textures a capture bound, in the order the vertices index them:
 * an image that cannot be decoded still has to occupy its slot or every id
 * after it would point at the wrong picture. */
void decode_captured_textures(mh_u32 count,
                              std::vector<melee::render::TextureImage>* images,
                              std::size_t* decoded_count)
{
    for (mh_u32 id = 0; id < count; ++id) {
        MeleeHostGxTextureDesc desc{};
        melee::render::TextureImage image{ 1, 1, 0, 0,
                                           { 255, 255, 255, 255 }, false };
        if (melee_host_gx_captured_texture_at(id, &desc) &&
            desc.image != nullptr)
        {
            const std::size_t byte_count = melee::assets::gx_texture_data_size(
                desc.width, desc.height, desc.format);
            const std::span<const std::byte> data{
                static_cast<const std::byte*>(desc.image), byte_count
            };
            try {
                melee::assets::DecodedTexture decoded{};
                MeleeHostGxTlutDesc tlut{};
                if (desc.color_indexed &&
                    melee_host_gx_loaded_tlut(desc.tlut_name, &tlut) &&
                    tlut.loaded && tlut.entries != nullptr)
                {
                    decoded = melee::assets::decode_gx_texture_with_tlut(
                        data, desc.width, desc.height, desc.format,
                        { static_cast<const std::byte*>(tlut.entries),
                          static_cast<std::size_t>(tlut.entry_count) * 2 },
                        tlut.format);
                } else if (!desc.color_indexed) {
                    decoded = melee::assets::decode_gx_texture(
                        data, desc.width, desc.height, desc.format);
                }
                if (!decoded.rgba.empty()) {
                    image = { decoded.width, decoded.height, desc.wrap_s,
                              desc.wrap_t, std::move(decoded.rgba),
                              desc.mag_filter != 0 };
                    *decoded_count += 1;
                }
            } catch (const std::exception& error) {
                std::cerr << "texture " << id << " not decoded: "
                          << error.what() << '\n';
            }
        }
        images->push_back(std::move(image));
    }
}

int view_scene(const char* path, const char* symbol, bool scene_model,
               mh_u32 model_index)
{
    MeleeHostSceneModel model = 0;
    const MeleeHostStatus status =
        scene_model
            ? melee_host_scene_graphics_load_model(path, symbol, model_index,
                                                   &model)
            : melee_host_scene_graphics_load_joint(path, symbol, &model);
    if (status != MELEE_HOST_OK) {
        std::cerr << "scene load failed: "
                  << melee_host_status_string(status) << ": "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }

    MeleeHostSceneRenderStats drawn{};
    if (melee_host_scene_graphics_render(model, MELEE_HOST_SCENE_VIEW_WORLD,
                                         &drawn) != MELEE_HOST_OK) {
        std::cerr << "scene render failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }

    std::vector<melee::render::TextureImage> images;
    std::size_t decoded_count = 0;
    decode_captured_textures(drawn.textures, &images, &decoded_count);

    std::cout << "showing " << drawn.triangles << " triangles from "
              << symbol << " through the original display path\n"
              << "  textures decoded: " << decoded_count << " of "
              << drawn.textures << '\n';
    if (drawn.display_list_errors != 0 || drawn.rejected_indices != 0) {
        std::cerr << "capture incomplete: " << drawn.display_list_errors
                  << " display list errors, " << drawn.rejected_indices
                  << " rejected indices\n";
        melee_host_scene_graphics_release(model);
        return 1;
    }

    melee::render::set_texture_images(std::move(images));
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr,
                                  .headless = false };
    if (melee_host_create(&config, &context) != MELEE_HOST_OK ||
        melee_host_activate_pad_backend(context) != MELEE_HOST_OK)
    {
        melee_host_destroy(context);
        melee_host_scene_graphics_release(model);
        std::cerr << "could not initialize SDL input context\n";
        return 1;
    }
    std::string error;
    const bool shown = melee::render::show_captured_geometry(context, &error);
    melee_host_destroy(context);
    /* The captured textures point into the archive payload the handle owns, so
     * the model outlives the window. */
    melee_host_scene_graphics_release(model);
    if (!shown) {
        std::cerr << "preview failed: " << error << '\n';
        return 1;
    }
    return 0;
}

/* Draws a model through the original display path, then holds the generated
 * TEV shaders to the CPU reference for every program and pixel state the
 * capture used.  A mismatch means the GLSL and melee::gx::evaluate_tev
 * disagree about what the asset's material computes. */
int tev_conformance(const char* path, const char* symbol, bool scene_model,
                    mh_u32 model_index)
{
    MeleeHostSceneModel model = 0;
    const MeleeHostStatus status =
        scene_model
            ? melee_host_scene_graphics_load_model(path, symbol, model_index,
                                                   &model)
            : melee_host_scene_graphics_load_joint(path, symbol, &model);
    if (status != MELEE_HOST_OK) {
        std::cerr << "scene load failed: "
                  << melee_host_status_string(status) << ": "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }
    MeleeHostSceneRenderStats drawn{};
    const bool rendered =
        melee_host_scene_graphics_render(model, MELEE_HOST_SCENE_VIEW_WORLD,
                                         &drawn) == MELEE_HOST_OK;
    melee_host_scene_graphics_release(model);
    if (!rendered) {
        std::cerr << "scene render failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }

    melee::render::TevConformanceReport report;
    std::string error;
    if (!melee::render::run_tev_conformance(64, &report, &error)) {
        std::cerr << "TEV conformance did not run: " << error << '\n';
        return 1;
    }
    std::cout << "TEV conformance for " << symbol << '\n'
              << "  programs: " << report.programs << " (of "
              << drawn.tev_states << " captured)\n"
              << "  cases: " << report.cases << '\n'
              << "  mismatches: " << report.mismatches << '\n';
    if (report.mismatches != 0) {
        std::cout << "  first: " << report.first_mismatch << '\n';
        return 1;
    }
    return 0;
}
#endif


/* Loads a model, attaches an animation to it and advances it, reporting where
 * a joint ends up.  A joint that moved is the proof the keyframe streams were
 * translated and the original interpreter read them. */
int animate_scene(const char* model_path, const char* model_symbol,
                  bool scene_model, const char* anim_path,
                  const char* anim_symbol, const char* mat_anim_symbol,
                  unsigned frames, float rate)
{
    MeleeHostSceneModel model = 0;
    const MeleeHostStatus status =
        scene_model ? melee_host_scene_graphics_load_model(
                          model_path, model_symbol, 0, &model)
                    : melee_host_scene_graphics_load_joint(
                          model_path, model_symbol, &model);
    if (status != MELEE_HOST_OK) {
        std::cerr << "scene load failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }

    MeleeHostSceneModelStats tree{};
    melee_host_scene_graphics_stats(model, &tree);
    std::vector<std::array<float, 3>> before(tree.jobjs);
    for (mh_u32 index = 0; index < tree.jobjs; ++index) {
        melee_host_scene_graphics_joint_world_position(model, index,
                                                       before[index].data());
    }

    if (melee_host_scene_graphics_attach_animation(
            model, anim_path, anim_symbol, mat_anim_symbol, nullptr) !=
        MELEE_HOST_OK)
    {
        std::cerr << "attach failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }

    MeleeHostSceneAnimationStats anim{};
    melee_host_scene_graphics_animation_stats(model, &anim);
    std::cout << "animation from " << anim_path << '\n'
              << "  AnimJoint: " << anim.anim_joints
              << "  MatAnimJoint: " << anim.mat_anim_joints
              << "  ShapeAnimJoint: " << anim.shape_anim_joints << '\n'
              << "  AObjDesc: " << anim.aobj_descs
              << "  FObjDesc: " << anim.fobj_descs
              << "  keyframe bytes: " << anim.anim_data_bytes << '\n'
              << "  live AObj: " << anim.aobjs_live
              << "  live FObj: " << anim.fobjs_live
              << "  AObj na arvore: " << anim.aobjs_in_tree << '\n';

    if (melee_host_scene_graphics_run_animation(model, 0.0F, rate, frames) !=
        MELEE_HOST_OK) {
        std::cerr << "run failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }

    std::size_t moved = 0;
    float largest = 0.0F;
    mh_u32 largest_joint = 0;
    std::array<float, 3> sample_before{};
    std::array<float, 3> sample_after{};
    for (mh_u32 index = 0; index < tree.jobjs; ++index) {
        std::array<float, 3> after{};
        if (melee_host_scene_graphics_joint_world_position(
                model, index, after.data()) != MELEE_HOST_OK) {
            continue;
        }
        float distance = 0.0F;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const float delta = after[axis] - before[index][axis];
            distance += delta * delta;
        }
        distance = std::sqrt(distance);
        if (distance > 1.0e-4F) {
            ++moved;
        }
        if (distance > largest) {
            largest = distance;
            largest_joint = index;
            sample_before = before[index];
            sample_after = after;
        }
    }
    melee_host_scene_graphics_animation_stats(model, &anim);
    std::cout << "  ran " << frames << " frames to " << anim.current_frame
              << '\n'
              << "  joints moved: " << moved << " of " << tree.jobjs << '\n'
              << "  AObj frame: " << anim.aobj_frame << " of "
              << anim.aobj_end_frame << '\n';
    if (moved != 0) {
        std::cout << "  largest move, joint " << largest_joint << ": "
                  << sample_before[0] << ' ' << sample_before[1] << ' '
                  << sample_before[2] << "  ->  " << sample_after[0] << ' '
                  << sample_after[1] << ' ' << sample_after[2] << "  ("
                  << largest << ")\n";
    }

    melee_host_scene_graphics_release(model);
    return 0;
}

#if defined(MELEE_HOST_SDL_RENDERER)
namespace {
struct AnimationPlayback {
    MeleeHostSceneModel model;
    float end_frame;
    mh_u32 frame;
};

/* Runs once per displayed frame: advance the animation, then draw the tree
 * again so the window reads this frame's geometry.  The animation is requested
 * again when it runs out, which is what makes the action loop. */
void advance_animation_frame(void* user_data)
{
    auto* const playback = static_cast<AnimationPlayback*>(user_data);
    if (playback->end_frame > 0.0F &&
        static_cast<float>(playback->frame) >= playback->end_frame)
    {
        melee_host_scene_graphics_run_animation(playback->model, 0.0F, 1.0F,
                                                0);
        playback->frame = 0;
    }
    melee_host_scene_graphics_step_animation(playback->model, 1);
    playback->frame += 1;
    MeleeHostSceneRenderStats drawn{};
    melee_host_scene_graphics_render(playback->model,
                                     MELEE_HOST_SCENE_VIEW_WORLD, &drawn);
}
} // namespace

/* Shows a character model playing one of its actions. */
int view_animation(const char* model_path, const char* model_symbol,
                   const char* anim_path, const char* anim_symbol)
{
    MeleeHostSceneModel model = 0;
    if (melee_host_scene_graphics_load_joint(model_path, model_symbol,
                                             &model) != MELEE_HOST_OK) {
        std::cerr << "scene load failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }
    if (melee_host_scene_graphics_attach_named_animation(
            model, anim_path, anim_symbol) != MELEE_HOST_OK) {
        std::cerr << "attach failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }
    if (melee_host_scene_graphics_run_animation(model, 0.0F, 1.0F, 0) !=
        MELEE_HOST_OK) {
        std::cerr << "run failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }

    /* One capture up front, so the window has geometry and textures before the
     * first callback runs. */
    MeleeHostSceneRenderStats drawn{};
    if (melee_host_scene_graphics_render(model, MELEE_HOST_SCENE_VIEW_WORLD,
                                         &drawn) != MELEE_HOST_OK) {
        std::cerr << "render failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }

    std::vector<melee::render::TextureImage> images;
    std::size_t decoded_count = 0;
    decode_captured_textures(drawn.textures, &images, &decoded_count);

    MeleeHostSceneAnimationStats anim{};
    melee_host_scene_graphics_animation_stats(model, &anim);
    std::cout << anim_symbol << " on " << model_symbol << '\n'
              << "  " << drawn.triangles << " triangles, "
              << decoded_count << " of " << drawn.textures
              << " textures decoded\n"
              << "  " << anim.aobjs_in_tree << " animated joints over "
              << anim.aobj_end_frame << " frames\n";

    melee::render::set_texture_images(std::move(images));
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr,
                                  .headless = false };
    if (melee_host_create(&config, &context) != MELEE_HOST_OK ||
        melee_host_activate_pad_backend(context) != MELEE_HOST_OK)
    {
        melee_host_destroy(context);
        melee_host_scene_graphics_release(model);
        std::cerr << "could not initialize SDL input context\n";
        return 1;
    }
    AnimationPlayback playback{ model, anim.aobj_end_frame, 0 };
    std::string error;
    const bool shown = melee::render::show_captured_geometry(
        context, &error, advance_animation_frame, &playback);
    melee_host_destroy(context);
    melee_host_scene_graphics_release(model);
    if (!shown) {
        std::cerr << "preview failed: " << error << '\n';
        return 1;
    }
    return 0;
}
#endif

int list_animations(const char* path)
{
    mh_u32 count = 0;
    if (melee_host_scene_graphics_list_animations(path, 0, nullptr, 0,
                                                  &count) != MELEE_HOST_OK) {
        std::cerr << "list failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }
    std::cout << path << ": " << count << " animations\n";
    for (mh_u32 index = 0; index < count; ++index) {
        std::array<char, 128> symbol{};
        if (melee_host_scene_graphics_list_animations(
                path, index, symbol.data(), symbol.size(), nullptr) ==
            MELEE_HOST_OK)
        {
            std::cout << "  " << index << ": " << symbol.data() << '\n';
        }
    }
    return 0;
}

/* Loads a model and drives it with one named animation out of a file that
 * holds several, which is how a character keeps one archive per action. */
int animate_named(const char* model_path, const char* model_symbol,
                  const char* anim_path, const char* anim_symbol,
                  unsigned frames)
{
    MeleeHostSceneModel model = 0;
    if (melee_host_scene_graphics_load_joint(model_path, model_symbol,
                                             &model) != MELEE_HOST_OK) {
        std::cerr << "scene load failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }

    MeleeHostSceneModelStats tree{};
    melee_host_scene_graphics_stats(model, &tree);
    std::vector<std::array<float, 3>> before(tree.jobjs);
    for (mh_u32 index = 0; index < tree.jobjs; ++index) {
        melee_host_scene_graphics_joint_world_position(model, index,
                                                       before[index].data());
    }

    if (melee_host_scene_graphics_attach_named_animation(
            model, anim_path, anim_symbol) != MELEE_HOST_OK) {
        std::cerr << "attach failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }

    MeleeHostSceneAnimationStats anim{};
    melee_host_scene_graphics_animation_stats(model, &anim);
    std::cout << anim_symbol << " on " << model_symbol << '\n'
              << "  AnimJoint: " << anim.anim_joints
              << "  AObjDesc: " << anim.aobj_descs
              << "  FObjDesc: " << anim.fobj_descs
              << "  keyframe bytes: " << anim.anim_data_bytes << '\n'
              << "  AObj attached to the tree: " << anim.aobjs_in_tree
              << " of " << tree.jobjs << " joints\n";

    if (melee_host_scene_graphics_run_animation(model, 0.0F, 1.0F, frames) !=
        MELEE_HOST_OK) {
        std::cerr << "run failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        melee_host_scene_graphics_release(model);
        return 1;
    }

    std::size_t moved = 0;
    float largest = 0.0F;
    mh_u32 largest_joint = 0;
    for (mh_u32 index = 0; index < tree.jobjs; ++index) {
        std::array<float, 3> after{};
        if (melee_host_scene_graphics_joint_world_position(
                model, index, after.data()) != MELEE_HOST_OK) {
            continue;
        }
        float distance = 0.0F;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const float delta = after[axis] - before[index][axis];
            distance += delta * delta;
        }
        distance = std::sqrt(distance);
        if (distance > 1.0e-4F) {
            ++moved;
        }
        if (distance > largest) {
            largest = distance;
            largest_joint = index;
        }
    }
    melee_host_scene_graphics_animation_stats(model, &anim);
    std::cout << "  ran " << frames << " frames, AObj frame "
              << anim.aobj_frame << " of " << anim.aobj_end_frame << '\n'
              << "  joints moved: " << moved << " of " << tree.jobjs
              << ", largest " << largest << " at joint " << largest_joint
              << '\n';

    /* Moving joints are not the same claim as moving geometry: the vertices
     * only follow if the draw runs again and picks up the new matrices.  This
     * renders two consecutive frames and compares what the recorder captured,
     * which is exactly what a window would be showing. */
    MeleeHostSceneRenderStats drawn{};
    std::vector<MeleeHostGxPosition3f32> first;
    if (melee_host_scene_graphics_render(model, MELEE_HOST_SCENE_VIEW_WORLD,
                                         &drawn) == MELEE_HOST_OK) {
        first.reserve(drawn.triangles);
        for (mh_u32 index = 0; index < drawn.triangles; ++index) {
            MeleeHostGxCapturedTriangle triangle{};
            if (melee_host_gx_captured_triangle_at(index, &triangle)) {
                first.push_back(triangle.vertices[0].position);
            }
        }
        melee_host_scene_graphics_step_animation(model, 1);
        if (melee_host_scene_graphics_render(
                model, MELEE_HOST_SCENE_VIEW_WORLD, &drawn) == MELEE_HOST_OK)
        {
            std::size_t changed = 0;
            float biggest = 0.0F;
            for (mh_u32 index = 0;
                 index < drawn.triangles && index < first.size(); ++index) {
                MeleeHostGxCapturedTriangle triangle{};
                if (!melee_host_gx_captured_triangle_at(index, &triangle)) {
                    continue;
                }
                const auto& before_vertex = first[index];
                const auto& after_vertex = triangle.vertices[0].position;
                const float dx = after_vertex.x - before_vertex.x;
                const float dy = after_vertex.y - before_vertex.y;
                const float dz = after_vertex.z - before_vertex.z;
                const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (distance > 1.0e-4F) {
                    ++changed;
                }
                biggest = std::max(biggest, distance);
            }
            std::cout << "  geometry between two frames: " << changed
                      << " of " << first.size()
                      << " triangles moved, largest " << biggest << '\n';
        }
    }

    melee_host_scene_graphics_release(model);
    return moved == 0 ? 1 : 0;
}

struct ArchiveKindTally {
    mh_u32 symbols = 0;
    mh_u32 translated = 0;
    mh_u32 load_attempted = 0;
    mh_u32 loaded = 0;
    mh_u64 objects = 0;
};

struct ArchiveProbeReport {
    std::array<ArchiveKindTally, MELEE_HOST_HSD_SYMBOL_KIND_COUNT> kinds{};
    std::vector<std::string> failures;
    mh_u32 failed = 0;
    std::string file;
    bool verbose = false;
};

void record_archive_symbol(const MeleeHostArchiveProbeSymbol* result,
                           void* user_data)
{
    auto& report = *static_cast<ArchiveProbeReport*>(user_data);
    ArchiveKindTally& tally = report.kinds[result->kind];
    tally.symbols += 1;
    tally.translated += static_cast<mh_u32>(result->translated);
    tally.load_attempted += static_cast<mh_u32>(result->load_attempted);
    tally.loaded += static_cast<mh_u32>(result->loaded);
    tally.objects += result->objects;

    const bool supported = result->kind != MELEE_HOST_HSD_SYMBOL_UNSUPPORTED;
    const bool failed =
        supported && (result->translated == 0 ||
                      (result->load_attempted != 0 && result->loaded == 0));
    std::string error = result->error != nullptr ? result->error : "";
    /* A refusal from the host archive already leads with the symbol. */
    const std::string prefix = std::string(result->symbol) + ": ";
    if (error.starts_with(prefix)) {
        error.erase(0, prefix.size());
    }
    if (report.verbose) {
        std::cout << "  " << result->symbol << " ["
                  << melee_host_hsd_symbol_kind_name(result->kind) << "] ";
        if (!supported) {
            std::cout << "no host translation for this kind";
        } else if (result->translated == 0) {
            std::cout << "refused: " << error;
        } else if (result->load_attempted == 0) {
            std::cout << "translated";
        } else if (result->loaded != 0) {
            std::cout << "loaded, " << result->objects << " objects";
        } else {
            std::cout << "loader failed: " << error;
        }
        std::cout << '\n';
    }
    if (failed) {
        report.failed += 1;
        if (report.failures.size() < 20) {
            report.failures.push_back(report.file + ": " + result->symbol +
                                      ": " + error);
        }
    }
}

void print_archive_report(const ArchiveProbeReport& report)
{
    std::cout << "  kind              symbols translated  loaded objects\n";
    for (std::size_t kind = 0; kind < report.kinds.size(); ++kind) {
        const ArchiveKindTally& tally = report.kinds[kind];
        if (tally.symbols == 0) {
            continue;
        }
        std::cout << "  " << std::left << std::setw(17)
                  << melee_host_hsd_symbol_kind_name(
                         static_cast<MeleeHostHsdSymbolKind>(kind))
                  << std::right << std::setw(8) << tally.symbols
                  << std::setw(11) << tally.translated;
        if (tally.load_attempted != 0) {
            std::cout << std::setw(8) << tally.loaded << std::setw(8)
                      << tally.objects;
        }
        std::cout << '\n';
    }
    std::cout << "  failed: " << report.failed << '\n';
    for (const std::string& failure : report.failures) {
        std::cout << "    " << failure << '\n';
    }
}

/* One file, the symbols a lbArchive_LoadSymbols call would name, or all of
 * them. */
int load_archive(const char* path, const std::vector<const char*>& symbols)
{
    ArchiveProbeReport report;
    report.file = std::filesystem::path(path).filename().string();
    report.verbose = true;
    std::cout << "parsed " << path << " through HSD_ArchiveParse\n";
    const MeleeHostStatus status = melee_host_archive_probe_file(
        path, symbols.data(), static_cast<mh_u32>(symbols.size()),
        record_archive_symbol, &report);
    if (status != MELEE_HOST_OK) {
        std::cerr << "archive load failed: "
                  << melee_host_archive_probe_last_error() << '\n';
        return 1;
    }
    print_archive_report(report);
    return report.failed == 0 ? 0 : 1;
}

/* Every file on the disc that is one archive, every public symbol in it. */
int sweep_archives(const std::filesystem::path& root)
{
    std::vector<std::filesystem::path> paths;
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(root)) {
        const std::string extension = entry.path().extension().string();
        if (entry.is_regular_file() &&
            (extension == ".dat" || extension == ".usd")) {
            paths.push_back(entry.path());
        }
    }
    std::sort(paths.begin(), paths.end());

    ArchiveProbeReport report;
    mh_u32 archives = 0;
    mh_u32 not_single = 0;
    mh_u32 unreadable = 0;
    for (const auto& path : paths) {
        /* A file of several archives laid end to end is not loaded whole by
         * lbArchive; its members reach the parse through other paths. */
        std::ifstream stream(path, std::ios::binary);
        std::array<unsigned char, 4> head{};
        stream.read(reinterpret_cast<char*>(head.data()),
                    static_cast<std::streamsize>(head.size()));
        const std::uintmax_t declared =
            (static_cast<std::uintmax_t>(head[0]) << 24U) |
            (static_cast<std::uintmax_t>(head[1]) << 16U) |
            (static_cast<std::uintmax_t>(head[2]) << 8U) | head[3];
        if (!stream || declared != std::filesystem::file_size(path)) {
            not_single += 1;
            continue;
        }
        report.file = path.filename().string();
        if (melee_host_archive_probe_file(path.string().c_str(), nullptr, 0,
                                          record_archive_symbol, &report) !=
            MELEE_HOST_OK)
        {
            unreadable += 1;
            if (report.failures.size() < 20) {
                report.failures.push_back(
                    report.file + ": " + melee_host_archive_probe_last_error());
            }
            continue;
        }
        archives += 1;
    }

    std::cout << "swept " << root.string() << ": " << archives
              << " archives, " << not_single
              << " files that are not a single archive, " << unreadable
              << " unreadable\n";
    print_archive_report(report);
    return report.failed == 0 && unreadable == 0 ? 0 : 1;
}

/* The title screen's archive through the game's own loader, after the memory
 * sequence gmMain runs. */
int boot_title_archive(const std::filesystem::path& root)
{
    MeleeHostContext* context = nullptr;
    const std::string root_string = root.string();
    const MeleeHostConfig config{ .resource_root = root_string.c_str(),
                                  .headless = true };
    if (melee_host_create(&config, &context) != MELEE_HOST_OK ||
        melee_host_activate_dvd_backend(context) != MELEE_HOST_OK) {
        std::cerr << "could not initialize the virtual DVD\n";
        melee_host_destroy(context);
        return 1;
    }
    if (melee_host_boot_memory_init(0) != MELEE_HOST_OK) {
        std::cerr << "the boot memory sequence failed\n";
        melee_host_destroy(context);
        return 1;
    }
    MeleeHostBootMemoryStats memory{};
    static_cast<void>(melee_host_boot_memory_stats(&memory));
    std::cout << "booted memory through HSD_InitComponent, lbMemory and lbHeap\n"
              << "  arena bytes: " << memory.arena_bytes << '\n'
              << "  lb heaps created: " << memory.lb_heaps_created
              << " of 6\n";

    MeleeHostTitleArchiveReport report{};
    const MeleeHostStatus status = melee_host_boot_load_title_archive(&report);
    melee_host_destroy(context);
    if (status != MELEE_HOST_OK) {
        std::cerr << "title archive load failed: "
                  << melee_host_status_string(status) << '\n';
        return 1;
    }
    std::cout << "loaded GmTtAll.usd through lbArchive_LoadSymbols\n"
              << "  file bytes: " << report.file_bytes << '\n'
              << "  symbols resolved: " << report.symbols_resolved
              << " of 12\n"
              << "  title JObjs: " << report.title_jobjs << " (animated "
              << report.title_animated_jobjs << ")\n"
              << "  background JObjs: " << report.background_jobjs
              << " (animated " << report.background_animated_jobjs << ")\n"
              << "  lights: " << report.lights << '\n'
              << "  camera loaded: " << static_cast<int>(report.camera_loaded)
              << '\n'
              << "  fog loaded: " << static_cast<int>(report.fog_loaded)
              << '\n'
              << "  title mark image: "
              << static_cast<int>(report.mark_has_image) << '\n';
    const bool complete = report.symbols_resolved == 12 &&
                          report.title_jobjs != 0 &&
                          report.background_jobjs != 0 &&
                          report.lights != 0 && report.camera_loaded != 0 &&
                          report.fog_loaded != 0 && report.mark_has_image != 0;
    return complete ? 0 : 1;
}

int load_scene(const char* path, const char* symbol, bool scene_model,
               mh_u32 model_index, bool render = false,
               MeleeHostSceneView view = MELEE_HOST_SCENE_VIEW_SCENE_CAMERA)
{
    MeleeHostSceneModel model = 0;
    const MeleeHostStatus status =
        scene_model
            ? melee_host_scene_graphics_load_model(path, symbol, model_index,
                                                   &model)
            : melee_host_scene_graphics_load_joint(path, symbol, &model);
    if (status != MELEE_HOST_OK) {
        std::cerr << "scene load failed: "
                  << melee_host_status_string(status) << ": "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }

    MeleeHostSceneModelStats stats{};
    if (melee_host_scene_graphics_stats(model, &stats) != MELEE_HOST_OK) {
        std::cerr << "scene stats failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }

    std::cout << "loaded " << symbol << " from " << path;
    if (scene_model) {
        std::cout << " model " << model_index;
    }
    std::cout << " through the original HSD object layer\n"
              << "  JObj: " << stats.jobjs << " (depth " << stats.tree_depth
              << ")\n"
              << "  DObj: " << stats.dobjs << '\n'
              << "  MObj: " << stats.mobjs << " (TObj " << stats.tobjs
              << ")\n"
              << "  PObj: " << stats.pobjs << " (textured "
              << stats.textured_pobjs << ")\n"
              << "  display list blocks: " << stats.display_blocks << '\n'
              << "  not drawn: " << stats.undrawn_pobjs << " PObj (hidden "
              << stats.hidden_jobjs << " JObj / " << stats.hidden_dobjs
              << " DObj, both-face cull " << stats.culled_pobjs << ")\n"
              << "  host descriptor bytes: " << stats.descriptor_bytes << '\n'
              << "  archive payload bytes: " << stats.payload_bytes << '\n';

    float position[3] = { 0.0F, 0.0F, 0.0F };
    if (melee_host_scene_graphics_joint_world_position(model, 0, position) ==
        MELEE_HOST_OK)
    {
        std::cout << "  root world position: " << position[0] << ' '
                  << position[1] << ' ' << position[2] << '\n';
    }

    if (render) {
        MeleeHostSceneRenderStats drawn{};
        if (melee_host_scene_graphics_render(model, view, &drawn) !=
            MELEE_HOST_OK) {
            std::cerr << "scene render failed: "
                      << melee_host_scene_graphics_last_error() << '\n';
            return 1;
        }
        std::cout << "rendered through HSD_JObjDispAll"
                  << (drawn.used_scene_camera ? " with the scene camera"
                                              : " with the host camera")
                  << (view == MELEE_HOST_SCENE_VIEW_WORLD
                          ? ", world space"
                          : ", view space")
                  << '\n'
                  << "  triangles: " << drawn.triangles << " (textured "
                  << drawn.textured_triangles << ")\n"
                  << "  textures bound: " << drawn.textures << '\n'
                  << "  distinct draw states: " << drawn.draw_states << "\n"
                  << "  vertices: " << drawn.vertices << '\n'
                  << "  per pass opa/texedge/xlu: " << drawn.pass_triangles[0]
                  << '/' << drawn.pass_triangles[1] << '/'
                  << drawn.pass_triangles[2] << '\n'
                  << "  display list errors: " << drawn.display_list_errors
                  << '\n'
                  << "  rejected vertex indices: " << drawn.rejected_indices
                  << '\n';
        std::cout << "  distinct TEV states: " << drawn.tev_states << '\n'
                  << "  texture sets: " << drawn.texture_sets << '\n'
                  << "  TEV evaluated per fragment: "
                  << drawn.tev_evaluated_triangles << " triangles, "
                  << drawn.tev_unmodelled_triangles
                  << " with unmodelled features\n";
        for (std::size_t id = 0;
             id < melee_host_gx_captured_tev_state_count() ; ++id) {
            MeleeHostGxTevState tev{};
            if (!melee_host_gx_captured_tev_state_at(id, &tev)) {
                continue;
            }
            std::cout << "  tev " << id << ": stages="
                      << static_cast<unsigned>(tev.stage_count)
                      << " texgens="
                      << static_cast<unsigned>(tev.texcoord_gen_count)
                      << " channels="
                      << static_cast<unsigned>(tev.channel_count);
            for (std::size_t stage = 0; stage < tev.stage_count && stage < 4;
                 ++stage) {
                std::cout << " [" << stage << " mode="
                          << (tev.stages[stage].mode ==
                                      MELEE_HOST_GX_TEV_MODE_CUSTOM
                                  ? std::string("custom")
                                  : std::to_string(tev.stages[stage].mode))
                          << " map=" << tev.stages[stage].texmap
                          << " chan=" << tev.stages[stage].color_channel
                          << " cin=" << tev.stages[stage].color_input[0] << ','
                          << tev.stages[stage].color_input[1] << ','
                          << tev.stages[stage].color_input[2] << ','
                          << tev.stages[stage].color_input[3]
                          << " cop=" << tev.stages[stage].color_op
                          << " cbias=" << tev.stages[stage].color_bias
                          << " cscale=" << tev.stages[stage].color_scale
                          << " creg=" << tev.stages[stage].color_out_reg
                          << " ain=" << tev.stages[stage].alpha_input[0] << ','
                          << tev.stages[stage].alpha_input[1] << ','
                          << tev.stages[stage].alpha_input[2] << ','
                          << tev.stages[stage].alpha_input[3]
                          << " areg=" << tev.stages[stage].alpha_out_reg
                          << ']';
            }
            const auto unmodelled = melee::gx::tev_unmodelled_features(tev);
            if (unmodelled != melee::gx::kTevUnmodelledNone) {
                std::cout << " unmodelled="
                          << melee::gx::describe_tev_unmodelled(unmodelled);
            }
            std::cout << '\n';
        }

        /* The state each group of triangles ran under, which is what a viewer
         * has to follow instead of assuming fixed passes. */
        for (mh_u32 id = 0; id < drawn.draw_states && id < 8; ++id) {
            MeleeHostGxDrawState state{};
            if (!melee_host_gx_captured_draw_state_at(id, &state)) {
                continue;
            }
            std::cout << "  state " << id << ": cull=" << state.cull_mode
                      << " ztest=" << state.z_compare_enable
                      << " zwrite=" << state.z_update_enable
                      << " zfunc=" << state.z_func
                      << " blend=" << state.blend_mode << '('
                      << state.blend_src_factor << ','
                      << state.blend_dst_factor << ')'
                      << " alpha=" << state.alpha_compare_0 << '@'
                      << static_cast<unsigned>(state.alpha_ref_0) << " op="
                      << state.alpha_op << ' ' << state.alpha_compare_1 << '@'
                      << static_cast<unsigned>(state.alpha_ref_1) << '\n';
        }
        if (drawn.display_list_errors != 0 || drawn.rejected_indices != 0) {
            return 1;
        }
    }

    if (melee_host_scene_graphics_release(model) != MELEE_HOST_OK) {
        std::cerr << "scene release failed: "
                  << melee_host_scene_graphics_last_error() << '\n';
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        if (argc == 2 && std::string(argv[1]) == "--diagnose") {
            return diagnose();
        }
        if (argc == 2 && std::string(argv[1]) == "--inspect-match-rules") {
            return inspect_match_rules();
        }
        if (argc == 2 && std::string(argv[1]) == "--reset-match-rules") {
            return reset_match_rules();
        }
        if (argc == 2 && std::string(argv[1]) == "--diagnose-native-menu") {
            return diagnose_native_menu();
        }
        if (argc == 2 && std::string(argv[1]) == "--diagnose-local-match") {
            return diagnose_local_match();
        }
        if (argc >= 2 && std::string(argv[1]) == "--diagnose-scene-runtime") {
            unsigned frames = 60;
            if (argc == 3) {
                frames = static_cast<unsigned>(std::stoul(argv[2]));
            } else if (argc != 2) {
                print_usage(argv[0]);
                return 2;
            }
            return diagnose_scene_runtime(frames);
        }
        if (argc == 3 && std::string(argv[1]) == "--inspect-hsd") {
            return inspect_hsd(argv[2]);
        }
        if (argc >= 3 && std::string(argv[1]) == "--load-archive") {
            return load_archive(
                argv[2], std::vector<const char*>(argv + 3, argv + argc));
        }
        if (argc == 3 && std::string(argv[1]) == "--sweep-archives") {
            return sweep_archives(argv[2]);
        }
        if (argc == 3 && std::string(argv[1]) == "--boot-title-archive") {
            return boot_title_archive(argv[2]);
        }
        if (argc == 3 && std::string(argv[1]) == "--boot-title-scene") {
            MeleeHostContext* context = nullptr;
            const std::string root = argv[2];
            const MeleeHostConfig config{ .resource_root = root.c_str(),
                                          .headless = true };
            if (melee_host_create(&config, &context) != MELEE_HOST_OK ||
                melee_host_activate_dvd_backend(context) != MELEE_HOST_OK ||
                melee_host_boot_memory_init(0) != MELEE_HOST_OK ||
                melee_host_boot_load_dol_data(
                    (root + "/sys/main.dol").c_str()) != MELEE_HOST_OK) {
                std::cerr << "boot failed\n";
                melee_host_destroy(context);
                return 1;
            }
            melee_host_title_scene_enter();
            melee_host_destroy(context);
            return 0;
        }
        if ((argc == 4 || argc == 5) &&
            std::string(argv[1]) == "--load-scene") {
            mh_u32 model_index = 0;
            if (argc == 5) {
                model_index = static_cast<mh_u32>(std::stoul(argv[4]));
            }
            return load_scene(argv[2], argv[3], true, model_index);
        }
        if (argc == 4 && std::string(argv[1]) == "--load-joint") {
            return load_scene(argv[2], argv[3], false, 0);
        }
        if ((argc == 4 || argc == 5) &&
            std::string(argv[1]) == "--render-scene") {
            mh_u32 model_index = 0;
            if (argc == 5) {
                model_index = static_cast<mh_u32>(std::stoul(argv[4]));
            }
            return load_scene(argv[2], argv[3], true, model_index, true);
        }
        if (argc == 4 && std::string(argv[1]) == "--render-joint") {
            return load_scene(argv[2], argv[3], false, 0, true);
        }
#if defined(MELEE_HOST_SDL_RENDERER)
        if (argc == 6 && std::string(argv[1]) == "--view-animation") {
            return view_animation(argv[2], argv[3], argv[4], argv[5]);
        }
#endif
        if (argc == 3 && std::string(argv[1]) == "--list-animations") {
            return list_animations(argv[2]);
        }
        if ((argc == 6 || argc == 7) &&
            std::string(argv[1]) == "--animate-named") {
            const unsigned frames =
                argc == 7 ? static_cast<unsigned>(std::stoul(argv[6])) : 60U;
            return animate_named(argv[2], argv[3], argv[4], argv[5], frames);
        }
        if ((argc == 7 || argc == 8) &&
            std::string(argv[1]) == "--animate-joint") {
            const unsigned frames =
                argc == 8 ? static_cast<unsigned>(std::stoul(argv[7])) : 30U;
            const char* const anim = std::string(argv[5]) == "-" ? nullptr
                                                                 : argv[5];
            const char* const mat = std::string(argv[6]) == "-" ? nullptr
                                                                : argv[6];
            return animate_scene(argv[2], argv[3], false, argv[4], anim, mat,
                                 frames, 1.0F);
        }
#if defined(MELEE_HOST_SDL_RENDERER)
        if ((argc == 4 || argc == 5) &&
            std::string(argv[1]) == "--view-scene") {
            mh_u32 model_index = 0;
            if (argc == 5) {
                model_index = static_cast<mh_u32>(std::stoul(argv[4]));
            }
            return view_scene(argv[2], argv[3], true, model_index);
        }
        if (argc == 4 && std::string(argv[1]) == "--view-joint") {
            return view_scene(argv[2], argv[3], false, 0);
        }
        if ((argc == 4 || argc == 5) &&
            std::string(argv[1]) == "--tev-conformance-scene") {
            mh_u32 model_index = 0;
            if (argc == 5) {
                model_index = static_cast<mh_u32>(std::stoul(argv[4]));
            }
            return tev_conformance(argv[2], argv[3], true, model_index);
        }
        if (argc == 4 && std::string(argv[1]) == "--tev-conformance-joint") {
            return tev_conformance(argv[2], argv[3], false, 0);
        }
#endif
        if (argc == 4 && std::string(argv[1]) == "--inspect-pobj") {
            return inspect_pobj(argv[2], argv[3]);
        }
#if defined(MELEE_HOST_SDL_RENDERER)
        if (argc == 4 && std::string(argv[1]) == "--view-pobj") {
            return inspect_pobj(argv[2], argv[3], true);
        }
#endif
        if (argc == 3 && std::string(argv[1]) == "--inspect-resources") {
            return inspect_resources(argv[2]);
        }
        if (argc == 4 && std::string(argv[1]) == "--read-resource") {
            return read_resource(argv[2], argv[3]);
        }
        print_usage(argv[0]);
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

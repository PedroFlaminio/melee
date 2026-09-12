#include "assets/hsd_archive.hpp"
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
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/pad.h>
#include <melee_host/baselib.h>
#include <melee_host/gx.h>
#include <melee_host/host.h>
#include <melee_host/input.h>
#include <melee_host/local_match.h>
#include <melee_host/match_rules.h>
#include <melee_host/menu_native.h>
#include <melee_host/scene_runtime.h>

#include <algorithm>
#include <array>
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
              << "  " << executable << " --inspect-pobj FILE SYMBOL\n"
#if defined(MELEE_HOST_SDL_RENDERER)
              << "  " << executable << " --view-pobj FILE SYMBOL\n"
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

    std::cout << "native HSD scene runtime\n"
              << "  frames run: " << (end.frame_count - start.frame_count)
              << "\n  live objects: " << end.objects_live
              << "\n  live processes: " << end.procs_live
              << "\n  pooled objects: " << end.objects_pooled
              << "\n  unpaused process calls: " << running.frames
              << "\n  paused process calls: " << paused.frames << '\n';

    const bool ok = running.frames == frames && paused.frames == 0 &&
                    end.frame_count - start.frame_count == frames;
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
                        geometry.material.texture_wrap_t, decoded.rgba
                    });
                }
#else
                static_cast<void>(decoded);
#endif
                ++decoded_textures;
            }
        }
    }

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

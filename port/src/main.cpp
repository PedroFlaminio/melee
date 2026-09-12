#include "assets/hsd_archive.hpp"
#include "assets/hsd_runtime_archive.hpp"
#include "assets/schemas/db_common.hpp"
#include "assets/schemas/scene_graphics.hpp"
#include "assets/virtual_disc.hpp"

#include <dolphin/dvd.h>
#include <dolphin/gx/GXDispList.h>
#include <dolphin/gx/GXGeometry.h>
#include <melee_host/baselib.h>
#include <melee_host/gx.h>
#include <melee_host/host.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
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
              << "  " << executable << " --inspect-hsd FILE\n"
              << "  " << executable << " --inspect-pobj FILE SYMBOL\n"
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

int inspect_pobj(const std::filesystem::path& path, std::string_view symbol)
{
    const std::vector<std::byte> bytes = read_file(path);
    const melee::assets::HsdRuntimeArchive runtime(bytes);
    const auto geometries =
        melee::assets::schemas::find_scene_pobjs(runtime, symbol);

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
                GXSetArray(attribute, array.data(),
                           static_cast<u8>(descriptor.stride));
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
        display_bytes += display.size();
    }

    std::cout << "HSD scene geometry: " << path << " :: " << symbol << '\n'
              << "  drawable PObjs: " << geometries.size() << '\n'
              << "  display list bytes: " << display_bytes << '\n'
              << "  decoded vertices: "
              << melee_host_gx_captured_vertex_count() << '\n'
              << "  decoded triangles: " << melee_host_gx_triangle_count()
              << '\n'
              << "  display list errors: "
              << melee_host_gx_display_list_error_count() << '\n';
    return melee_host_gx_display_list_error_count() == 0 ? 0 : 1;
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
        if (argc == 3 && std::string(argv[1]) == "--inspect-hsd") {
            return inspect_hsd(argv[2]);
        }
        if (argc == 4 && std::string(argv[1]) == "--inspect-pobj") {
            return inspect_pobj(argv[2], argv[3]);
        }
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

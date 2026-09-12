#include "assets/hsd_archive.hpp"
#include "assets/hsd_runtime_archive.hpp"
#include "assets/schemas/db_common.hpp"
#include "assets/virtual_disc.hpp"

#include <dolphin/dvd.h>
#include <melee_host/baselib.h>
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

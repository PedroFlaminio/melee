#include "test.hpp"

#include "assets/virtual_disc.hpp"

#include <melee_host/host.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TemporaryDirectory final {
public:
    TemporaryDirectory()
    {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        path_ = std::filesystem::temp_directory_path() /
                ("melee-host-test-" + std::to_string(now.count()));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void append_be32(std::vector<std::byte>& output, std::uint32_t value)
{
    output.push_back(static_cast<std::byte>(value >> 24U));
    output.push_back(static_cast<std::byte>(value >> 16U));
    output.push_back(static_cast<std::byte>(value >> 8U));
    output.push_back(static_cast<std::byte>(value));
}

void append_be64(std::vector<std::byte>& output, std::uint64_t value)
{
    append_be32(output, static_cast<std::uint32_t>(value >> 32U));
    append_be32(output, static_cast<std::uint32_t>(value));
}

void write_index(const std::filesystem::path& directory, std::uint32_t entry,
                 std::string_view path, std::uint64_t size)
{
    std::vector<std::byte> bytes;
    for (const char value : std::array{ 'M', 'D', 'V', 'D', 'I', 'D', 'X', '1' }) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    append_be32(bytes, 1);
    append_be32(bytes, 1);
    append_be32(bytes, entry);
    append_be64(bytes, size);
    append_be32(bytes, static_cast<std::uint32_t>(path.size()));
    bytes.insert(bytes.end(), 20, std::byte{ 0 });
    for (const char value : path) {
        bytes.push_back(static_cast<std::byte>(value));
    }

    std::ofstream stream(directory / "dvd-index.bin", std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
}

} // namespace

TEST_CASE("virtual DVD resolves FST entries without exposing host paths")
{
    TemporaryDirectory temporary;
    const auto file = temporary.path() / "Pl" / "PlFx.dat";
    std::filesystem::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << "melee";
    write_index(temporary.path(), 42, "Pl/PlFx.dat", 5);

    const melee::assets::VirtualDisc disc(temporary.path());
    REQUIRE(disc.entry_count() == 1);
    REQUIRE(disc.entry(42).relative_path.generic_string() == "Pl/PlFx.dat");
    REQUIRE(disc.entry("Pl/PlFx.dat").entry_number == 42);
    const auto contents = disc.read(42);
    REQUIRE(contents.size() == 5);
    REQUIRE(std::to_integer<char>(contents[0]) == 'm');
    const auto range = disc.read_range(42, 1, 3);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(range.data()),
                             range.size()) == "ele");
}

TEST_CASE("host context exposes virtual DVD reads through a C ABI")
{
    TemporaryDirectory temporary;
    const auto file = temporary.path() / "Pl" / "PlFx.dat";
    std::filesystem::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << "melee";
    write_index(temporary.path(), 42, "Pl/PlFx.dat", 5);

    const auto root = temporary.path().string();
    const MeleeHostConfig config{ .resource_root = root.c_str(), .headless = true };
    MeleeHostContext* context = nullptr;
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_dvd_entry_count(context) == 1);

    mh_u32 entry_number = 0;
    REQUIRE(melee_host_dvd_entry_from_path(context, "Pl/PlFx.dat", &entry_number) ==
            MELEE_HOST_OK);
    REQUIRE(entry_number == 42);

    size_t size = 0;
    REQUIRE(melee_host_dvd_read_entry(context, entry_number, nullptr, 0, &size) ==
            MELEE_HOST_OK);
    REQUIRE(size == 5);
    std::array<char, 5> output{};
    REQUIRE(melee_host_dvd_read_entry(context, entry_number, output.data(),
                                      output.size(), &size) == MELEE_HOST_OK);
    REQUIRE(std::string_view(output.data(), output.size()) == "melee");
    std::array<char, 3> range{};
    REQUIRE(melee_host_dvd_read_entry_range(context, entry_number, 1,
                                             range.data(), range.size()) ==
            MELEE_HOST_OK);
    REQUIRE(std::string_view(range.data(), range.size()) == "ele");
    melee_host_destroy(context);
}

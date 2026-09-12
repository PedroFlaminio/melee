#include "test.hpp"

#include <dolphin/dvd.h>
#include <melee_host/host.h>
extern "C" {
#include <sysdolphin/baselib/devcom.h>
}

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TemporaryDirectory final {
public:
    TemporaryDirectory()
    {
        path_ = std::filesystem::temp_directory_path() /
                ("melee-dvd-test-" + std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path_ / "Pl");
    }
    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }
    const std::filesystem::path& path() const { return path_; }
private:
    std::filesystem::path path_;
};

void append_be32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (int shift = 24; shift >= 0; shift -= 8) {
        bytes.push_back(static_cast<std::byte>(value >> shift));
    }
}

void write_index(const std::filesystem::path& root, std::uint32_t size = 6)
{
    std::vector<std::byte> bytes;
    for (char value : std::array{ 'M', 'D', 'V', 'D', 'I', 'D', 'X', '1' }) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    append_be32(bytes, 1);
    append_be32(bytes, 1);
    append_be32(bytes, 7);
    append_be32(bytes, 0);
    append_be32(bytes, size);
    append_be32(bytes, 11);
    bytes.insert(bytes.end(), 20, std::byte{ 0 });
    for (char value : std::string_view("Pl/PlFx.dat")) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    std::ofstream(root / "dvd-index.bin", std::ios::binary)
        .write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}

struct AsyncResult {
    bool called = false;
    s32 result = DVD_RESULT_FATAL_ERROR;
};

void read_complete(s32 result, DVDFileInfo* file_info)
{
    auto* state = static_cast<AsyncResult*>(file_info->cb.userData);
    state->called = true;
    state->result = result;
}

struct DevComResult {
    bool called = false;
    bool canceled = true;
};

void devcom_complete(int, HSD_DevComArg argument, void*, bool canceled)
{
    auto* state = reinterpret_cast<DevComResult*>(argument);
    state->called = true;
    state->canceled = canceled;
}

} // namespace

TEST_CASE("synchronous Dolphin DVD facade reads indexed resources")
{
    TemporaryDirectory temporary;
    std::ofstream(temporary.path() / "Pl" / "PlFx.dat", std::ios::binary) << "native";
    write_index(temporary.path());
    const std::string root = temporary.path().string();
    const MeleeHostConfig config{ .resource_root = root.c_str(), .headless = true };
    MeleeHostContext* context = nullptr;
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_activate_dvd_backend(context) == MELEE_HOST_OK);
    REQUIRE(DVDConvertPathToEntrynum("/Pl/PlFx.dat") == 7);

    DVDFileInfo file{};
    REQUIRE(DVDFastOpen(7, &file) != 0);
    REQUIRE(file.length == 6);
    std::array<char, 3> bytes{};
    REQUIRE(DVDReadPrio(&file, bytes.data(), 3, 2, 2) == 3);
    REQUIRE(std::string_view(bytes.data(), bytes.size()) == "tiv");
    REQUIRE(DVDSeekPrio(&file, 6, 2) == 6);
    REQUIRE(DVDClose(&file) != 0);
    melee_host_destroy(context);
}

TEST_CASE("asynchronous DVD reads complete on the next simulation tick")
{
    TemporaryDirectory temporary;
    std::ofstream(temporary.path() / "Pl" / "PlFx.dat", std::ios::binary) << "native";
    write_index(temporary.path());
    const std::string root = temporary.path().string();
    const MeleeHostConfig config{ .resource_root = root.c_str(), .headless = true };
    MeleeHostContext* context = nullptr;
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_activate_dvd_backend(context) == MELEE_HOST_OK);

    DVDFileInfo file{};
    REQUIRE(DVDFastOpen(7, &file) != 0);
    AsyncResult result{};
    file.cb.userData = &result;
    std::array<char, 4> bytes{};
    REQUIRE(DVDReadAsyncPrio(&file, bytes.data(), 4, 1, read_complete, 2) != 0);
    REQUIRE(file.cb.state == DVD_STATE_WAITING);
    REQUIRE(result.called == false);

    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    REQUIRE(result.called == true);
    REQUIRE(result.result == 4);
    REQUIRE(file.cb.state == DVD_STATE_END);
    REQUIRE(std::string_view(bytes.data(), bytes.size()) == "ativ");
    REQUIRE(DVDClose(&file) != 0);
    melee_host_destroy(context);
}

TEST_CASE("original baselib devcom queue reads through the host DVD scheduler")
{
    TemporaryDirectory temporary;
    const std::string contents = "0123456789abcdefghijklmnopqrstuv";
    REQUIRE(contents.size() == 32);
    std::ofstream(temporary.path() / "Pl" / "PlFx.dat", std::ios::binary)
        << contents;
    write_index(temporary.path(), 32);
    const std::string root = temporary.path().string();
    const MeleeHostConfig config{ .resource_root = root.c_str(), .headless = true };
    MeleeHostContext* context = nullptr;
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_activate_dvd_backend(context) == MELEE_HOST_OK);

    alignas(32) std::array<char, 32> output{};
    DevComResult completion{};
    const int request = HSD_DevComRequest(
        7, 0, reinterpret_cast<uintptr_t>(output.data()), output.size(), 0x21,
        0, devcom_complete, &completion);
    REQUIRE(request > 0);
    REQUIRE(HSD_DevComIsBusy(request & 3));
    REQUIRE(completion.called == false);

    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    REQUIRE(completion.called == true);
    REQUIRE(completion.canceled == false);
    REQUIRE(HSD_DevComIsBusy(request & 3) == false);
    REQUIRE(std::string_view(output.data(), output.size()) == contents);
    melee_host_destroy(context);
}

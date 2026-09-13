#include "test.hpp"

#include <dolphin/dvd.h>
#include <melee_host/host.h>
extern "C" {
#include <dolphin/ar.h>
#include <sysdolphin/baselib/devcom.h>
}

#include <algorithm>
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

TEST_CASE("a DVD read may end short of one transfer unit past the file")
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
    // lbFile rounds a six-byte file up to one 32-byte unit.  The SDK accepts
    // a read ending less than DVD_MIN_TRANSFER_SIZE past the file, and what
    // lies beyond it on the disc is padding.
    std::array<char, 32> bytes{};
    bytes.fill('x');
    REQUIRE(DVDReadPrio(&file, bytes.data(), 32, 0, 2) == 32);
    REQUIRE(std::string_view(bytes.data(), 6) == "native");
    REQUIRE(bytes[6] == 0);
    REQUIRE(bytes[31] == 0);
    // A read reaching a whole unit past the file is out of range, as is one
    // starting beyond it.
    std::array<char, 38> too_long{};
    REQUIRE(DVDReadPrio(&file, too_long.data(), 38, 0, 2) ==
            DVD_RESULT_FATAL_ERROR);
    REQUIRE(DVDReadPrio(&file, bytes.data(), 1, 7, 2) ==
            DVD_RESULT_FATAL_ERROR);
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

namespace {

std::vector<const ARQRequest*> completed_transfers;

void record_transfer(ARQRequest* request)
{
    completed_transfers.push_back(request);
}

} // namespace

TEST_CASE("ARAM transfers complete on the next backend step, in posting order")
{
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    MeleeHostContext* context = nullptr;
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_activate_dvd_backend(context) == MELEE_HOST_OK);
    completed_transfers.clear();

    const std::string_view contents = "0123456789abcdefghijklmnopqrstuv";
    const u32 block = ARAlloc(32);
    std::array<char, 32> source{};
    std::copy(contents.begin(), contents.end(), source.begin());
    std::array<char, 32> copy{};

    // The read is posted after the write, so it sees what the write stored.
    ARQRequest write{};
    ARQRequest read{};
    ARQPostRequest(&write, 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
                   reinterpret_cast<uintptr_t>(source.data()), block, 32,
                   record_transfer);
    ARQPostRequest(&read, 0, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH, block,
                   reinterpret_cast<uintptr_t>(copy.data()), 32,
                   record_transfer);
    REQUIRE(completed_transfers.empty());
    REQUIRE(copy[0] == 0);

    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    REQUIRE(completed_transfers.size() == 2);
    REQUIRE(completed_transfers[0] == &write);
    REQUIRE(completed_transfers[1] == &read);
    REQUIRE(std::string_view(copy.data(), copy.size()) == contents);

    REQUIRE(ARFree(nullptr) == block);
    melee_host_destroy(context);
}

TEST_CASE("devcom reads into ARAM through a relay buffer with a request queued behind")
{
    TemporaryDirectory temporary;
    const std::string contents = "0123456789abcdefghijklmnopqrstuv";
    std::ofstream(temporary.path() / "Pl" / "PlFx.dat", std::ios::binary)
        << contents;
    write_index(temporary.path(), 32);
    const std::string root = temporary.path().string();
    const MeleeHostConfig config{ .resource_root = root.c_str(), .headless = true };
    MeleeHostContext* context = nullptr;
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_activate_dvd_backend(context) == MELEE_HOST_OK);

    // A destination below 0x80000000 is ARAM, which type 0x23 reaches through
    // a relay buffer.  devcom posts the last ARAM transfer before it unlinks
    // the request, and the transfer's callback returns the request to the free
    // list; the second request stays queued only if that callback comes later.
    const u32 first_block = ARAlloc(32);
    const u32 second_block = ARAlloc(32);
    DevComResult first{};
    DevComResult second{};
    const int first_request = HSD_DevComRequest(
        7, 0, first_block, 32, 0x23, 0, devcom_complete, &first);
    const int second_request = HSD_DevComRequest(
        7, 0, second_block, 32, 0x23, 0, devcom_complete, &second);
    REQUIRE((first_request & 3) == (second_request & 3));

    for (int step = 0; step < 8 && !(first.called && second.called); ++step) {
        REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    }
    REQUIRE(first.called == true);
    REQUIRE(first.canceled == false);
    REQUIRE(second.called == true);
    REQUIRE(second.canceled == false);
    REQUIRE(HSD_DevComIsBusy(first_request & 3) == false);

    std::array<char, 32> first_copy{};
    std::array<char, 32> second_copy{};
    ARQRequest read_first{};
    ARQRequest read_second{};
    ARQPostRequest(&read_first, 0, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH,
                   first_block, reinterpret_cast<uintptr_t>(first_copy.data()),
                   32, nullptr);
    ARQPostRequest(&read_second, 0, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH,
                   second_block,
                   reinterpret_cast<uintptr_t>(second_copy.data()), 32,
                   nullptr);
    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    REQUIRE(std::string_view(first_copy.data(), first_copy.size()) == contents);
    REQUIRE(std::string_view(second_copy.data(), second_copy.size()) ==
            contents);

    REQUIRE(ARFree(nullptr) == second_block);
    REQUIRE(ARFree(nullptr) == first_block);
    melee_host_destroy(context);
}

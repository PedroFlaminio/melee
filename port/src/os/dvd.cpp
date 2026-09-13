#include <melee_host/host.h>

#include <dolphin/dvd.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>

namespace {

std::mutex dvd_mutex;
MeleeHostContext* active_context = nullptr;
std::unordered_map<DVDFileInfo*, mh_u32> open_files;

bool entry_size(mh_u32 entry, size_t* out_size)
{
    return melee_host_dvd_read_entry(active_context, entry, nullptr, 0, out_size) ==
           MELEE_HOST_OK;
}

bool entry_for_file(DVDFileInfo* file_info, mh_u32* out_entry)
{
    const auto found = open_files.find(file_info);
    if (found == open_files.end()) {
        return false;
    }
    *out_entry = found->second;
    return true;
}

struct AsyncRead {
    DVDFileInfo* file_info;
    void* destination;
    s32 length;
    s32 offset;
    DVDCallback callback;
    s32 priority;
};

struct AsyncSeek {
    DVDFileInfo* file_info;
    long offset;
    void (*callback)(long, DVDFileInfo*);
    long priority;
};

void execute_async_read(void* user_data)
{
    auto* operation = static_cast<AsyncRead*>(user_data);
    const long result = DVDReadPrio(operation->file_info, operation->destination,
                                    operation->length, operation->offset,
                                    operation->priority);
    if (operation->callback != nullptr) {
        operation->callback(static_cast<s32>(result), operation->file_info);
    }
}

void execute_async_seek(void* user_data)
{
    auto* operation = static_cast<AsyncSeek*>(user_data);
    const long result = DVDSeekPrio(operation->file_info, operation->offset,
                                    operation->priority);
    if (operation->callback != nullptr) {
        operation->callback(result, operation->file_info);
    }
}

template <typename Operation> void delete_operation(void* user_data)
{
    delete static_cast<Operation*>(user_data);
}

} // namespace

extern "C" MeleeHostStatus
melee_host_activate_dvd_backend(MeleeHostContext* context)
{
    if (context == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    const std::lock_guard<std::mutex> lock(dvd_mutex);
    active_context = context;
    open_files.clear();
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_dvd_step_backend(void)
{
    MeleeHostContext* context = nullptr;
    {
        const std::lock_guard<std::mutex> lock(dvd_mutex);
        context = active_context;
    }
    if (context == nullptr) {
        return MELEE_HOST_UNSUPPORTED;
    }
    /* The step runs the read and its callback, which enter the DVD functions
     * and take the lock again, so it is released before stepping. */
    return melee_host_step(context);
}

extern "C" s32 DVDConvertPathToEntrynum(const char* path)
{
    if (path == nullptr) {
        return -1;
    }
    const std::lock_guard<std::mutex> lock(dvd_mutex);
    if (active_context == nullptr) {
        return -1;
    }
    while (*path == '/') {
        ++path;
    }
    mh_u32 entry = 0;
    if (melee_host_dvd_entry_from_path(active_context, path, &entry) != MELEE_HOST_OK ||
        entry > static_cast<mh_u32>(std::numeric_limits<s32>::max())) {
        return -1;
    }
    return static_cast<s32>(entry);
}

extern "C" BOOL DVDFastOpen(s32 entrynum, DVDFileInfo* file_info)
{
    if (entrynum < 0 || file_info == nullptr) {
        return FALSE;
    }
    const std::lock_guard<std::mutex> lock(dvd_mutex);
    if (active_context == nullptr) {
        return FALSE;
    }
    size_t size = 0;
    const mh_u32 entry = static_cast<mh_u32>(entrynum);
    if (!entry_size(entry, &size) ||
        size > static_cast<size_t>(std::numeric_limits<u32>::max())) {
        return FALSE;
    }
    std::memset(file_info, 0, sizeof(*file_info));
    file_info->length = static_cast<u32>(size);
    file_info->cb.state = DVD_STATE_END;
    open_files[file_info] = entry;
    return TRUE;
}

extern "C" BOOL DVDOpen(char* path, DVDFileInfo* file_info)
{
    const s32 entry = DVDConvertPathToEntrynum(path);
    return entry < 0 ? FALSE : DVDFastOpen(entry, file_info);
}

extern "C" BOOL DVDClose(DVDFileInfo* file_info)
{
    if (file_info == nullptr) {
        return FALSE;
    }
    const std::lock_guard<std::mutex> lock(dvd_mutex);
    return open_files.erase(file_info) != 0 ? TRUE : FALSE;
}

extern "C" long DVDReadPrio(DVDFileInfo* file_info, void* destination,
                             long length, long offset, long)
{
    if (file_info == nullptr || destination == nullptr || length < 0 || offset < 0) {
        return DVD_RESULT_FATAL_ERROR;
    }
    const std::lock_guard<std::mutex> lock(dvd_mutex);
    mh_u32 entry = 0;
    const unsigned long long start = static_cast<unsigned long long>(offset);
    const unsigned long long end =
        start + static_cast<unsigned long long>(length);
    /* The SDK accepts a read that ends less than DVD_MIN_TRANSFER_SIZE past
     * the file (dvdfs.c).  The drive moves data in 32-byte units and the game
     * rounds its sizes to them, so lbFile asks for whole units even when the
     * file does not fill the last one. */
    if (active_context == nullptr || !entry_for_file(file_info, &entry) ||
        start > file_info->length ||
        end >= static_cast<unsigned long long>(file_info->length) +
                   DVD_MIN_TRANSFER_SIZE) {
        return DVD_RESULT_FATAL_ERROR;
    }
    const auto available = static_cast<size_t>(
        std::min<unsigned long long>(end, file_info->length) - start);
    const auto status = melee_host_dvd_read_entry_range(
        active_context, entry, static_cast<mh_u64>(offset), destination,
        available);
    if (status != MELEE_HOST_OK) {
        file_info->cb.state = DVD_STATE_FATAL_ERROR;
        return DVD_RESULT_FATAL_ERROR;
    }
    /* Past the end of a file the disc holds padding, which the extracted file
     * does not carry, so the host supplies zeros. */
    std::memset(static_cast<char*>(destination) + available, 0,
                static_cast<size_t>(length) - available);
    file_info->cb.offset = static_cast<u32>(offset);
    file_info->cb.length = static_cast<u32>(length);
    file_info->cb.transferredSize = static_cast<u32>(length);
    file_info->cb.state = DVD_STATE_END;
    return length;
}

extern "C" BOOL DVDReadAsyncPrio(DVDFileInfo* file_info, void* destination,
                                  s32 length, s32 offset,
                                  DVDCallback callback, s32 priority)
{
    if (file_info == nullptr || destination == nullptr || length < 0 || offset < 0) {
        return FALSE;
    }
    auto* operation = new (std::nothrow)
        AsyncRead{ file_info, destination, length, offset, callback, priority };
    if (operation == nullptr) {
        return FALSE;
    }

    const std::lock_guard<std::mutex> lock(dvd_mutex);
    mh_u32 entry = 0;
    if (active_context == nullptr || !entry_for_file(file_info, &entry)) {
        delete operation;
        return FALSE;
    }
    file_info->cb.state = DVD_STATE_WAITING;
    file_info->callback = callback;
    const mh_u64 due_tick = melee_host_tick_count(active_context) + 1;
    if (melee_host_schedule_task_with_cleanup(
            active_context, due_tick, execute_async_read,
            delete_operation<AsyncRead>, operation) != MELEE_HOST_OK) {
        file_info->cb.state = DVD_STATE_FATAL_ERROR;
        delete operation;
        return FALSE;
    }
    return TRUE;
}

extern "C" long DVDSeekPrio(DVDFileInfo* file_info, long offset, long)
{
    if (file_info == nullptr || offset < 0 ||
        static_cast<unsigned long long>(offset) > file_info->length) {
        return DVD_RESULT_FATAL_ERROR;
    }
    const std::lock_guard<std::mutex> lock(dvd_mutex);
    mh_u32 entry = 0;
    if (!entry_for_file(file_info, &entry)) {
        return DVD_RESULT_FATAL_ERROR;
    }
    file_info->cb.offset = static_cast<u32>(offset);
    file_info->cb.state = DVD_STATE_END;
    return offset;
}

extern "C" int DVDSeekAsyncPrio(
    DVDFileInfo* file_info, long offset,
    void (*callback)(long, DVDFileInfo*), long priority)
{
    if (file_info == nullptr || offset < 0) {
        return FALSE;
    }
    auto* operation = new (std::nothrow)
        AsyncSeek{ file_info, offset, callback, priority };
    if (operation == nullptr) {
        return FALSE;
    }

    const std::lock_guard<std::mutex> lock(dvd_mutex);
    mh_u32 entry = 0;
    if (active_context == nullptr || !entry_for_file(file_info, &entry)) {
        delete operation;
        return FALSE;
    }
    file_info->cb.state = DVD_STATE_WAITING;
    const mh_u64 due_tick = melee_host_tick_count(active_context) + 1;
    if (melee_host_schedule_task_with_cleanup(
            active_context, due_tick, execute_async_seek,
            delete_operation<AsyncSeek>, operation) != MELEE_HOST_OK) {
        file_info->cb.state = DVD_STATE_FATAL_ERROR;
        delete operation;
        return FALSE;
    }
    return TRUE;
}

extern "C" long DVDGetFileInfoStatus(DVDFileInfo* file_info)
{
    return file_info == nullptr ? DVD_STATE_FATAL_ERROR : file_info->cb.state;
}

namespace melee::host {

void detach_dvd_backend(MeleeHostContext* context)
{
    const std::lock_guard<std::mutex> lock(dvd_mutex);
    if (active_context == context) {
        active_context = nullptr;
        open_files.clear();
    }
}

} // namespace melee::host

#include <melee_host/host.h>
#include <melee_host/input.h>

#include "assets/virtual_disc.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <vector>

namespace melee::host {
void detach_pad_backend(MeleeHostContext* context);
void detach_dvd_backend(MeleeHostContext* context);
}

namespace {

struct ScheduledTask {
    mh_u64 due_tick;
    mh_u64 sequence;
    MeleeHostTaskCallback callback;
    MeleeHostTaskCleanup cleanup;
    void* user_data;
};

} // namespace

struct MeleeHostContext {
    std::filesystem::path resource_root;
    std::unique_ptr<melee::assets::VirtualDisc> disc;
    bool headless = false;
    mh_u64 ticks = 0;
    mh_u64 next_task_sequence = 0;
    mutable std::mutex task_mutex;
    std::vector<ScheduledTask> tasks;
    mutable std::mutex input_mutex;
    MeleeHostInputSnapshot pending_input{};
    MeleeHostInputSnapshot current_input{};
};

extern "C" MeleeHostStatus
melee_host_create(const MeleeHostConfig* config, MeleeHostContext** out_context)
{
    if (config == nullptr || out_context == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }

    *out_context = nullptr;

    auto* context = new (std::nothrow) MeleeHostContext;
    if (context == nullptr) {
        return MELEE_HOST_INTERNAL_ERROR;
    }

    if (config->resource_root != nullptr) {
        context->resource_root = config->resource_root;
        if (!context->resource_root.empty()) {
            try {
                context->disc = std::make_unique<melee::assets::VirtualDisc>(
                    context->resource_root);
            } catch (const melee::assets::VirtualDiscError&) {
                delete context;
                return MELEE_HOST_IO_ERROR;
            }
        }
    }
    context->headless = config->headless;
    *out_context = context;
    return MELEE_HOST_OK;
}

extern "C" void melee_host_destroy(MeleeHostContext* context)
{
    if (context == nullptr) {
        return;
    }
    melee::host::detach_pad_backend(context);
    melee::host::detach_dvd_backend(context);
    for (const ScheduledTask& task : context->tasks) {
        if (task.cleanup != nullptr) {
            task.cleanup(task.user_data);
        }
    }
    delete context;
}

extern "C" MeleeHostStatus melee_host_dvd_read_entry_range(
    const MeleeHostContext* context, mh_u32 entry_number, mh_u64 offset,
    void* output, size_t length)
{
    if (context == nullptr || (output == nullptr && length != 0)) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (context->disc == nullptr) {
        return MELEE_HOST_NOT_READY;
    }
    try {
        const auto bytes = context->disc->read_range(entry_number, offset, length);
        if (!bytes.empty()) {
            std::memcpy(output, bytes.data(), bytes.size());
        }
        return MELEE_HOST_OK;
    } catch (const melee::assets::VirtualDiscError&) {
        return MELEE_HOST_IO_ERROR;
    }
}

extern "C" MeleeHostStatus melee_host_step(MeleeHostContext* context)
{
    if (context == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }

    std::vector<ScheduledTask> ready;
    {
        const std::lock_guard<std::mutex> lock(context->task_mutex);
        ++context->ticks;
        const auto first_pending = std::stable_partition(
            context->tasks.begin(), context->tasks.end(),
            [&](const ScheduledTask& task) { return task.due_tick > context->ticks; });
        ready.assign(first_pending, context->tasks.end());
        context->tasks.erase(first_pending, context->tasks.end());
    }
    {
        const std::lock_guard<std::mutex> lock(context->input_mutex);
        context->pending_input.tick = context->ticks;
        context->current_input = context->pending_input;
    }
    std::sort(ready.begin(), ready.end(), [](const ScheduledTask& left,
                                              const ScheduledTask& right) {
        if (left.due_tick != right.due_tick) {
            return left.due_tick < right.due_tick;
        }
        return left.sequence < right.sequence;
    });
    for (const ScheduledTask& task : ready) {
        task.callback(task.user_data);
        if (task.cleanup != nullptr) {
            task.cleanup(task.user_data);
        }
    }
    return MELEE_HOST_NOT_READY;
}

extern "C" MeleeHostStatus melee_host_submit_pad_state(
    MeleeHostContext* context, mh_u32 port, const MeleeHostPadState* state)
{
    if (context == nullptr || state == nullptr || port >= MELEE_HOST_MAX_CONTROLLERS) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    const std::lock_guard<std::mutex> lock(context->input_mutex);
    context->pending_input.pads[port] = *state;
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_input_snapshot(
    const MeleeHostContext* context, MeleeHostInputSnapshot* out_snapshot)
{
    if (context == nullptr || out_snapshot == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    const std::lock_guard<std::mutex> lock(context->input_mutex);
    *out_snapshot = context->current_input;
    return MELEE_HOST_OK;
}

extern "C" mh_u64 melee_host_tick_count(const MeleeHostContext* context)
{
    if (context == nullptr) {
        return 0;
    }
    const std::lock_guard<std::mutex> lock(context->task_mutex);
    return context->ticks;
}

extern "C" MeleeHostStatus melee_host_schedule_task(
    MeleeHostContext* context, mh_u64 due_tick, MeleeHostTaskCallback callback,
    void* user_data)
{
    return melee_host_schedule_task_with_cleanup(context, due_tick, callback,
                                                 nullptr, user_data);
}

extern "C" MeleeHostStatus melee_host_schedule_task_with_cleanup(
    MeleeHostContext* context, mh_u64 due_tick, MeleeHostTaskCallback callback,
    MeleeHostTaskCleanup cleanup, void* user_data)
{
    if (context == nullptr || callback == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }

    const std::lock_guard<std::mutex> lock(context->task_mutex);
    if (due_tick < context->ticks) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    context->tasks.push_back({ due_tick, context->next_task_sequence++, callback,
                               cleanup, user_data });
    return MELEE_HOST_OK;
}

extern "C" const char* melee_host_status_string(MeleeHostStatus status)
{
    switch (status) {
    case MELEE_HOST_OK:
        return "ok";
    case MELEE_HOST_INVALID_ARGUMENT:
        return "invalid argument";
    case MELEE_HOST_NOT_READY:
        return "not ready";
    case MELEE_HOST_IO_ERROR:
        return "I/O error";
    case MELEE_HOST_UNSUPPORTED:
        return "unsupported";
    case MELEE_HOST_INTERNAL_ERROR:
        return "internal error";
    }
    return "unknown status";
}

extern "C" size_t melee_host_dvd_entry_count(const MeleeHostContext* context)
{
    if (context == nullptr || context->disc == nullptr) {
        return 0;
    }
    return context->disc->entry_count();
}

extern "C" MeleeHostStatus melee_host_dvd_entry_from_path(
    const MeleeHostContext* context, const char* path, mh_u32* out_entry_number)
{
    if (context == nullptr || path == nullptr || out_entry_number == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (context->disc == nullptr) {
        return MELEE_HOST_NOT_READY;
    }

    try {
        *out_entry_number = context->disc->entry(path).entry_number;
        return MELEE_HOST_OK;
    } catch (const melee::assets::VirtualDiscError&) {
        return MELEE_HOST_IO_ERROR;
    }
}

extern "C" MeleeHostStatus melee_host_dvd_read_entry(
    const MeleeHostContext* context, mh_u32 entry_number, void* output,
    size_t output_capacity, size_t* out_size)
{
    if (context == nullptr || out_size == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (context->disc == nullptr) {
        return MELEE_HOST_NOT_READY;
    }

    try {
        const auto bytes = context->disc->read(entry_number);
        *out_size = bytes.size();
        if (output == nullptr) {
            return output_capacity == 0 ? MELEE_HOST_OK
                                        : MELEE_HOST_INVALID_ARGUMENT;
        }
        if (output_capacity < bytes.size()) {
            return MELEE_HOST_IO_ERROR;
        }
        std::memcpy(output, bytes.data(), bytes.size());
        return MELEE_HOST_OK;
    } catch (const melee::assets::VirtualDiscError&) {
        return MELEE_HOST_IO_ERROR;
    }
}

#ifndef MELEE_HOST_HOST_H
#define MELEE_HOST_HOST_H

#include <melee_host/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MeleeHostStatus {
    MELEE_HOST_OK = 0,
    MELEE_HOST_INVALID_ARGUMENT = 1,
    MELEE_HOST_NOT_READY = 2,
    MELEE_HOST_IO_ERROR = 3,
    MELEE_HOST_UNSUPPORTED = 4,
    MELEE_HOST_INTERNAL_ERROR = 5,
} MeleeHostStatus;

typedef struct MeleeHostConfig {
    const char* resource_root;
    bool headless;
} MeleeHostConfig;

typedef struct MeleeHostContext MeleeHostContext;
typedef void (*MeleeHostTaskCallback)(void* user_data);
typedef void (*MeleeHostTaskCleanup)(void* user_data);

MeleeHostStatus melee_host_create(const MeleeHostConfig* config,
                                  MeleeHostContext** out_context);
void melee_host_destroy(MeleeHostContext* context);
MeleeHostStatus melee_host_step(MeleeHostContext* context);
mh_u64 melee_host_tick_count(const MeleeHostContext* context);
MeleeHostStatus melee_host_schedule_task(MeleeHostContext* context,
                                         mh_u64 due_tick,
                                         MeleeHostTaskCallback callback,
                                         void* user_data);
MeleeHostStatus melee_host_schedule_task_with_cleanup(
    MeleeHostContext* context, mh_u64 due_tick,
    MeleeHostTaskCallback callback, MeleeHostTaskCleanup cleanup,
    void* user_data);
const char* melee_host_status_string(MeleeHostStatus status);

mh_u64 melee_host_monotonic_nanoseconds(void);
size_t melee_host_dvd_entry_count(const MeleeHostContext* context);
MeleeHostStatus melee_host_dvd_entry_from_path(const MeleeHostContext* context,
                                               const char* path,
                                               mh_u32* out_entry_number);
MeleeHostStatus melee_host_dvd_read_entry(const MeleeHostContext* context,
                                          mh_u32 entry_number, void* output,
                                          size_t output_capacity,
                                          size_t* out_size);
MeleeHostStatus melee_host_dvd_read_entry_range(const MeleeHostContext* context,
                                                mh_u32 entry_number,
                                                mh_u64 offset, void* output,
                                                size_t length);
MeleeHostStatus melee_host_activate_dvd_backend(MeleeHostContext* context);

#ifdef __cplusplus
}
#endif

#endif

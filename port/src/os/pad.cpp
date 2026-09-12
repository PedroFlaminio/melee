#include <melee_host/input.h>

#include <dolphin/pad.h>

#include <cstring>
#include <mutex>

namespace {

std::mutex active_context_mutex;
MeleeHostContext* active_context = nullptr;
unsigned long active_spec = PAD_SPEC_5;

void clear_active_context(MeleeHostContext* context)
{
    const std::lock_guard<std::mutex> lock(active_context_mutex);
    if (active_context == context) {
        active_context = nullptr;
    }
}

} // namespace

extern "C" MeleeHostStatus
melee_host_activate_pad_backend(MeleeHostContext* context)
{
    if (context == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    const std::lock_guard<std::mutex> lock(active_context_mutex);
    active_context = context;
    return MELEE_HOST_OK;
}

extern "C" BOOL PADInit()
{
    return TRUE;
}

extern "C" u32 PADRead(PADStatus* status)
{
    if (status == nullptr) {
        return 0;
    }

    const std::lock_guard<std::mutex> lock(active_context_mutex);
    MeleeHostInputSnapshot snapshot{};
    if (active_context == nullptr ||
        melee_host_input_snapshot(active_context, &snapshot) != MELEE_HOST_OK) {
        for (int port = 0; port < PAD_MAX_CONTROLLERS; ++port) {
            std::memset(&status[port], 0, sizeof(PADStatus));
            status[port].err = PAD_ERR_NO_CONTROLLER;
        }
        return 0;
    }

    u32 unavailable_mask = 0;
    for (int port = 0; port < PAD_MAX_CONTROLLERS; ++port) {
        const MeleeHostPadState& pad = snapshot.pads[port];
        PADStatus& output = status[port];
        std::memset(&output, 0, sizeof(output));
        output.button = pad.buttons;
        output.stickX = pad.stick_x;
        output.stickY = pad.stick_y;
        output.substickX = pad.c_stick_x;
        output.substickY = pad.c_stick_y;
        output.triggerLeft = pad.trigger_left;
        output.triggerRight = pad.trigger_right;
        output.err = pad.connected ? PAD_ERR_NONE : PAD_ERR_NO_CONTROLLER;
        if (!pad.connected) {
            unavailable_mask |= PAD_CHAN0_BIT >> port;
        }
    }
    return unavailable_mask;
}

extern "C" int PADReset(unsigned long)
{
    return TRUE;
}

extern "C" BOOL PADRecalibrate(u32)
{
    return TRUE;
}

extern "C" void PADSetSamplingRate(unsigned long) {}
extern "C" void __PADTestSamplingRate(unsigned long) {}
extern "C" void PADControlAllMotors(const u32*) {}
extern "C" void PADControlMotor(s32, u32) {}
extern "C" void PADSetSpec(u32 spec) { active_spec = spec; }
extern "C" unsigned long PADGetSpec() { return active_spec; }
extern "C" int PADGetType(long, unsigned long* type)
{
    if (type != nullptr) {
        *type = 0;
    }
    return PAD_ERR_NO_CONTROLLER;
}
extern "C" BOOL PADSync(void) { return TRUE; }
extern "C" void PADSetAnalogMode(u32) {}
extern "C" BOOL __PADDisableRecalibration(int) { return TRUE; }
extern "C" void SIRefreshSamplingRate(void) {}

namespace melee::host {

void detach_pad_backend(MeleeHostContext* context)
{
    clear_active_context(context);
}

} // namespace melee::host

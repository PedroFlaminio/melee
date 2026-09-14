/* Runs lbcommand.c's generic commands over a command stream the host archive
 * API converted, for hsd_host_archive_test.cpp, which cannot include the
 * game's C types. */

#include <melee/lb/lbcommand.h>
#include <melee/lb/types.h>

#include <stddef.h>

typedef struct MeleeHostCommandRun {
    int steps;
    float timer;
    unsigned loop_count;
    int finished;
} MeleeHostCommandRun;

void melee_host_test_run_generic_commands(void* stream, int max_steps,
                                          MeleeHostCommandRun* out);

void melee_host_test_run_generic_commands(void* stream, int max_steps,
                                          MeleeHostCommandRun* out)
{
    CommandInfo info = { 0 };

    info.u = stream;
    out->steps = 0;
    while (info.u != NULL && out->steps < max_steps) {
        if (!Command_Execute(&info, info.u->unk0.opcode)) {
            break;
        }
        out->steps += 1;
    }
    out->timer = info.timer;
    out->loop_count = info.loop_count;
    out->finished = info.u == NULL;
}

/* Runs lbcommand.c's generic commands over a command stream the host archive
 * API converted, for hsd_host_archive_test.cpp, which cannot include the
 * game's C types. */

#include <melee/ft/types.h>
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

void melee_host_test_read_script_event(unsigned word, unsigned* opcode,
                                       unsigned* value1);

/* ftAction_80073240 reads a fighter script word's opcode through ft/types.h's
 * gmScriptEventDefault, not through the lb/types.h commands. */
void melee_host_test_read_script_event(unsigned word, unsigned* opcode,
                                       unsigned* value1)
{
    const u32 native = word;
    const gmScriptEventDefault* const event =
        (const gmScriptEventDefault*) &native;

    *opcode = event->opcode;
    *value1 = event->value1;
}

void melee_host_test_read_fighter_anim_flags(unsigned flags, unsigned* loop,
                                             unsigned* first, unsigned* parts,
                                             unsigned* bone, unsigned* kind);

/* fighter.c and ftwaitanim.c store an action's x10_animCurrFlags in Fighter's
 * x594 union as one s32; ftanim.c and fighter.c read it back by field. */
void melee_host_test_read_fighter_anim_flags(unsigned flags, unsigned* loop,
                                             unsigned* first, unsigned* parts,
                                             unsigned* bone, unsigned* kind)
{
    static Fighter fighter;

    fighter.x594_s32 = (s32) flags;
    *loop = fighter.x594_b1_loop;
    *first = fighter.x594_b0;
    *parts = fighter.x594_bits;
    *bone = fighter.x596_bits.x7;
    *kind = fighter.x597_bits;
}

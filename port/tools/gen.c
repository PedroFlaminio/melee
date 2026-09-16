#include <stdio.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/kinds/ftFox/types.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/kinds/ftMario/types.h>
#include <melee/ft/kinds/ftCaptain/types.h>
#include <melee/ft/kinds/ftDonkey/types.h>
#include <melee/ft/kinds/ftDrMario/types.h>
#include <melee/ft/kinds/ftFalco/types.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/ft/kinds/ftGanon/types.h>
#include <melee/ft/kinds/ftKirby/types.h>
#include <melee/ft/kinds/ftKoopa/types.h>
#include <melee/ft/kinds/ftLuigi/types.h>
#include <melee/ft/kinds/ftMars/types.h>
#include <melee/ft/kinds/ftMewtwo/types.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/ft/kinds/ftPeach/types.h>
#include <melee/ft/kinds/ftPichu/types.h>
#include <melee/ft/kinds/ftPikachu/types.h>
#include <melee/ft/kinds/ftPopo/types.h>
#include <melee/ft/kinds/ftPurin/types.h>
#include <melee/ft/kinds/ftSamus/types.h>
#include <melee/ft/kinds/ftSeak/types.h>
#include <melee/ft/kinds/ftYoshi/types.h>
#include <melee/ft/kinds/ftZelda/types.h>
#include <melee/ft/kinds/ftCLink/types.h>
#include <melee/ft/kinds/ftEmblem/types.h>

#define P(name, type) printf("static void* fighter_%s_attrs(MeleeHostHsdReader* reader, mh_u32 at) {\n    %s* const attrs = melee_host_hsd_reader_allocate(reader, sizeof(*attrs), alignof(%s));\n    if (attrs == NULL) return NULL;\n    copy_words(reader, at, attrs, 0x00, 0x%zX);\n    return attrs;\n}\n", #name, #type, #type, sizeof(type));

int main() {
    P("drmario", ftDrMarioAttributes);
    P("falco", struct ftFalco_DatAttrs);
    P("gamewatch", ftGameWatchAttributes);
    P("ganon", ftGanonAttributes);
    P("kirby", struct ftKb_DatAttrs);
    P("koopa", ftKoopaAttributes);
    P("luigi", ftLuigiAttributes);
    P("mars", MarsAttributes);
    P("mewtwo", ftMewtwoAttributes);
    P("ness", ftNessAttributes);
    P("peach", ftPe_DatAttrs);
    P("pichu", ftPichuAttributes);
    P("pikachu", ftPikachuAttributes);
    P("popo", ftIceClimberAttributes);
    P("purin", ftPurinAttributes);
    P("samus", ftSs_DatAttrs);
    P("seak", ftSeakAttributes);
    P("yoshi", struct ftYs_DatAttrs);
    P("zelda", ftZelda_DatAttrs);
    P("clink", struct ftCLink_DatAttrs);
    P("emblem", EmblemAttributes);
    return 0;
}

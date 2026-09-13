/* Game data that lives in main.dol instead of a file on the disc.
 *
 * The matching build compiles these from bytes extracted out of the original
 * DOL (sislib_font.inc).  Those bytes belong to the game, so the host neither
 * builds them in nor ships them: it reads them at boot from the user's
 * extracted main.dol, at the address the game refers to them by.
 */

#include <melee_host/boot.h>
#include <melee_host/dol.h>

#include <sysdolphin/baselib/hsd_3915.h>
#include <sysdolphin/baselib/sislib_font.h>

/* Addresses from the headers' own annotations; sizes from
 * config/GALE01/symbols.txt (0x23E00 and 0x1C00 bytes). */
#define HSD_SIS_FONT_ATLAS_ADDRESS 0x8040CD40U
#define HSD_DEBUG_FONT_ATLAS_ADDRESS 0x804088B8U

TextGlyphTexture HSD_SisLib_FontAtlas[287] ATTRIBUTE_ALIGN(32);
DebugFontGlyph HSD_DebugFontAtlas[128];

MeleeHostStatus melee_host_boot_load_dol_data(const char* dol_path)
{
    MeleeHostStatus status = melee_host_dol_read(
        dol_path, HSD_SIS_FONT_ATLAS_ADDRESS, HSD_SisLib_FontAtlas,
        sizeof(HSD_SisLib_FontAtlas));
    if (status != MELEE_HOST_OK) {
        return status;
    }
    return melee_host_dol_read(dol_path, HSD_DEBUG_FONT_ATLAS_ADDRESS,
                               HSD_DebugFontAtlas, sizeof(HSD_DebugFontAtlas));
}

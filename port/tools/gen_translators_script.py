import sys

characters = [
    ("DrMario", "ftDrMarioAttributes", "ftDrMario/types.h"),
    ("Falco", "struct ftFox_DatAttrs", "ftFox/types.h"),
    ("GameWatch", "ftGameWatchAttributes", "ftGameWatch/types.h"),
    ("Ganon", "ftCaptain_DatAttrs", "ftCaptain/types.h"),
    ("Kirby", "struct ftKb_DatAttrs", "ftKirby/types.h"),
    ("Koopa", "ftKoopaAttributes", "ftKoopa/types.h"),
    ("Luigi", "ftLuigiAttributes", "ftLuigi/types.h"),
    ("Mars", "MarsAttributes", "ftMars/types.h"),
    ("Mewtwo", "ftMewtwoAttributes", "ftMewtwo/types.h"),
    ("Nana", "ftIceClimberAttributes", "ftPopo/types.h"),
    ("Ness", "ftNessAttributes", "ftNess/types.h"),
    ("Peach", "ftPe_DatAttrs", "ftPeach/types.h"),
    ("Pichu", "ftPichuAttributes", "ftPichu/types.h"),
    ("Pikachu", "ftPikachuAttributes", "ftPikachu/types.h"),
    ("Popo", "ftIceClimberAttributes", "ftPopo/types.h"),
    ("Purin", "ftPurinAttributes", "ftPurin/types.h"),
    ("Samus", "ftSs_DatAttrs", "ftSamus/types.h"),
    ("Seak", "ftSeakAttributes", "ftSeak/types.h"),
    ("Yoshi", "struct ftYs_DatAttrs", "ftYoshi/types.h"),
    ("Zelda", "ftZelda_DatAttrs", "ftZelda/types.h"),
    ("CLink", "struct ftLk_DatAttrs", "ftLink/types.h"),
    ("Emblem", "MarsAttributes", "ftMars/types.h"),
    ("GigaKoopa", "ftKoopaAttributes", "ftKoopa/types.h"),
]

with open('/home/pedro/projects/project-melee/melee/port/src/game/game_data_translators.c', 'r') as f:
    content = f.read()

# 1. Add missing includes
includes = []
for _, _, header in characters:
    inc = f'#include <melee/ft/kinds/{header}>'
    if inc not in content and inc not in includes:
        includes.append(inc)

if includes:
    include_str = "\n".join(includes)
    # inject after #include <melee/ft/kinds/ftFox/types.h>
    content = content.replace(
        "#include <melee/ft/kinds/ftFox/types.h>",
        f"#include <melee/ft/kinds/ftFox/types.h>\n{include_str}"
    )

# 2. Add attr parsers
parsers = []
for name, struct_type, _ in characters:
    lc_name = name.lower()
    parser = f"""
static void* fighter_{lc_name}_attrs(MeleeHostHsdReader* reader, mh_u32 at)
{{
    {struct_type}* const attrs = melee_host_hsd_reader_allocate(
        reader, sizeof(*attrs), alignof({struct_type}));

    if (attrs == NULL) {{
        return NULL;
    }}
    copy_words(reader, at, attrs, 0x00, sizeof(*attrs) & ~3);
    return attrs;
}}
"""
    if f"fighter_{lc_name}_attrs" not in content:
        parsers.append(parser)

if parsers:
    parsers_str = "".join(parsers)
    content = content.replace(
        "static Fighter_WaitAnimData* fighter_actions",
        f"{parsers_str}\nstatic Fighter_WaitAnimData* fighter_actions"
    )

# 3. Add fighter_data_* wrappers
wrappers = []
for name, _, _ in characters:
    lc_name = name.lower()
    wrapper = f"""
static void* fighter_data_{lc_name}(MeleeHostHsdReader* reader, mh_u32 root)
{{
    static const struct FighterItemAttrs {lc_name}_items = {{
        NULL, 0, NULL, 0, NULL, 0,
    }};
    return fighter_data(reader, root, fighter_{lc_name}_attrs, &{lc_name}_items);
}}
"""
    if f"fighter_data_{lc_name}" not in content:
        wrappers.append(wrapper)

if wrappers:
    wrappers_str = "".join(wrappers)
    content = content.replace(
        "void melee_host_game_register_data_translators(void)",
        f"{wrappers_str}\nvoid melee_host_game_register_data_translators(void)"
    )

# 4. Add registrations
regs = []
for name, _, _ in characters:
    lc_name = name.lower()
    reg = f'    (void) melee_host_hsd_register_translator("ftData{name}", fighter_data_{lc_name});'
    if reg not in content:
        regs.append(reg)

if regs:
    regs_str = "\n".join(regs)
    content = content.replace(
        '(void) melee_host_hsd_register_translator("ftDataDonkey", fighter_data_donkey);',
        f'(void) melee_host_hsd_register_translator("ftDataDonkey", fighter_data_donkey);\n{regs_str}'
    )

with open('/home/pedro/projects/project-melee/melee/port/src/game/game_data_translators.c', 'w') as f:
    f.write(content)


/* sysdolphin's archive API for a 64-bit host.
 *
 * archive.c parses the header in place and then adds the data base to every
 * relocated field, so the file itself becomes the object graph.  The game then
 * asks for a public symbol and casts what comes back.  Neither step survives
 * pointers eight bytes wide, so this file keeps the API and changes what is
 * behind it: the parse builds a validated view of the file, and a public
 * symbol is rebuilt in host layout by assets/hsd_materialize when it is first
 * requested.
 *
 * The file does not say what type a symbol has; the game knows from the name
 * it asks for.  The host reads the same thing from the name's suffix, and a
 * suffix it has no translation for is refused with a report rather than
 * returned as a pointer into bytes the caller would misread.
 */

#include <melee_host/hsd_archive.h>

#include "assets/hsd_archive.hpp"
#include "assets/hsd_materialize.hpp"
#include "assets/hsd_runtime_archive.hpp"

MELEE_HOST_HSD_BEGIN
#include <sysdolphin/baselib/archive.h>
/* dolphin/os.h is the only header declaring it, and it does not compile as
 * C++ (OSBootInfo_s reuses a type name for a member). */
void OSReport(const char* format, ...);
MELEE_HOST_HSD_END

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using melee::assets::HsdMaterializedArchive;
using melee::assets::HsdPublicSymbol;
using melee::assets::HsdRuntimeArchive;

constexpr std::size_t kHeaderSize = 0x20;
constexpr std::uint32_t kExternChainEnd = 0xFFFFFFFFU;

struct HostArchive {
    std::unique_ptr<HsdRuntimeArchive> runtime;
    std::unique_ptr<HsdMaterializedArchive> descriptors;
    std::vector<HsdPublicSymbol> publics;
    std::vector<HsdPublicSymbol> externs;
    /* A symbol asked for twice must come back as the same descriptor: the
     * game compares and stores these pointers, and the ID table keys joints by
     * address. */
    std::unordered_map<std::string, void*> translated;
};

struct Registry {
    /* Keyed by the buffer the file was parsed from, not by the HSD_Archive:
     * ftdata.c parses each action into an HSD_Archive on its stack and keeps
     * using what it found after that frame is gone. */
    std::unordered_map<const u8*, std::unique_ptr<HostArchive>> archives;
    std::string last_error = "no error";
    mh_u32 translated = 0;
    mh_u32 refused = 0;
    mh_u32 nulled = 0;
    mh_u32 externs_refused = 0;
};

Registry& registry()
{
    static Registry instance;
    return instance;
}

struct SuffixKind {
    std::string_view suffix;
    MeleeHostHsdSymbolKind kind;
};

/* Longest first where one suffix ends another. */
constexpr std::array<SuffixKind, 9> kSuffixes{ {
    { "_matanim_joint", MELEE_HOST_HSD_SYMBOL_MAT_ANIM_JOINT },
    { "_shapeanim_joint", MELEE_HOST_HSD_SYMBOL_SHAPE_ANIM_JOINT },
    { "_animjoint", MELEE_HOST_HSD_SYMBOL_ANIM_JOINT },
    { "_joint", MELEE_HOST_HSD_SYMBOL_JOINT },
    { "_camera", MELEE_HOST_HSD_SYMBOL_CAMERA },
    { "_scene_lights", MELEE_HOST_HSD_SYMBOL_SCENE_LIGHTS },
    { "_fog", MELEE_HOST_HSD_SYMBOL_FOG },
    { "_sobjdesc", MELEE_HOST_HSD_SYMBOL_SOBJ_DESC },
    { "_figatree", MELEE_HOST_HSD_SYMBOL_FIGATREE },
} };

std::uint32_t read_be32(const u8* bytes)
{
    return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
           (static_cast<std::uint32_t>(bytes[1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[2]) << 8U) |
           static_cast<std::uint32_t>(bytes[3]);
}

HostArchive* entry_of(HSD_Archive* archive)
{
    if (archive == nullptr || archive->top_ptr == nullptr) {
        return nullptr;
    }
    auto& archives = registry().archives;
    const auto found = archives.find(static_cast<const u8*>(archive->top_ptr));
    return found == archives.end() ? nullptr : found->second.get();
}

void refuse(std::string_view symbol, std::string_view reason)
{
    Registry& state = registry();
    state.refused += 1;
    state.last_error = std::string(symbol) + ": " + std::string(reason);
    OSReport("host HSD archive: cannot translate %s\n",
             state.last_error.c_str());
}

void* translate(HsdMaterializedArchive& descriptors,
                MeleeHostHsdSymbolKind kind, std::string_view symbol)
{
    switch (kind) {
    case MELEE_HOST_HSD_SYMBOL_JOINT:
        return descriptors.joint(symbol);
    case MELEE_HOST_HSD_SYMBOL_ANIM_JOINT:
        return descriptors.anim_joint(symbol);
    case MELEE_HOST_HSD_SYMBOL_MAT_ANIM_JOINT:
        return descriptors.mat_anim_joint(symbol);
    case MELEE_HOST_HSD_SYMBOL_SHAPE_ANIM_JOINT:
        return descriptors.shape_anim_joint(symbol);
    case MELEE_HOST_HSD_SYMBOL_CAMERA:
        return descriptors.camera(symbol);
    case MELEE_HOST_HSD_SYMBOL_SCENE_LIGHTS:
        return descriptors.scene_lights(symbol);
    case MELEE_HOST_HSD_SYMBOL_FOG:
        return descriptors.fog(symbol);
    case MELEE_HOST_HSD_SYMBOL_SOBJ_DESC:
        return descriptors.sobj_desc(symbol);
    case MELEE_HOST_HSD_SYMBOL_FIGATREE:
        return descriptors.figa_tree(symbol);
    case MELEE_HOST_HSD_SYMBOL_RUMBLE_TABLE:
        return descriptors.rumble_table(symbol);
    case MELEE_HOST_HSD_SYMBOL_UNSUPPORTED:
    case MELEE_HOST_HSD_SYMBOL_KIND_COUNT:
        break;
    }
    return nullptr;
}

} // namespace

extern "C" MeleeHostHsdSymbolKind melee_host_hsd_symbol_kind(const char* symbol)
{
    if (symbol == nullptr) {
        return MELEE_HOST_HSD_SYMBOL_UNSUPPORTED;
    }
    const std::string_view name(symbol);
    /* Symbols the game names without a kind suffix. */
    if (name == "lbRumbleData") {
        return MELEE_HOST_HSD_SYMBOL_RUMBLE_TABLE;
    }
    for (const SuffixKind& entry : kSuffixes) {
        if (name.size() > entry.suffix.size() && name.ends_with(entry.suffix)) {
            return entry.kind;
        }
    }
    return MELEE_HOST_HSD_SYMBOL_UNSUPPORTED;
}

extern "C" const char*
melee_host_hsd_symbol_kind_name(MeleeHostHsdSymbolKind kind)
{
    switch (kind) {
    case MELEE_HOST_HSD_SYMBOL_JOINT:
        return "joint";
    case MELEE_HOST_HSD_SYMBOL_ANIM_JOINT:
        return "animjoint";
    case MELEE_HOST_HSD_SYMBOL_MAT_ANIM_JOINT:
        return "matanim_joint";
    case MELEE_HOST_HSD_SYMBOL_SHAPE_ANIM_JOINT:
        return "shapeanim_joint";
    case MELEE_HOST_HSD_SYMBOL_CAMERA:
        return "camera";
    case MELEE_HOST_HSD_SYMBOL_SCENE_LIGHTS:
        return "scene_lights";
    case MELEE_HOST_HSD_SYMBOL_FOG:
        return "fog";
    case MELEE_HOST_HSD_SYMBOL_SOBJ_DESC:
        return "sobjdesc";
    case MELEE_HOST_HSD_SYMBOL_FIGATREE:
        return "figatree";
    case MELEE_HOST_HSD_SYMBOL_RUMBLE_TABLE:
        return "rumble_table";
    case MELEE_HOST_HSD_SYMBOL_UNSUPPORTED:
    case MELEE_HOST_HSD_SYMBOL_KIND_COUNT:
        break;
    }
    return "unsupported";
}

extern "C" s32 HSD_ArchiveParse(HSD_Archive* archive, u8* src,
                                size_t file_size)
{
    Registry& state = registry();
    if (archive == nullptr) {
        return -1;
    }
    std::memset(archive, 0, sizeof(HSD_Archive));
    archive->flags |= 1;
    if (src == nullptr || file_size < kHeaderSize) {
        state.last_error = "HSD_ArchiveParse: no header to parse";
        return -1;
    }

    /* The header holds the same numbers archive.c copies out, in host order.
     * The table pointers stay NULL: those tables are big-endian, and nothing
     * outside this file may read them as if they were not. */
    archive->header.file_size = read_be32(src);
    archive->header.data_size = read_be32(src + 0x04);
    archive->header.nb_reloc = read_be32(src + 0x08);
    archive->header.nb_public = read_be32(src + 0x0C);
    archive->header.nb_extern = read_be32(src + 0x10);
    std::memcpy(archive->header.version, src + 0x14,
                sizeof(archive->header.version));
    if (archive->header.file_size != file_size) {
        OSReport("HSD_ArchiveParse: byte-order mismatch! Please check data "
                 "format %x %x\n",
                 archive->header.file_size, static_cast<u32>(file_size));
        state.last_error = "HSD_ArchiveParse: file size mismatch";
        return -1;
    }
    if (archive->header.data_size != 0) {
        /* lbArchive_80016EFC frees `data - 0x20`, so this has to stay the
         * address the original computes. */
        archive->data = src + kHeaderSize;
    }
    archive->top_ptr = src;

    try {
        auto entry = std::make_unique<HostArchive>();
        entry->runtime = std::make_unique<HsdRuntimeArchive>(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(src),
                                       file_size));
        entry->descriptors =
            std::make_unique<HsdMaterializedArchive>(*entry->runtime);
        entry->publics = entry->runtime->disk_view().public_symbols();
        entry->externs = entry->runtime->disk_view().extern_symbols();
        /* Parsing a buffer again is the console overwriting it, so whatever
         * was built from the old bytes goes. */
        state.archives[src] = std::move(entry);
    } catch (const std::exception& error) {
        state.archives.erase(src);
        state.last_error = std::string("HSD_ArchiveParse: ") + error.what();
        OSReport("%s\n", state.last_error.c_str());
        return -1;
    }
    return 0;
}

extern "C" void* HSD_ArchiveGetPublicAddress(HSD_Archive* archive,
                                             const char* symbols)
{
    Registry& state = registry();
    HostArchive* const entry = entry_of(archive);
    if (entry == nullptr || symbols == nullptr) {
        state.last_error = "the archive was not parsed by the host";
        return nullptr;
    }
    const std::string name(symbols);
    if (const auto cached = entry->translated.find(name);
        cached != entry->translated.end())
    {
        return cached->second;
    }
    const bool is_public = std::any_of(
        entry->publics.begin(), entry->publics.end(),
        [&](const HsdPublicSymbol& candidate) { return candidate.name == name; });
    if (!is_public) {
        /* archive.c returns NULL here and leaves the report to the caller. */
        state.last_error = name + ": not a public symbol of the archive";
        return nullptr;
    }

    const MeleeHostHsdSymbolKind kind = melee_host_hsd_symbol_kind(symbols);
    if (kind == MELEE_HOST_HSD_SYMBOL_UNSUPPORTED) {
        refuse(name, "the host has no translation for this kind of symbol");
        return nullptr;
    }
    try {
        void* const result = translate(*entry->descriptors, kind, name);
        entry->translated.emplace(name, result);
        state.translated += 1;
        return result;
    } catch (const std::exception& error) {
        refuse(name, error.what());
        return nullptr;
    }
}

extern "C" char* HSD_ArchiveGetExtern(HSD_Archive* archive, int offset)
{
    HostArchive* const entry = entry_of(archive);
    if (entry == nullptr || offset < 0 ||
        static_cast<std::size_t>(offset) >= entry->externs.size())
    {
        return nullptr;
    }
    /* The view validated the terminator, and the name lives in the runtime
     * archive's own copy of the file for as long as the entry does.  The
     * caller only reads it. */
    return const_cast<char*>(
        entry->externs[static_cast<std::size_t>(offset)].name.data());
}

extern "C" void HSD_ArchiveLocateExtern(HSD_Archive* archive,
                                        const char* symbols, void* addr)
{
    Registry& state = registry();
    HostArchive* const entry = entry_of(archive);
    if (entry == nullptr || symbols == nullptr) {
        return;
    }
    const std::string_view name(symbols);
    const auto found = std::find_if(
        entry->externs.begin(), entry->externs.end(),
        [&](const HsdPublicSymbol& candidate) { return candidate.name == name; });
    if (found == entry->externs.end()) {
        return;
    }
    if (addr != nullptr) {
        /* Binding would mean writing a host pointer into a descriptor that
         * does not exist until the symbol is asked for.  Until something needs
         * it, the fields stay unresolved and a lookup that reaches one is
         * refused. */
        state.externs_refused += 1;
        state.last_error =
            std::string(name) + ": the host cannot bind an extern to an address";
        OSReport("host HSD archive: %s\n", state.last_error.c_str());
        return;
    }

    /* The chain runs through the fields themselves: each holds the offset of
     * the next field that refers to the same symbol, and -1 ends it. */
    const std::uint32_t data_size =
        entry->runtime->disk_view().header().data_size;
    std::uint32_t offset = found->data_offset;
    std::size_t links = 0;
    try {
        while (offset != kExternChainEnd && offset < data_size) {
            if (++links > data_size / 4U + 1U) {
                throw melee::assets::HsdArchiveError(
                    "extern chain does not end");
            }
            const std::uint32_t next = entry->runtime->read_u32({ offset }, 0);
            entry->descriptors->declare_null_field(offset);
            state.nulled += 1;
            offset = next;
        }
    } catch (const std::exception& error) {
        state.last_error = std::string(name) + ": " + error.what();
        OSReport("host HSD archive: %s\n", state.last_error.c_str());
    }
}

extern "C" MeleeHostStatus
melee_host_hsd_archive_stats(MeleeHostHsdArchiveStats* out_stats)
{
    if (out_stats == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    const Registry& state = registry();
    MeleeHostHsdArchiveStats stats{};
    stats.archives_live = static_cast<mh_u32>(state.archives.size());
    stats.symbols_translated = state.translated;
    stats.symbols_refused = state.refused;
    stats.extern_fields_nulled = state.nulled;
    stats.externs_refused = state.externs_refused;
    for (const auto& [bytes, entry] : state.archives) {
        static_cast<void>(bytes);
        stats.descriptor_bytes += entry->descriptors->stats().descriptor_bytes;
        stats.payload_bytes += entry->descriptors->stats().payload_bytes;
    }
    *out_stats = stats;
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_hsd_archive_release(const void* bytes)
{
    auto& archives = registry().archives;
    const auto found = archives.find(static_cast<const u8*>(bytes));
    if (found == archives.end()) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    archives.erase(found);
    return MELEE_HOST_OK;
}

extern "C" void melee_host_hsd_archive_release_all(void)
{
    registry().archives.clear();
}

extern "C" const char* melee_host_hsd_archive_last_error(void)
{
    return registry().last_error.c_str();
}

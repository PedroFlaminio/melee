/* Drives the host archive the way the game does, one file at a time.
 *
 * lbArchive_LoadSymbols reads a file into a heap buffer, parses it, runs the
 * extern loop and asks for symbols by name.  This does the same without the
 * game's heaps or DVD layer and then hands each descriptor to the original
 * loader for its kind, so a sweep over the disc shows both that a symbol
 * translates and that the code consuming it accepts the result.
 */

#include <melee_host/archive_probe.h>

#include <melee_host/baselib.h>
#include <melee_host/hsd_archive.h>
#include <melee_host/memory.h>

#include "assets/hsd_archive.hpp"
#include "assets/hsd_materialize.hpp"

MELEE_HOST_HSD_BEGIN
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/object.h>
/* lobj.h declares the class as hsdLobj, but lobj.c defines hsdLObj; nothing
 * in the game refers to it by the header's spelling. */
extern HSD_LObjInfo hsdLObj;
MELEE_HOST_HSD_END

#include <cstring>
#include <exception>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace {

/* OSRoundUp32B, which is how the game sizes the buffer a file is read into. */
constexpr std::size_t kBufferAlignment = 32;

std::string last_error = "no error";

MeleeHostStatus fail(MeleeHostStatus status, std::string message)
{
    last_error = std::move(message);
    return status;
}

/* The file's bytes, which are also the archive's identity to the host.  The
 * descriptors built from them go when the buffer does. */
class FileBuffer final {
public:
    FileBuffer() = default;
    FileBuffer(const FileBuffer&) = delete;
    FileBuffer& operator=(const FileBuffer&) = delete;
    ~FileBuffer()
    {
        if (bytes_ != nullptr) {
            static_cast<void>(melee_host_hsd_archive_release(bytes_));
            melee_host_aligned_free(bytes_, kBufferAlignment);
        }
    }

    bool read(const char* path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            return false;
        }
        stream.seekg(0, std::ios::end);
        const std::streamoff length = stream.tellg();
        if (length <= 0) {
            return false;
        }
        stream.seekg(0, std::ios::beg);
        size_ = static_cast<std::size_t>(length);
        const std::size_t capacity =
            (size_ + kBufferAlignment - 1) / kBufferAlignment * kBufferAlignment;
        bytes_ = static_cast<u8*>(
            melee_host_aligned_alloc(capacity, kBufferAlignment));
        if (bytes_ == nullptr) {
            return false;
        }
        std::memset(bytes_, 0, capacity);
        return static_cast<bool>(
            stream.read(reinterpret_cast<char*>(bytes_), length));
    }

    [[nodiscard]] u8* bytes() const noexcept { return bytes_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    u8* bytes_ = nullptr;
    std::size_t size_ = 0;
};

void load(HSD_Archive* archive, MeleeHostArchiveProbeSymbol* result)
{
    void* const descriptor =
        HSD_ArchiveGetPublicAddress(archive, result->symbol);
    if (descriptor == nullptr) {
        result->error = melee_host_hsd_archive_last_error();
        return;
    }
    result->translated = 1;

    switch (result->kind) {
    case MELEE_HOST_HSD_SYMBOL_JOINT: {
        result->load_attempted = 1;
        const u32 before = hsdJObj.parent.parent.head.nb_exist;
        HSD_JObj* const jobj =
            HSD_JObjLoadJoint(static_cast<HSD_Joint*>(descriptor));
        if (jobj == nullptr) {
            result->error = "HSD_JObjLoadJoint built no object";
            return;
        }
        result->objects = hsdJObj.parent.parent.head.nb_exist - before;
        HSD_JObjRemoveAll(jobj);
        result->loaded = 1;
        return;
    }
    case MELEE_HOST_HSD_SYMBOL_CAMERA: {
        result->load_attempted = 1;
        HSD_CObj* const cobj =
            HSD_CObjLoadDesc(static_cast<HSD_CObjDesc*>(descriptor));
        if (cobj == nullptr) {
            result->error = "HSD_CObjLoadDesc built no object";
            return;
        }
        result->objects = 1;
        hsdDelete(cobj);
        result->loaded = 1;
        return;
    }
    case MELEE_HOST_HSD_SYMBOL_SCENE_LIGHTS: {
        /* The walk lb_80011AC4 performs, which only attaches an animation
         * when the list has a table. */
        result->load_attempted = 1;
        const u32 before = hsdLObj.parent.parent.head.nb_exist;
        std::vector<HSD_LObj*> built;
        for (auto** list =
                 static_cast<melee::assets::MaterializedLightList**>(
                     descriptor);
             *list != nullptr; ++list)
        {
            HSD_LObj* const lobj = HSD_LObjLoadDesc((*list)->desc);
            if (lobj == nullptr) {
                result->error = "a light list has no light";
                break;
            }
            if ((*list)->anims != nullptr) {
                HSD_LObjAddAnimAll(lobj, (*list)->anims[0]);
            }
            built.push_back(lobj);
        }
        result->objects = hsdLObj.parent.parent.head.nb_exist - before;
        for (HSD_LObj* const lobj : built) {
            HSD_LObjRemoveAll(lobj);
        }
        result->loaded = result->error == nullptr ? 1 : 0;
        return;
    }
    case MELEE_HOST_HSD_SYMBOL_FOG: {
        result->load_attempted = 1;
        HSD_Fog* const fog =
            HSD_FogLoadDesc(static_cast<HSD_FogDesc*>(descriptor));
        if (fog == nullptr) {
            result->error = "HSD_FogLoadDesc built no object";
            return;
        }
        result->objects = 1;
        hsdDelete(fog);
        result->loaded = 1;
        return;
    }
    case MELEE_HOST_HSD_SYMBOL_ANIM_JOINT:
    case MELEE_HOST_HSD_SYMBOL_MAT_ANIM_JOINT:
    case MELEE_HOST_HSD_SYMBOL_SHAPE_ANIM_JOINT:
    case MELEE_HOST_HSD_SYMBOL_SOBJ_DESC:
    case MELEE_HOST_HSD_SYMBOL_FIGATREE:
    case MELEE_HOST_HSD_SYMBOL_RUMBLE_TABLE:
    case MELEE_HOST_HSD_SYMBOL_SIS_TABLE:
    case MELEE_HOST_HSD_SYMBOL_GAME_DATA:
    case MELEE_HOST_HSD_SYMBOL_UNSUPPORTED:
    case MELEE_HOST_HSD_SYMBOL_KIND_COUNT:
        return;
    }
}

} // namespace

extern "C" MeleeHostStatus
melee_host_archive_probe_file(const char* path, const char* const* symbols,
                              mh_u32 symbol_count,
                              MeleeHostArchiveProbeCallback callback,
                              void* user_data)
{
    if (path == nullptr || callback == nullptr ||
        (symbol_count != 0 && symbols == nullptr))
    {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "null argument");
    }
    const MeleeHostStatus ready = melee_host_baselib_bootstrap();
    if (ready != MELEE_HOST_OK) {
        return fail(ready, "the baselib bootstrap failed");
    }

    FileBuffer buffer;
    if (!buffer.read(path)) {
        return fail(MELEE_HOST_IO_ERROR, std::string("cannot read ") + path);
    }
    HSD_Archive archive{};
    if (HSD_ArchiveParse(&archive, buffer.bytes(), buffer.size()) != 0) {
        return fail(MELEE_HOST_IO_ERROR, melee_host_hsd_archive_last_error());
    }
    /* lbArchive_InitializeDAT resolves every extern to NULL right after the
     * parse. */
    for (int index = 0;; ++index) {
        const char* const name = HSD_ArchiveGetExtern(&archive, index);
        if (name == nullptr) {
            break;
        }
        HSD_ArchiveLocateExtern(&archive, name, nullptr);
    }

    std::vector<std::string> names;
    if (symbol_count != 0) {
        for (mh_u32 index = 0; index < symbol_count; ++index) {
            names.emplace_back(symbols[index] != nullptr ? symbols[index] : "");
        }
    } else {
        try {
            const melee::assets::HsdArchiveView view(std::span<const std::byte>(
                reinterpret_cast<const std::byte*>(buffer.bytes()),
                buffer.size()));
            for (const auto& entry : view.public_symbols()) {
                names.emplace_back(entry.name);
            }
        } catch (const std::exception& error) {
            return fail(MELEE_HOST_IO_ERROR, error.what());
        }
    }

    for (const std::string& name : names) {
        MeleeHostArchiveProbeSymbol result{};
        result.symbol = name.c_str();
        result.kind = melee_host_hsd_symbol_kind(name.c_str());
        if (result.kind != MELEE_HOST_HSD_SYMBOL_UNSUPPORTED) {
            load(&archive, &result);
        }
        callback(&result, user_data);
    }
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" const char* melee_host_archive_probe_last_error(void)
{
    return last_error.c_str();
}

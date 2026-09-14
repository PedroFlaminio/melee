#include "test.hpp"

#include "assets/hsd_materialize.hpp"

#include <melee_host/baselib.h>
#include <melee_host/boot.h>
#include <melee_host/hsd_archive.h>
#include "hsd_include.hpp"
MELEE_HOST_TEST_HSD_BEGIN
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/psstructs.h>
MELEE_HOST_TEST_HSD_END

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

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

/* Lays out an archive the way the tools that built the game's files did:
 * header, data section, relocation table, public and extern tables, then the
 * names they point at. */
class ArchiveBuilder final {
public:
    explicit ArchiveBuilder(std::uint32_t data_size) : data_(data_size) {}

    void u8(std::uint32_t at, std::uint8_t value)
    {
        data_.at(at) = static_cast<std::byte>(value);
    }

    void u16(std::uint32_t at, std::uint16_t value)
    {
        u8(at, static_cast<std::uint8_t>(value >> 8U));
        u8(at + 1, static_cast<std::uint8_t>(value));
    }

    void u32(std::uint32_t at, std::uint32_t value)
    {
        u8(at, static_cast<std::uint8_t>(value >> 24U));
        u8(at + 1, static_cast<std::uint8_t>(value >> 16U));
        u8(at + 2, static_cast<std::uint8_t>(value >> 8U));
        u8(at + 3, static_cast<std::uint8_t>(value));
    }

    void f32(std::uint32_t at, float value)
    {
        u32(at, std::bit_cast<std::uint32_t>(value));
    }

    /* A pointer field is an offset plus the relocation entry that makes it
     * one; a field without the entry must read as NULL. */
    void pointer(std::uint32_t at, std::uint32_t target)
    {
        u32(at, target);
        relocations_.push_back(at);
    }

    void public_symbol(std::uint32_t at, std::string name)
    {
        publics_.emplace_back(at, std::move(name));
    }

    void extern_symbol(std::uint32_t chain_head, std::string name)
    {
        externs_.emplace_back(chain_head, std::move(name));
    }

    [[nodiscard]] std::vector<std::byte> build() const
    {
        std::string names;
        std::vector<std::uint32_t> public_names;
        std::vector<std::uint32_t> extern_names;
        for (const auto& [offset, name] : publics_) {
            static_cast<void>(offset);
            public_names.push_back(static_cast<std::uint32_t>(names.size()));
            names += name;
            names.push_back('\0');
        }
        for (const auto& [offset, name] : externs_) {
            static_cast<void>(offset);
            extern_names.push_back(static_cast<std::uint32_t>(names.size()));
            names += name;
            names.push_back('\0');
        }

        const std::size_t file_size =
            0x20 + data_.size() + relocations_.size() * 4 +
            (publics_.size() + externs_.size()) * 8 + names.size();
        std::vector<std::byte> out;
        out.reserve(file_size);
        const auto put32 = [&out](std::size_t value) {
            const auto word = static_cast<std::uint32_t>(value);
            out.push_back(static_cast<std::byte>(word >> 24U));
            out.push_back(static_cast<std::byte>(word >> 16U));
            out.push_back(static_cast<std::byte>(word >> 8U));
            out.push_back(static_cast<std::byte>(word));
        };
        put32(file_size);
        put32(data_.size());
        put32(relocations_.size());
        put32(publics_.size());
        put32(externs_.size());
        put32(0x001B0000U);
        put32(0);
        put32(0);
        out.insert(out.end(), data_.begin(), data_.end());
        for (const std::uint32_t relocation : relocations_) {
            put32(relocation);
        }
        for (std::size_t index = 0; index < publics_.size(); ++index) {
            put32(publics_[index].first);
            put32(public_names[index]);
        }
        for (std::size_t index = 0; index < externs_.size(); ++index) {
            put32(externs_[index].first);
            put32(extern_names[index]);
        }
        for (const char letter : names) {
            out.push_back(static_cast<std::byte>(letter));
        }
        return out;
    }

private:
    std::vector<std::byte> data_;
    std::vector<std::uint32_t> relocations_;
    std::vector<std::pair<std::uint32_t, std::string>> publics_;
    std::vector<std::pair<std::uint32_t, std::string>> externs_;
};

/* Data offsets of the records a title-like archive holds, each on its own
 * boundary so a field written by mistake cannot land in another record. */
enum : std::uint32_t {
    kCamera = 0x000,
    kEye = 0x040,
    kInterest = 0x060,
    kLightTable = 0x080,
    kAmbientList = 0x090,
    kPointList = 0x0A0,
    kAmbientDesc = 0x0B0,
    kPointDesc = 0x0D0,
    kPointParameters = 0x0F0,
    kLightPosition = 0x100,
    kLightAnimTable = 0x120,
    kLightAnim = 0x130,
    kLightAObj = 0x140,
    kLightPositionAnim = 0x150,
    kFog = 0x160,
    kFogAdj = 0x180,
    kSprite = 0x1D0,
    kSpriteImage = 0x1E0,
    kSpriteTlut = 0x200,
    kImageBytes = 0x220,
    kPaletteBytes = 0x230,
    kJoint = 0x240,
    kDataSize = 0x280,
};

std::vector<std::byte> make_title_like_archive()
{
    ArchiveBuilder archive(kDataSize);

    // A perspective camera looking at the origin from +z.
    archive.u16(kCamera + 0x06, PROJ_PERSPECTIVE);
    archive.u16(kCamera + 0x0A, 640); // viewport xmax
    archive.u16(kCamera + 0x0E, 480); // viewport ymax
    archive.u16(kCamera + 0x12, 640); // scissor right
    archive.u16(kCamera + 0x16, 480); // scissor bottom
    archive.pointer(kCamera + 0x18, kEye);
    archive.pointer(kCamera + 0x1C, kInterest);
    archive.f32(kCamera + 0x28, 1.0F);    // near
    archive.f32(kCamera + 0x2C, 1000.0F); // far
    archive.f32(kCamera + 0x30, 30.0F);   // fov
    archive.f32(kCamera + 0x34, 1.18F);   // aspect
    archive.f32(kEye + 0x0C, 100.0F);     // eye z

    // Two light lists: an ambient light, and a point light carrying an
    // animation that also moves its position.
    archive.pointer(kLightTable + 0x00, kAmbientList);
    archive.pointer(kLightTable + 0x04, kPointList);
    archive.pointer(kAmbientList + 0x00, kAmbientDesc);
    archive.pointer(kPointList + 0x00, kPointDesc);
    archive.pointer(kPointList + 0x04, kLightAnimTable);

    archive.u16(kAmbientDesc + 0x08, 0x0004); // ambient, diffuse
    archive.u8(kAmbientDesc + 0x0C, 40);
    archive.u8(kAmbientDesc + 0x0D, 40);
    archive.u8(kAmbientDesc + 0x0E, 40);
    archive.u8(kAmbientDesc + 0x0F, 255);

    archive.u16(kPointDesc + 0x08, 0x0006); // point, diffuse
    archive.u8(kPointDesc + 0x0C, 200);
    archive.u8(kPointDesc + 0x0D, 180);
    archive.u8(kPointDesc + 0x0E, 160);
    archive.u8(kPointDesc + 0x0F, 255);
    archive.pointer(kPointDesc + 0x10, kLightPosition);
    archive.pointer(kPointDesc + 0x18, kPointParameters);
    archive.f32(kPointParameters + 0x00, 0.5F);   // ref_br
    archive.f32(kPointParameters + 0x04, 400.0F); // ref_dist
    archive.u32(kPointParameters + 0x08, 2);      // dist_func
    archive.f32(kLightPosition + 0x04, 10.0F);
    archive.f32(kLightPosition + 0x08, 20.0F);
    archive.f32(kLightPosition + 0x0C, 30.0F);

    archive.pointer(kLightAnimTable + 0x00, kLightAnim);
    archive.pointer(kLightAnim + 0x04, kLightAObj);
    archive.pointer(kLightAnim + 0x08, kLightPositionAnim);
    archive.f32(kLightAObj + 0x04, 30.0F); // end frame
    archive.pointer(kLightPositionAnim + 0x00, kLightAObj);

    // Linear fog with a range adjustment table.
    archive.u32(kFog + 0x00, 2);
    archive.pointer(kFog + 0x04, kFogAdj);
    archive.f32(kFog + 0x08, 80.0F);
    archive.f32(kFog + 0x0C, 300.0F);
    archive.u8(kFog + 0x10, 0x26);
    archive.u8(kFog + 0x11, 0x26);
    archive.u8(kFog + 0x12, 0x26);
    archive.u8(kFog + 0x13, 0xFF);
    archive.u16(kFogAdj + 0x00, 320);
    archive.u16(kFogAdj + 0x02, 640);
    for (std::uint32_t index = 0; index < 16; ++index) {
        archive.f32(kFogAdj + 0x04 + index * 4,
                    static_cast<float>(index) + 1.0F);
    }

    // A screen sprite: a 4x4 C4 image and its two-entry palette.
    archive.pointer(kSprite + 0x00, kSpriteImage);
    archive.pointer(kSprite + 0x04, kSpriteTlut);
    archive.pointer(kSpriteImage + 0x00, kImageBytes);
    archive.u16(kSpriteImage + 0x04, 4);
    archive.u16(kSpriteImage + 0x06, 4);
    archive.u32(kSpriteImage + 0x08, GX_TF_C4);
    archive.pointer(kSpriteTlut + 0x00, kPaletteBytes);
    archive.u32(kSpriteTlut + 0x04, GX_TL_RGB565);
    archive.u16(kSpriteTlut + 0x0C, 2);
    for (std::uint32_t index = 0; index < 8; ++index) {
        archive.u8(kImageBytes + index,
                   static_cast<std::uint8_t>(0x01 + index * 0x22));
    }
    archive.u16(kPaletteBytes + 0x00, 0xF800);
    archive.u16(kPaletteBytes + 0x02, 0x07E0);

    // A joint whose child and matrix are both references to a joint another
    // archive provides.  Neither is relocated: each holds the next link of
    // the extern chain, and the last holds -1.
    archive.u32(kJoint + 0x08, kJoint + 0x38);
    archive.f32(kJoint + 0x20, 1.0F);
    archive.f32(kJoint + 0x24, 1.0F);
    archive.f32(kJoint + 0x28, 1.0F);
    archive.u32(kJoint + 0x38, 0xFFFFFFFFU);

    archive.public_symbol(kCamera, "test_cam_int1_camera");
    archive.public_symbol(kLightTable, "test_scene_lights");
    archive.public_symbol(kFog, "test_fog");
    archive.public_symbol(kSprite, "test_sobjdesc");
    archive.public_symbol(kJoint, "test_joint");
    archive.public_symbol(kCamera, "test_scene_data");
    archive.extern_symbol(kJoint + 0x08, "shared_joint");
    return archive.build();
}

u8* bytes_of(std::vector<std::byte>& bytes)
{
    return reinterpret_cast<u8*>(bytes.data());
}

/* The loop lbArchive_InitializeDAT runs after every parse. */
void locate_externs_as_null(HSD_Archive* archive)
{
    for (int index = 0;; ++index) {
        const char* const symbol = HSD_ArchiveGetExtern(archive, index);
        if (symbol == nullptr) {
            return;
        }
        HSD_ArchiveLocateExtern(archive, symbol, nullptr);
    }
}

MeleeHostHsdArchiveStats archive_stats()
{
    MeleeHostHsdArchiveStats stats{};
    REQUIRE(melee_host_hsd_archive_stats(&stats) == MELEE_HOST_OK);
    return stats;
}

} // namespace

TEST_CASE("the host archive exposes the header archive.c parses")
{
    std::vector<std::byte> bytes = make_title_like_archive();
    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    REQUIRE(archive.header.file_size == bytes.size());
    REQUIRE(archive.header.data_size == kDataSize);
    REQUIRE(archive.header.nb_public == 6);
    REQUIRE(archive.header.nb_extern == 1);
    REQUIRE(archive.data == bytes_of(bytes) + 0x20);
    REQUIRE(archive.top_ptr == bytes_of(bytes));
    // The tables stay big-endian, so nothing is handed a pointer to them.
    REQUIRE(archive.public_info == nullptr);
    REQUIRE(archive.reloc_info == nullptr);

    HSD_Archive truncated{};
    REQUIRE(HSD_ArchiveParse(&truncated, bytes_of(bytes), bytes.size() - 1) ==
            -1);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
}

TEST_CASE("the host archive rebuilds a title screen's camera, lights, fog "
          "and sprite")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);
    std::vector<std::byte> bytes = make_title_like_archive();
    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    locate_externs_as_null(&archive);

    auto* const camera = static_cast<HSD_CObjDesc*>(
        HSD_ArchiveGetPublicAddress(&archive, "test_cam_int1_camera"));
    REQUIRE(camera != nullptr);
    REQUIRE(camera->common.projection_type == PROJ_PERSPECTIVE);
    REQUIRE(camera->common.eyepos != nullptr);
    REQUIRE(camera->common.eyepos->pos.z == 100.0F);
    REQUIRE(camera->common.interest != nullptr);
    REQUIRE(camera->perspective.fov == 30.0F);
    HSD_CObj* const cobj = HSD_CObjLoadDesc(camera);
    REQUIRE(cobj != nullptr);
    hsdDelete(cobj);

    auto** const lights = static_cast<melee::assets::MaterializedLightList**>(
        HSD_ArchiveGetPublicAddress(&archive, "test_scene_lights"));
    REQUIRE(lights != nullptr);
    REQUIRE(lights[0] != nullptr);
    REQUIRE(lights[1] != nullptr);
    REQUIRE(lights[2] == nullptr);
    const HSD_LightDesc* const ambient = lights[0]->desc;
    REQUIRE(ambient != nullptr);
    REQUIRE((ambient->flags & LOBJ_TYPE_MASK) == LOBJ_AMBIENT);
    REQUIRE(ambient->color.r == 40);
    REQUIRE(ambient->position == nullptr);
    REQUIRE(lights[0]->anims == nullptr);
    const HSD_LightDesc* const point = lights[1]->desc;
    REQUIRE(point != nullptr);
    REQUIRE((point->flags & LOBJ_TYPE_MASK) == LOBJ_POINT);
    REQUIRE(point->position != nullptr);
    REQUIRE(point->position->pos.y == 20.0F);
    REQUIRE(point->u.point != nullptr);
    REQUIRE(point->u.point->ref_dist == 400.0F);
    REQUIRE(point->u.point->dist_func == 2);
    REQUIRE(lights[1]->anims != nullptr);
    REQUIRE(lights[1]->anims[0] != nullptr);
    REQUIRE(lights[1]->anims[1] == nullptr);
    const HSD_LightAnim* const light_anim = lights[1]->anims[0];
    REQUIRE(light_anim->aobjdesc != nullptr);
    REQUIRE(light_anim->aobjdesc->end_frame == 30.0F);
    REQUIRE(light_anim->position_anim != nullptr);
    // Both fields name the same record, so they share one descriptor.
    REQUIRE(light_anim->position_anim->aobjdesc == light_anim->aobjdesc);

    const u32 lobjs_before = hsdLObj.parent.parent.head.nb_exist;
    HSD_LObj* const ambient_lobj = HSD_LObjLoadDesc(lights[0]->desc);
    HSD_LObj* const point_lobj = HSD_LObjLoadDesc(lights[1]->desc);
    REQUIRE(ambient_lobj != nullptr);
    REQUIRE(point_lobj != nullptr);
    HSD_LObjAddAnimAll(point_lobj, lights[1]->anims[0]);
    REQUIRE(point_lobj->aobj != nullptr);
    REQUIRE(point_lobj->position != nullptr);
    REQUIRE(point_lobj->position->aobj != nullptr);
    HSD_LObjRemoveAll(point_lobj);
    HSD_LObjRemoveAll(ambient_lobj);
    REQUIRE(hsdLObj.parent.parent.head.nb_exist == lobjs_before);

    auto* const fog = static_cast<HSD_FogDesc*>(
        HSD_ArchiveGetPublicAddress(&archive, "test_fog"));
    REQUIRE(fog != nullptr);
    REQUIRE(fog->type == 2);
    REQUIRE(fog->start == 80.0F);
    REQUIRE(fog->end == 300.0F);
    REQUIRE(fog->color.r == 0x26);
    REQUIRE(fog->color.a == 0xFF);
    REQUIRE(fog->fogadjdesc != nullptr);
    REQUIRE(fog->fogadjdesc->center == 320);
    REQUIRE(fog->fogadjdesc->width == 640);
    REQUIRE(fog->fogadjdesc->mtx[2][3] == 12.0F);
    HSD_Fog* const fog_object = HSD_FogLoadDesc(fog);
    REQUIRE(fog_object != nullptr);
    REQUIRE(fog_object->end == 300.0F);
    REQUIRE(fog_object->fog_adj != nullptr);
    hsdDelete(fog_object);

    auto* const sprite = static_cast<HSD_SObjDesc*>(
        HSD_ArchiveGetPublicAddress(&archive, "test_sobjdesc"));
    REQUIRE(sprite != nullptr);
    REQUIRE(sprite->image != nullptr);
    REQUIRE(sprite->image->width == 4);
    REQUIRE(sprite->image->format == GX_TF_C4);
    REQUIRE(static_cast<const u8*>(sprite->image->image_ptr)[0] == 0x01);
    REQUIRE(sprite->tlut != nullptr);
    REQUIRE(sprite->tlut->n_entries == 2);
    REQUIRE(sprite->tlut->fmt == GX_TL_RGB565);
    // Palettes keep the file's byte order, which is what GX reads.
    REQUIRE(static_cast<const u8*>(sprite->tlut->lut)[0] == 0xF8);

    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
}

TEST_CASE("a public symbol comes back as the same descriptor every time")
{
    std::vector<std::byte> bytes = make_title_like_archive();
    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    locate_externs_as_null(&archive);

    const MeleeHostHsdArchiveStats before = archive_stats();
    void* const first =
        HSD_ArchiveGetPublicAddress(&archive, "test_cam_int1_camera");
    void* const second =
        HSD_ArchiveGetPublicAddress(&archive, "test_cam_int1_camera");
    REQUIRE(first != nullptr);
    REQUIRE(first == second);
    REQUIRE(archive_stats().symbols_translated ==
            before.symbols_translated + 1);

    // A second HSD_Archive over the same buffer is the same archive, which is
    // how ftdata.c's stack archives keep working after their frame is gone.
    HSD_Archive alias{};
    alias.top_ptr = bytes_of(bytes);
    REQUIRE(HSD_ArchiveGetPublicAddress(&alias, "test_cam_int1_camera") ==
            first);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
}

TEST_CASE("an extern resolved to NULL reads as a null pointer field")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);
    std::vector<std::byte> bytes = make_title_like_archive();
    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);

    // Until the extern is located, the joint's child holds a chain link, a
    // non-zero value with no relocation, and the lookup refuses it.
    const MeleeHostHsdArchiveStats before = archive_stats();
    REQUIRE(HSD_ArchiveGetPublicAddress(&archive, "test_joint") == nullptr);
    const MeleeHostHsdArchiveStats refused = archive_stats();
    REQUIRE(refused.symbols_refused == before.symbols_refused + 1);

    // Parsing the buffer again starts over, as the console would.
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    REQUIRE(std::string_view(HSD_ArchiveGetExtern(&archive, 0)) ==
            "shared_joint");
    REQUIRE(HSD_ArchiveGetExtern(&archive, 1) == nullptr);
    REQUIRE(HSD_ArchiveGetExtern(&archive, -1) == nullptr);
    locate_externs_as_null(&archive);
    REQUIRE(archive_stats().extern_fields_nulled ==
            refused.extern_fields_nulled + 2);

    auto* const joint = static_cast<HSD_Joint*>(
        HSD_ArchiveGetPublicAddress(&archive, "test_joint"));
    REQUIRE(joint != nullptr);
    REQUIRE(joint->child == nullptr);
    REQUIRE(joint->mtx == nullptr);
    REQUIRE(joint->scale.x == 1.0F);
    HSD_JObj* const jobj = HSD_JObjLoadJoint(joint);
    REQUIRE(jobj != nullptr);
    HSD_JObjRemoveAll(jobj);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
}

TEST_CASE("a symbol the host has no translation for is refused, not guessed")
{
    std::vector<std::byte> bytes = make_title_like_archive();
    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    locate_externs_as_null(&archive);

    REQUIRE(HSD_ArchiveGetPublicAddress(&archive, "test_scene_data") ==
            nullptr);
    REQUIRE(std::string_view(melee_host_hsd_archive_last_error())
                .find("test_scene_data") != std::string_view::npos);
    REQUIRE(HSD_ArchiveGetPublicAddress(&archive, "missing_joint") == nullptr);

    HSD_Archive stranger{};
    REQUIRE(HSD_ArchiveGetPublicAddress(&stranger, "test_joint") == nullptr);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
}

TEST_CASE("an SIS text table points each entry at its verbatim string")
{
    // Two strings, and a first entry pointing at the end of the data, which
    // four of the game's text archives have.
    constexpr std::uint32_t kDataSize = 0x20;
    ArchiveBuilder builder(kDataSize);
    builder.pointer(0x00, kDataSize);
    builder.pointer(0x04, 0x0C);
    builder.pointer(0x08, 0x14);
    builder.u16(0x0C, 0x2041);
    builder.u16(0x14, 0x0C00);
    builder.public_symbol(0x00, "SIS_TestData");
    std::vector<std::byte> bytes = builder.build();

    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    auto** const table = static_cast<std::uint8_t**>(
        HSD_ArchiveGetPublicAddress(&archive, "SIS_TestData"));
    REQUIRE(table != nullptr);
    // The strings keep the file's byte order and their distance apart.
    REQUIRE(table[1][0] == 0x20);
    REQUIRE(table[1][1] == 0x41);
    REQUIRE(table[1][2] == 0x00);
    REQUIRE(table[2][0] == 0x0C);
    REQUIRE(table[2] - table[1] == 8);
    REQUIRE(table[0][0] == 0x00);
    REQUIRE(table[0][3] == 0x00);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
}

namespace {

struct TestGameData {
    std::uint16_t word;
    float number;
    std::uint8_t* bytes;
    TestGameData* next;
};

void* translate_test_game_data(MeleeHostHsdReader* reader, mh_u32 root)
{
    auto* const data = static_cast<TestGameData*>(melee_host_hsd_reader_allocate(
        reader, sizeof(TestGameData), alignof(TestGameData)));
    if (data == nullptr) {
        return nullptr;
    }
    data->word = melee_host_hsd_reader_u16(reader, root);
    data->number = melee_host_hsd_reader_f32(reader, root + 4);
    mh_u32 target = 0;
    if (melee_host_hsd_reader_pointer(reader, root + 8, &target)) {
        data->bytes = static_cast<std::uint8_t*>(
            melee_host_hsd_reader_payload(reader, target, 2));
    }
    if (melee_host_hsd_reader_pointer(reader, root + 12, &target)) {
        data->next = data;
    }
    return melee_host_hsd_reader_failed(reader) ? nullptr : data;
}

} // namespace

TEST_CASE("a translator registered by name reads the archive through the C "
          "reader")
{
    // Two records: the second holds a pointer-sized value no relocation
    // vouches for, which the reader must refuse rather than follow.
    ArchiveBuilder builder(0x30);
    builder.u16(0x00, 0x1234);
    builder.f32(0x04, 2.5F);
    builder.pointer(0x08, 0x10);
    builder.u16(0x10, 0xABCD);
    builder.u16(0x18, 0x0001);
    builder.pointer(0x20, 0x10);
    builder.u32(0x24, 0x11111111U);
    builder.public_symbol(0x00, "test_game_data");
    builder.public_symbol(0x18, "test_bad_game_data");
    std::vector<std::byte> bytes = builder.build();

    REQUIRE(melee_host_hsd_register_translator(
                "test_game_data", translate_test_game_data) == MELEE_HOST_OK);
    REQUIRE(melee_host_hsd_register_translator(
                "test_bad_game_data", translate_test_game_data) ==
            MELEE_HOST_OK);
    REQUIRE(melee_host_hsd_symbol_kind("test_game_data") ==
            MELEE_HOST_HSD_SYMBOL_GAME_DATA);

    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    auto* const data = static_cast<TestGameData*>(
        HSD_ArchiveGetPublicAddress(&archive, "test_game_data"));
    REQUIRE(data != nullptr);
    REQUIRE(data->word == 0x1234);
    REQUIRE(data->number == 2.5F);
    REQUIRE(data->bytes != nullptr);
    REQUIRE(data->bytes[0] == 0xAB);
    REQUIRE(data->bytes[1] == 0xCD);
    // A NULL pointer field reads as absent.
    REQUIRE(data->next == nullptr);

    REQUIRE(HSD_ArchiveGetPublicAddress(&archive, "test_bad_game_data") ==
            nullptr);
    REQUIRE(std::string_view(melee_host_hsd_archive_last_error())
                .find("no relocation") != std::string_view::npos);

    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
    REQUIRE(melee_host_hsd_register_translator("test_game_data", nullptr) ==
            MELEE_HOST_OK);
    REQUIRE(melee_host_hsd_register_translator("test_bad_game_data",
                                               nullptr) == MELEE_HOST_OK);
    REQUIRE(melee_host_hsd_symbol_kind("test_game_data") ==
            MELEE_HOST_HSD_SYMBOL_UNSUPPORTED);
}

TEST_CASE("the refraction table translates to a count and host floats")
{
    // LbRf.dat's layout: six floats, then the record with the count and a
    // pointer back to them.  The host record is lbrefract.c's private one.
    struct HostRefractData {
        std::uint8_t count;
        float* params;
    };
    const float floats[6] = { 0.0F, 0.0F, 0.0F, 0.1F, 0.2F, 5.0F };
    ArchiveBuilder builder(0x20);
    for (std::uint32_t i = 0; i < 6; ++i) {
        builder.f32(i * 4, floats[i]);
    }
    builder.u8(0x18, 3);
    builder.pointer(0x1C, 0x00);
    builder.public_symbol(0x18, "lbRefData");
    std::vector<std::byte> bytes = builder.build();

    melee_host_game_register_data_translators();
    REQUIRE(melee_host_hsd_symbol_kind("lbRefData") ==
            MELEE_HOST_HSD_SYMBOL_GAME_DATA);

    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    auto* const data = static_cast<HostRefractData*>(
        HSD_ArchiveGetPublicAddress(&archive, "lbRefData"));
    REQUIRE(data != nullptr);
    REQUIRE(data->count == 3);
    REQUIRE(data->params != nullptr);
    for (std::size_t i = 0; i < 6; ++i) {
        REQUIRE(data->params[i] == floats[i]);
    }
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);

    // Without the pointer there is nothing to count; the table is refused.
    ArchiveBuilder empty(0x20);
    empty.u8(0x18, 3);
    empty.public_symbol(0x18, "lbRefData");
    std::vector<std::byte> empty_bytes = empty.build();
    HSD_Archive empty_archive{};
    REQUIRE(HSD_ArchiveParse(&empty_archive, bytes_of(empty_bytes),
                             empty_bytes.size()) == 0);
    REQUIRE(HSD_ArchiveGetPublicAddress(&empty_archive, "lbRefData") ==
            nullptr);
    REQUIRE(melee_host_hsd_archive_release(empty_bytes.data()) ==
            MELEE_HOST_OK);

    REQUIRE(melee_host_hsd_register_translator("lbRefData", nullptr) ==
            MELEE_HOST_OK);
}

TEST_CASE("the player common data translates to a pointer and host words")
{
    // PdPm.dat's layout: the 0x184-byte table, then the pointer to it, which
    // is the public symbol.  Floats and integers convert in place; the four
    // bytes at +0xC0 keep their order.
    ArchiveBuilder builder(0x188);
    builder.f32(0x000, 2.5F);
    builder.u32(0x004, 6);
    builder.u32(0x0BC, 0x12345678U);
    builder.u8(0x0C0, 1);
    builder.u8(0x0C1, 2);
    builder.u8(0x0C2, 3);
    builder.u8(0x0C3, 4);
    builder.f32(0x180, 90.0F);
    builder.pointer(0x184, 0x000);
    builder.public_symbol(0x184, "plLoadCommonData");
    std::vector<std::byte> bytes = builder.build();

    melee_host_game_register_data_translators();
    REQUIRE(melee_host_hsd_symbol_kind("plLoadCommonData") ==
            MELEE_HOST_HSD_SYMBOL_GAME_DATA);

    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    auto* const record = static_cast<unsigned char**>(
        HSD_ArchiveGetPublicAddress(&archive, "plLoadCommonData"));
    REQUIRE(record != nullptr);
    const unsigned char* const table = *record;
    REQUIRE(table != nullptr);
    float first = 0.0F;
    std::uint32_t count = 0;
    std::uint32_t before_bytes = 0;
    float last = 0.0F;
    std::memcpy(&first, table + 0x000, sizeof(first));
    std::memcpy(&count, table + 0x004, sizeof(count));
    std::memcpy(&before_bytes, table + 0x0BC, sizeof(before_bytes));
    std::memcpy(&last, table + 0x180, sizeof(last));
    REQUIRE(first == 2.5F);
    REQUIRE(count == 6);
    REQUIRE(before_bytes == 0x12345678U);
    REQUIRE(table[0x0C0] == 1);
    REQUIRE(table[0x0C1] == 2);
    REQUIRE(table[0x0C2] == 3);
    REQUIRE(table[0x0C3] == 4);
    REQUIRE(last == 90.0F);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);

    // A NULL pointer leaves no table to read; the symbol is refused.
    ArchiveBuilder empty(0x188);
    empty.public_symbol(0x184, "plLoadCommonData");
    std::vector<std::byte> empty_bytes = empty.build();
    HSD_Archive empty_archive{};
    REQUIRE(HSD_ArchiveParse(&empty_archive, bytes_of(empty_bytes),
                             empty_bytes.size()) == 0);
    REQUIRE(HSD_ArchiveGetPublicAddress(&empty_archive, "plLoadCommonData") ==
            nullptr);
    REQUIRE(std::string_view(melee_host_hsd_archive_last_error())
                .find("pointer is NULL") != std::string_view::npos);
    REQUIRE(melee_host_hsd_archive_release(empty_bytes.data()) ==
            MELEE_HOST_OK);
}

TEST_CASE("the ground parameters translate with their stage rows")
{
    // GrSh.dat's layout: the StageParam rows (0x64 bytes each), then the
    // parameters, whose pointer to the rows sits at +0xB0 with the count
    // after it and nine colors from +0xB8.  On the host the pointer is eight
    // bytes wide, so the count and the colors move by four.
    struct HostGroundParam {
        unsigned char prefix[0xB0];
        unsigned char* stage_params;
        std::int32_t stage_param_count;
        std::uint8_t colors[9 * 4];
    };
    REQUIRE(offsetof(HostGroundParam, stage_param_count) == 0xB8);
    REQUIRE(offsetof(HostGroundParam, colors) == 0xBC);

    ArchiveBuilder builder(0x200);
    // Two rows at data+0x00.
    builder.u32(0x00, 14);
    builder.u32(0x04, 0xFFFFFFFFU);
    builder.u16(0x14, 0xFFF6);
    builder.u16(0x1A, 7);
    builder.u16(0x62, 9);
    builder.u32(0x64, 15);
    builder.u16(0xC6, 11);
    // The parameters at data+0x100.
    builder.f32(0x100, 0.9F);
    builder.u16(0x104, 195);
    builder.u32(0x10C, 83);
    builder.u32(0x114, 0xFFFFFFF6U);
    builder.f32(0x118, 0.15F);
    builder.u16(0x12E, 60);
    builder.u8(0x14C, 1);
    builder.f32(0x160, -1.0F);
    builder.u16(0x168, 76);
    builder.u16(0x16A, 160);
    builder.u16(0x1AE, 40);
    builder.pointer(0x1B0, 0x00);
    builder.u32(0x1B4, 2);
    builder.u32(0x1B8, 0x649BFAFFU);
    builder.u32(0x1D8, 0x5A78D2FFU);
    builder.public_symbol(0x100, "grGroundParam");
    std::vector<std::byte> bytes = builder.build();

    melee_host_game_register_data_translators();
    REQUIRE(melee_host_hsd_symbol_kind("grGroundParam") ==
            MELEE_HOST_HSD_SYMBOL_GAME_DATA);

    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    auto* const param = static_cast<HostGroundParam*>(
        HSD_ArchiveGetPublicAddress(&archive, "grGroundParam"));
    REQUIRE(param != nullptr);
    const auto f32_at = [](const unsigned char* base, std::size_t at) {
        float value = 0.0F;
        std::memcpy(&value, base + at, sizeof(value));
        return value;
    };
    const auto s32_at = [](const unsigned char* base, std::size_t at) {
        std::int32_t value = 0;
        std::memcpy(&value, base + at, sizeof(value));
        return value;
    };
    const auto s16_at = [](const unsigned char* base, std::size_t at) {
        std::int16_t value = 0;
        std::memcpy(&value, base + at, sizeof(value));
        return value;
    };
    REQUIRE(f32_at(param->prefix, 0x00) == 0.9F);
    REQUIRE(s16_at(param->prefix, 0x04) == 195);
    REQUIRE(s32_at(param->prefix, 0x0C) == 83);
    REQUIRE(s32_at(param->prefix, 0x14) == -10);
    REQUIRE(f32_at(param->prefix, 0x18) == 0.15F);
    REQUIRE(s16_at(param->prefix, 0x2E) == 60);
    REQUIRE(param->prefix[0x4C] == 1);
    REQUIRE(f32_at(param->prefix, 0x60) == -1.0F);
    REQUIRE(s16_at(param->prefix, 0x68) == 76);
    REQUIRE(s16_at(param->prefix, 0x6A) == 160);
    REQUIRE(s16_at(param->prefix, 0xAE) == 40);
    REQUIRE(param->stage_param_count == 2);
    REQUIRE(param->colors[0] == 0x64);
    REQUIRE(param->colors[3] == 0xFF);
    REQUIRE(param->colors[32] == 0x5A);
    REQUIRE(param->colors[34] == 0xD2);

    const unsigned char* const rows = param->stage_params;
    REQUIRE(rows != nullptr);
    REQUIRE(s32_at(rows, 0x00) == 14);
    REQUIRE(s32_at(rows, 0x04) == -1);
    REQUIRE(s16_at(rows, 0x14) == -10);
    REQUIRE(s16_at(rows, 0x1A) == 7);
    REQUIRE(s16_at(rows, 0x62) == 9);
    REQUIRE(s32_at(rows, 0x64) == 15);
    REQUIRE(s16_at(rows, 0xC6) == 11);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);

    // Rows counted with no pointer to them are refused.
    ArchiveBuilder lost(0x200);
    lost.u32(0x1B4, 2);
    lost.public_symbol(0x100, "grGroundParam");
    std::vector<std::byte> lost_bytes = lost.build();
    HSD_Archive lost_archive{};
    REQUIRE(HSD_ArchiveParse(&lost_archive, bytes_of(lost_bytes),
                             lost_bytes.size()) == 0);
    REQUIRE(HSD_ArchiveGetPublicAddress(&lost_archive, "grGroundParam") ==
            nullptr);
    REQUIRE(std::string_view(melee_host_hsd_archive_last_error())
                .find("stage rows") != std::string_view::npos);
    REQUIRE(melee_host_hsd_archive_release(lost_bytes.data()) ==
            MELEE_HOST_OK);
}

TEST_CASE("an effect table's particle banks load through the particle system")
{
    // The table points at a command bank and a texture bank and is followed
    // by two effect records before the first bank.  Inside the banks every
    // offset is bank-relative, not an archive relocation.
    ArchiveBuilder builder(0x100);
    builder.pointer(0x00, 0x40);
    builder.pointer(0x04, 0xC0);
    builder.f32(0x08, 7.5F);
    // Command bank: version 0x42, list IDs from 100, one list at +0x10.
    builder.u32(0x40, 0x00420000U);
    builder.u32(0x44, 100);
    builder.u32(0x48, 1);
    builder.u32(0x4C, 0x10);
    builder.u16(0x50, 1);
    builder.u16(0x52, 0);
    builder.u16(0x54, 2);
    builder.u16(0x56, 3);
    builder.u32(0x58, 0x06000001U);
    builder.f32(0x5C, 0.5F);
    builder.f32(0x88, 9.0F);
    builder.u8(0x8C, 0xAB);
    // Texture bank: one I8 group at +0x08 with its image at +0x30.
    builder.u32(0xC0, 1);
    builder.u32(0xC4, 0x08);
    builder.u32(0xC8, 1);
    builder.u32(0xCC, 1);
    builder.u32(0xD4, 4);
    builder.u32(0xD8, 4);
    builder.u32(0xE0, 0x30);
    builder.u8(0xF0, 0x5A);
    builder.public_symbol(0x00, "effTestDataTable");
    std::vector<std::byte> bytes = builder.build();

    struct HostEffectDesc {
        float lifetime;
        void* model[4];
    };
    struct HostEffectTable {
        MeleeHostParticleCmdBank* commands;
        MeleeHostParticleTexBank* textures;
        HostEffectDesc effects[2];
    };

    REQUIRE(melee_host_hsd_symbol_kind("effTestDataTable") ==
            MELEE_HOST_HSD_SYMBOL_GAME_DATA);
    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    auto* const table = static_cast<HostEffectTable*>(
        HSD_ArchiveGetPublicAddress(&archive, "effTestDataTable"));
    REQUIRE(table != nullptr);

    MeleeHostParticleCmdBank* const commands = table->commands;
    REQUIRE(commands != nullptr);
    REQUIRE(commands->magic == MELEE_HOST_PARTICLE_CMD_BANK_MAGIC);
    REQUIRE(commands->list_end == 101);
    REQUIRE(commands->lists[99] == nullptr);
    HSD_PSCmdList* const list = commands->lists[100];
    REQUIRE(list != nullptr);
    REQUIRE(list->type == 1);
    REQUIRE(list->genLife == 2);
    REQUIRE(list->life == 3);
    // psInitDataBankLocate clears bits 0x0E000000 and sets 0x08000000.
    REQUIRE(list->kind == 0x08000001U);
    REQUIRE(list->grav == 0.5F);
    REQUIRE(list->param3 == 9.0F);
    REQUIRE(list->cmdList[0] == 0xAB);

    MeleeHostParticleTexBank* const textures = table->textures;
    REQUIRE(textures != nullptr);
    REQUIRE(textures->magic == MELEE_HOST_PARTICLE_TEX_BANK_MAGIC);
    REQUIRE(textures->group_count == 1);
    HSD_PSTexGroup* const group = textures->groups[0];
    REQUIRE(group != nullptr);
    REQUIRE(group->num == 1);
    REQUIRE(group->fmt == 1);
    REQUIRE(group->width == 4);
    REQUIRE(group->texTable[0] != nullptr);
    REQUIRE(group->texTable[0][0] == 0x5A);

    REQUIRE(table->effects[0].lifetime == 7.5F);
    REQUIRE(table->effects[1].lifetime == 0.0F);
    REQUIRE(table->effects[0].model[0] == nullptr);

    // The particle system takes the tables from the host banks.
    psInitDataBank(7, static_cast<int*>(static_cast<void*>(commands)),
                   static_cast<int*>(static_cast<void*>(textures)), nullptr,
                   nullptr);
    REQUIRE(psCmdListArray[7] == 101);
    REQUIRE(ptclref_804D0E5C[7][100] == list);
    REQUIRE(psTexGroupArray[7][0] == group);
    psCmdListArray[7] = 0;
    ptclref_804D0E5C[7] = nullptr;
    psTexGroupArray[7] = nullptr;
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);

    // A table with one bank and not the other is refused.
    ArchiveBuilder lopsided(0x100);
    lopsided.pointer(0x00, 0x40);
    lopsided.u32(0x40, 0x00420000U);
    lopsided.public_symbol(0x00, "effTestDataTable");
    std::vector<std::byte> lopsided_bytes = lopsided.build();
    HSD_Archive lopsided_archive{};
    REQUIRE(HSD_ArchiveParse(&lopsided_archive, bytes_of(lopsided_bytes),
                             lopsided_bytes.size()) == 0);
    REQUIRE(HSD_ArchiveGetPublicAddress(&lopsided_archive,
                                        "effTestDataTable") == nullptr);
    REQUIRE(std::string_view(melee_host_hsd_archive_last_error())
                .find("one particle bank") != std::string_view::npos);
    REQUIRE(melee_host_hsd_archive_release(lopsided_bytes.data()) ==
            MELEE_HOST_OK);
}

TEST_CASE("parsing a buffer again replaces what was built from it")
{
    std::vector<std::byte> bytes = make_title_like_archive();
    const MeleeHostHsdArchiveStats before = archive_stats();
    HSD_Archive archive{};
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    REQUIRE(HSD_ArchiveParse(&archive, bytes_of(bytes), bytes.size()) == 0);
    REQUIRE(archive_stats().archives_live == before.archives_live + 1);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) == MELEE_HOST_OK);
    REQUIRE(archive_stats().archives_live == before.archives_live);
    REQUIRE(melee_host_hsd_archive_release(bytes.data()) ==
            MELEE_HOST_INVALID_ARGUMENT);
}

TEST_CASE("symbol kinds follow the name's suffix, longest first")
{
    REQUIRE(melee_host_hsd_symbol_kind("TtlMoji_Top_joint") ==
            MELEE_HOST_HSD_SYMBOL_JOINT);
    REQUIRE(melee_host_hsd_symbol_kind("TtlMoji_Top_matanim_joint") ==
            MELEE_HOST_HSD_SYMBOL_MAT_ANIM_JOINT);
    REQUIRE(melee_host_hsd_symbol_kind("TtlMoji_Top_shapeanim_joint") ==
            MELEE_HOST_HSD_SYMBOL_SHAPE_ANIM_JOINT);
    REQUIRE(melee_host_hsd_symbol_kind("TtlMoji_Top_animjoint") ==
            MELEE_HOST_HSD_SYMBOL_ANIM_JOINT);
    REQUIRE(melee_host_hsd_symbol_kind("ScTitle_cam_int1_camera") ==
            MELEE_HOST_HSD_SYMBOL_CAMERA);
    REQUIRE(melee_host_hsd_symbol_kind("ScTitle_scene_lights") ==
            MELEE_HOST_HSD_SYMBOL_SCENE_LIGHTS);
    REQUIRE(melee_host_hsd_symbol_kind("ScTitle_fog") ==
            MELEE_HOST_HSD_SYMBOL_FOG);
    REQUIRE(melee_host_hsd_symbol_kind("TitleMark_sobjdesc") ==
            MELEE_HOST_HSD_SYMBOL_SOBJ_DESC);
    REQUIRE(melee_host_hsd_symbol_kind(
                "PlyMario5K_Share_ACTION_Wait1_figatree") ==
            MELEE_HOST_HSD_SYMBOL_FIGATREE);
    REQUIRE(melee_host_hsd_symbol_kind("ScTitle_scene_data") ==
            MELEE_HOST_HSD_SYMBOL_UNSUPPORTED);
    // Text tables are named by a prefix instead.
    REQUIRE(melee_host_hsd_symbol_kind("SIS_MenuData") ==
            MELEE_HOST_HSD_SYMBOL_SIS_TABLE);
    REQUIRE(melee_host_hsd_symbol_kind("SIS_") ==
            MELEE_HOST_HSD_SYMBOL_UNSUPPORTED);
    // A suffix on its own names nothing.
    REQUIRE(melee_host_hsd_symbol_kind("_joint") ==
            MELEE_HOST_HSD_SYMBOL_UNSUPPORTED);
    REQUIRE(melee_host_hsd_symbol_kind(nullptr) ==
            MELEE_HOST_HSD_SYMBOL_UNSUPPORTED);
}

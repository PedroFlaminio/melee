#include "test.hpp"

#include "assets/hsd_materialize.hpp"

#include <melee_host/baselib.h>
#include <melee_host/hsd_archive.h>

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

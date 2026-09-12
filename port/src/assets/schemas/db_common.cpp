#include "assets/schemas/db_common.hpp"

namespace melee::assets::schemas {
namespace {

std::vector<std::string_view> decode_name_table(
    const HsdRuntimeArchive& archive, HsdRuntimeNode begin,
    HsdRuntimeNode end)
{
    if (end.data_offset < begin.data_offset ||
        (end.data_offset - begin.data_offset) % 4 != 0) {
        throw HsdArchiveError("DbCo name table has invalid bounds");
    }
    const std::uint32_t count =
        (end.data_offset - begin.data_offset) / 4;
    std::vector<std::string_view> names;
    names.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        const HsdRuntimeNode string = archive.reference_at(begin, index * 4);
        names.push_back(archive.read_c_string(string));
    }
    return names;
}

} // namespace

DbLoadCommonData decode_db_load_common_data(const HsdRuntimeArchive& archive)
{
    const HsdRuntimeNode root = archive.public_root("dbLoadCommonData");
    return {
        archive.reference_at(root, 0),
        archive.reference_at(root, 4),
        archive.reference_at(root, 8),
    };
}

DbCommonNameTables
decode_db_common_name_tables(const HsdRuntimeArchive& archive)
{
    const HsdRuntimeNode root = archive.public_root("dbLoadCommonData");
    const DbLoadCommonData data = decode_db_load_common_data(archive);
    return {
        decode_name_table(archive, data.bonus_names,
                          data.motion_state_names),
        decode_name_table(archive, data.motion_state_names,
                          data.submotion_names),
        decode_name_table(archive, data.submotion_names, root),
    };
}

} // namespace melee::assets::schemas

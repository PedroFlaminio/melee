#ifndef MELEE_HOST_DB_COMMON_SCHEMA_HPP
#define MELEE_HOST_DB_COMMON_SCHEMA_HPP

#include "assets/hsd_runtime_archive.hpp"

#include <string_view>
#include <vector>

namespace melee::assets::schemas {

struct DbLoadCommonData {
    HsdRuntimeNode bonus_names;
    HsdRuntimeNode motion_state_names;
    HsdRuntimeNode submotion_names;
};

struct DbCommonNameTables {
    std::vector<std::string_view> bonus_names;
    std::vector<std::string_view> motion_state_names;
    std::vector<std::string_view> submotion_names;
};

[[nodiscard]] DbLoadCommonData
decode_db_load_common_data(const HsdRuntimeArchive& archive);
[[nodiscard]] DbCommonNameTables
decode_db_common_name_tables(const HsdRuntimeArchive& archive);

} // namespace melee::assets::schemas

#endif

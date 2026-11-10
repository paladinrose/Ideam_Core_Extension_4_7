#include "splines_metadata_task_registry.h"
#include "../metadata_logic/spline_arc_length_lut_metadata_logic.h"
#include "../../../metadata_task.h"

#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::core {

const std::array<LogicVariantGroup, SplinesMetadataTaskRegistry::STORAGE_COUNT>
SplinesMetadataTaskRegistry::logic_variants = {{
    { true, 1, nullptr }
}};

void SplinesMetadataTaskRegistry::init_execution_routing() {}

void SplinesMetadataTaskRegistry::cleanup_execution_routing() {}

void SplinesMetadataTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    godot::StringName key("0");
    godot::Dictionary dict;
    dict["properties"] = SplineArcLengthLUTMetadataLogic<>::get_ui_properties();
    dict["name"]       = godot::String(SplineArcLengthLUTMetadataLogic<>::display_name.data());
    dict["category"]   = godot::String(SplineArcLengthLUTMetadataLogic<>::ui_category.data());

    godot::PackedInt64Array combos;
    const size_t flat_idx =
        static_cast<size_t>(MemoryView::StencilView) * (S_COUNT * T_COUNT) +
        static_cast<size_t>(MemoryStrategy::FlatStrategy) * T_COUNT +
        static_cast<size_t>(MemoryTypes::INT32);
    combos.push_back(static_cast<int64_t>(flat_idx));
    dict["valid_combinations"] = combos;
    p_matrix[key] = dict;
}

std::unique_ptr<INativeTask> SplinesMetadataTaskRegistry::create(
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    (void)p_view_id;
    (void)p_strategy_id;
    (void)p_type_id;
    if (p_logic_id == static_cast<uint32_t>(SplinesMetadataLogicID::SplineArcLengthLUT)) {
        return std::make_unique<MetadataTask<SplineArcLengthLUTMetadataLogic<>>>();
    }
    return nullptr;
}

} // namespace ideam::core

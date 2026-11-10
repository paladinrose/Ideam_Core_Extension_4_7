#include "tiles_transform_task_registry.h"
#include "../transform_logic/tiling_instance_transform_logic.h"
#include "../transform_logic/tiling_boundary_clip_and_seam_stitch_transform_logic.h"
#include "../transform_logic/tiling_hexize_and_border_extract_transform_logic.h"
#include "../transform_logic/tiling_quotient_astar_and_flow_field_transform_logic.h"
#include "../../../transform_task.h"

#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::core {

const std::array<LogicVariantGroup, TilesTransformTaskRegistry::STORAGE_COUNT>
TilesTransformTaskRegistry::logic_variants = {{
    { true, 1, nullptr },
    { true, 1, nullptr },
    { true, 1, nullptr },
    { true, 1, nullptr }
}};

void TilesTransformTaskRegistry::init_execution_routing() {}

void TilesTransformTaskRegistry::cleanup_execution_routing() {}

namespace {

template <typename T_Logic>
void append_tiles_transform_ui_entry(
    godot::Dictionary &r_matrix,
    TilesTransformLogicID p_id,
    MemoryTypes p_default_type
) {
    godot::StringName key(godot::String::num_int64(static_cast<int64_t>(p_id)));
    godot::Dictionary dict;
    dict["properties"] = T_Logic::get_ui_properties();
    dict["name"]       = godot::String(T_Logic::display_name.data());
    dict["category"]   = godot::String(T_Logic::ui_category.data());

    godot::PackedInt64Array combos;
    const size_t flat_idx =
        static_cast<size_t>(MemoryView::SingleElementView) *
            (TilesTransformTaskRegistry::S_COUNT * TilesTransformTaskRegistry::T_COUNT) +
        static_cast<size_t>(MemoryStrategy::FlatStrategy) * TilesTransformTaskRegistry::T_COUNT +
        static_cast<size_t>(p_default_type);
    combos.push_back(static_cast<int64_t>(flat_idx));
    dict["valid_combinations"] = combos;
    r_matrix[key] = dict;
}

} // namespace

void TilesTransformTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    append_tiles_transform_ui_entry<TilingInstanceTransformLogic<>>(
        p_matrix, TilesTransformLogicID::TilingInstanceTransform, MemoryTypes::INT32
    );
    append_tiles_transform_ui_entry<TilingBoundaryClipAndSeamStitchTransformLogic<>>(
        p_matrix, TilesTransformLogicID::TilingBoundaryClipAndSeamStitch, MemoryTypes::INT32
    );
    append_tiles_transform_ui_entry<TilingHexizeAndBorderExtractTransformLogic<>>(
        p_matrix, TilesTransformLogicID::TilingHexizeAndBorderExtract, MemoryTypes::INT32
    );
    append_tiles_transform_ui_entry<TilingQuotientAStarAndFlowFieldTransformLogic<>>(
        p_matrix, TilesTransformLogicID::TilingQuotientAStarAndFlowField, MemoryTypes::INT32
    );
}

std::unique_ptr<INativeTask> TilesTransformTaskRegistry::create(
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    (void)p_view_id;
    (void)p_strategy_id;
    (void)p_type_id;
    switch (static_cast<TilesTransformLogicID>(p_logic_id)) {
        case TilesTransformLogicID::TilingInstanceTransform:
            return std::make_unique<TransformTask<TilingInstanceTransformLogic<>>>();
        case TilesTransformLogicID::TilingBoundaryClipAndSeamStitch:
            return std::make_unique<TransformTask<TilingBoundaryClipAndSeamStitchTransformLogic<>>>();
        case TilesTransformLogicID::TilingHexizeAndBorderExtract:
            return std::make_unique<TransformTask<TilingHexizeAndBorderExtractTransformLogic<>>>();
        case TilesTransformLogicID::TilingQuotientAStarAndFlowField:
            return std::make_unique<TransformTask<TilingQuotientAStarAndFlowFieldTransformLogic<>>>();
        default:
            return nullptr;
    }
}

} // namespace ideam::core

#include "splines_transform_task_registry.h"
#include "../transform_logic/spline_batch_evaluate_transform_logic.h"
#include "../transform_logic/spline_flatten_to_polyline_transform_logic.h"
#include "../transform_logic/spline_bishop_frame_sweep_transform_logic.h"
#include "../transform_logic/spline_closest_point_project_transform_logic.h"
#include "../../../transform_task.h"

#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::core {

const std::array<LogicVariantGroup, SplinesTransformTaskRegistry::STORAGE_COUNT>
SplinesTransformTaskRegistry::logic_variants = {{
    { true, 1, nullptr },
    { true, 1, nullptr },
    { true, 1, nullptr },
    { true, 1, nullptr }
}};

void SplinesTransformTaskRegistry::init_execution_routing() {}

void SplinesTransformTaskRegistry::cleanup_execution_routing() {}

namespace {

template <typename T_Logic>
void append_spline_transform_ui_entry(
    godot::Dictionary &r_matrix,
    SplinesTransformLogicID p_id,
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
            (SplinesTransformTaskRegistry::S_COUNT * SplinesTransformTaskRegistry::T_COUNT) +
        static_cast<size_t>(MemoryStrategy::FlatStrategy) * SplinesTransformTaskRegistry::T_COUNT +
        static_cast<size_t>(p_default_type);
    combos.push_back(static_cast<int64_t>(flat_idx));
    dict["valid_combinations"] = combos;
    r_matrix[key] = dict;
}

} // namespace

void SplinesTransformTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    append_spline_transform_ui_entry<SplineBatchEvaluateTransformLogic<>>(
        p_matrix, SplinesTransformLogicID::SplineBatchEvaluate, MemoryTypes::INT32
    );
    append_spline_transform_ui_entry<SplineFlattenToPolylineTransformLogic<>>(
        p_matrix, SplinesTransformLogicID::SplineFlattenToPolyline, MemoryTypes::INT32
    );
    append_spline_transform_ui_entry<SplineBishopFrameSweepTransformLogic<>>(
        p_matrix, SplinesTransformLogicID::SplineBishopFrameSweep, MemoryTypes::INT32
    );
    append_spline_transform_ui_entry<SplineClosestPointProjectTransformLogic<>>(
        p_matrix, SplinesTransformLogicID::SplineClosestPointProject, MemoryTypes::INT32
    );
}

std::unique_ptr<INativeTask> SplinesTransformTaskRegistry::create(
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    (void)p_view_id;
    (void)p_strategy_id;
    (void)p_type_id;
    switch (static_cast<SplinesTransformLogicID>(p_logic_id)) {
        case SplinesTransformLogicID::SplineBatchEvaluate:
            return std::make_unique<TransformTask<SplineBatchEvaluateTransformLogic<>>>();
        case SplinesTransformLogicID::SplineFlattenToPolyline:
            return std::make_unique<TransformTask<SplineFlattenToPolylineTransformLogic<>>>();
        case SplinesTransformLogicID::SplineBishopFrameSweep:
            return std::make_unique<TransformTask<SplineBishopFrameSweepTransformLogic<>>>();
        case SplinesTransformLogicID::SplineClosestPointProject:
            return std::make_unique<TransformTask<SplineClosestPointProjectTransformLogic<>>>();
        default:
            return nullptr;
    }
}

} // namespace ideam::core

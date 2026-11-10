#include "geometry_query_task_registry.h"
#include "../query_logic/polygon_boundary_cull_query_logic.h"

#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::core {

const std::array<LogicVariantGroup, GeometryQueryTaskRegistry::STORAGE_COUNT>
GeometryQueryTaskRegistry::logic_variants = {{
    { true, 1, nullptr }
}};

void GeometryQueryTaskRegistry::init_execution_routing() {}

void GeometryQueryTaskRegistry::cleanup_execution_routing() {}

void GeometryQueryTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    godot::StringName key("0");
    godot::Dictionary dict;
    dict["properties"] = PolygonBoundaryCullQueryLogic<>::get_ui_properties();
    dict["name"]       = godot::String(PolygonBoundaryCullQueryLogic<>::display_name.data());
    dict["category"]   = godot::String(PolygonBoundaryCullQueryLogic<>::ui_category.data());

    godot::PackedInt64Array combos;
    for (size_t op = 0; op < O_COUNT; ++op) {
        const size_t flat_idx =
            op * (V_COUNT * S_COUNT * T_COUNT) +
            static_cast<size_t>(MemoryView::SingleElementView) * (S_COUNT * T_COUNT) +
            static_cast<size_t>(MemoryStrategy::FlatStrategy) * T_COUNT +
            static_cast<size_t>(MemoryTypes::INT32);
        combos.push_back(static_cast<int64_t>(flat_idx));
    }
    dict["valid_combinations"] = combos;
    p_matrix[key] = dict;
}

std::unique_ptr<INativeTask> GeometryQueryTaskRegistry::create(
    uint32_t p_op_id,
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    (void)p_view_id;
    (void)p_strategy_id;
    (void)p_type_id;
    if (p_logic_id == static_cast<uint32_t>(GeometryQueryLogicID::PolygonBoundaryCull)) {
        if (p_op_id == static_cast<uint32_t>(QueryOp::ADD)) {
            return std::make_unique<QueryTask<PolygonBoundaryCullQueryLogic<>, QueryOp::ADD>>();
        }
        return std::make_unique<QueryTask<PolygonBoundaryCullQueryLogic<>, QueryOp::CULL>>();
    }
    return nullptr;
}

} // namespace ideam::core

#include "geomorphology_query_task_registry.h"
#include "geomorphology_query_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

const std::array<LogicVariantGroup, GeomorphologyQueryTaskRegistry::STORAGE_COUNT> GeomorphologyQueryTaskRegistry::logic_variants = {{
    { true, 1, nullptr } // GeomorphicHazardAndResourceVein
}};

std::array<const GeomorphologyQueryTaskRegistry::SubMatrix *, GeomorphologyQueryTaskRegistry::STORAGE_COUNT> GeomorphologyQueryTaskRegistry::logic_matrices = {};

namespace {
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (GeomorphologyQueryLogicSubRegistry<static_cast<GeomorphologyQueryLogicID>(Is)>::init_execution_routing(), ...);
        ((GeomorphologyQueryTaskRegistry::logic_matrices[Is] = &GeomorphologyQueryLogicSubRegistry<static_cast<GeomorphologyQueryLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (GeomorphologyQueryLogicSubRegistry<static_cast<GeomorphologyQueryLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary &p_matrix, std::index_sequence<Is...>) {
        (void)p_matrix;
        (GeomorphologyQueryLogicSubRegistry<static_cast<GeomorphologyQueryLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void GeomorphologyQueryTaskRegistry::init_execution_routing() {
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void GeomorphologyQueryTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void GeomorphologyQueryTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
}

std::unique_ptr<INativeTask> GeomorphologyQueryTaskRegistry::create(
    uint32_t p_op_id,
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    if (p_op_id >= O_COUNT || p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }
    const SubMatrix *sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    const size_t flat_idx =
        p_op_id * (V_COUNT * S_COUNT * T_COUNT) +
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }
    return nullptr;
}

} // namespace ideam::core

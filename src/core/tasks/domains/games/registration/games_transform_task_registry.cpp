#include "games_transform_task_registry.h"
#include "games_transform_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

const std::array<LogicVariantGroup, GamesTransformTaskRegistry::STORAGE_COUNT> GamesTransformTaskRegistry::logic_variants = {{
    { false, 0, nullptr }
}};

std::array<const GamesTransformTaskRegistry::SubMatrix*, GamesTransformTaskRegistry::STORAGE_COUNT> GamesTransformTaskRegistry::logic_matrices = {};

namespace {
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (GamesTransformLogicSubRegistry<static_cast<GamesTransformLogicID>(Is)>::init_execution_routing(), ...);
        ((GamesTransformTaskRegistry::logic_matrices[Is] = &GamesTransformLogicSubRegistry<static_cast<GamesTransformLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (GamesTransformLogicSubRegistry<static_cast<GamesTransformLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary& p_matrix, std::index_sequence<Is...>) {
        (void)p_matrix;
        (GamesTransformLogicSubRegistry<static_cast<GamesTransformLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void GamesTransformTaskRegistry::init_execution_routing() {
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void GamesTransformTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void GamesTransformTaskRegistry::generate_ui_matrices(godot::Dictionary& p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> GamesTransformTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> GamesTransformTaskRegistry: Template unrolling complete.");
}

std::unique_ptr<INativeTask> GamesTransformTaskRegistry::create(
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    if (p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }

    const SubMatrix* sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    size_t flat_idx =
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }

    return nullptr;
}

} // namespace ideam::core
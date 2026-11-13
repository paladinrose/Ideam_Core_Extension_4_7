#include "communications_query_task_registry.h"
#include "communications_query_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

const std::array<LogicVariantGroup, CommunicationsQueryTaskRegistry::STORAGE_COUNT> CommunicationsQueryTaskRegistry::logic_variants = {{
    // ChannelSNRAudibility (0)
    { true, 1, nullptr }
}};

std::array<const CommunicationsQueryTaskRegistry::SubMatrix *, CommunicationsQueryTaskRegistry::STORAGE_COUNT> CommunicationsQueryTaskRegistry::logic_matrices = {};

namespace {
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (CommunicationsQueryLogicSubRegistry<static_cast<CommunicationsQueryLogicID>(Is)>::init_execution_routing(), ...);
        ((CommunicationsQueryTaskRegistry::logic_matrices[Is] = &CommunicationsQueryLogicSubRegistry<static_cast<CommunicationsQueryLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (CommunicationsQueryLogicSubRegistry<static_cast<CommunicationsQueryLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary &p_matrix, std::index_sequence<Is...>) {
        (void)p_matrix;
        (CommunicationsQueryLogicSubRegistry<static_cast<CommunicationsQueryLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void CommunicationsQueryTaskRegistry::init_execution_routing() {
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void CommunicationsQueryTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void CommunicationsQueryTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> CommunicationsQueryTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> CommunicationsQueryTaskRegistry: Template unrolling complete.");
}

std::unique_ptr<INativeTask> CommunicationsQueryTaskRegistry::create(
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

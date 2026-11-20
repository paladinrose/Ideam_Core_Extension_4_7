#include "cymatics_metadata_task_registry.h"
#include "cymatics_metadata_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

const std::array<LogicVariantGroup, CymaticsMetadataTaskRegistry::STORAGE_COUNT> CymaticsMetadataTaskRegistry::logic_variants = {{
    { true, 1, nullptr }
}};

std::array<const CymaticsMetadataTaskRegistry::SubMatrix *, CymaticsMetadataTaskRegistry::STORAGE_COUNT> CymaticsMetadataTaskRegistry::logic_matrices = {};

namespace {
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (CymaticsMetadataLogicSubRegistry<static_cast<CymaticsMetadataLogicID>(Is)>::init_execution_routing(), ...);
        ((CymaticsMetadataTaskRegistry::logic_matrices[Is] = &CymaticsMetadataLogicSubRegistry<static_cast<CymaticsMetadataLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (CymaticsMetadataLogicSubRegistry<static_cast<CymaticsMetadataLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary &p_matrix, std::index_sequence<Is...>) {
        (void)p_matrix;
        (CymaticsMetadataLogicSubRegistry<static_cast<CymaticsMetadataLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void CymaticsMetadataTaskRegistry::init_execution_routing() {
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void CymaticsMetadataTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void CymaticsMetadataTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> CymaticsMetadataTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> CymaticsMetadataTaskRegistry: Template unrolling complete.");
}

std::unique_ptr<INativeTask> CymaticsMetadataTaskRegistry::create(
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    if (p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }
    const SubMatrix *sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    const size_t flat_idx =
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }
    return nullptr;
}

} // namespace ideam::core

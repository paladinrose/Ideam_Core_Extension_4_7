#include "quantum_darwinism_metadata_task_registry.h"
#include "quantum_darwinism_metadata_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

const std::array<LogicVariantGroup, QuantumDarwinismMetadataTaskRegistry::STORAGE_COUNT> QuantumDarwinismMetadataTaskRegistry::logic_variants = {{
    { true, 1, nullptr }
}};

std::array<const QuantumDarwinismMetadataTaskRegistry::SubMatrix *, QuantumDarwinismMetadataTaskRegistry::STORAGE_COUNT> QuantumDarwinismMetadataTaskRegistry::logic_matrices = {};

namespace {
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (QuantumDarwinismMetadataLogicSubRegistry<static_cast<QuantumDarwinismMetadataLogicID>(Is)>::init_execution_routing(), ...);
        ((QuantumDarwinismMetadataTaskRegistry::logic_matrices[Is] = &QuantumDarwinismMetadataLogicSubRegistry<static_cast<QuantumDarwinismMetadataLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (QuantumDarwinismMetadataLogicSubRegistry<static_cast<QuantumDarwinismMetadataLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary &p_matrix, std::index_sequence<Is...>) {
        (void)p_matrix;
        (QuantumDarwinismMetadataLogicSubRegistry<static_cast<QuantumDarwinismMetadataLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void QuantumDarwinismMetadataTaskRegistry::init_execution_routing() {
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void QuantumDarwinismMetadataTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void QuantumDarwinismMetadataTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> QuantumDarwinismMetadataTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> QuantumDarwinismMetadataTaskRegistry: Template unrolling complete.");
}

std::unique_ptr<INativeTask> QuantumDarwinismMetadataTaskRegistry::create(
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

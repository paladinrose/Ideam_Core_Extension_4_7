#include "geomorphology_metadata_task_registry.h"
#include "geomorphology_metadata_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

const std::array<LogicVariantGroup, GeomorphologyMetadataTaskRegistry::STORAGE_COUNT> GeomorphologyMetadataTaskRegistry::logic_variants = {{
    { true, 1, nullptr } // DrainageBasinAndStrahlerPartition
}};

std::array<const GeomorphologyMetadataTaskRegistry::SubMatrix *, GeomorphologyMetadataTaskRegistry::STORAGE_COUNT> GeomorphologyMetadataTaskRegistry::logic_matrices = {};

namespace {
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (GeomorphologyMetadataLogicSubRegistry<static_cast<GeomorphologyMetadataLogicID>(Is)>::init_execution_routing(), ...);
        ((GeomorphologyMetadataTaskRegistry::logic_matrices[Is] = &GeomorphologyMetadataLogicSubRegistry<static_cast<GeomorphologyMetadataLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (GeomorphologyMetadataLogicSubRegistry<static_cast<GeomorphologyMetadataLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary &p_matrix, std::index_sequence<Is...>) {
        (void)p_matrix;
        (GeomorphologyMetadataLogicSubRegistry<static_cast<GeomorphologyMetadataLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void GeomorphologyMetadataTaskRegistry::init_execution_routing() {
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void GeomorphologyMetadataTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void GeomorphologyMetadataTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
}

std::unique_ptr<INativeTask> GeomorphologyMetadataTaskRegistry::create(
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

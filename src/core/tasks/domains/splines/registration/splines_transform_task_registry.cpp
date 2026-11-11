#include "splines_transform_task_registry.h"
#include "splines_transform_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

// --- Populate the Variant Map ---
const std::array<LogicVariantGroup, SplinesTransformTaskRegistry::STORAGE_COUNT> SplinesTransformTaskRegistry::logic_variants = {{
    // SplineBatchEvaluate (0)
    { true, 1, nullptr },
    // SplineFlattenToPolyline (1)
    { true, 1, nullptr },
    // SplineBishopFrameSweep (2)
    { true, 1, nullptr },
    // SplineClosestPointProject (3)
    { true, 1, nullptr }
}};

std::array<const SplinesTransformTaskRegistry::SubMatrix*, SplinesTransformTaskRegistry::STORAGE_COUNT> SplinesTransformTaskRegistry::logic_matrices = {};

namespace { // Translation Unit Firewall
    // --- Execution Routing (Fast Path) ---
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (SplinesTransformLogicSubRegistry<static_cast<SplinesTransformLogicID>(Is)>::init_execution_routing(), ...);
        ((SplinesTransformTaskRegistry::logic_matrices[Is] = &SplinesTransformLogicSubRegistry<static_cast<SplinesTransformLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (SplinesTransformLogicSubRegistry<static_cast<SplinesTransformLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    // --- UI Matrices (Heavy Path) ---
    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary& p_matrix, std::index_sequence<Is...>) {
        (void)p_matrix;
        (SplinesTransformLogicSubRegistry<static_cast<SplinesTransformLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void SplinesTransformTaskRegistry::init_execution_routing() {
    // Compiles into consecutive fast-path init calls and pointer assignments
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void SplinesTransformTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void SplinesTransformTaskRegistry::generate_ui_matrices(godot::Dictionary& p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> SplinesTransformTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    // Rips through valid template permutations to build the Godot Inspector UI
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> SplinesTransformTaskRegistry: Template unrolling complete.");
}

// --- The O(1) Dispatcher ---
std::unique_ptr<INativeTask> SplinesTransformTaskRegistry::create(
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    if (p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }

    // 1. O(1) Cache-line hit for the sub-registry pointer
    const SubMatrix* sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    // 2. O(1) Flat Index Math (Dropped the L dimension)
    size_t flat_idx =
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    // 3. Hot-path instantiation
    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }

    return nullptr;
}

} // namespace ideam::core
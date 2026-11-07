#pragma once

#include "../../../i_native_task.h"
#include "../../../../memory/memory_common.h"
#include "../../../registration/ideam_task_registry.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <array>
#include <cstdint>

namespace ideam::core {

// --- Compile-Time Coordinates for IdeamGames Transform Tasks ---
enum class GamesTransformLogicID : uint32_t {
    AgentActionSequenceStep = 0,
    AgentPossessionTransformBlend,
    ConsequenceRingDrain,
    ControlMapInputFilter,
    EntityTimeScaleCascade,
    GameEntitySnapshotSync,
    GameplayRuleApply,
    InteractionTugOfWarResolve,
    PieceActionScoreRefresh,
    PropertyModifierAggregate,
    PropertyModifierTick,
    PropertyRangeClampAndExhaust,
    Count
};

using GamesTransformTaskFactoryFn = INativeTask* (*)();

class GamesTransformTaskRegistry {
public:
    static constexpr size_t L_COUNT = static_cast<size_t>(GamesTransformLogicID::Count);
    static constexpr size_t STORAGE_COUNT = (L_COUNT > 0) ? L_COUNT : 1;
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);

    static const std::array<LogicVariantGroup, STORAGE_COUNT> logic_variants;

    // --- DOD Matrix Bounds (3D Sub-Matrix) ---
    static constexpr size_t SUB_MATRIX_SIZE = V_COUNT * S_COUNT * T_COUNT;

    using SubMatrix = std::array<GamesTransformTaskFactoryFn, SUB_MATRIX_SIZE>;

    // --- The O(1) Multi-Dimensional Factory Routers ---
    static std::array<const SubMatrix*, STORAGE_COUNT> logic_matrices;

    // --- Lifecycle Management ---
    static void init_execution_routing();
    static void cleanup_execution_routing();
    static void generate_ui_matrices(godot::Dictionary& p_matrix);

    static std::unique_ptr<INativeTask> create(
        uint32_t p_logic_id,
        uint32_t p_view_id,
        uint32_t p_strategy_id,
        uint32_t p_type_id
    );
};

} // namespace ideam::core
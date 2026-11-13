#pragma once

/**
 * linguistics_dod.h
 * DOD Orchestrator for `IdeamLinguistics` (`src/core/tasks/domains/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Bridges `SemeDescriptorPOD`, `SememeDescriptorPOD`, `PhonemePOD` (16-bit distinctive
 *   feature bitvectors), `MorphemePOD`, and `LexemePOD` onto `TaskGraphDOD`.
 * - `SemeComponentialAffinityQueryLogic`: Matches assets, `GameplayRule`s, and `ProceduralPattern`
 *   rules against natural-language Wizard prompts (e.g. `"top down, medieval, fantasy RPG"`)
 *   via Nuclear & Peripheral `Seme` bitmask intersection.
 * - `PhoneticHammingLexemeDecodeTransformLogic`: Reconstructs the most likely `Lexeme` and
 *   `Sememe` from noise-degraded `uint16_t` phoneme streams (biased by listener `AgentMindResource`
 *   expectations) and compiles grammar/vocabulary logit masks for optional local SLMs.
 */

#include "linguistics_dod_common.h"

namespace ideam::core {

class LinguisticsDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

public:
    LinguisticsDOD() = default;
    ~LinguisticsDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }
};

} // namespace ideam::core

#pragma once

/**
 * communications_dod.h
 * DOD Orchestrator for `IdeamCommunications` (`src/core/tasks/domains/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Implements the programmatic Shannon-Weaver communication pipeline on `TaskGraphDOD`:
 *     1. `DiscourseTurnAllocationMetadataLogic`: Evaluates Sacks-Schegloff-Jefferson Turn
 *        Construction Units (`LEXICAL`, `PHRASAL`, `CLAUSAL`, `SENTENTIAL`) and prosody resolution.
 *     2. `ChannelSNRAudibilityQueryLogic`: Combines Transmitter projection power + elocution skill,
 *        Medium attenuation + occlusion + noise floor, and Receiver listen/perception skill.
 *     3. `NoisyMediumPhonemeDegradeTransformLogic`: Flips bits in `PhonemePOD` 16-bit articulatory
 *        feature masks according to psycholinguistic vulnerability (Place -> Manner -> Voicing),
 *        feeding into `PhoneticHammingLexemeDecodeTransformLogic` (`IdeamLinguistics`) and
 *        `SensorySignalDecodeTransformLogic` (`IdeamMinds`).
 */

#include "communications_dod_common.h"

namespace ideam::core {

class CommunicationsDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

public:
    CommunicationsDOD() = default;
    ~CommunicationsDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }
};

} // namespace ideam::core

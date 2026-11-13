#pragma once

/**
 * linguistics_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedLinguistics` (`src/godot/tasks/domains/linguistics/`).
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

#include "data_oriented_linguistics.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class LinguisticsTaskHost : public TaskGraphHost {
    GDCLASS(LinguisticsTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedLinguistics> domain_resource_;

protected:
    static void _bind_methods();

public:
    LinguisticsTaskHost() = default;
    ~LinguisticsTaskHost() override = default;

    void set_domain_resource(const godot::Ref<DataOrientedLinguistics> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedLinguistics> get_domain_resource() const { return domain_resource_; }
};

} // namespace ideam::godot_ext

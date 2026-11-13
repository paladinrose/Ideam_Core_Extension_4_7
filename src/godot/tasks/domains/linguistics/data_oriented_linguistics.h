#pragma once

/**
 * data_oriented_linguistics.h
 * Godot `Resource` bridge for `LinguisticsDOD` (`src/godot/tasks/domains/linguistics/`).
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

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../../../core/tasks/domains/linguistics/linguistics_dod.h"
#include <memory>

namespace ideam::godot_ext {

class DataOrientedLinguistics : public godot::Resource {
    GDCLASS(DataOrientedLinguistics, godot::Resource)

private:
    std::unique_ptr<core::LinguisticsDOD> dod_engine_;

protected:
    static void _bind_methods();

public:
    DataOrientedLinguistics();
    ~DataOrientedLinguistics() override = default;

    [[nodiscard]] core::LinguisticsDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }
};

} // namespace ideam::godot_ext

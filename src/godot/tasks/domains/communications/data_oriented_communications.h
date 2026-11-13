#pragma once

/**
 * data_oriented_communications.h
 * Godot `Resource` bridge for `CommunicationsDOD` (`src/godot/tasks/domains/communications/`).
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

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../../../core/tasks/domains/communications/communications_dod.h"
#include <memory>

namespace ideam::godot_ext {

class DataOrientedCommunications : public godot::Resource {
    GDCLASS(DataOrientedCommunications, godot::Resource)

private:
    std::unique_ptr<core::CommunicationsDOD> dod_engine_;

protected:
    static void _bind_methods();

public:
    DataOrientedCommunications();
    ~DataOrientedCommunications() override = default;

    [[nodiscard]] core::CommunicationsDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }
};

} // namespace ideam::godot_ext

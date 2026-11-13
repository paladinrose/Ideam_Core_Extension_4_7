#pragma once

/**
 * communications_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedCommunications` (`src/godot/tasks/domains/communications/`).
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

#include "data_oriented_communications.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class CommunicationsTaskHost : public TaskGraphHost {
    GDCLASS(CommunicationsTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedCommunications> domain_resource_;

protected:
    static void _bind_methods();

public:
    CommunicationsTaskHost() = default;
    ~CommunicationsTaskHost() override = default;

    void set_domain_resource(const godot::Ref<DataOrientedCommunications> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedCommunications> get_domain_resource() const { return domain_resource_; }
};

} // namespace ideam::godot_ext

#pragma once

/**
 * transceiver_nodes.h
 * `CommunicationTransmitterNode` and `CommunicationReceiverNode` (`src/godot/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * SceneTree nodes succeeding `Speaker.cs` and `Listener.cs`:
 *   - `CommunicationTransmitterNode`: Reads RPG skills (`elocution_skill`, `projection_power_db`,
 *     or linked `GameProperty` nodes from `IdeamGames`), encodes utterances into `PhonemePOD`
 *     feature streams, and transmits them into a `CommunicationMediumResource`.
 *   - `CommunicationReceiverNode`: Reads RPG skills (`listen_skill`, `perception_bonus_db`),
 *     receives noise-degraded `PhonemePOD` streams from the medium, decodes them against the
 *     listener's `LanguageAssetResource` and `AgentMindResource` expectations, and emits
 *     the resulting speech-box text (including phonetically misheard words or garbled segments).
 */

#include "discourse_resource.h"
#include "../linguistics/lexicon_resource.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class CommunicationTransmitterNode : public godot::Node {
    GDCLASS(CommunicationTransmitterNode, godot::Node)

private:
    float elocution_skill_     = 1.0f;
    float projection_power_db_ = 65.0f;

protected:
    static void _bind_methods();

public:
    CommunicationTransmitterNode() = default;
    ~CommunicationTransmitterNode() override = default;

    void set_elocution_skill(float p_skill) { elocution_skill_ = p_skill; }
    [[nodiscard]] float get_elocution_skill() const noexcept { return elocution_skill_; }

    void set_projection_power_db(float p_db) { projection_power_db_ = p_db; }
    [[nodiscard]] float get_projection_power_db() const noexcept { return projection_power_db_; }
};

class CommunicationReceiverNode : public godot::Node {
    GDCLASS(CommunicationReceiverNode, godot::Node)

private:
    float listen_skill_        = 1.0f;
    float perception_bonus_db_ = 0.0f;

protected:
    static void _bind_methods();

public:
    CommunicationReceiverNode() = default;
    ~CommunicationReceiverNode() override = default;

    void set_listen_skill(float p_skill) { listen_skill_ = p_skill; }
    [[nodiscard]] float get_listen_skill() const noexcept { return listen_skill_; }

    void set_perception_bonus_db(float p_db) { perception_bonus_db_ = p_db; }
    [[nodiscard]] float get_perception_bonus_db() const noexcept { return perception_bonus_db_; }
};

} // namespace ideam::godot_ext

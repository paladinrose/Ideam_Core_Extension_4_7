#pragma once

/**
 * agent_mind_resource.h
 * Per-agent epistemic belief profile, instinctive trait weights, `KnowledgeThreshold`
 * (`Reason.cs`), and `IMentalState` excitation level (`UnityMind.cs`) (`src/godot/minds/`).
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../core/minds/mind_common.h"

namespace ideam::godot_ext {

class AgentMindResource : public godot::Resource {
    GDCLASS(AgentMindResource, godot::Resource)

private:
    uint32_t agent_id_            = 0u;
    core::InstinctiveTraitWeightsPod traits_{};
    float attention_threshold_    = 0.25f;
    float knowledge_threshold_    = 1.0f; ///< `Reason.cs`: `(confidence + acceptance) >= knowledge_threshold_`
    float excitation_level_       = 0.0f; ///< `UnityMind.cs`: `IMentalState.ExcitationLevel`
    uint32_t active_emotion_mask_ = 0u;   ///< Bitmask of active emotions (`UnityEmotion`)
    uint16_t primary_tome_id_     = core::TOME_PHYSICAL_REALITY; ///< Default Akashic Tome this mind consults

protected:
    static void _bind_methods();

public:
    AgentMindResource() = default;
    ~AgentMindResource() override = default;

    void set_agent_id(int p_id);
    [[nodiscard]] int get_agent_id() const { return static_cast<int>(agent_id_); }

    void set_primary_tome_id(int p_tome_id);
    [[nodiscard]] int get_primary_tome_id() const { return static_cast<int>(primary_tome_id_); }

    void set_fear_weight(float p_val);
    [[nodiscard]] float get_fear_weight() const { return traits_.fear; }

    void set_curiosity_weight(float p_val);
    [[nodiscard]] float get_curiosity_weight() const { return traits_.curiosity; }

    void set_guilt_weight(float p_val);
    [[nodiscard]] float get_guilt_weight() const { return traits_.guilt; }

    void set_loyalty_weight(float p_val);
    [[nodiscard]] float get_loyalty_weight() const { return traits_.loyalty; }

    void set_self_preservation_weight(float p_val);
    [[nodiscard]] float get_self_preservation_weight() const { return traits_.self_preservation; }

    void set_trust_in_listener(float p_val);
    [[nodiscard]] float get_trust_in_listener() const { return traits_.trust_in_listener; }

    void set_attention_threshold(float p_val);
    [[nodiscard]] float get_attention_threshold() const { return attention_threshold_; }

    void set_knowledge_threshold(float p_val);
    [[nodiscard]] float get_knowledge_threshold() const { return knowledge_threshold_; }

    void set_excitation_level(float p_val);
    [[nodiscard]] float get_excitation_level() const { return excitation_level_; }

    void set_active_emotion_mask(int p_mask);
    [[nodiscard]] int get_active_emotion_mask() const { return static_cast<int>(active_emotion_mask_); }

    [[nodiscard]] const core::InstinctiveTraitWeightsPod &get_core_traits() const noexcept { return traits_; }
};

} // namespace ideam::godot_ext

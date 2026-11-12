#pragma once

/**
 * agent_mind_resource.h
 * Per-agent epistemic belief profile and instinctive trait weights (`src/godot/minds/`).
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../core/minds/mind_common.h"

namespace ideam::godot_ext {

class AgentMindResource : public godot::Resource {
    GDCLASS(AgentMindResource, godot::Resource)

private:
    uint32_t agent_id_ = 0;
    core::InstinctiveTraitWeightsPod traits_{};
    float attention_threshold_ = 0.25f;

protected:
    static void _bind_methods();

public:
    AgentMindResource() = default;
    ~AgentMindResource() override = default;

    void set_agent_id(int p_id);
    [[nodiscard]] int get_agent_id() const { return static_cast<int>(agent_id_); }

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

    void set_attention_threshold(float p_val);
    [[nodiscard]] float get_attention_threshold() const { return attention_threshold_; }

    [[nodiscard]] const core::InstinctiveTraitWeightsPod &get_core_traits() const noexcept { return traits_; }
};

} // namespace ideam::godot_ext

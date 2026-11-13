#pragma once

/**
 * mind_agent_node.h
 * SceneTree node binding a `GameAgent` / `Character` to an `InformationMatrixResource`
 * (Akashic Tome registry) and `AgentMindResource` (Reference Catalogue profile) (`src/godot/minds/`).
 */

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "information_matrix_resource.h"
#include "agent_mind_resource.h"

namespace ideam::godot_ext {

class MindAgentNode : public godot::Node {
    GDCLASS(MindAgentNode, godot::Node)

private:
    godot::Ref<InformationMatrixResource> information_matrix_;
    godot::Ref<AgentMindResource> agent_mind_;

protected:
    static void _bind_methods();

public:
    MindAgentNode() = default;
    ~MindAgentNode() override = default;

    void set_information_matrix(const godot::Ref<InformationMatrixResource> &p_matrix);
    [[nodiscard]] godot::Ref<InformationMatrixResource> get_information_matrix() const { return information_matrix_; }

    void set_agent_mind(const godot::Ref<AgentMindResource> &p_mind);
    [[nodiscard]] godot::Ref<AgentMindResource> get_agent_mind() const { return agent_mind_; }

    void perceive_fact(int p_fact_id, int64_t p_perceived_value, float p_confidence = 1.0f, int p_source_agent_id = 0, float p_acceptance = 0.0f);
    void perceive_misreferenced_fact(
        int p_catalogued_fact_id,
        int p_true_fact_id,
        int64_t p_perceived_value,
        float p_confidence = 1.0f,
        float p_acceptance = 0.0f,
        int p_believed_tome_id = 0,
        int p_true_tome_id = 0
    );

    [[nodiscard]] int64_t query_believed_value(int p_fact_id) const;
    [[nodiscard]] float query_belief_confidence(int p_fact_id) const;
    [[nodiscard]] float query_belief_acceptance(int p_fact_id) const;
    [[nodiscard]] float query_belief_intuition_charge(int p_fact_id) const;
    [[nodiscard]] bool is_fact_known(int p_fact_id) const;
    [[nodiscard]] bool is_fact_misreferenced(int p_fact_id) const;
};

} // namespace ideam::godot_ext

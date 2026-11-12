#include "mind_agent_node.h"

namespace ideam::godot_ext {

void MindAgentNode::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_information_matrix", "matrix"), &MindAgentNode::set_information_matrix);
    ClassDB::bind_method(D_METHOD("get_information_matrix"), &MindAgentNode::get_information_matrix);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "information_matrix", PROPERTY_HINT_RESOURCE_TYPE, "InformationMatrixResource"),
        "set_information_matrix",
        "get_information_matrix"
    );

    ClassDB::bind_method(D_METHOD("set_agent_mind", "mind"), &MindAgentNode::set_agent_mind);
    ClassDB::bind_method(D_METHOD("get_agent_mind"), &MindAgentNode::get_agent_mind);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "agent_mind", PROPERTY_HINT_RESOURCE_TYPE, "AgentMindResource"),
        "set_agent_mind",
        "get_agent_mind"
    );

    ClassDB::bind_method(
        D_METHOD("perceive_fact", "fact_id", "perceived_value", "confidence", "source_agent_id"),
        &MindAgentNode::perceive_fact,
        DEFVAL(1.0f),
        DEFVAL(0)
    );
    ClassDB::bind_method(D_METHOD("query_believed_value", "fact_id"), &MindAgentNode::query_believed_value);
    ClassDB::bind_method(D_METHOD("query_belief_confidence", "fact_id"), &MindAgentNode::query_belief_confidence);
}

void MindAgentNode::set_information_matrix(const godot::Ref<InformationMatrixResource> &p_matrix) {
    information_matrix_ = p_matrix;
}

void MindAgentNode::set_agent_mind(const godot::Ref<AgentMindResource> &p_mind) {
    agent_mind_ = p_mind;
}

void MindAgentNode::perceive_fact(int p_fact_id, int64_t p_perceived_value, float p_confidence, int p_source_agent_id) {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return;
    }
    core::BeliefPod belief{};
    belief.fact_id         = static_cast<uint32_t>(p_fact_id);
    belief.source_agent_id = static_cast<uint32_t>(p_source_agent_id);
    belief.believed_value  = p_perceived_value;
    belief.confidence      = p_confidence;
    information_matrix_->get_core_matrix_mut().upsert_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        belief
    );
}

int64_t MindAgentNode::query_believed_value(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return 0;
    }
    const auto *b = information_matrix_->get_core_matrix().find_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(p_fact_id)
    );
    return b ? b->believed_value : 0;
}

float MindAgentNode::query_belief_confidence(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return 0.0f;
    }
    const auto *b = information_matrix_->get_core_matrix().find_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(p_fact_id)
    );
    return b ? b->confidence : 0.0f;
}

} // namespace ideam::godot_ext

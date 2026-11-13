#include "mind_agent_node.h"
#include <algorithm>

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
        D_METHOD("perceive_fact", "fact_id", "perceived_value", "confidence", "source_agent_id", "acceptance"),
        &MindAgentNode::perceive_fact,
        DEFVAL(1.0f),
        DEFVAL(0),
        DEFVAL(0.0f)
    );
    ClassDB::bind_method(
        D_METHOD("perceive_misreferenced_fact", "catalogued_fact_id", "true_fact_id", "perceived_value", "confidence", "acceptance", "believed_tome_id", "true_tome_id"),
        &MindAgentNode::perceive_misreferenced_fact,
        DEFVAL(1.0f),
        DEFVAL(0.0f),
        DEFVAL(0),
        DEFVAL(0)
    );

    ClassDB::bind_method(D_METHOD("query_believed_value", "fact_id"), &MindAgentNode::query_believed_value);
    ClassDB::bind_method(D_METHOD("query_belief_confidence", "fact_id"), &MindAgentNode::query_belief_confidence);
    ClassDB::bind_method(D_METHOD("query_belief_acceptance", "fact_id"), &MindAgentNode::query_belief_acceptance);
    ClassDB::bind_method(D_METHOD("query_belief_intuition_charge", "fact_id"), &MindAgentNode::query_belief_intuition_charge);
    ClassDB::bind_method(D_METHOD("is_fact_known", "fact_id"), &MindAgentNode::is_fact_known);
    ClassDB::bind_method(D_METHOD("is_fact_misreferenced", "fact_id"), &MindAgentNode::is_fact_misreferenced);
}

void MindAgentNode::set_information_matrix(const godot::Ref<InformationMatrixResource> &p_matrix) {
    information_matrix_ = p_matrix;
}

void MindAgentNode::set_agent_mind(const godot::Ref<AgentMindResource> &p_mind) {
    agent_mind_ = p_mind;
}

void MindAgentNode::perceive_fact(
    int p_fact_id,
    int64_t p_perceived_value,
    float p_confidence,
    int p_source_agent_id,
    float p_acceptance
) {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return;
    }
    const uint32_t fid    = static_cast<uint32_t>(std::max(0, p_fact_id));
    const uint16_t tome   = static_cast<uint16_t>(agent_mind_->get_primary_tome_id());
    core::BeliefPod belief{};
    belief.fact_id         = fid;
    belief.source_agent_id = static_cast<uint32_t>(std::max(0, p_source_agent_id));
    belief.believed_value  = p_perceived_value;
    belief.confidence      = std::clamp(p_confidence, 0.0f, 1.0f);
    belief.acceptance      = std::clamp(p_acceptance, 0.0f, 1.0f);

    information_matrix_->get_core_matrix_mut().upsert_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        belief,
        fid,
        tome,
        tome
    );
}

void MindAgentNode::perceive_misreferenced_fact(
    int p_catalogued_fact_id,
    int p_true_fact_id,
    int64_t p_perceived_value,
    float p_confidence,
    float p_acceptance,
    int p_believed_tome_id,
    int p_true_tome_id
) {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return;
    }
    information_matrix_->get_core_matrix_mut().upsert_misreferenced_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(std::max(0, p_catalogued_fact_id)),
        static_cast<uint32_t>(std::max(0, p_true_fact_id)),
        p_perceived_value,
        p_confidence,
        p_acceptance,
        static_cast<uint16_t>(std::clamp(p_believed_tome_id, 0, 65535)),
        static_cast<uint16_t>(std::clamp(p_true_tome_id, 0, 65535)),
        0u
    );
}

int64_t MindAgentNode::query_believed_value(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return 0;
    }
    const auto *b = information_matrix_->get_core_matrix().find_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(std::max(0, p_fact_id))
    );
    return b ? b->believed_value : 0;
}

float MindAgentNode::query_belief_confidence(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return 0.0f;
    }
    const auto *b = information_matrix_->get_core_matrix().find_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(std::max(0, p_fact_id))
    );
    return b ? b->confidence : 0.0f;
}

float MindAgentNode::query_belief_acceptance(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return 0.0f;
    }
    const auto *b = information_matrix_->get_core_matrix().find_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(std::max(0, p_fact_id))
    );
    return b ? b->acceptance : 0.0f;
}

float MindAgentNode::query_belief_intuition_charge(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return 0.0f;
    }
    const auto *b = information_matrix_->get_core_matrix().find_agent_belief(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(std::max(0, p_fact_id))
    );
    return b ? b->intuition_charge : 0.0f;
}

bool MindAgentNode::is_fact_known(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return false;
    }
    return information_matrix_->get_core_matrix().is_agent_belief_knowledge(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(std::max(0, p_fact_id)),
        agent_mind_->get_knowledge_threshold()
    );
}

bool MindAgentNode::is_fact_misreferenced(int p_fact_id) const {
    if (information_matrix_.is_null() || agent_mind_.is_null()) {
        return false;
    }
    return information_matrix_->get_core_matrix().is_agent_belief_misreferenced(
        static_cast<uint32_t>(agent_mind_->get_agent_id()),
        static_cast<uint32_t>(std::max(0, p_fact_id))
    );
}

} // namespace ideam::godot_ext

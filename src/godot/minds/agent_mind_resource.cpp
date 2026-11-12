#include "agent_mind_resource.h"
#include <algorithm>

namespace ideam::godot_ext {

void AgentMindResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_agent_id", "agent_id"), &AgentMindResource::set_agent_id);
    ClassDB::bind_method(D_METHOD("get_agent_id"), &AgentMindResource::get_agent_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "agent_id"), "set_agent_id", "get_agent_id");

    ClassDB::bind_method(D_METHOD("set_fear_weight", "weight"), &AgentMindResource::set_fear_weight);
    ClassDB::bind_method(D_METHOD("get_fear_weight"), &AgentMindResource::get_fear_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fear_weight", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_fear_weight", "get_fear_weight");

    ClassDB::bind_method(D_METHOD("set_curiosity_weight", "weight"), &AgentMindResource::set_curiosity_weight);
    ClassDB::bind_method(D_METHOD("get_curiosity_weight"), &AgentMindResource::get_curiosity_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "curiosity_weight", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_curiosity_weight", "get_curiosity_weight");

    ClassDB::bind_method(D_METHOD("set_guilt_weight", "weight"), &AgentMindResource::set_guilt_weight);
    ClassDB::bind_method(D_METHOD("get_guilt_weight"), &AgentMindResource::get_guilt_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "guilt_weight", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_guilt_weight", "get_guilt_weight");

    ClassDB::bind_method(D_METHOD("set_loyalty_weight", "weight"), &AgentMindResource::set_loyalty_weight);
    ClassDB::bind_method(D_METHOD("get_loyalty_weight"), &AgentMindResource::get_loyalty_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loyalty_weight", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_loyalty_weight", "get_loyalty_weight");

    ClassDB::bind_method(D_METHOD("set_self_preservation_weight", "weight"), &AgentMindResource::set_self_preservation_weight);
    ClassDB::bind_method(D_METHOD("get_self_preservation_weight"), &AgentMindResource::get_self_preservation_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "self_preservation_weight", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_self_preservation_weight", "get_self_preservation_weight");

    ClassDB::bind_method(D_METHOD("set_attention_threshold", "threshold"), &AgentMindResource::set_attention_threshold);
    ClassDB::bind_method(D_METHOD("get_attention_threshold"), &AgentMindResource::get_attention_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attention_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_attention_threshold", "get_attention_threshold");
}

void AgentMindResource::set_agent_id(int p_id) {
    agent_id_ = static_cast<uint32_t>(std::max(0, p_id));
    emit_changed();
}

void AgentMindResource::set_fear_weight(float p_val) {
    traits_.fear = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_curiosity_weight(float p_val) {
    traits_.curiosity = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_guilt_weight(float p_val) {
    traits_.guilt = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_loyalty_weight(float p_val) {
    traits_.loyalty = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_self_preservation_weight(float p_val) {
    traits_.self_preservation = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_attention_threshold(float p_val) {
    attention_threshold_ = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

} // namespace ideam::godot_ext

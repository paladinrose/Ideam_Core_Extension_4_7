#include "agent_mind_resource.h"
#include <algorithm>

namespace ideam::godot_ext {

void AgentMindResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_agent_id", "agent_id"), &AgentMindResource::set_agent_id);
    ClassDB::bind_method(D_METHOD("get_agent_id"), &AgentMindResource::get_agent_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "agent_id"), "set_agent_id", "get_agent_id");

    ClassDB::bind_method(D_METHOD("set_primary_tome_id", "tome_id"), &AgentMindResource::set_primary_tome_id);
    ClassDB::bind_method(D_METHOD("get_primary_tome_id"), &AgentMindResource::get_primary_tome_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "primary_tome_id"), "set_primary_tome_id", "get_primary_tome_id");

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

    ClassDB::bind_method(D_METHOD("set_trust_in_listener", "weight"), &AgentMindResource::set_trust_in_listener);
    ClassDB::bind_method(D_METHOD("get_trust_in_listener"), &AgentMindResource::get_trust_in_listener);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "trust_in_listener", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_trust_in_listener", "get_trust_in_listener");

    ClassDB::bind_method(D_METHOD("set_attention_threshold", "threshold"), &AgentMindResource::set_attention_threshold);
    ClassDB::bind_method(D_METHOD("get_attention_threshold"), &AgentMindResource::get_attention_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attention_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_attention_threshold", "get_attention_threshold");

    ClassDB::bind_method(D_METHOD("set_knowledge_threshold", "threshold"), &AgentMindResource::set_knowledge_threshold);
    ClassDB::bind_method(D_METHOD("get_knowledge_threshold"), &AgentMindResource::get_knowledge_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "knowledge_threshold", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"), "set_knowledge_threshold", "get_knowledge_threshold");

    ClassDB::bind_method(D_METHOD("set_excitation_level", "level"), &AgentMindResource::set_excitation_level);
    ClassDB::bind_method(D_METHOD("get_excitation_level"), &AgentMindResource::get_excitation_level);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "excitation_level", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_excitation_level", "get_excitation_level");

    ClassDB::bind_method(D_METHOD("set_active_emotion_mask", "mask"), &AgentMindResource::set_active_emotion_mask);
    ClassDB::bind_method(D_METHOD("get_active_emotion_mask"), &AgentMindResource::get_active_emotion_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_emotion_mask"), "set_active_emotion_mask", "get_active_emotion_mask");
}

void AgentMindResource::set_agent_id(int p_id) {
    agent_id_ = static_cast<uint32_t>(std::max(0, p_id));
    emit_changed();
}

void AgentMindResource::set_primary_tome_id(int p_tome_id) {
    primary_tome_id_ = static_cast<uint16_t>(std::clamp(p_tome_id, 0, 65535));
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

void AgentMindResource::set_trust_in_listener(float p_val) {
    traits_.trust_in_listener = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_attention_threshold(float p_val) {
    attention_threshold_ = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_knowledge_threshold(float p_val) {
    knowledge_threshold_ = std::clamp(p_val, 0.0f, 2.0f);
    emit_changed();
}

void AgentMindResource::set_excitation_level(float p_val) {
    excitation_level_ = std::clamp(p_val, 0.0f, 1.0f);
    emit_changed();
}

void AgentMindResource::set_active_emotion_mask(int p_mask) {
    active_emotion_mask_ = static_cast<uint32_t>(p_mask);
    emit_changed();
}

} // namespace ideam::godot_ext

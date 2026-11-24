#include "workflow_profile.h"

namespace ideam::godot_ext {

void WorkflowProfile::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_workflow_id", "id"), &WorkflowProfile::set_workflow_id);
    ClassDB::bind_method(D_METHOD("get_workflow_id"), &WorkflowProfile::get_workflow_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "workflow_id"), "set_workflow_id", "get_workflow_id");

    ClassDB::bind_method(D_METHOD("set_profile_name", "name"), &WorkflowProfile::set_profile_name);
    ClassDB::bind_method(D_METHOD("get_profile_name"), &WorkflowProfile::get_profile_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "profile_name"), "set_profile_name", "get_profile_name");

    ClassDB::bind_method(D_METHOD("set_smart_tool_overrides", "overrides"), &WorkflowProfile::set_smart_tool_overrides);
    ClassDB::bind_method(D_METHOD("get_smart_tool_overrides"), &WorkflowProfile::get_smart_tool_overrides);
    ADD_PROPERTY(
        PropertyInfo(Variant::DICTIONARY, "smart_tool_overrides"),
        "set_smart_tool_overrides",
        "get_smart_tool_overrides"
    );
    ClassDB::bind_method(
        D_METHOD("set_smart_tool_override", "tool_id", "profile"),
        &WorkflowProfile::set_smart_tool_override
    );
    ClassDB::bind_method(
        D_METHOD("get_smart_tool_override", "tool_id"),
        &WorkflowProfile::get_smart_tool_override
    );

    ClassDB::bind_method(D_METHOD("set_bridge_wiring", "wiring"), &WorkflowProfile::set_bridge_wiring);
    ClassDB::bind_method(D_METHOD("get_bridge_wiring"), &WorkflowProfile::get_bridge_wiring);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "bridge_wiring"), "set_bridge_wiring", "get_bridge_wiring");

    ClassDB::bind_method(D_METHOD("set_task_graph_path", "path"), &WorkflowProfile::set_task_graph_path);
    ClassDB::bind_method(D_METHOD("get_task_graph_path"), &WorkflowProfile::get_task_graph_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "task_graph_path", PROPERTY_HINT_FILE, "*.tres,*.res"),
        "set_task_graph_path",
        "get_task_graph_path"
    );

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &WorkflowProfile::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &WorkflowProfile::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &WorkflowProfile::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &WorkflowProfile::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(
        D_METHOD("apply_overrides_to_context", "context"),
        &WorkflowProfile::apply_overrides_to_context
    );
    ClassDB::bind_method(
        D_METHOD("release_overrides_from_context", "context"),
        &WorkflowProfile::release_overrides_from_context
    );
}

void WorkflowProfile::set_smart_tool_override(
    const godot::StringName &p_tool_id,
    const godot::Ref<SmartToolProfile> &p_profile
) {
    if (p_tool_id == godot::StringName()) {
        return;
    }
    if (p_profile.is_null()) {
        smart_tool_overrides_.erase(godot::String(p_tool_id));
    } else {
        smart_tool_overrides_[godot::String(p_tool_id)] = p_profile;
    }
}

godot::Ref<SmartToolProfile> WorkflowProfile::get_smart_tool_override(const godot::StringName &p_tool_id) const {
    const godot::String key(p_tool_id);
    if (smart_tool_overrides_.has(key)) {
        return smart_tool_overrides_[key];
    }
    return godot::Ref<SmartToolProfile>();
}

void WorkflowProfile::apply_overrides_to_context(const godot::Ref<ToolContext> &p_context) const {
    if (p_context.is_null()) {
        return;
    }
    const godot::Array keys = smart_tool_overrides_.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const godot::StringName tool_id{godot::String(keys[i])};
        godot::Ref<godot::Resource> profile = smart_tool_overrides_[keys[i]];
        if (profile.is_valid()) {
            p_context->push_preference_override(tool_id, profile);
        }
    }
}

void WorkflowProfile::release_overrides_from_context(const godot::Ref<ToolContext> &p_context) const {
    if (p_context.is_null()) {
        return;
    }
    const godot::Array keys = smart_tool_overrides_.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const godot::StringName tool_id{godot::String(keys[i])};
        godot::Ref<godot::Resource> profile = smart_tool_overrides_[keys[i]];
        if (profile.is_valid()) {
            p_context->pop_preference_override(tool_id);
        }
    }
}

} // namespace ideam::godot_ext

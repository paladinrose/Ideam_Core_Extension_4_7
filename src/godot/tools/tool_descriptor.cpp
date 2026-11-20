#include "tool_descriptor.h"

namespace ideam::godot_ext {

void ToolDescriptor::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(TOOL_TIER_DOMAIN);
    BIND_ENUM_CONSTANT(TOOL_TIER_BRIDGE);
    BIND_ENUM_CONSTANT(TOOL_TIER_SMART);
    BIND_ENUM_CONSTANT(TOOL_TIER_WORKFLOW);
    BIND_ENUM_CONSTANT(TOOL_TIER_AGENT);

    ClassDB::bind_method(D_METHOD("set_tool_id", "id"), &ToolDescriptor::set_tool_id);
    ClassDB::bind_method(D_METHOD("get_tool_id"), &ToolDescriptor::get_tool_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tool_id"), "set_tool_id", "get_tool_id");

    ClassDB::bind_method(D_METHOD("set_tier", "tier"), &ToolDescriptor::set_tier);
    ClassDB::bind_method(D_METHOD("get_tier"), &ToolDescriptor::get_tier);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "tier",
            PROPERTY_HINT_ENUM,
            "DomainTool:1,BridgeTool:2,SmartTool:3,WorkflowTool:4,AgentTool:5"
        ),
        "set_tier",
        "get_tier"
    );

    ClassDB::bind_method(D_METHOD("set_plugin_id", "plugin_id"), &ToolDescriptor::set_plugin_id);
    ClassDB::bind_method(D_METHOD("get_plugin_id"), &ToolDescriptor::get_plugin_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "plugin_id"), "set_plugin_id", "get_plugin_id");

    ClassDB::bind_method(D_METHOD("set_display_name", "name"), &ToolDescriptor::set_display_name);
    ClassDB::bind_method(D_METHOD("get_display_name"), &ToolDescriptor::get_display_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &ToolDescriptor::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &ToolDescriptor::get_description);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT),
        "set_description",
        "get_description"
    );

    ClassDB::bind_method(D_METHOD("set_handled_classes", "classes"), &ToolDescriptor::set_handled_classes);
    ClassDB::bind_method(D_METHOD("get_handled_classes"), &ToolDescriptor::get_handled_classes);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "handled_classes"), "set_handled_classes", "get_handled_classes");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &ToolDescriptor::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &ToolDescriptor::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &ToolDescriptor::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &ToolDescriptor::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_tutorial_ids", "tutorial_ids"), &ToolDescriptor::set_tutorial_ids);
    ClassDB::bind_method(D_METHOD("get_tutorial_ids"), &ToolDescriptor::get_tutorial_ids);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tutorial_ids"), "set_tutorial_ids", "get_tutorial_ids");

    ClassDB::bind_method(D_METHOD("set_input_schema", "schema"), &ToolDescriptor::set_input_schema);
    ClassDB::bind_method(D_METHOD("get_input_schema"), &ToolDescriptor::get_input_schema);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "input_schema"), "set_input_schema", "get_input_schema");

    ClassDB::bind_method(D_METHOD("to_schema_dictionary"), &ToolDescriptor::to_schema_dictionary);
    ClassDB::bind_static_method("ToolDescriptor", D_METHOD("tier_to_string", "tier"), &ToolDescriptor::tier_to_string);
}

godot::Dictionary ToolDescriptor::to_schema_dictionary() const {
    godot::Dictionary d;
    d["tool_id"]         = godot::String(tool_id_);
    d["tier"]            = static_cast<int32_t>(tier_);
    d["tier_name"]       = tier_to_string(tier_);
    d["plugin_id"]       = godot::String(plugin_id_);
    d["display_name"]    = display_name_;
    d["description"]     = description_;
    d["handled_classes"] = handled_classes_;
    d["semantic_tags"]   = semantic_tags_;
    d["help_topic_id"]   = godot::String(help_topic_id_);
    d["tutorial_ids"]    = tutorial_ids_;
    if (!input_schema_.is_empty()) {
        d["input_schema"] = input_schema_;
    }
    return d;
}

godot::String ToolDescriptor::tier_to_string(ToolTier p_tier) {
    switch (p_tier) {
        case TOOL_TIER_DOMAIN:   return "DomainTool";
        case TOOL_TIER_BRIDGE:   return "BridgeTool";
        case TOOL_TIER_SMART:    return "SmartTool";
        case TOOL_TIER_WORKFLOW: return "WorkflowTool";
        case TOOL_TIER_AGENT:    return "AgentTool";
    }
    return "Unknown";
}

} // namespace ideam::godot_ext

#pragma once

/**
 * tool_descriptor.h
 * Shared metadata resource (`godot::Resource`) describing any Ideam tool across all five tiers:
 *   1. DomainTool   — Single-plugin atomic control/editor
 *   2. BridgeTool   — Managed cross-plugin bridge / workspace
 *   3. SmartTool    — Stateful single-domain tool (preferences, templates, custom asset banks)
 *   4. WorkflowTool — Multi-plugin smart workflow orchestrator (preference override bundles, TaskGraphs, ProcGen)
 *   5. AgentTool    — SLM/LLM-driven cognitive tool (rubric research, coverage heatmaps, reversible tool calling)
 *
 * Consumed by `ToolRegistry`, `AgentToolBridge`, and `IdeamTutorials` (`TutorialContextIndexer`).
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

enum ToolTier : int32_t {
    TOOL_TIER_DOMAIN   = 1, // Tier 1: DomainTool
    TOOL_TIER_BRIDGE   = 2, // Tier 2: BridgeTool
    TOOL_TIER_SMART    = 3, // Tier 3: SmartTool
    TOOL_TIER_WORKFLOW = 4, // Tier 4: WorkflowTool
    TOOL_TIER_AGENT    = 5  // Tier 5: AgentTool
};

class ToolDescriptor : public godot::Resource {
    GDCLASS(ToolDescriptor, godot::Resource)

private:
    godot::StringName tool_id_;
    ToolTier tier_ = TOOL_TIER_DOMAIN;
    godot::StringName plugin_id_;
    godot::String display_name_;
    godot::String description_;
    godot::PackedStringArray handled_classes_;
    godot::PackedStringArray semantic_tags_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray tutorial_ids_;
    godot::Dictionary input_schema_;

protected:
    static void _bind_methods();

public:
    ToolDescriptor() = default;
    ~ToolDescriptor() override = default;

    void set_tool_id(const godot::StringName &p_id) { tool_id_ = p_id; }
    [[nodiscard]] godot::StringName get_tool_id() const { return tool_id_; }

    void set_tier(ToolTier p_tier) { tier_ = p_tier; }
    [[nodiscard]] ToolTier get_tier() const noexcept { return tier_; }

    void set_plugin_id(const godot::StringName &p_plugin_id) { plugin_id_ = p_plugin_id; }
    [[nodiscard]] godot::StringName get_plugin_id() const { return plugin_id_; }

    void set_display_name(const godot::String &p_name) { display_name_ = p_name; }
    [[nodiscard]] godot::String get_display_name() const { return display_name_; }

    void set_description(const godot::String &p_desc) { description_ = p_desc; }
    [[nodiscard]] godot::String get_description() const { return description_; }

    void set_handled_classes(const godot::PackedStringArray &p_classes) { handled_classes_ = p_classes; }
    [[nodiscard]] godot::PackedStringArray get_handled_classes() const { return handled_classes_; }

    void set_semantic_tags(const godot::PackedStringArray &p_tags) { semantic_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const { return help_topic_id_; }

    void set_tutorial_ids(const godot::PackedStringArray &p_ids) { tutorial_ids_ = p_ids; }
    [[nodiscard]] godot::PackedStringArray get_tutorial_ids() const { return tutorial_ids_; }

    void set_input_schema(const godot::Dictionary &p_schema) { input_schema_ = p_schema; }
    [[nodiscard]] godot::Dictionary get_input_schema() const { return input_schema_; }

    [[nodiscard]] godot::Dictionary to_schema_dictionary() const;
    [[nodiscard]] static godot::String tier_to_string(ToolTier p_tier);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ToolTier);

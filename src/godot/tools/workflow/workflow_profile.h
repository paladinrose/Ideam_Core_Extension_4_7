#pragma once

/**
 * workflow_profile.h
 * Tier 4 (`WorkflowTool`) composite preference & orchestration bundle (`src/godot/tools/workflow/`).
 *
 * Bundles:
 *   - Per-`SmartTool` preference overrides (`smart_tool_overrides`: `tool_id -> Ref<SmartToolProfile>`)
 *     that bypass normal user preferences strictly while this workflow runs inside a `ToolContext`.
 *   - `BridgeTool` wiring rules (`bridge_wiring` Dictionary).
 *   - Optional `IdeamGraphResource` path (`task_graph_path`) for TaskGraph / ProcGen orchestration.
 *   - Semantic tags and `help_topic_id`.
 */

#include "../smart/smart_tool_profile.h"
#include "../tool_context.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class WorkflowProfile : public godot::Resource {
    GDCLASS(WorkflowProfile, godot::Resource)

private:
    godot::StringName workflow_id_;
    godot::String profile_name_ = "Default Workflow";
    godot::Dictionary smart_tool_overrides_; // tool_id (String/StringName) -> Ref<SmartToolProfile>
    godot::Dictionary bridge_wiring_;        // bridge_id (String) -> config Dictionary
    godot::String task_graph_path_;          // Optional res:// path to IdeamGraphResource
    godot::PackedStringArray semantic_tags_;
    godot::StringName help_topic_id_;

protected:
    static void _bind_methods();

public:
    WorkflowProfile() = default;
    ~WorkflowProfile() override = default;

    void set_workflow_id(const godot::StringName &p_id) { workflow_id_ = p_id; }
    [[nodiscard]] godot::StringName get_workflow_id() const { return workflow_id_; }

    void set_profile_name(const godot::String &p_name) { profile_name_ = p_name; }
    [[nodiscard]] godot::String get_profile_name() const { return profile_name_; }

    void set_smart_tool_overrides(const godot::Dictionary &p_overrides) { smart_tool_overrides_ = p_overrides; }
    [[nodiscard]] godot::Dictionary get_smart_tool_overrides() const { return smart_tool_overrides_; }
    void set_smart_tool_override(
        const godot::StringName &p_tool_id,
        const godot::Ref<SmartToolProfile> &p_profile
    );
    [[nodiscard]] godot::Ref<SmartToolProfile> get_smart_tool_override(const godot::StringName &p_tool_id) const;

    void set_bridge_wiring(const godot::Dictionary &p_wiring) { bridge_wiring_ = p_wiring; }
    [[nodiscard]] godot::Dictionary get_bridge_wiring() const { return bridge_wiring_; }

    void set_task_graph_path(const godot::String &p_path) { task_graph_path_ = p_path; }
    [[nodiscard]] godot::String get_task_graph_path() const { return task_graph_path_; }

    void set_semantic_tags(const godot::PackedStringArray &p_tags) { semantic_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const { return help_topic_id_; }

    // Scoped override lifecycle helpers for WorkflowOrchestrator
    void apply_overrides_to_context(const godot::Ref<ToolContext> &p_context) const;
    void release_overrides_from_context(const godot::Ref<ToolContext> &p_context) const;
};

} // namespace ideam::godot_ext

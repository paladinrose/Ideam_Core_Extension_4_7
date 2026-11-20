#pragma once

/**
 * tool_registry.h
 * Unified C++ singleton registry decoupling all five Ideam tool tiers (`src/godot/tools/`):
 *   - Tier 1 (`DomainTool`):   `DomainToolDescriptor` control factories for single-plugin assets
 *   - Tier 2 (`BridgeTool`):   `BridgeLauncher` cross-plugin workspace handoff launchers
 *   - Tiers 1–5 Metadata:      `ToolDescriptor` catalog (queryable by `WorkflowTool`, `AgentTool`,
 *                              and `IdeamTutorials` / `TutorialContextIndexer`)
 *   - Decoupled Help Bus:      `ToolHelpHandler` callbacks allowing `IdeamTutorialsPlugin` to
 *                              intercept Help (`?`) / How-To triggers without circular includes.
 *
 * Plugins register their descriptors and launchers in `_enter_tree()` and unregister them in
 * `_exit_tree()` with zero compile-time coupling between domain plugins.
 */

#include "tool_context.h"
#include "tool_descriptor.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <functional>
#include <vector>

namespace ideam::godot_ext {

/**
 * Tier 1 (`DomainTool`) control factory descriptor.
 * Leading fields preserve positional aggregate-initializer order for concise plugin registration;
 * trailing metadata fields default cleanly for semantic tagging and IdeamTutorials integration.
 */
struct DomainToolDescriptor {
    godot::StringName workspace_id;   // Target BridgeTool workspace (e.g. "GeometryComposer")
    godot::StringName resource_class; // Handled Godot class name (e.g. "Spline2DResource")
    godot::String sidebar_tab;        // Sidebar tab category (e.g. "General", "Splines", "Tiles")
    godot::String display_label;      // Human-readable type label (e.g. "2D Spline")
    UpdateCommitPolicy default_policy = UPDATE_ON_COMMIT;

    // Instantiates and initially binds a DomainTool Control for `p_asset`.
    std::function<godot::Control *(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    )> create_control;

    // Optional rebind callback allowing a BridgeTool workspace to reuse a cached DomainTool Control
    // when switching between open assets of the same class.
    std::function<void(
        godot::Control *p_control,
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    )> bind_control;

    // Optional semantic & IdeamTutorials metadata (default-initialized for ergonomic registration)
    godot::PackedStringArray semantic_tags;
    godot::StringName help_topic_id;
    godot::PackedStringArray tutorial_ids;
};

/**
 * Tier 2 (`BridgeTool`) workspace handoff launcher descriptor.
 */
struct BridgeLauncher {
    godot::StringName workspace_id; // e.g. "GeometryComposer", "ProjectWorkspace"
    godot::String display_name;     // e.g. "Geometry Composer"

    // Opens `p_asset` in the sibling BridgeTool workspace, optionally inheriting `p_context`.
    std::function<void(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    )> open_asset;

    // Optional semantic & IdeamTutorials metadata
    godot::PackedStringArray semantic_tags;
    godot::StringName help_topic_id;
};

/**
 * Decoupled Help / Tutorial handler registered by `IdeamTutorialsPlugin` (or custom doc providers).
 */
struct ToolHelpHandler {
    godot::StringName provider_id;
    int32_t priority = 100; // Lower runs first

    // Returns true if the handler consumed the help request (e.g. opened TutorialChecklistDock / Spotlight).
    std::function<bool(
        const godot::StringName &p_tool_id,
        const godot::String &p_ui_selector,
        const godot::Dictionary &p_context
    )> on_help_requested;
};

class ToolRegistry {
private:
    std::vector<DomainToolDescriptor> domain_descriptors_;
    std::vector<BridgeLauncher> bridge_launchers_;
    std::vector<godot::Ref<ToolDescriptor>> tool_descriptors_;
    std::vector<ToolHelpHandler> help_handlers_;

    ToolRegistry() = default;

public:
    static ToolRegistry &get_singleton();

    // --- Tier 1: DomainTool Control Factories ---
    void register_control(DomainToolDescriptor p_descriptor);
    void unregister_class(const godot::StringName &p_workspace_id, const godot::StringName &p_resource_class);
    void unregister_workspace_controls(const godot::StringName &p_workspace_id);

    [[nodiscard]] const DomainToolDescriptor *find_descriptor(
        const godot::StringName &p_workspace_id,
        const godot::StringName &p_resource_class
    ) const;

    [[nodiscard]] const DomainToolDescriptor *find_descriptor_for_resource(
        const godot::StringName &p_workspace_id,
        const godot::Ref<godot::Resource> &p_asset
    ) const;

    // Cross-workspace discovery helpers used by BridgeTool, WorkflowTool, and AgentTool orchestrators
    [[nodiscard]] const DomainToolDescriptor *find_any_descriptor_for_class(
        const godot::StringName &p_resource_class
    ) const;

    [[nodiscard]] const DomainToolDescriptor *find_any_descriptor_for_resource(
        const godot::Ref<godot::Resource> &p_asset
    ) const;

    [[nodiscard]] const std::vector<DomainToolDescriptor> &get_all_descriptors() const noexcept {
        return domain_descriptors_;
    }

    [[nodiscard]] std::vector<DomainToolDescriptor> get_descriptors_for_workspace(
        const godot::StringName &p_workspace_id
    ) const;

    [[nodiscard]] std::vector<godot::String> get_sidebar_tabs_for_workspace(
        const godot::StringName &p_workspace_id
    ) const;

    // --- Tier 2: BridgeTool Workspace Launchers ---
    void register_workspace_launcher(BridgeLauncher p_launcher);
    void unregister_workspace_launcher(const godot::StringName &p_workspace_id);
    [[nodiscard]] const BridgeLauncher *find_workspace_launcher(
        const godot::StringName &p_workspace_id
    ) const;
    [[nodiscard]] const std::vector<BridgeLauncher> &get_all_workspace_launchers() const noexcept {
        return bridge_launchers_;
    }
    bool launch_asset_in_workspace(
        const godot::StringName &p_workspace_id,
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context = godot::Ref<ToolContext>()
    ) const;

    // --- Tiers 1–5: Unified ToolDescriptor Catalog (SmartTool / WorkflowTool / AgentTool / Tutorials) ---
    void register_tool_descriptor(const godot::Ref<ToolDescriptor> &p_descriptor);
    void unregister_tool_descriptor(const godot::StringName &p_tool_id);
    void unregister_plugin_tools(const godot::StringName &p_plugin_id);
    [[nodiscard]] godot::Ref<ToolDescriptor> find_tool_descriptor(const godot::StringName &p_tool_id) const;
    [[nodiscard]] std::vector<godot::Ref<ToolDescriptor>> get_tool_descriptors_by_tier(ToolTier p_tier) const;
    [[nodiscard]] const std::vector<godot::Ref<ToolDescriptor>> &get_all_tool_descriptors() const noexcept {
        return tool_descriptors_;
    }
    [[nodiscard]] godot::Array export_tool_catalog_schema() const;

    // --- Cross-Tier Help / IdeamTutorials Dispatch ---
    void register_help_handler(ToolHelpHandler p_handler);
    void unregister_help_handler(const godot::StringName &p_provider_id);
    bool dispatch_help_request(
        const godot::StringName &p_tool_id,
        const godot::String &p_ui_selector,
        const godot::Dictionary &p_context = godot::Dictionary()
    ) const;

    void clear();
};

} // namespace ideam::godot_ext

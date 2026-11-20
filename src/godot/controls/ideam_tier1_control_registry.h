#pragma once

/**
 * ideam_tier1_control_registry.h
 * Lightweight C++ factory registry decoupling Tier 2 / Tier 3+ workspaces (`IdeamToolWorkspace`)
 * from domain-specific Tier 1 controls (`src/godot/controls/`) and sibling Tier 2 launchers.
 *
 * Plugins register their Tier 1 control factories and Tier 2 workspace launchers during
 * `_enter_tree()` and unregister them during `_exit_tree()`, allowing multi-domain Tier 2 tools
 * (`GeometryComposer`, `PlanetarySystemsModeler`, `ProceduralComposer`, `DesignComposer`,
 * `CharacterModeler`, `ProjectWorkspace`) and future Tier 3–5 tools to interoperate without
 * circular include or plugin load-order dependencies.
 */

#include "ideam_tool_context.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <functional>
#include <vector>

namespace ideam::godot_ext {

struct Tier1ControlDescriptor {
    godot::StringName workspace_id;   // Target Tier 2 workspace (e.g. "GeometryComposer")
    godot::StringName resource_class; // Handled Godot class name (e.g. "Spline2DResource")
    godot::String sidebar_tab;        // Sidebar tab category (e.g. "General", "Splines", "Tiles")
    godot::String display_label;      // Human-readable type label (e.g. "2D Spline")
    IdeamUpdateCommitPolicy default_policy = UPDATE_ON_COMMIT;

    // Instantiates and initially binds a Tier 1 Control for `p_asset`.
    std::function<godot::Control *(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<IdeamToolContext> &p_context
    )> create_control;

    // Optional rebind callback allowing a Tier 2 workspace to reuse a cached Tier 1 Control
    // when switching between open assets of the same class.
    std::function<void(
        godot::Control *p_control,
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<IdeamToolContext> &p_context
    )> bind_control;
};

struct Tier2WorkspaceLauncher {
    godot::StringName workspace_id; // e.g. "GeometryComposer"
    godot::String display_name;     // e.g. "Geometry Composer"

    // Opens `p_asset` in the sibling Tier 2 workspace, optionally inheriting `p_context`.
    std::function<void(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<IdeamToolContext> &p_context
    )> open_asset;
};

class IdeamTier1ControlRegistry {
private:
    std::vector<Tier1ControlDescriptor> descriptors_;
    std::vector<Tier2WorkspaceLauncher> workspace_launchers_;

    IdeamTier1ControlRegistry() = default;

public:
    static IdeamTier1ControlRegistry &get_singleton();

    void register_control(Tier1ControlDescriptor p_descriptor);
    void unregister_class(const godot::StringName &p_workspace_id, const godot::StringName &p_resource_class);
    void unregister_workspace_controls(const godot::StringName &p_workspace_id);

    // Cross-Tier-2 Workspace Launcher Registration (Phase 3 Handoff)
    void register_workspace_launcher(Tier2WorkspaceLauncher p_launcher);
    void unregister_workspace_launcher(const godot::StringName &p_workspace_id);
    [[nodiscard]] const Tier2WorkspaceLauncher *find_workspace_launcher(
        const godot::StringName &p_workspace_id
    ) const;
    bool launch_asset_in_workspace(
        const godot::StringName &p_workspace_id,
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<IdeamToolContext> &p_context = godot::Ref<IdeamToolContext>()
    ) const;

    void clear();

    [[nodiscard]] const Tier1ControlDescriptor *find_descriptor(
        const godot::StringName &p_workspace_id,
        const godot::StringName &p_resource_class
    ) const;

    [[nodiscard]] const Tier1ControlDescriptor *find_descriptor_for_resource(
        const godot::StringName &p_workspace_id,
        const godot::Ref<godot::Resource> &p_asset
    ) const;

    // Cross-workspace discovery helpers used by ProjectWorkspace and Tier 3–5 orchestrators
    // to mine registered Tier 1 controls and resolve sibling Tier 2 workspace ownership.
    [[nodiscard]] const Tier1ControlDescriptor *find_any_descriptor_for_class(
        const godot::StringName &p_resource_class
    ) const;

    [[nodiscard]] const Tier1ControlDescriptor *find_any_descriptor_for_resource(
        const godot::Ref<godot::Resource> &p_asset
    ) const;

    [[nodiscard]] const std::vector<Tier1ControlDescriptor> &get_all_descriptors() const noexcept {
        return descriptors_;
    }

    [[nodiscard]] std::vector<Tier1ControlDescriptor> get_descriptors_for_workspace(
        const godot::StringName &p_workspace_id
    ) const;

    [[nodiscard]] std::vector<godot::String> get_sidebar_tabs_for_workspace(
        const godot::StringName &p_workspace_id
    ) const;
};

} // namespace ideam::godot_ext

#pragma once

/**
 * surfaces_composer.h
 * Context-aware `ToolWorkspace` window (`src/godot/surfaces/ui/`) mirroring `NarrativesComposer`:
 *   - Sidebar tabs for `Patches` (`NurbsPatch3DResource` / `CoonsPatch3DResource`) and `Networks` (`SurfaceNetworkResource`).
 *   - Command bar quick-create buttons (New NURBS Patch, New Coons/Gordon Patch, New Surface Network)
 *     with stable `ideam_ui_selector` metadata for `IdeamTutorials`.
 *   - Built-in `?` Help hook (`IdeamSurfaces`) and shared `ToolContext` + `IdeamUndoRedo`.
 */

#include "../../tools/tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class SurfacesComposer : public ToolWorkspace {
    GDCLASS(SurfacesComposer, ToolWorkspace)

private:
    static SurfacesComposer *shared_instance_;
    static godot::Window *shared_window_;

    godot::Button *btn_new_nurbs_   = nullptr;
    godot::Button *btn_new_coons_   = nullptr;
    godot::Button *btn_new_network_ = nullptr;

protected:
    static void _bind_methods();

    void _populate_command_bar(godot::HBoxContainer *p_custom_actions) override;
    godot::Control *_create_domain_control_for_asset(const godot::Ref<godot::Resource> &p_asset) override;

public:
    static constexpr const char *WORKSPACE_ID = "SurfacesComposer";

    SurfacesComposer();
    ~SurfacesComposer() override;

    void _on_new_nurbs_pressed();
    void _on_new_coons_pressed();
    void _on_new_network_pressed();

    static SurfacesComposer *get_or_create_shared_composer(godot::Control *p_owner = nullptr);
    static void open_surface_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext

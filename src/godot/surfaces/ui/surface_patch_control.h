#pragma once

/**
 * surface_patch_control.h
 * Tier 1 DomainTool (`src/godot/surfaces/ui/`): Interactive UV Isoparm & LOD Inspector/Editor
 * for `SurfacePatchResource` (`NurbsPatch3DResource`, `CoonsPatch3DResource`) and
 * `SurfaceNetworkResource`.
 *
 * Mirrors `IdeamNarratives` DomainTools (`LocationMap` / `NarrativeControl`):
 *   - Implements `DomainToolInterface` (`bind_domain_asset`, `get_domain_tool_id`, `get_domain_semantic_tags`)
 *   - Stamps stable `ideam_ui_selector` metadata keys on controls for `IdeamTutorials`
 *   - Exposes a built-in `?` Help button dispatching through `domain_tool::request_control_help(...)`
 *   - Provides live 2D UV isoparm/frame-probe preview canvas, LOD 0..3 budget readout,
 *     base U/V subdivision controls, and quick preset buttons.
 */

#include "../surface_network_resource.h"
#include "../surface_patch_resource.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class SurfacePatchControl : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(SurfacePatchControl, godot::VBoxContainer)

private:
    godot::Ref<SurfacePatchResource> patch_;
    godot::Ref<SurfaceNetworkResource> network_;
    godot::Ref<ToolContext> tool_context_;

    int preview_lod_ = 0;
    godot::Vector2 probe_uv_ = godot::Vector2(0.5f, 0.5f);
    bool dragging_probe_ = false;
    bool updating_ui_ = false;

    godot::OptionButton *lod_opt_    = nullptr;
    godot::SpinBox *u_divs_spin_     = nullptr;
    godot::SpinBox *v_divs_spin_     = nullptr;
    godot::Button *preset_flat_btn_  = nullptr;
    godot::Button *preset_cyl_btn_   = nullptr;
    godot::Button *preset_dome_btn_  = nullptr;
    godot::Control *uv_canvas_       = nullptr;
    godot::Label *telemetry_lbl_     = nullptr;
    godot::Label *probe_lbl_         = nullptr;

    void _build_ui();
    void _sync_ui_from_asset();
    void _update_probe_from_canvas_pos(const godot::Vector2 &p_pos);
    [[nodiscard]] godot::Ref<SurfacePatchResource> _active_patch() const;

protected:
    static void _bind_methods();

public:
    SurfacePatchControl();
    ~SurfacePatchControl() override = default;

    // --- DomainToolInterface ---
    void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;

    [[nodiscard]] godot::StringName get_domain_tool_id() const override {
        return godot::StringName("SurfacePatchControl");
    }

    [[nodiscard]] godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void set_surface_patch(const godot::Ref<SurfacePatchResource> &p_patch);
    [[nodiscard]] godot::Ref<SurfacePatchResource> get_surface_patch() const { return patch_; }

    void set_surface_network(const godot::Ref<SurfaceNetworkResource> &p_network);
    [[nodiscard]] godot::Ref<SurfaceNetworkResource> get_surface_network() const { return network_; }

    // --- UI Signal Handlers ---
    void _on_lod_selected(int p_index);
    void _on_u_divs_changed(double p_val);
    void _on_v_divs_changed(double p_val);
    void _on_preset_flat_pressed();
    void _on_preset_cylinder_pressed();
    void _on_preset_dome_pressed();
    void _on_help_pressed();
    void _on_uv_canvas_draw();
    void _on_uv_canvas_gui_input(const godot::Ref<godot::InputEvent> &p_event);
    void _on_asset_changed();
};

} // namespace ideam::godot_ext

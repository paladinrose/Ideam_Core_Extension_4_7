#pragma once

/**
 * blueprint_editor_control.h
 * Tier 1 interactive 2D floorplan authoring & preview control (`godot::VBoxContainer`)
 * for `BlueprintResource` (`src/godot/design/ui/`).
 *
 * Allows quick room/aperture authoring, live 2D layer preview (Spaces, Exterior/Interior Walls,
 * Doors/Windows, Labels), and immediate validation of `BlueprintResource` & `BlueprintCanvas2D`.
 */

#include "../blueprint_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

class BlueprintPreviewSurface : public godot::Control {
    GDCLASS(BlueprintPreviewSurface, godot::Control)

private:
    godot::Ref<BlueprintResource> blueprint_;
    float pixels_per_meter_ = 36.0f;

protected:
    static void _bind_methods() {}

public:
    BlueprintPreviewSurface();
    ~BlueprintPreviewSurface() override = default;

    void set_blueprint(const godot::Ref<BlueprintResource> &p_bp);
    void _draw() override;
};

class BlueprintEditorControl : public godot::VBoxContainer {
    GDCLASS(BlueprintEditorControl, godot::VBoxContainer)

private:
    godot::Ref<BlueprintResource> blueprint_;
    godot::Label *telemetry_label_       = nullptr;
    BlueprintPreviewSurface *preview_    = nullptr;

    void _refresh_ui();

protected:
    static void _bind_methods();

public:
    BlueprintEditorControl();
    ~BlueprintEditorControl() override;

    void set_blueprint(const godot::Ref<BlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<BlueprintResource> get_blueprint() const { return blueprint_; }

    void _on_add_sample_room_pressed();
    void _on_add_door_pressed();
    void _on_add_window_pressed();
    void _on_clear_pressed();
    void _on_blueprint_changed();
};

} // namespace ideam::godot_ext

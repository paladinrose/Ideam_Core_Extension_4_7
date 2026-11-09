#pragma once

/**
 * spline_3d_control.h
 * Editor & Runtime UI Control (`Spline3DControl`, extending `SplineControlBase`)
 * for inspecting and editing a `Spline3DResource`.
 *
 * Design Notes:
 *   - Configures `SplineControlBase` with `_get_expected_dimension() == 3`, automatically
 *     populating each `SplinePointField` in the alternating Point/Section chain with
 *     three color-coded `SpinBox` fields (`X:`, `Y:`, `Z:`) for the 3D anchor position
 *     and collapsible 3D Back/Fore tangent handles.
 *   - Omits an embedded 2D canvas (since 3D spatial visualization lives in the 3D Viewport
 *     via `SplineNode3D` and `Spline3DGizmo`), replacing it with a compact 3D Shape Presets,
 *     Euler Rotation, and live 3D AABB telemetry bar.
 *   - Provides bidirectional **3D Gizmo Selection Hooks** (`set_bound_gizmo_node`,
 *     `_on_gizmo_point_selected`, `_on_gizmo_section_selected`) so selecting a point or
 *     section row in `Spline3DControl` highlights that handle in the 3D Viewport Gizmo
 *     and vice versa.
 */

#include "../ui/spline_control_base.h"
#include "spline_3d_resource.h"
#include "spline_node3d.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace ideam::godot_ext {

class Spline3DControl : public SplineControlBase {
    GDCLASS(Spline3DControl, SplineControlBase)

private:
    godot::SpinBox *preset_points_spin = nullptr;
    godot::Label *aabb_telemetry_label = nullptr;

    // Optional weak reference (ObjectID) to a SplineNode3D in the scene for 3D Gizmo selection sync
    godot::ObjectID bound_gizmo_node_id;

    void _build_3d_extensions();
    [[nodiscard]] SplineNode3D *_get_bound_gizmo_node() const;

    // 3D Preset & Transform Callbacks
    void _on_preset_line();
    void _on_preset_circle_xz();
    void _on_preset_helix();
    void _on_rotate_y_45_pressed();
    void _on_rotate_x_45_pressed();

    // 3D Gizmo Sync Callbacks
    void _on_gizmo_node_point_selected(int p_point_index);
    void _on_gizmo_node_section_selected(int p_section_index);

protected:
    static void _bind_methods();

    // --- SplineControlBase Overrides ---
    [[nodiscard]] int _get_expected_dimension() const override { return 3; }
    void _on_resource_rebound() override;
    void _on_ui_refreshed() override;
    void _on_selection_updated() override;

    void _populate_point_field(int p_index, SplinePointField *p_field) override;
    void _commit_point_position(int p_index, const godot::Variant &p_position) override;
    void _commit_point_back_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) override;
    void _commit_point_fore_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) override;
    void _commit_add_default_point() override;
    void _commit_split_section(int p_section_index) override;

public:
    Spline3DControl();
    ~Spline3DControl() override;

    // Typed 3D Resource Binding
    void set_spline_3d(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_spline_3d() const;

    // Optional 3D Gizmo Host Node Bridge (for future Spline3DGizmo synchronization)
    void set_bound_gizmo_node(SplineNode3D *p_node);
    [[nodiscard]] SplineNode3D *get_bound_gizmo_node() const { return _get_bound_gizmo_node(); }
};

// Alias so code referencing either `Spline3DControl` or `Spline3DResourceControl` compiles cleanly
using Spline3DResourceControl = Spline3DControl;

} // namespace ideam::godot_ext
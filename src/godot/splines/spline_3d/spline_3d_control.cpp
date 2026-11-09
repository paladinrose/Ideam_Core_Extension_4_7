#include "spline_3d_control.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <numbers>

namespace ideam::godot_ext {

void Spline3DControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline_3d", "spline"), &Spline3DControl::set_spline_3d);
    ClassDB::bind_method(D_METHOD("get_spline_3d"), &Spline3DControl::get_spline_3d);

    ClassDB::bind_method(D_METHOD("set_bound_gizmo_node", "node"), &Spline3DControl::set_bound_gizmo_node);
    ClassDB::bind_method(D_METHOD("get_bound_gizmo_node"), &Spline3DControl::get_bound_gizmo_node);

    ClassDB::bind_method(D_METHOD("_on_preset_line"), &Spline3DControl::_on_preset_line);
    ClassDB::bind_method(D_METHOD("_on_preset_circle_xz"), &Spline3DControl::_on_preset_circle_xz);
    ClassDB::bind_method(D_METHOD("_on_preset_helix"), &Spline3DControl::_on_preset_helix);
    ClassDB::bind_method(D_METHOD("_on_rotate_y_45_pressed"), &Spline3DControl::_on_rotate_y_45_pressed);
    ClassDB::bind_method(D_METHOD("_on_rotate_x_45_pressed"), &Spline3DControl::_on_rotate_x_45_pressed);

    ClassDB::bind_method(D_METHOD("_on_gizmo_node_point_selected", "point_index"), &Spline3DControl::_on_gizmo_node_point_selected);
    ClassDB::bind_method(D_METHOD("_on_gizmo_node_section_selected", "section_index"), &Spline3DControl::_on_gizmo_node_section_selected);
}

Spline3DControl::Spline3DControl() {
    _build_3d_extensions();
}

Spline3DControl::~Spline3DControl() {
    set_bound_gizmo_node(nullptr);
}

void Spline3DControl::_build_3d_extensions() {
    using namespace godot;

    // Hide the 2D visual canvas slot since 3D spatial editing uses 3D Viewport Gizmos
    if (visual_editor_slot) {
        visual_editor_slot->set_visible(false);
    }

    if (custom_header_slot) {
        // Row 1: 3D Shape Presets & Bounding-Box Centered Euler Rotations
        HBoxContainer *preset_bar = memnew(HBoxContainer);
        preset_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_theme_constant_override("separation", 6);
        custom_header_slot->add_child(preset_bar);

        Label *preset_lbl = memnew(Label);
        preset_lbl->set_text("3D Presets:");
        preset_bar->add_child(preset_lbl);

        preset_points_spin = memnew(SpinBox);
        preset_points_spin->set_min(2.0);
        preset_points_spin->set_max(64.0);
        preset_points_spin->set_step(1.0);
        preset_points_spin->set_value(8.0);
        preset_points_spin->set_custom_minimum_size(Vector2(54.0f, 0.0f));
        preset_points_spin->set_tooltip_text("Point count for Line, Circle XZ, and Helix presets.");
        preset_bar->add_child(preset_points_spin);

        Button *btn_line = memnew(Button);
        btn_line->set_text("Line (-Z)");
        btn_line->set_tooltip_text("Generate a 3D line along the -Z forward axis (0,0,0 -> 0,0,-10).");
        btn_line->connect("pressed", Callable(this, "_on_preset_line"));
        preset_bar->add_child(btn_line);

        Button *btn_circle = memnew(Button);
        btn_circle->set_text("Circle XZ");
        btn_circle->set_tooltip_text("Generate a closed horizontal Catmull-Rom circle on the XZ plane.");
        btn_circle->connect("pressed", Callable(this, "_on_preset_circle_xz"));
        preset_bar->add_child(btn_circle);

        Button *btn_helix = memnew(Button);
        btn_helix->set_text("Helix");
        btn_helix->set_tooltip_text("Generate a 3D vertical spiral helix along the +Y axis.");
        btn_helix->connect("pressed", Callable(this, "_on_preset_helix"));
        preset_bar->add_child(btn_helix);

        Control *spacer = memnew(Control);
        spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_child(spacer);

        Button *btn_rot_y = memnew(Button);
        btn_rot_y->set_text("↻ Y 45°");
        btn_rot_y->set_tooltip_text("Rotate all 3D points and handles by 45° around the Y (Up) axis.");
        btn_rot_y->connect("pressed", Callable(this, "_on_rotate_y_45_pressed"));
        preset_bar->add_child(btn_rot_y);

        Button *btn_rot_x = memnew(Button);
        btn_rot_x->set_text("↻ X 45°");
        btn_rot_x->set_tooltip_text("Rotate all 3D points and handles by 45° around the X (Pitch) axis.");
        btn_rot_x->connect("pressed", Callable(this, "_on_rotate_x_45_pressed"));
        preset_bar->add_child(btn_rot_x);

        // Row 2: Live 3D Bounding Box (AABB) Telemetry Readout
        aabb_telemetry_label = memnew(Label);
        aabb_telemetry_label->set_text("AABB: (0, 0, 0)");
        aabb_telemetry_label->add_theme_color_override("font_color", Color(0.62f, 0.78f, 0.94f, 0.92f));
        custom_header_slot->add_child(aabb_telemetry_label);
    }
}

void Spline3DControl::set_spline_3d(const godot::Ref<Spline3DResource> &p_spline) {
    set_spline_resource(p_spline);
}

godot::Ref<Spline3DResource> Spline3DControl::get_spline_3d() const {
    return godot::Ref<Spline3DResource>(godot::Object::cast_to<Spline3DResource>(spline_resource.ptr()));
}

SplineNode3D *Spline3DControl::_get_bound_gizmo_node() const {
    if (bound_gizmo_node_id.is_null()) return nullptr;
    return godot::Object::cast_to<SplineNode3D>(godot::ObjectDB::get_instance(bound_gizmo_node_id));
}

void Spline3DControl::set_bound_gizmo_node(SplineNode3D *p_node) {
    SplineNode3D *prev = _get_bound_gizmo_node();
    if (prev == p_node) return;

    const godot::Callable cb_pt(this, "_on_gizmo_node_point_selected");
    const godot::Callable cb_sec(this, "_on_gizmo_node_section_selected");
    if (prev) {
        if (prev->is_connected("gizmo_point_selected", cb_pt)) {
            prev->disconnect("gizmo_point_selected", cb_pt);
        }
        if (prev->is_connected("gizmo_section_selected", cb_sec)) {
            prev->disconnect("gizmo_section_selected", cb_sec);
        }
    }

    bound_gizmo_node_id = p_node ? godot::ObjectID(p_node->get_instance_id()) : godot::ObjectID();

    if (p_node) {
        if (!p_node->is_connected("gizmo_point_selected", cb_pt)) {
            p_node->connect("gizmo_point_selected", cb_pt);
        }
        if (!p_node->is_connected("gizmo_section_selected", cb_sec)) {
            p_node->connect("gizmo_section_selected", cb_sec);
        }
    }
}

void Spline3DControl::_on_resource_rebound() {
    _on_ui_refreshed();
}

void Spline3DControl::_on_ui_refreshed() {
    using namespace godot;
    if (!aabb_telemetry_label) return;

    Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null() || s3d->get_point_count() == 0) {
        aabb_telemetry_label->set_text("3D AABB: Empty");
        return;
    }

    const AABB box = s3d->get_aabb(true);
    const Vector3 pos = box.position;
    const Vector3 sz  = box.size;
    aabb_telemetry_label->set_text(
        String("3D AABB Origin: (") +
        String::num(pos.x, 2) + String(", ") + String::num(pos.y, 2) + String(", ") + String::num(pos.z, 2) +
        String(")   |   Size: (") +
        String::num(sz.x, 2) + String(", ") + String::num(sz.y, 2) + String(", ") + String::num(sz.z, 2) +
        String(")")
    );
}

void Spline3DControl::_on_selection_updated() {
    if (SplineNode3D *gizmo_node = _get_bound_gizmo_node()) {
        if (selected_point_index >= 0) {
            gizmo_node->set_selected_gizmo_point_index(selected_point_index);
        } else if (selected_section_index >= 0) {
            gizmo_node->set_selected_gizmo_section_index(selected_section_index);
        } else {
            gizmo_node->set_selected_gizmo_point_index(-1);
        }
    }
}

void Spline3DControl::_populate_point_field(int p_index, SplinePointField *p_field) {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (!p_field || s3d.is_null()) return;

    p_field->set_dimension(3);
    p_field->configure_point_variant(
        p_index,
        s3d->get_point_position(p_index),
        s3d->get_point_back_handle(p_index),
        s3d->get_point_use_back_handle(p_index),
        s3d->get_point_fore_handle(p_index),
        s3d->get_point_use_fore_handle(p_index),
        s3d->get_point_count() > 0
    );
}

void Spline3DControl::_commit_point_position(int p_index, const godot::Variant &p_position) {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    s3d->commit_set_point_position(p_index, static_cast<godot::Vector3>(p_position));
}

void Spline3DControl::_commit_point_back_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    s3d->commit_set_point_back_handle(p_index, static_cast<godot::Vector3>(p_handle), p_enabled);
}

void Spline3DControl::_commit_point_fore_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    s3d->commit_set_point_fore_handle(p_index, static_cast<godot::Vector3>(p_handle), p_enabled);
}

void Spline3DControl::_commit_add_default_point() {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    const int n = s3d->get_point_count();
    godot::Vector3 next_pos(0.0f, 0.0f, 0.0f);
    if (n >= 2) {
        const godot::Vector3 last = s3d->get_point_position(n - 1);
        const godot::Vector3 prev = s3d->get_point_position(n - 2);
        const godot::Vector3 step = (last - prev).length_squared() > 1e-4f
            ? (last - prev).normalized() * 2.5f
            : godot::Vector3(0.0f, 0.0f, -2.5f);
        next_pos = last + step;
    } else if (n == 1) {
        next_pos = s3d->get_point_position(0) + godot::Vector3(0.0f, 0.0f, -2.5f);
    }
    s3d->commit_add_point(next_pos);
}

void Spline3DControl::_commit_split_section(int p_section_index) {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;

    const int sec_count = s3d->get_section_count();
    if (p_section_index < 0 || p_section_index >= sec_count) return;

    const godot::Vector3 mid_pos = s3d->sample_section(p_section_index, static_cast<real_t>(0.5));
    const SplineCurveType ctype  = s3d->get_section_curve_type(p_section_index);
    const real_t half_w          = std::max(
        static_cast<real_t>(0.01),
        s3d->get_section_weight(p_section_index) * static_cast<real_t>(0.5)
    );

    const godot::Dictionary before = s3d->get_snapshot_state();
    s3d->set_section_weight(p_section_index, half_w);
    if (p_section_index + 1 < s3d->get_point_count()) {
        s3d->insert_point(p_section_index + 1, mid_pos, ctype, half_w);
    } else {
        s3d->add_point(mid_pos, ctype, half_w);
    }
    s3d->commit_snapshot_action("Split 3D Spline Section", before, s3d->get_snapshot_state());
}

// --- 3D Preset & Transform Callbacks ---

void Spline3DControl::_on_preset_line() {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    const int pts = preset_points_spin ? std::max(2, static_cast<int>(preset_points_spin->get_value())) : 4;
    const godot::Dictionary before = s3d->get_snapshot_state();
    s3d->setup_line(
        godot::Vector3(0.0f, 0.0f, 0.0f),
        godot::Vector3(0.0f, 0.0f, -10.0f),
        pts,
        s3d->get_default_curve_type()
    );
    s3d->commit_snapshot_action("Preset 3D Spline Line", before, s3d->get_snapshot_state());
}

void Spline3DControl::_on_preset_circle_xz() {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    const int pts = preset_points_spin ? std::max(4, static_cast<int>(preset_points_spin->get_value())) : 8;
    const godot::Dictionary before = s3d->get_snapshot_state();
    s3d->setup_circle_xz(static_cast<real_t>(5.0), pts);
    s3d->commit_snapshot_action("Preset 3D Spline Circle XZ", before, s3d->get_snapshot_state());
}

void Spline3DControl::_on_preset_helix() {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    const int ppt = preset_points_spin ? std::max(4, static_cast<int>(preset_points_spin->get_value())) : 8;
    const godot::Dictionary before = s3d->get_snapshot_state();
    s3d->setup_helix(
        static_cast<real_t>(5.0),
        static_cast<real_t>(10.0),
        static_cast<real_t>(2.0),
        ppt
    );
    s3d->commit_snapshot_action("Preset 3D Spline Helix", before, s3d->get_snapshot_state());
}

void Spline3DControl::_on_rotate_y_45_pressed() {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    constexpr float DEG_45 = static_cast<float>(std::numbers::pi_v<double> * 0.25);
    s3d->commit_rotate_spline(godot::Vector3(0.0f, DEG_45, 0.0f));
}

void Spline3DControl::_on_rotate_x_45_pressed() {
    godot::Ref<Spline3DResource> s3d = get_spline_3d();
    if (s3d.is_null()) return;
    constexpr float DEG_45 = static_cast<float>(std::numbers::pi_v<double> * 0.25);
    s3d->commit_rotate_spline(godot::Vector3(DEG_45, 0.0f, 0.0f));
}

void Spline3DControl::_on_gizmo_node_point_selected(int p_point_index) {
    select_point(p_point_index);
}

void Spline3DControl::_on_gizmo_node_section_selected(int p_section_index) {
    select_section(p_section_index);
}

} // namespace ideam::godot_ext
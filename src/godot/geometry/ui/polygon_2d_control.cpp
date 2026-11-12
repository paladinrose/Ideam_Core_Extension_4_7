#include "polygon_2d_control.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void Polygon2DControl::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(POLY_PRESET_RECTANGLE);
    BIND_ENUM_CONSTANT(POLY_PRESET_REGULAR);
    BIND_ENUM_CONSTANT(POLY_PRESET_STAR);

    ClassDB::bind_method(D_METHOD("set_polygon_resource", "polygon"), &Polygon2DControl::set_polygon_resource);
    ClassDB::bind_method(D_METHOD("get_polygon_resource"), &Polygon2DControl::get_polygon_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "polygon_resource", PROPERTY_HINT_RESOURCE_TYPE, "Polygon2DResource"),
        "set_polygon_resource",
        "get_polygon_resource"
    );

    ClassDB::bind_method(D_METHOD("get_polygon_canvas"), &Polygon2DControl::get_polygon_canvas);
    ClassDB::bind_method(D_METHOD("get_csg_bridge_control"), &Polygon2DControl::get_csg_bridge_control);
    ClassDB::bind_method(D_METHOD("refresh_ui"), &Polygon2DControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &Polygon2DControl::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_closed_toggled", "closed"), &Polygon2DControl::_on_closed_toggled);
    ClassDB::bind_method(D_METHOD("_on_triangulation_toggled", "show"), &Polygon2DControl::_on_triangulation_toggled);
    ClassDB::bind_method(D_METHOD("_on_centroid_toggled", "show"), &Polygon2DControl::_on_centroid_toggled);
    ClassDB::bind_method(D_METHOD("_on_hull_overlay_toggled", "show"), &Polygon2DControl::_on_hull_overlay_toggled);
    ClassDB::bind_method(D_METHOD("_on_csg_workbench_toggled", "show"), &Polygon2DControl::_on_csg_workbench_toggled);
    ClassDB::bind_method(D_METHOD("_on_ensure_ccw_pressed"), &Polygon2DControl::_on_ensure_ccw_pressed);
    ClassDB::bind_method(D_METHOD("_on_ensure_cw_pressed"), &Polygon2DControl::_on_ensure_cw_pressed);
    ClassDB::bind_method(D_METHOD("_on_bake_convex_hull_pressed"), &Polygon2DControl::_on_bake_convex_hull_pressed);
    ClassDB::bind_method(D_METHOD("_on_fit_view_pressed"), &Polygon2DControl::_on_fit_view_pressed);

    ClassDB::bind_method(D_METHOD("_on_add_vertex_pressed"), &Polygon2DControl::_on_add_vertex_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_selected_vertex_pressed"), &Polygon2DControl::_on_remove_selected_vertex_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_pressed"), &Polygon2DControl::_on_clear_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_shape_pressed"), &Polygon2DControl::_on_generate_shape_pressed);

    ClassDB::bind_method(D_METHOD("_on_offset_preview_toggled", "show"), &Polygon2DControl::_on_offset_preview_toggled);
    ClassDB::bind_method(D_METHOD("_on_offset_params_changed", "unused"), &Polygon2DControl::_on_offset_params_changed);
    ClassDB::bind_method(D_METHOD("_on_join_style_selected", "idx"), &Polygon2DControl::_on_join_style_selected);
    ClassDB::bind_method(D_METHOD("_on_commit_offset_pressed"), &Polygon2DControl::_on_commit_offset_pressed);

    ClassDB::bind_method(D_METHOD("_on_canvas_point_selected", "vertex_index"), &Polygon2DControl::_on_canvas_point_selected);
    ClassDB::bind_method(D_METHOD("_on_selected_vertex_coords_edited", "unused"), &Polygon2DControl::_on_selected_vertex_coords_edited);
}

Polygon2DControl::Polygon2DControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
}

Polygon2DControl::~Polygon2DControl() {
    if (polygon_resource_.is_valid()) {
        const godot::Callable cb(this, "_on_resource_changed");
        if (polygon_resource_->is_connected("changed", cb)) {
            polygon_resource_->disconnect("changed", cb);
        }
    }
}

void Polygon2DControl::_build_ui() {
    using namespace godot;

    // 1. Top Viewport & Winding Toolbar
    HBoxContainer *top_bar = memnew(HBoxContainer);
    top_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(top_bar);

    closed_check_ = memnew(CheckBox);
    closed_check_->set_text("Closed");
    closed_check_->set_pressed(true);
    closed_check_->connect("toggled", Callable(this, "_on_closed_toggled"));
    top_bar->add_child(closed_check_);

    triangulation_check_ = memnew(CheckBox);
    triangulation_check_->set_text("Triangles");
    triangulation_check_->set_pressed(true);
    triangulation_check_->connect("toggled", Callable(this, "_on_triangulation_toggled"));
    top_bar->add_child(triangulation_check_);

    centroid_check_ = memnew(CheckBox);
    centroid_check_->set_text("Centroid");
    centroid_check_->set_pressed(true);
    centroid_check_->connect("toggled", Callable(this, "_on_centroid_toggled"));
    top_bar->add_child(centroid_check_);

    hull_overlay_check_ = memnew(CheckBox);
    hull_overlay_check_->set_text("Hull");
    hull_overlay_check_->set_pressed(false);
    hull_overlay_check_->connect("toggled", Callable(this, "_on_hull_overlay_toggled"));
    top_bar->add_child(hull_overlay_check_);

    csg_workbench_check_ = memnew(CheckBox);
    csg_workbench_check_->set_text("CSG & Seam Bridge");
    csg_workbench_check_->set_pressed(false);
    csg_workbench_check_->connect("toggled", Callable(this, "_on_csg_workbench_toggled"));
    top_bar->add_child(csg_workbench_check_);

    Button *ccw_btn = memnew(Button);
    ccw_btn->set_text("CCW");
    ccw_btn->connect("pressed", Callable(this, "_on_ensure_ccw_pressed"));
    top_bar->add_child(ccw_btn);

    Button *cw_btn = memnew(Button);
    cw_btn->set_text("CW");
    cw_btn->connect("pressed", Callable(this, "_on_ensure_cw_pressed"));
    top_bar->add_child(cw_btn);

    Button *hull_btn = memnew(Button);
    hull_btn->set_text("Bake Hull");
    hull_btn->connect("pressed", Callable(this, "_on_bake_convex_hull_pressed"));
    top_bar->add_child(hull_btn);

    Button *fit_btn = memnew(Button);
    fit_btn->set_text("Fit View");
    fit_btn->connect("pressed", Callable(this, "_on_fit_view_pressed"));
    top_bar->add_child(fit_btn);

    // 2. Interactive 2D Polygon Canvas
    polygon_canvas_ = memnew(Polygon2DCanvas);
    polygon_canvas_->connect("point_selected", Callable(this, "_on_canvas_point_selected"));
    add_child(polygon_canvas_);

    // 3. Parametric Shape Generator Row
    HBoxContainer *shape_bar = memnew(HBoxContainer);
    shape_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(shape_bar);

    Button *add_v_btn = memnew(Button);
    add_v_btn->set_text("+ Vertex");
    add_v_btn->connect("pressed", Callable(this, "_on_add_vertex_pressed"));
    shape_bar->add_child(add_v_btn);

    Button *rem_v_btn = memnew(Button);
    rem_v_btn->set_text("- Selected");
    rem_v_btn->connect("pressed", Callable(this, "_on_remove_selected_vertex_pressed"));
    shape_bar->add_child(rem_v_btn);

    Button *clr_btn = memnew(Button);
    clr_btn->set_text("Clear");
    clr_btn->connect("pressed", Callable(this, "_on_clear_pressed"));
    shape_bar->add_child(clr_btn);

    preset_opt_ = memnew(OptionButton);
    preset_opt_->add_item("Preset: Regular N-Gon", POLY_PRESET_REGULAR);
    preset_opt_->add_item("Preset: Star Polygon", POLY_PRESET_STAR);
    preset_opt_->add_item("Preset: Rectangle", POLY_PRESET_RECTANGLE);
    shape_bar->add_child(preset_opt_);

    sides_spin_ = memnew(SpinBox);
    sides_spin_->set_min(3.0);
    sides_spin_->set_max(64.0);
    sides_spin_->set_step(1.0);
    sides_spin_->set_value(6.0);
    sides_spin_->set_prefix("N:");
    shape_bar->add_child(sides_spin_);

    outer_radius_spin_ = memnew(SpinBox);
    outer_radius_spin_->set_min(0.05);
    outer_radius_spin_->set_max(100.0);
    outer_radius_spin_->set_step(0.05);
    outer_radius_spin_->set_value(0.8);
    outer_radius_spin_->set_prefix("R:");
    shape_bar->add_child(outer_radius_spin_);

    inner_radius_spin_ = memnew(SpinBox);
    inner_radius_spin_->set_min(0.02);
    inner_radius_spin_->set_max(100.0);
    inner_radius_spin_->set_step(0.05);
    inner_radius_spin_->set_value(0.4);
    inner_radius_spin_->set_prefix("r:");
    shape_bar->add_child(inner_radius_spin_);

    Button *gen_btn = memnew(Button);
    gen_btn->set_text("Generate");
    gen_btn->connect("pressed", Callable(this, "_on_generate_shape_pressed"));
    shape_bar->add_child(gen_btn);

    // 4. Polygon Offset / Buffer & Miter Join Row
    HBoxContainer *offset_bar = memnew(HBoxContainer);
    offset_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(offset_bar);

    offset_preview_check_ = memnew(CheckBox);
    offset_preview_check_->set_text("Preview Offset");
    offset_preview_check_->set_pressed(false);
    offset_preview_check_->connect("toggled", Callable(this, "_on_offset_preview_toggled"));
    offset_bar->add_child(offset_preview_check_);

    offset_delta_spin_ = memnew(SpinBox);
    offset_delta_spin_->set_min(-20.0);
    offset_delta_spin_->set_max(20.0);
    offset_delta_spin_->set_step(0.02);
    offset_delta_spin_->set_value(0.12);
    offset_delta_spin_->set_prefix("Δ:");
    offset_delta_spin_->connect("value_changed", Callable(this, "_on_offset_params_changed"));
    offset_bar->add_child(offset_delta_spin_);

    join_style_opt_ = memnew(OptionButton);
    join_style_opt_->add_item("Join: Miter", JOIN_STYLE_MITER);
    join_style_opt_->add_item("Join: Bevel", JOIN_STYLE_BEVEL);
    join_style_opt_->add_item("Join: Round", JOIN_STYLE_ROUND);
    join_style_opt_->connect("item_selected", Callable(this, "_on_join_style_selected"));
    offset_bar->add_child(join_style_opt_);

    miter_limit_spin_ = memnew(SpinBox);
    miter_limit_spin_->set_min(1.0);
    miter_limit_spin_->set_max(16.0);
    miter_limit_spin_->set_step(0.25);
    miter_limit_spin_->set_value(2.0);
    miter_limit_spin_->set_prefix("Miter:");
    miter_limit_spin_->connect("value_changed", Callable(this, "_on_offset_params_changed"));
    offset_bar->add_child(miter_limit_spin_);

    Button *commit_off_btn = memnew(Button);
    commit_off_btn->set_text("Commit Offset");
    commit_off_btn->connect("pressed", Callable(this, "_on_commit_offset_pressed"));
    offset_bar->add_child(commit_off_btn);

    // 5. Collapsible Cross-Plugin CSG & Seam Bridge Workbench
    csg_bridge_control_ = memnew(GeometryCSGBridgeControl);
    csg_bridge_control_->set_visible(false);
    add_child(csg_bridge_control_);

    // 6. Selected Vertex Coordinate Editor & Telemetry Footer
    HBoxContainer *sel_bar = memnew(HBoxContainer);
    sel_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(sel_bar);

    Label *sel_lbl = memnew(Label);
    sel_lbl->set_text("Selected Vertex:");
    sel_bar->add_child(sel_lbl);

    sel_vertex_x_spin_ = memnew(SpinBox);
    sel_vertex_x_spin_->set_min(-1000.0);
    sel_vertex_x_spin_->set_max(1000.0);
    sel_vertex_x_spin_->set_step(0.02);
    sel_vertex_x_spin_->set_prefix("X:");
    sel_vertex_x_spin_->connect("value_changed", Callable(this, "_on_selected_vertex_coords_edited"));
    sel_bar->add_child(sel_vertex_x_spin_);

    sel_vertex_y_spin_ = memnew(SpinBox);
    sel_vertex_y_spin_->set_min(-1000.0);
    sel_vertex_y_spin_->set_max(1000.0);
    sel_vertex_y_spin_->set_step(0.02);
    sel_vertex_y_spin_->set_prefix("Y:");
    sel_vertex_y_spin_->connect("value_changed", Callable(this, "_on_selected_vertex_coords_edited"));
    sel_bar->add_child(sel_vertex_y_spin_);

    telemetry_label_ = memnew(Label);
    telemetry_label_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    sel_bar->add_child(telemetry_label_);
}

void Polygon2DControl::set_polygon_resource(const godot::Ref<Polygon2DResource> &p_polygon) {
    if (polygon_resource_ == p_polygon) return;

    const godot::Callable cb(this, "_on_resource_changed");
    if (polygon_resource_.is_valid() && polygon_resource_->is_connected("changed", cb)) {
        polygon_resource_->disconnect("changed", cb);
    }

    polygon_resource_ = p_polygon;

    if (polygon_canvas_) {
        polygon_canvas_->set_polygon_resource(polygon_resource_);
        if (polygon_resource_.is_valid() && polygon_resource_->get_vertex_count() > 0) {
            polygon_canvas_->fit_view_to_data(true, true, 0.2f);
        }
    }
    if (csg_bridge_control_ && polygon_resource_.is_valid()) {
        csg_bridge_control_->set_subject_polygon(polygon_resource_);
    }

    if (polygon_resource_.is_valid() && !polygon_resource_->is_connected("changed", cb)) {
        polygon_resource_->connect("changed", cb);
    }

    refresh_ui();
}

void Polygon2DControl::_on_resource_changed() {
    refresh_ui();
}

void Polygon2DControl::refresh_ui() {
    is_updating_ui_ = true;

    if (polygon_resource_.is_valid()) {
        if (closed_check_) closed_check_->set_pressed(polygon_resource_->is_closed());

        const int sel = polygon_canvas_ ? polygon_canvas_->get_selected_point_index() : -1;
        if (sel >= 0 && sel < polygon_resource_->get_vertex_count()) {
            const godot::Vector2 v = polygon_resource_->get_vertex(sel);
            if (sel_vertex_x_spin_) {
                sel_vertex_x_spin_->set_editable(true);
                sel_vertex_x_spin_->set_value(v.x);
            }
            if (sel_vertex_y_spin_) {
                sel_vertex_y_spin_->set_editable(true);
                sel_vertex_y_spin_->set_value(v.y);
            }
        } else {
            if (sel_vertex_x_spin_) sel_vertex_x_spin_->set_editable(false);
            if (sel_vertex_y_spin_) sel_vertex_y_spin_->set_editable(false);
        }
    }

    _sync_offset_preview();
    _update_telemetry_label();
    if (polygon_canvas_) {
        polygon_canvas_->queue_redraw();
    }
    if (csg_bridge_control_ && csg_bridge_control_->is_visible()) {
        csg_bridge_control_->refresh_ui();
    }

    is_updating_ui_ = false;
}

void Polygon2DControl::_sync_offset_preview() {
    if (!polygon_canvas_) return;
    const real_t delta = offset_delta_spin_ ? static_cast<real_t>(offset_delta_spin_->get_value()) : static_cast<real_t>(0.12);
    const GeometryJoinStyle2D join = (join_style_opt_ && join_style_opt_->get_selected() >= 0)
        ? static_cast<GeometryJoinStyle2D>(join_style_opt_->get_item_id(join_style_opt_->get_selected()))
        : JOIN_STYLE_MITER;
    const real_t miter = miter_limit_spin_ ? static_cast<real_t>(miter_limit_spin_->get_value()) : static_cast<real_t>(2.0);
    polygon_canvas_->set_preview_offset_params(delta, join, miter);
}

void Polygon2DControl::_update_telemetry_label() {
    using namespace godot;
    if (!telemetry_label_) return;
    if (polygon_resource_.is_null()) {
        telemetry_label_->set_text("  No Polygon2DResource bound.");
        return;
    }

    const int n_verts       = polygon_resource_->get_vertex_count();
    const int n_tris        = static_cast<int>(polygon_resource_->get_triangles().size() / 3);
    const real_t area       = polygon_resource_->get_abs_area();
    const real_t perim      = polygon_resource_->get_perimeter();
    const GeometryWindingOrder w = polygon_resource_->get_winding_order();
    const String w_str = (w == WINDING_COUNTER_CLOCKWISE)
        ? "CCW"
        : ((w == WINDING_CLOCKWISE) ? "CW" : "Collinear");

    telemetry_label_->set_text(
        "  V: " + String::num_int64(n_verts) +
        " | Tris: " + String::num_int64(n_tris) +
        " | Area: " + String::num(static_cast<double>(area), 3) +
        " | Perim: " + String::num(static_cast<double>(perim), 3) +
        " | Winding: " + w_str
    );
}

void Polygon2DControl::_on_closed_toggled(bool p_closed) {
    if (is_updating_ui_ || polygon_resource_.is_null()) return;
    polygon_resource_->commit_set_closed(p_closed);
}

void Polygon2DControl::_on_triangulation_toggled(bool p_show) {
    if (polygon_canvas_) polygon_canvas_->set_show_triangulation(p_show);
}

void Polygon2DControl::_on_centroid_toggled(bool p_show) {
    if (polygon_canvas_) polygon_canvas_->set_show_centroid(p_show);
}

void Polygon2DControl::_on_hull_overlay_toggled(bool p_show) {
    if (polygon_canvas_) polygon_canvas_->set_show_convex_hull(p_show);
}

void Polygon2DControl::_on_csg_workbench_toggled(bool p_show) {
    if (csg_bridge_control_) {
        csg_bridge_control_->set_visible(p_show);
        if (p_show && polygon_resource_.is_valid()) {
            csg_bridge_control_->set_subject_polygon(polygon_resource_);
            csg_bridge_control_->refresh_ui();
        }
    }
}

void Polygon2DControl::_on_ensure_ccw_pressed() {
    if (polygon_resource_.is_valid()) {
        polygon_resource_->commit_ensure_winding(WINDING_COUNTER_CLOCKWISE);
    }
}

void Polygon2DControl::_on_ensure_cw_pressed() {
    if (polygon_resource_.is_valid()) {
        polygon_resource_->commit_ensure_winding(WINDING_CLOCKWISE);
    }
}

void Polygon2DControl::_on_bake_convex_hull_pressed() {
    if (polygon_resource_.is_valid()) {
        polygon_resource_->commit_replace_with_convex_hull();
    }
}

void Polygon2DControl::_on_fit_view_pressed() {
    if (polygon_canvas_) {
        polygon_canvas_->fit_view_to_data(true, true, 0.2f);
    }
}

void Polygon2DControl::_on_add_vertex_pressed() {
    if (polygon_resource_.is_null()) return;
    const int n = polygon_resource_->get_vertex_count();
    godot::Vector2 pt(0.0f, 0.0f);
    if (n >= 2) {
        pt = (polygon_resource_->get_vertex(n - 1) + polygon_resource_->get_vertex(0)) * 0.5f;
    } else if (n == 1) {
        pt = polygon_resource_->get_vertex(0) + godot::Vector2(0.5f, 0.0f);
    }
    polygon_resource_->commit_add_vertex(pt);
    if (polygon_canvas_) {
        polygon_canvas_->set_selected_point_index(polygon_resource_->get_vertex_count() - 1);
    }
    refresh_ui();
}

void Polygon2DControl::_on_remove_selected_vertex_pressed() {
    if (polygon_resource_.is_null() || !polygon_canvas_) return;
    const int sel = polygon_canvas_->get_selected_point_index();
    if (sel >= 0 && sel < polygon_resource_->get_vertex_count()) {
        polygon_resource_->commit_remove_vertex(sel);
        polygon_canvas_->set_selected_point_index(std::min(sel, polygon_resource_->get_vertex_count() - 1));
        refresh_ui();
    }
}

void Polygon2DControl::_on_clear_pressed() {
    if (polygon_resource_.is_null()) return;
    polygon_resource_->commit_clear();
    if (polygon_canvas_) polygon_canvas_->set_selected_point_index(-1);
    refresh_ui();
}

void Polygon2DControl::_on_generate_shape_pressed() {
    if (polygon_resource_.is_null() || !preset_opt_) return;
    const int preset   = preset_opt_->get_item_id(preset_opt_->get_selected());
    const int sides    = sides_spin_ ? static_cast<int>(sides_spin_->get_value()) : 6;
    const real_t r_out = outer_radius_spin_ ? static_cast<real_t>(outer_radius_spin_->get_value()) : static_cast<real_t>(0.8);
    const real_t r_in  = inner_radius_spin_ ? static_cast<real_t>(inner_radius_spin_->get_value()) : static_cast<real_t>(0.4);

    if (preset == POLY_PRESET_RECTANGLE) {
        polygon_resource_->commit_create_rectangle(
            godot::Vector2(0.0f, 0.0f),
            godot::Vector2(static_cast<float>(r_out * 2.0), static_cast<float>(r_in * 2.0))
        );
    } else if (preset == POLY_PRESET_STAR) {
        polygon_resource_->commit_create_star(sides, r_out, r_in);
    } else {
        polygon_resource_->commit_create_regular_polygon(sides, r_out);
    }

    if (polygon_canvas_) {
        polygon_canvas_->fit_view_to_data(true, true, 0.2f);
    }
    refresh_ui();
}

void Polygon2DControl::_on_offset_preview_toggled(bool p_show) {
    if (polygon_canvas_) {
        _sync_offset_preview();
        polygon_canvas_->set_show_offset_preview(p_show);
    }
}

void Polygon2DControl::_on_offset_params_changed(double p_unused) {
    (void)p_unused;
    _sync_offset_preview();
}

void Polygon2DControl::_on_join_style_selected(int p_idx) {
    (void)p_idx;
    _sync_offset_preview();
}

void Polygon2DControl::_on_commit_offset_pressed() {
    if (polygon_resource_.is_null()) return;
    const real_t delta = offset_delta_spin_ ? static_cast<real_t>(offset_delta_spin_->get_value()) : static_cast<real_t>(0.12);
    const GeometryJoinStyle2D join = (join_style_opt_ && join_style_opt_->get_selected() >= 0)
        ? static_cast<GeometryJoinStyle2D>(join_style_opt_->get_item_id(join_style_opt_->get_selected()))
        : JOIN_STYLE_MITER;
    const real_t miter = miter_limit_spin_ ? static_cast<real_t>(miter_limit_spin_->get_value()) : static_cast<real_t>(2.0);
    polygon_resource_->commit_apply_offset(delta, join, miter);
    refresh_ui();
}

void Polygon2DControl::_on_canvas_point_selected(int p_vertex_index) {
    (void)p_vertex_index;
    refresh_ui();
}

void Polygon2DControl::_on_selected_vertex_coords_edited(double p_unused) {
    (void)p_unused;
    if (is_updating_ui_ || polygon_resource_.is_null() || !polygon_canvas_) return;
    const int sel = polygon_canvas_->get_selected_point_index();
    if (sel < 0 || sel >= polygon_resource_->get_vertex_count()) return;
    if (!sel_vertex_x_spin_ || !sel_vertex_y_spin_) return;
    const godot::Vector2 pt(
        static_cast<float>(sel_vertex_x_spin_->get_value()),
        static_cast<float>(sel_vertex_y_spin_->get_value())
    );
    polygon_resource_->commit_set_vertex(sel, pt);
}

} // namespace ideam::godot_ext
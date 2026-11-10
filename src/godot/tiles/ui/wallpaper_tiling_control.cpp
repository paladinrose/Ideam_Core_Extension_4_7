#include "wallpaper_tiling_control.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void WallpaperTilingControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_wallpaper_tiling", "tiling"), &WallpaperTilingControl::set_wallpaper_tiling);
    ClassDB::bind_method(D_METHOD("get_wallpaper_tiling"), &WallpaperTilingControl::get_wallpaper_tiling);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "wallpaper_tiling", PROPERTY_HINT_RESOURCE_TYPE, "WallpaperTilingResource"),
        "set_wallpaper_tiling",
        "get_wallpaper_tiling"
    );

    ClassDB::bind_method(D_METHOD("get_group_selector"), &WallpaperTilingControl::get_group_selector);
    ClassDB::bind_method(D_METHOD("get_domain_canvas"), &WallpaperTilingControl::get_domain_canvas);
    ClassDB::bind_method(D_METHOD("get_edge_spline_canvas"), &WallpaperTilingControl::get_edge_spline_canvas);
    ClassDB::bind_method(D_METHOD("refresh_ui"), &WallpaperTilingControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &WallpaperTilingControl::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_fit_view_pressed"), &WallpaperTilingControl::_on_fit_view_pressed);
    ClassDB::bind_method(D_METHOD("_on_neighborhood_radius_changed", "val"), &WallpaperTilingControl::_on_neighborhood_radius_changed);
    ClassDB::bind_method(D_METHOD("_on_three_color_toggled", "show"), &WallpaperTilingControl::_on_three_color_toggled);
    ClassDB::bind_method(D_METHOD("_on_basis_vectors_toggled", "show"), &WallpaperTilingControl::_on_basis_vectors_toggled);
    ClassDB::bind_method(D_METHOD("_on_symmetry_handles_toggled", "show"), &WallpaperTilingControl::_on_symmetry_handles_toggled);
    ClassDB::bind_method(D_METHOD("_on_edge_badges_toggled", "show"), &WallpaperTilingControl::_on_edge_badges_toggled);
    ClassDB::bind_method(D_METHOD("_on_triangulation_toggled", "show"), &WallpaperTilingControl::_on_triangulation_toggled);

    ClassDB::bind_method(D_METHOD("_on_tactile_param_changed", "val", "param_idx"), &WallpaperTilingControl::_on_tactile_param_changed);
    ClassDB::bind_method(D_METHOD("_on_lattice_transform_edited", "unused"), &WallpaperTilingControl::_on_lattice_transform_edited);
    ClassDB::bind_method(D_METHOD("_on_reset_lattice_pressed"), &WallpaperTilingControl::_on_reset_lattice_pressed);

    ClassDB::bind_method(D_METHOD("_on_canvas_edge_shape_selected", "edge_shape_id"), &WallpaperTilingControl::_on_canvas_edge_shape_selected);
    ClassDB::bind_method(D_METHOD("_on_edge_slot_selected", "idx"), &WallpaperTilingControl::_on_edge_slot_selected);
    ClassDB::bind_method(D_METHOD("_on_edge_preset_straight_pressed"), &WallpaperTilingControl::_on_edge_preset_straight_pressed);
    ClassDB::bind_method(D_METHOD("_on_edge_preset_s_wave_pressed"), &WallpaperTilingControl::_on_edge_preset_s_wave_pressed);
    ClassDB::bind_method(D_METHOD("_on_edge_preset_u_arc_pressed"), &WallpaperTilingControl::_on_edge_preset_u_arc_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_all_edges_pressed"), &WallpaperTilingControl::_on_clear_all_edges_pressed);
    ClassDB::bind_method(D_METHOD("_on_show_edge_canvas_toggled", "show"), &WallpaperTilingControl::_on_show_edge_canvas_toggled);
}

WallpaperTilingControl::WallpaperTilingControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
}

WallpaperTilingControl::~WallpaperTilingControl() {
    if (wallpaper_tiling_.is_valid()) {
        const godot::Callable cb(this, "_on_resource_changed");
        if (wallpaper_tiling_->is_connected("changed", cb)) {
            wallpaper_tiling_->disconnect("changed", cb);
        }
    }
}

void WallpaperTilingControl::_build_ui() {
    using namespace godot;

    // 1. Top Crystallographic & Tactile Group Selector
    group_selector_ = memnew(WallpaperGroupSelectorControl);
    add_child(group_selector_);

    // 2. Canvas Viewport Toolbar
    HBoxContainer *canvas_bar = memnew(HBoxContainer);
    canvas_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(canvas_bar);

    Button *fit_btn = memnew(Button);
    fit_btn->set_text("Fit View");
    fit_btn->connect("pressed", Callable(this, "_on_fit_view_pressed"));
    canvas_bar->add_child(fit_btn);

    Label *rad_lbl = memnew(Label);
    rad_lbl->set_text("Rings:");
    canvas_bar->add_child(rad_lbl);

    neighborhood_radius_spin_ = memnew(SpinBox);
    neighborhood_radius_spin_->set_min(0.0);
    neighborhood_radius_spin_->set_max(3.0);
    neighborhood_radius_spin_->set_step(1.0);
    neighborhood_radius_spin_->set_value(1.0);
    neighborhood_radius_spin_->connect("value_changed", Callable(this, "_on_neighborhood_radius_changed"));
    canvas_bar->add_child(neighborhood_radius_spin_);

    three_color_check_ = memnew(CheckBox);
    three_color_check_->set_text("3-Color");
    three_color_check_->set_pressed(true);
    three_color_check_->connect("toggled", Callable(this, "_on_three_color_toggled"));
    canvas_bar->add_child(three_color_check_);

    basis_vectors_check_ = memnew(CheckBox);
    basis_vectors_check_->set_text("Basis T1/T2");
    basis_vectors_check_->set_pressed(true);
    basis_vectors_check_->connect("toggled", Callable(this, "_on_basis_vectors_toggled"));
    canvas_bar->add_child(basis_vectors_check_);

    symmetry_handles_check_ = memnew(CheckBox);
    symmetry_handles_check_->set_text("Symmetry");
    symmetry_handles_check_->set_pressed(true);
    symmetry_handles_check_->connect("toggled", Callable(this, "_on_symmetry_handles_toggled"));
    canvas_bar->add_child(symmetry_handles_check_);

    edge_badges_check_ = memnew(CheckBox);
    edge_badges_check_->set_text("J/U/S/I");
    edge_badges_check_->set_pressed(true);
    edge_badges_check_->connect("toggled", Callable(this, "_on_edge_badges_toggled"));
    canvas_bar->add_child(edge_badges_check_);

    triangulation_check_ = memnew(CheckBox);
    triangulation_check_->set_text("Tris");
    triangulation_check_->set_pressed(false);
    triangulation_check_->connect("toggled", Callable(this, "_on_triangulation_toggled"));
    canvas_bar->add_child(triangulation_check_);

    // 3. Interactive 2D Fundamental Domain Canvas
    domain_canvas_ = memnew(TileFundamentalDomainCanvas);
    domain_canvas_->connect("edge_shape_selected", Callable(this, "_on_canvas_edge_shape_selected"));
    add_child(domain_canvas_);

    // 4. Dynamic Tactile Vertex Parameters Section (`0..6` parameters)
    tactile_params_section_ = memnew(VBoxContainer);
    tactile_params_section_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(tactile_params_section_);

    Label *params_hdr = memnew(Label);
    params_hdr->set_text("Tactile Vertex Parameters (P0..P5):");
    tactile_params_section_->add_child(params_hdr);

    for (size_t i = 0; i < MAX_TACTILE_PARAMS; ++i) {
        HBoxContainer *row = memnew(HBoxContainer);
        row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        tactile_params_section_->add_child(row);

        Label *lbl = memnew(Label);
        lbl->set_text("P" + String::num_int64(static_cast<int64_t>(i)) + ":");
        lbl->set_custom_minimum_size(Vector2(28.0f, 0.0f));
        row->add_child(lbl);

        HSlider *slider = memnew(HSlider);
        slider->set_min(0.0);
        slider->set_max(1.0);
        slider->set_step(0.005);
        slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        slider->connect("value_changed", Callable(this, "_on_tactile_param_changed").bind(static_cast<int>(i)));
        row->add_child(slider);

        SpinBox *spin = memnew(SpinBox);
        spin->set_min(0.0);
        spin->set_max(1.0);
        spin->set_step(0.005);
        spin->connect("value_changed", Callable(this, "_on_tactile_param_changed").bind(static_cast<int>(i)));
        row->add_child(spin);

        param_rows_[i]    = row;
        param_sliders_[i] = slider;
        param_spins_[i]   = spin;
    }

    // 5. Lattice Basis Transform Row
    HBoxContainer *lattice_row = memnew(HBoxContainer);
    lattice_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(lattice_row);

    Label *sc_lbl = memnew(Label);
    sc_lbl->set_text("Scale:");
    lattice_row->add_child(sc_lbl);

    scale_x_spin_ = memnew(SpinBox);
    scale_x_spin_->set_min(0.05);
    scale_x_spin_->set_max(100.0);
    scale_x_spin_->set_step(0.05);
    scale_x_spin_->set_value(1.0);
    scale_x_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    scale_x_spin_->connect("value_changed", Callable(this, "_on_lattice_transform_edited"));
    lattice_row->add_child(scale_x_spin_);

    scale_y_spin_ = memnew(SpinBox);
    scale_y_spin_->set_min(0.05);
    scale_y_spin_->set_max(100.0);
    scale_y_spin_->set_step(0.05);
    scale_y_spin_->set_value(1.0);
    scale_y_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    scale_y_spin_->connect("value_changed", Callable(this, "_on_lattice_transform_edited"));
    lattice_row->add_child(scale_y_spin_);

    Label *rot_lbl = memnew(Label);
    rot_lbl->set_text("Rot°:");
    lattice_row->add_child(rot_lbl);

    rotation_deg_spin_ = memnew(SpinBox);
    rotation_deg_spin_->set_min(-360.0);
    rotation_deg_spin_->set_max(360.0);
    rotation_deg_spin_->set_step(1.0);
    rotation_deg_spin_->set_value(0.0);
    rotation_deg_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    rotation_deg_spin_->connect("value_changed", Callable(this, "_on_lattice_transform_edited"));
    lattice_row->add_child(rotation_deg_spin_);

    Label *orig_lbl = memnew(Label);
    orig_lbl->set_text("Origin:");
    lattice_row->add_child(orig_lbl);

    origin_x_spin_ = memnew(SpinBox);
    origin_x_spin_->set_min(-1000.0);
    origin_x_spin_->set_max(1000.0);
    origin_x_spin_->set_step(0.1);
    origin_x_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    origin_x_spin_->connect("value_changed", Callable(this, "_on_lattice_transform_edited"));
    lattice_row->add_child(origin_x_spin_);

    origin_y_spin_ = memnew(SpinBox);
    origin_y_spin_->set_min(-1000.0);
    origin_y_spin_->set_max(1000.0);
    origin_y_spin_->set_step(0.1);
    origin_y_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    origin_y_spin_->connect("value_changed", Callable(this, "_on_lattice_transform_edited"));
    lattice_row->add_child(origin_y_spin_);

    Button *reset_lat_btn = memnew(Button);
    reset_lat_btn->set_text("Reset Basis");
    reset_lat_btn->connect("pressed", Callable(this, "_on_reset_lattice_pressed"));
    lattice_row->add_child(reset_lat_btn);

    // 6. Escher Edge Splines (`IdeamSplines` Lateral Handshake) Bar + Embedded Spline2DCanvas
    HBoxContainer *edge_bar = memnew(HBoxContainer);
    edge_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(edge_bar);

    Label *edge_lbl = memnew(Label);
    edge_lbl->set_text("Escher Edge:");
    edge_bar->add_child(edge_lbl);

    edge_slot_opt_ = memnew(OptionButton);
    edge_slot_opt_->connect("item_selected", Callable(this, "_on_edge_slot_selected"));
    edge_bar->add_child(edge_slot_opt_);

    edge_constraint_label_ = memnew(Label);
    edge_constraint_label_->set_text("Rule: J");
    edge_bar->add_child(edge_constraint_label_);

    Button *straight_btn = memnew(Button);
    straight_btn->set_text("Straight (I)");
    straight_btn->connect("pressed", Callable(this, "_on_edge_preset_straight_pressed"));
    edge_bar->add_child(straight_btn);

    Button *swave_btn = memnew(Button);
    swave_btn->set_text("S-Wave");
    swave_btn->connect("pressed", Callable(this, "_on_edge_preset_s_wave_pressed"));
    edge_bar->add_child(swave_btn);

    Button *uarc_btn = memnew(Button);
    uarc_btn->set_text("U-Arc");
    uarc_btn->connect("pressed", Callable(this, "_on_edge_preset_u_arc_pressed"));
    edge_bar->add_child(uarc_btn);

    Button *clear_edges_btn = memnew(Button);
    clear_edges_btn->set_text("Clear All");
    clear_edges_btn->connect("pressed", Callable(this, "_on_clear_all_edges_pressed"));
    edge_bar->add_child(clear_edges_btn);

    show_edge_canvas_check_ = memnew(CheckBox);
    show_edge_canvas_check_->set_text("Edit Curve");
    show_edge_canvas_check_->set_pressed(false);
    show_edge_canvas_check_->connect("toggled", Callable(this, "_on_show_edge_canvas_toggled"));
    edge_bar->add_child(show_edge_canvas_check_);

    edge_spline_canvas_ = memnew(Spline2DCanvas);
    edge_spline_canvas_->set_custom_minimum_size(Vector2(0.0f, 175.0f));
    edge_spline_canvas_->set_show_polygon_fill(false);
    edge_spline_canvas_->set_visible(false);
    add_child(edge_spline_canvas_);
}

void WallpaperTilingControl::set_wallpaper_tiling(const godot::Ref<WallpaperTilingResource> &p_tiling) {
    if (wallpaper_tiling_ == p_tiling) return;

    const godot::Callable cb(this, "_on_resource_changed");
    if (wallpaper_tiling_.is_valid() && wallpaper_tiling_->is_connected("changed", cb)) {
        wallpaper_tiling_->disconnect("changed", cb);
    }

    wallpaper_tiling_ = p_tiling;

    if (wallpaper_tiling_.is_valid() && !wallpaper_tiling_->is_connected("changed", cb)) {
        wallpaper_tiling_->connect("changed", cb);
    }

    if (group_selector_) {
        group_selector_->set_wallpaper_tiling(wallpaper_tiling_);
    }
    if (domain_canvas_) {
        domain_canvas_->set_wallpaper_tiling(wallpaper_tiling_);
    }

    refresh_ui();
}

void WallpaperTilingControl::_rebuild_edge_slot_dropdown() {
    if (!edge_slot_opt_) return;
    edge_slot_opt_->clear();

    if (wallpaper_tiling_.is_null()) return;
    const int edge_count = wallpaper_tiling_->get_edge_shape_count();
    const int active_id  = domain_canvas_ ? domain_canvas_->get_selected_edge_shape_id() : 0;
    int sel_idx          = 0;

    for (int i = 0; i < edge_count; ++i) {
        const TilingEdgeShapeType rule = wallpaper_tiling_->get_edge_shape_constraint(i);
        godot::String rule_tag = "J";
        if (rule == EDGE_SHAPE_U) rule_tag = "U";
        else if (rule == EDGE_SHAPE_S) rule_tag = "S";
        else if (rule == EDGE_SHAPE_I) rule_tag = "I";

        edge_slot_opt_->add_item("Edge Shape #" + godot::String::num_int64(i) + " [" + rule_tag + "]", i);
        if (i == active_id) {
            sel_idx = i;
        }
    }

    if (edge_slot_opt_->get_item_count() > 0) {
        edge_slot_opt_->select(sel_idx);
    }
}

void WallpaperTilingControl::_sync_active_edge_spline_canvas() {
    if (!edge_spline_canvas_ || wallpaper_tiling_.is_null()) return;
    const int edge_id = domain_canvas_ ? domain_canvas_->get_selected_edge_shape_id() : 0;
    const TilingEdgeShapeType rule = wallpaper_tiling_->get_edge_shape_constraint(edge_id);

    switch (rule) {
        case EDGE_SHAPE_J: edge_constraint_label_->set_text("Rule: J (Free Asymmetric)"); break;
        case EDGE_SHAPE_U: edge_constraint_label_->set_text("Rule: U (Mirror Midpoint)"); break;
        case EDGE_SHAPE_S: edge_constraint_label_->set_text("Rule: S (180° Centrosymmetric)"); break;
        case EDGE_SHAPE_I: edge_constraint_label_->set_text("Rule: I (Rigid Straight)"); break;
    }

    godot::Ref<Spline2DResource> sp = wallpaper_tiling_->get_edge_spline(edge_id);
    edge_spline_canvas_->set_spline(sp);
}

void WallpaperTilingControl::refresh_ui() {
    if (is_updating_ui_) return;
    is_updating_ui_ = true;

    if (group_selector_) {
        group_selector_->refresh_ui();
    }

    if (wallpaper_tiling_.is_valid()) {
        // 1. Update Tactile Parameter Rows
        const int param_count = wallpaper_tiling_->get_parameter_count();
        if (tactile_params_section_) {
            tactile_params_section_->set_visible(param_count > 0);
        }
        for (size_t i = 0; i < MAX_TACTILE_PARAMS; ++i) {
            const bool vis = static_cast<int>(i) < param_count;
            if (param_rows_[i]) {
                param_rows_[i]->set_visible(vis);
            }
            if (vis) {
                const double val = static_cast<double>(wallpaper_tiling_->get_tactile_parameter(static_cast<int>(i)));
                if (param_sliders_[i]) param_sliders_[i]->set_value_no_signal(val);
                if (param_spins_[i]) param_spins_[i]->set_value_no_signal(val);
            }
        }

        // 2. Update Lattice Transform SpinBoxes
        const godot::Vector2 sc = wallpaper_tiling_->get_lattice_scale();
        const double rot_deg    = godot::Math::rad_to_deg(static_cast<double>(wallpaper_tiling_->get_lattice_rotation_radians()));
        const godot::Vector2 orig = wallpaper_tiling_->get_lattice_origin();

        if (scale_x_spin_) scale_x_spin_->set_value_no_signal(sc.x);
        if (scale_y_spin_) scale_y_spin_->set_value_no_signal(sc.y);
        if (rotation_deg_spin_) rotation_deg_spin_->set_value_no_signal(rot_deg);
        if (origin_x_spin_) origin_x_spin_->set_value_no_signal(orig.x);
        if (origin_y_spin_) origin_y_spin_->set_value_no_signal(orig.y);

        // 3. Update Escher Edge Spline Selector & Canvas
        _rebuild_edge_slot_dropdown();
        _sync_active_edge_spline_canvas();
    }

    if (domain_canvas_) {
        domain_canvas_->queue_redraw();
    }

    is_updating_ui_ = false;
}

void WallpaperTilingControl::_on_resource_changed() {
    refresh_ui();
}

void WallpaperTilingControl::_on_fit_view_pressed() {
    if (domain_canvas_) {
        domain_canvas_->fit_view_to_data(true, true, static_cast<real_t>(0.25));
    }
}

void WallpaperTilingControl::_on_neighborhood_radius_changed(double p_val) {
    if (domain_canvas_) {
        domain_canvas_->set_neighborhood_radius(static_cast<int>(p_val));
    }
}

void WallpaperTilingControl::_on_three_color_toggled(bool p_show) {
    if (domain_canvas_) domain_canvas_->set_show_three_coloring(p_show);
}

void WallpaperTilingControl::_on_basis_vectors_toggled(bool p_show) {
    if (domain_canvas_) domain_canvas_->set_show_lattice_vectors(p_show);
}

void WallpaperTilingControl::_on_symmetry_handles_toggled(bool p_show) {
    if (domain_canvas_) domain_canvas_->set_show_symmetry_elements(p_show);
}

void WallpaperTilingControl::_on_edge_badges_toggled(bool p_show) {
    if (domain_canvas_) domain_canvas_->set_show_edge_constraints(p_show);
}

void WallpaperTilingControl::_on_triangulation_toggled(bool p_show) {
    if (domain_canvas_) domain_canvas_->set_show_triangulation(p_show);
}

void WallpaperTilingControl::_on_tactile_param_changed(double p_val, int p_param_idx) {
    if (is_updating_ui_ || wallpaper_tiling_.is_null()) return;
    wallpaper_tiling_->commit_set_tactile_parameter(p_param_idx, static_cast<real_t>(p_val));
    refresh_ui();
}

void WallpaperTilingControl::_on_lattice_transform_edited(double p_unused) {
    (void)p_unused;
    if (is_updating_ui_ || wallpaper_tiling_.is_null()) return;

    const godot::Vector2 new_sc(
        static_cast<float>(scale_x_spin_->get_value()),
        static_cast<float>(scale_y_spin_->get_value())
    );
    const auto new_rot = static_cast<real_t>(godot::Math::deg_to_rad(rotation_deg_spin_->get_value()));
    const godot::Vector2 new_orig(
        static_cast<float>(origin_x_spin_->get_value()),
        static_cast<float>(origin_y_spin_->get_value())
    );

    if (new_sc != wallpaper_tiling_->get_lattice_scale()) {
        wallpaper_tiling_->commit_set_lattice_scale(new_sc);
    }
    if (std::abs(new_rot - wallpaper_tiling_->get_lattice_rotation_radians()) > 1e-4) {
        wallpaper_tiling_->commit_set_lattice_rotation_radians(new_rot);
    }
    if (new_orig != wallpaper_tiling_->get_lattice_origin()) {
        wallpaper_tiling_->commit_set_lattice_origin(new_orig);
    }
}

void WallpaperTilingControl::_on_reset_lattice_pressed() {
    if (wallpaper_tiling_.is_null()) return;
    wallpaper_tiling_->commit_set_lattice_scale(godot::Vector2(1.0f, 1.0f));
    wallpaper_tiling_->commit_set_lattice_rotation_radians(static_cast<real_t>(0.0));
    wallpaper_tiling_->commit_set_lattice_origin(godot::Vector2(0.0f, 0.0f));
    refresh_ui();
}

void WallpaperTilingControl::_on_canvas_edge_shape_selected(int p_edge_shape_id) {
    if (is_updating_ui_) return;
    is_updating_ui_ = true;
    if (edge_slot_opt_) {
        const int idx = edge_slot_opt_->get_item_index(p_edge_shape_id);
        if (idx >= 0) edge_slot_opt_->select(idx);
    }
    _sync_active_edge_spline_canvas();
    is_updating_ui_ = false;
}

void WallpaperTilingControl::_on_edge_slot_selected(int p_idx) {
    if (is_updating_ui_ || !edge_slot_opt_) return;
    const int edge_id = edge_slot_opt_->get_item_id(p_idx);
    if (domain_canvas_) {
        domain_canvas_->set_selected_edge_shape_id(edge_id);
    }
    _sync_active_edge_spline_canvas();
}

void WallpaperTilingControl::_on_edge_preset_straight_pressed() {
    if (wallpaper_tiling_.is_null()) return;
    const int edge_id = domain_canvas_ ? domain_canvas_->get_selected_edge_shape_id() : 0;
    wallpaper_tiling_->commit_set_edge_spline(edge_id, godot::Ref<Spline2DResource>());
    refresh_ui();
}

void WallpaperTilingControl::_on_edge_preset_s_wave_pressed() {
    if (wallpaper_tiling_.is_null()) return;
    const int edge_id = domain_canvas_ ? domain_canvas_->get_selected_edge_shape_id() : 0;

    godot::Ref<Spline2DResource> sp;
    sp.instantiate();
    sp->set_undo_redo(wallpaper_tiling_->get_undo_redo());
    sp->set_wrap(false);
    sp->add_point(godot::Vector2(0.0f, 0.0f));
    sp->add_point(godot::Vector2(0.25f, 0.14f));
    sp->add_point(godot::Vector2(0.50f, 0.0f));
    sp->add_point(godot::Vector2(0.75f, -0.14f));
    sp->add_point(godot::Vector2(1.0f, 0.0f));

    wallpaper_tiling_->commit_set_edge_spline(edge_id, sp);
    if (show_edge_canvas_check_ && !show_edge_canvas_check_->is_pressed()) {
        show_edge_canvas_check_->set_pressed(true);
    }
    refresh_ui();
}

void WallpaperTilingControl::_on_edge_preset_u_arc_pressed() {
    if (wallpaper_tiling_.is_null()) return;
    const int edge_id = domain_canvas_ ? domain_canvas_->get_selected_edge_shape_id() : 0;

    godot::Ref<Spline2DResource> sp;
    sp.instantiate();
    sp->set_undo_redo(wallpaper_tiling_->get_undo_redo());
    sp->set_wrap(false);
    sp->add_point(godot::Vector2(0.0f, 0.0f));
    sp->add_point(godot::Vector2(0.50f, 0.16f));
    sp->add_point(godot::Vector2(1.0f, 0.0f));

    wallpaper_tiling_->commit_set_edge_spline(edge_id, sp);
    if (show_edge_canvas_check_ && !show_edge_canvas_check_->is_pressed()) {
        show_edge_canvas_check_->set_pressed(true);
    }
    refresh_ui();
}

void WallpaperTilingControl::_on_clear_all_edges_pressed() {
    if (wallpaper_tiling_.is_null()) return;
    const godot::Dictionary before = wallpaper_tiling_->get_snapshot_state();
    wallpaper_tiling_->clear_edge_splines();
    const godot::Dictionary after  = wallpaper_tiling_->get_snapshot_state();
    wallpaper_tiling_->commit_snapshot_action("Clear All Escher Edge Splines", before, after);
    refresh_ui();
}

void WallpaperTilingControl::_on_show_edge_canvas_toggled(bool p_show) {
    if (!edge_spline_canvas_) return;
    edge_spline_canvas_->set_visible(p_show);
    if (p_show && wallpaper_tiling_.is_valid()) {
        const int edge_id = domain_canvas_ ? domain_canvas_->get_selected_edge_shape_id() : 0;
        godot::Ref<Spline2DResource> sp = wallpaper_tiling_->get_edge_spline(edge_id);
        if (sp.is_null()) {
            sp.instantiate();
            sp->set_undo_redo(wallpaper_tiling_->get_undo_redo());
            sp->set_wrap(false);
            sp->add_point(godot::Vector2(0.0f, 0.0f));
            sp->add_point(godot::Vector2(0.5f, 0.0f));
            sp->add_point(godot::Vector2(1.0f, 0.0f));
            wallpaper_tiling_->commit_set_edge_spline(edge_id, sp);
        }
        _sync_active_edge_spline_canvas();
        edge_spline_canvas_->fit_view_to_data(true, true, static_cast<real_t>(0.25));
    }
}

} // namespace ideam::godot_ext

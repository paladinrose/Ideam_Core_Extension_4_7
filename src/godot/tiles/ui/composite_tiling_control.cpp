#include "composite_tiling_control.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void CompositeTilingControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_composite_tiling", "composite"), &CompositeTilingControl::set_composite_tiling);
    ClassDB::bind_method(D_METHOD("get_composite_tiling"), &CompositeTilingControl::get_composite_tiling);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "composite_tiling", PROPERTY_HINT_RESOURCE_TYPE, "CompositeTilingResource"),
        "set_composite_tiling",
        "get_composite_tiling"
    );

    ClassDB::bind_method(D_METHOD("get_region_canvas"), &CompositeTilingControl::get_region_canvas);
    ClassDB::bind_method(D_METHOD("refresh_ui"), &CompositeTilingControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &CompositeTilingControl::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_rebuild_pressed"), &CompositeTilingControl::_on_rebuild_pressed);
    ClassDB::bind_method(D_METHOD("_on_fit_view_pressed"), &CompositeTilingControl::_on_fit_view_pressed);
    ClassDB::bind_method(D_METHOD("_on_coverage_mode_selected", "idx"), &CompositeTilingControl::_on_coverage_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_stitch_mode_selected", "idx"), &CompositeTilingControl::_on_stitch_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_bridge_span_changed", "val"), &CompositeTilingControl::_on_bridge_span_changed);
    ClassDB::bind_method(D_METHOD("_on_edge_subdivs_changed", "val"), &CompositeTilingControl::_on_edge_subdivs_changed);
    ClassDB::bind_method(D_METHOD("_on_auto_hexize_toggled", "enabled"), &CompositeTilingControl::_on_auto_hexize_toggled);
    ClassDB::bind_method(D_METHOD("_on_auto_rebuild_toggled", "enabled"), &CompositeTilingControl::_on_auto_rebuild_toggled);

    ClassDB::bind_method(D_METHOD("_on_color_mode_selected", "idx"), &CompositeTilingControl::_on_color_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_interact_mode_selected", "idx"), &CompositeTilingControl::_on_interact_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_hex_overlay_toggled", "show"), &CompositeTilingControl::_on_hex_overlay_toggled);
    ClassDB::bind_method(D_METHOD("_on_border_overlay_toggled", "show"), &CompositeTilingControl::_on_border_overlay_toggled);
    ClassDB::bind_method(D_METHOD("_on_flow_field_toggled", "show"), &CompositeTilingControl::_on_flow_field_toggled);
    ClassDB::bind_method(D_METHOD("_on_manhattan_toggled", "manhattan"), &CompositeTilingControl::_on_manhattan_toggled);

    ClassDB::bind_method(D_METHOD("_on_add_layer_pressed"), &CompositeTilingControl::_on_add_layer_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_boundary_spline_pressed"), &CompositeTilingControl::_on_add_boundary_spline_pressed);
    ClassDB::bind_method(D_METHOD("_on_canvas_cell_selected", "cell_id"), &CompositeTilingControl::_on_canvas_cell_selected);
    ClassDB::bind_method(D_METHOD("_on_canvas_path_updated", "start_cell", "goal_cell", "path_len"), &CompositeTilingControl::_on_canvas_path_updated);
}

CompositeTilingControl::CompositeTilingControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
}

CompositeTilingControl::~CompositeTilingControl() {
    if (composite_tiling_.is_valid()) {
        const godot::Callable cb(this, "_on_resource_changed");
        if (composite_tiling_->is_connected("changed", cb)) {
            composite_tiling_->disconnect("changed", cb);
        }
    }
}

void CompositeTilingControl::_build_ui() {
    using namespace godot;

    // Row 1: Coverage Mode + Seam Stitch Mode + Rebuild Controls
    HBoxContainer *top_row = memnew(HBoxContainer);
    top_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(top_row);

    Label *cov_lbl = memnew(Label);
    cov_lbl->set_text("Coverage:");
    top_row->add_child(cov_lbl);

    coverage_mode_opt_ = memnew(OptionButton);
    coverage_mode_opt_->add_item("Strict Interior", COVERAGE_STRICT_INTERIOR);
    coverage_mode_opt_->add_item("Centroid Inside", COVERAGE_CENTROID_INSIDE);
    coverage_mode_opt_->add_item("Include Straddling", COVERAGE_INCLUDE_STRADDLING);
    coverage_mode_opt_->add_item("Clip Exact to Spline", COVERAGE_CLIP_EXACT_TO_SPLINE);
    coverage_mode_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    coverage_mode_opt_->connect("item_selected", Callable(this, "_on_coverage_mode_selected"));
    top_row->add_child(coverage_mode_opt_);

    Label *st_lbl = memnew(Label);
    st_lbl->set_text("Seam Stitch:");
    top_row->add_child(st_lbl);

    stitch_mode_opt_ = memnew(OptionButton);
    stitch_mode_opt_->add_item("Quad Bridge Cells", STITCH_QUAD_BRIDGE);
    stitch_mode_opt_->add_item("Topological Seam Weld", STITCH_TOPOLOGICAL_SEAM_WELD);
    stitch_mode_opt_->add_item("Curvilinear Metric Morph", STITCH_CURVILINEAR_METRIC_MORPH);
    stitch_mode_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    stitch_mode_opt_->connect("item_selected", Callable(this, "_on_stitch_mode_selected"));
    top_row->add_child(stitch_mode_opt_);

    Button *rebuild_btn = memnew(Button);
    rebuild_btn->set_text("Rebuild");
    rebuild_btn->connect("pressed", Callable(this, "_on_rebuild_pressed"));
    top_row->add_child(rebuild_btn);

    // Row 2: Bridge Span, Edge Subdivisions, Auto-Hexize, Auto-Rebuild
    HBoxContainer *param_row = memnew(HBoxContainer);
    param_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(param_row);

    Label *span_lbl = memnew(Label);
    span_lbl->set_text("Bridge Span:");
    param_row->add_child(span_lbl);

    bridge_span_spin_ = memnew(SpinBox);
    bridge_span_spin_->set_min(0.0);
    bridge_span_spin_->set_max(50.0);
    bridge_span_spin_->set_step(0.05);
    bridge_span_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bridge_span_spin_->connect("value_changed", Callable(this, "_on_bridge_span_changed"));
    param_row->add_child(bridge_span_spin_);

    Label *sub_lbl = memnew(Label);
    sub_lbl->set_text("Edge Subdivs:");
    param_row->add_child(sub_lbl);

    edge_subdivs_spin_ = memnew(SpinBox);
    edge_subdivs_spin_->set_min(1.0);
    edge_subdivs_spin_->set_max(64.0);
    edge_subdivs_spin_->set_step(1.0);
    edge_subdivs_spin_->set_value(12.0);
    edge_subdivs_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    edge_subdivs_spin_->connect("value_changed", Callable(this, "_on_edge_subdivs_changed"));
    param_row->add_child(edge_subdivs_spin_);

    auto_hexize_check_ = memnew(CheckBox);
    auto_hexize_check_->set_text("Auto Hexize");
    auto_hexize_check_->set_pressed(true);
    auto_hexize_check_->connect("toggled", Callable(this, "_on_auto_hexize_toggled"));
    param_row->add_child(auto_hexize_check_);

    auto_rebuild_check_ = memnew(CheckBox);
    auto_rebuild_check_->set_text("Auto Rebuild");
    auto_rebuild_check_->set_pressed(true);
    auto_rebuild_check_->connect("toggled", Callable(this, "_on_auto_rebuild_toggled"));
    param_row->add_child(auto_rebuild_check_);

    // Row 3: Viewport Color Mode, Sandbox Interaction Mode & Overlay Toggles
    HBoxContainer *view_row = memnew(HBoxContainer);
    view_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(view_row);

    Button *fit_btn = memnew(Button);
    fit_btn->set_text("Fit");
    fit_btn->connect("pressed", Callable(this, "_on_fit_view_pressed"));
    view_row->add_child(fit_btn);

    color_mode_opt_ = memnew(OptionButton);
    color_mode_opt_->add_item("Color: 3-Coloring", PREVIEW_COLOR_THREE_COLORING);
    color_mode_opt_->add_item("Color: Layer Group", PREVIEW_COLOR_LAYER_GROUP);
    color_mode_opt_->add_item("Color: Generation Ring", PREVIEW_COLOR_GENERATION_RING);
    color_mode_opt_->add_item("Color: Cell Flags", PREVIEW_COLOR_CELL_FLAGS);
    color_mode_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    color_mode_opt_->connect("item_selected", Callable(this, "_on_color_mode_selected"));
    view_row->add_child(color_mode_opt_);

    interact_mode_opt_ = memnew(OptionButton);
    interact_mode_opt_->add_item("Tool: Inspect Cell", REGION_INTERACT_INSPECT);
    interact_mode_opt_->add_item("Tool: Carve/Remove Cell", REGION_INTERACT_CARVE_CELL);
    interact_mode_opt_->add_item("Tool: A* Path Sandbox", REGION_INTERACT_PATHFIND_SANDBOX);
    interact_mode_opt_->add_item("Tool: Cycle Edge Port", REGION_INTERACT_CYCLE_EDGE_PORT);
    interact_mode_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    interact_mode_opt_->connect("item_selected", Callable(this, "_on_interact_mode_selected"));
    view_row->add_child(interact_mode_opt_);

    hex_overlay_check_ = memnew(CheckBox);
    hex_overlay_check_->set_text("Hexize");
    hex_overlay_check_->set_pressed(true);
    hex_overlay_check_->connect("toggled", Callable(this, "_on_hex_overlay_toggled"));
    view_row->add_child(hex_overlay_check_);

    border_overlay_check_ = memnew(CheckBox);
    border_overlay_check_->set_text("Borders");
    border_overlay_check_->set_pressed(true);
    border_overlay_check_->connect("toggled", Callable(this, "_on_border_overlay_toggled"));
    view_row->add_child(border_overlay_check_);

    flow_field_check_ = memnew(CheckBox);
    flow_field_check_->set_text("Flow");
    flow_field_check_->set_pressed(false);
    flow_field_check_->connect("toggled", Callable(this, "_on_flow_field_toggled"));
    view_row->add_child(flow_field_check_);

    manhattan_check_ = memnew(CheckBox);
    manhattan_check_->set_text("Manhattan");
    manhattan_check_->set_pressed(false);
    manhattan_check_->connect("toggled", Callable(this, "_on_manhattan_toggled"));
    view_row->add_child(manhattan_check_);

    // Row 4: Interactive 2D Region Preview Canvas
    region_canvas_ = memnew(TilingRegionPreviewCanvas);
    region_canvas_->connect("cell_selected", Callable(this, "_on_canvas_cell_selected"));
    region_canvas_->connect("path_updated", Callable(this, "_on_canvas_path_updated"));
    add_child(region_canvas_);

    // Row 5: Quick Layer / Boundary Builders
    HBoxContainer *builder_row = memnew(HBoxContainer);
    builder_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(builder_row);

    layers_summary_label_ = memnew(Label);
    layers_summary_label_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    builder_row->add_child(layers_summary_label_);

    Button *add_layer_btn = memnew(Button);
    add_layer_btn->set_text("+ Add Wallpaper Layer");
    add_layer_btn->connect("pressed", Callable(this, "_on_add_layer_pressed"));
    builder_row->add_child(add_layer_btn);

    Button *add_spline_btn = memnew(Button);
    add_spline_btn->set_text("+ Add Boundary Spline2D");
    add_spline_btn->connect("pressed", Callable(this, "_on_add_boundary_spline_pressed"));
    builder_row->add_child(add_spline_btn);

    // Row 6: Selected Cell & Navigation Telemetry Footer
    cell_inspector_label_ = memnew(Label);
    cell_inspector_label_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    cell_inspector_label_->set_text("Click any cell in the canvas to inspect its topological coordinates, flags, and neighbors.");
    add_child(cell_inspector_label_);
}

void CompositeTilingControl::set_composite_tiling(const godot::Ref<CompositeTilingResource> &p_composite) {
    if (composite_tiling_ == p_composite) return;

    const godot::Callable cb(this, "_on_resource_changed");
    if (composite_tiling_.is_valid() && composite_tiling_->is_connected("changed", cb)) {
        composite_tiling_->disconnect("changed", cb);
    }

    composite_tiling_ = p_composite;

    if (composite_tiling_.is_valid() && !composite_tiling_->is_connected("changed", cb)) {
        composite_tiling_->connect("changed", cb);
    }

    if (region_canvas_) {
        region_canvas_->set_composite_tiling(composite_tiling_);
    }

    refresh_ui();
}

void CompositeTilingControl::_update_cell_inspector_label(int p_cell_id) {
    if (!cell_inspector_label_ || composite_tiling_.is_null()) return;

    const int n_cells = composite_tiling_->get_cell_count();
    if (p_cell_id < 0 || p_cell_id >= n_cells) {
        cell_inspector_label_->set_text(
            "Cells: " + godot::String::num_int64(n_cells) +
            "  |  Click a cell to inspect topology, or switch to 'A* Path Sandbox' (Left-Click = Start, Right/Shift-Click = Goal)."
        );
        return;
    }

    const godot::Vector2 ctr = composite_tiling_->get_cell_center(p_cell_id);
    const int grp            = composite_tiling_->get_cell_group_slot(p_cell_id);
    const int col            = composite_tiling_->get_cell_colour(p_cell_id);
    const int gen            = composite_tiling_->get_cell_generation(p_cell_id);
    const int flags          = composite_tiling_->get_cell_flags(p_cell_id);
    const int deg            = composite_tiling_->get_cell_neighbors(p_cell_id).size();

    cell_inspector_label_->set_text(
        "Cell #" + godot::String::num_int64(p_cell_id) +
        "  |  Center: (" + godot::String::num(ctr.x, 2) + ", " + godot::String::num(ctr.y, 2) + ")" +
        "  |  Layer: " + godot::String::num_int64(grp) +
        "  |  3-Color: " + godot::String::num_int64(col) +
        "  |  Gen Ring: " + godot::String::num_int64(gen) +
        "  |  Degree: " + godot::String::num_int64(deg) +
        "  |  Flags: 0x" + godot::String::num_int64(flags, 16)
    );
}

void CompositeTilingControl::refresh_ui() {
    if (is_updating_ui_) return;
    is_updating_ui_ = true;

    if (composite_tiling_.is_valid()) {
        const int cov_idx = coverage_mode_opt_->get_item_index(static_cast<int>(composite_tiling_->get_coverage_mode()));
        if (cov_idx >= 0) coverage_mode_opt_->select(cov_idx);

        const int st_idx = stitch_mode_opt_->get_item_index(static_cast<int>(composite_tiling_->get_stitch_mode()));
        if (st_idx >= 0) stitch_mode_opt_->select(st_idx);

        if (bridge_span_spin_) bridge_span_spin_->set_value_no_signal(composite_tiling_->get_max_bridge_span());
        if (edge_subdivs_spin_) edge_subdivs_spin_->set_value_no_signal(composite_tiling_->get_edge_subdivisions());
        if (auto_hexize_check_) auto_hexize_check_->set_pressed_no_signal(composite_tiling_->get_auto_hexize());
        if (auto_rebuild_check_) auto_rebuild_check_->set_pressed_no_signal(composite_tiling_->get_auto_rebuild());

        if (layers_summary_label_) {
            layers_summary_label_->set_text(
                "Layers: " + godot::String::num_int64(composite_tiling_->get_tiling_layers().size()) +
                "  |  Boundary Splines: " + godot::String::num_int64(composite_tiling_->get_region_splines().size()) +
                "  |  Extracted Border Loops: " + godot::String::num_int64(composite_tiling_->get_border_outline_splines().size())
            );
        }

        _update_cell_inspector_label(region_canvas_ ? region_canvas_->get_selected_cell_id() : -1);
    }

    if (region_canvas_) {
        region_canvas_->queue_redraw();
    }

    is_updating_ui_ = false;
}

void CompositeTilingControl::_on_resource_changed() {
    refresh_ui();
}

void CompositeTilingControl::_on_rebuild_pressed() {
    if (composite_tiling_.is_valid()) {
        composite_tiling_->rebuild();
        refresh_ui();
    }
}

void CompositeTilingControl::_on_fit_view_pressed() {
    if (region_canvas_) {
        region_canvas_->fit_view_to_data(true, true, static_cast<real_t>(0.15));
    }
}

void CompositeTilingControl::_on_coverage_mode_selected(int p_idx) {
    if (is_updating_ui_ || composite_tiling_.is_null()) return;
    const godot::Dictionary before = composite_tiling_->get_snapshot_state();
    composite_tiling_->set_coverage_mode(static_cast<TilingCoverageModeType>(coverage_mode_opt_->get_item_id(p_idx)));
    const godot::Dictionary after  = composite_tiling_->get_snapshot_state();
    composite_tiling_->commit_snapshot_action("Change Tiling Coverage Mode", before, after);
    refresh_ui();
}

void CompositeTilingControl::_on_stitch_mode_selected(int p_idx) {
    if (is_updating_ui_ || composite_tiling_.is_null()) return;
    const godot::Dictionary before = composite_tiling_->get_snapshot_state();
    composite_tiling_->set_stitch_mode(static_cast<TilingStitchModeType>(stitch_mode_opt_->get_item_id(p_idx)));
    const godot::Dictionary after  = composite_tiling_->get_snapshot_state();
    composite_tiling_->commit_snapshot_action("Change Tiling Seam Stitch Mode", before, after);
    refresh_ui();
}

void CompositeTilingControl::_on_bridge_span_changed(double p_val) {
    if (is_updating_ui_ || composite_tiling_.is_null()) return;
    composite_tiling_->set_max_bridge_span(static_cast<real_t>(p_val));
    refresh_ui();
}

void CompositeTilingControl::_on_edge_subdivs_changed(double p_val) {
    if (is_updating_ui_ || composite_tiling_.is_null()) return;
    composite_tiling_->set_edge_subdivisions(static_cast<int>(p_val));
    refresh_ui();
}

void CompositeTilingControl::_on_auto_hexize_toggled(bool p_enabled) {
    if (is_updating_ui_ || composite_tiling_.is_null()) return;
    composite_tiling_->set_auto_hexize(p_enabled);
    refresh_ui();
}

void CompositeTilingControl::_on_auto_rebuild_toggled(bool p_enabled) {
    if (is_updating_ui_ || composite_tiling_.is_null()) return;
    composite_tiling_->set_auto_rebuild(p_enabled);
}

void CompositeTilingControl::_on_color_mode_selected(int p_idx) {
    if (!region_canvas_ || !color_mode_opt_) return;
    region_canvas_->set_color_mode(static_cast<TilingPreviewColorMode>(color_mode_opt_->get_item_id(p_idx)));
}

void CompositeTilingControl::_on_interact_mode_selected(int p_idx) {
    if (!region_canvas_ || !interact_mode_opt_) return;
    region_canvas_->set_interaction_mode(static_cast<TilingRegionInteractionMode>(interact_mode_opt_->get_item_id(p_idx)));
}

void CompositeTilingControl::_on_hex_overlay_toggled(bool p_show) {
    if (region_canvas_) region_canvas_->set_show_hex_super_cells(p_show);
}

void CompositeTilingControl::_on_border_overlay_toggled(bool p_show) {
    if (region_canvas_) region_canvas_->set_show_border_outlines(p_show);
}

void CompositeTilingControl::_on_flow_field_toggled(bool p_show) {
    if (region_canvas_) region_canvas_->set_show_flow_field(p_show);
}

void CompositeTilingControl::_on_manhattan_toggled(bool p_manhattan) {
    if (region_canvas_) region_canvas_->set_use_manhattan_path(p_manhattan);
}

void CompositeTilingControl::_on_add_layer_pressed() {
    if (composite_tiling_.is_null()) return;
    const godot::Dictionary before = composite_tiling_->get_snapshot_state();

    godot::TypedArray<WallpaperTilingResource> layers = composite_tiling_->get_tiling_layers();
    godot::Ref<WallpaperTilingResource> new_layer;
    new_layer.instantiate();
    new_layer->set_undo_redo(composite_tiling_->get_undo_redo());
    if (layers.size() % 2 == 1) {
        new_layer->set_wallpaper_group(WALLPAPER_P6M);
    } else {
        new_layer->set_wallpaper_group(WALLPAPER_P4M);
    }
    layers.push_back(new_layer);
    composite_tiling_->set_tiling_layers(layers);

    const godot::Dictionary after = composite_tiling_->get_snapshot_state();
    composite_tiling_->commit_snapshot_action("Add Wallpaper Tiling Layer", before, after);
    refresh_ui();
}

void CompositeTilingControl::_on_add_boundary_spline_pressed() {
    if (composite_tiling_.is_null()) return;
    const godot::Dictionary before = composite_tiling_->get_snapshot_state();

    godot::TypedArray<Spline2DResource> splines = composite_tiling_->get_region_splines();
    godot::Ref<Spline2DResource> poly_spline    = Spline2DResource::create_regular_polygon(6, static_cast<real_t>(4.5));
    if (poly_spline.is_valid()) {
        poly_spline->set_undo_redo(composite_tiling_->get_undo_redo());
        splines.push_back(poly_spline);
        composite_tiling_->set_region_splines(splines);
    }

    const godot::Dictionary after = composite_tiling_->get_snapshot_state();
    composite_tiling_->commit_snapshot_action("Add Boundary Spline2D Contour", before, after);
    refresh_ui();
}

void CompositeTilingControl::_on_canvas_cell_selected(int p_cell_id) {
    _update_cell_inspector_label(p_cell_id);
}

void CompositeTilingControl::_on_canvas_path_updated(int p_start_cell, int p_goal_cell, int p_path_len) {
    if (!cell_inspector_label_) return;
    cell_inspector_label_->set_text(
        "A* Path Sandbox  |  Start Cell: #" + godot::String::num_int64(p_start_cell) +
        "  ->  Goal Cell: #" + godot::String::num_int64(p_goal_cell) +
        "  |  Path Steps: " + godot::String::num_int64(p_path_len)
    );
}

} // namespace ideam::godot_ext

#include "voronoi_2d_canvas.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// Voronoi2DCanvas Implementation
// ============================================================================

void Voronoi2DCanvas::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(VORONOI_COLOR_INDEX_PALETTE);
    BIND_ENUM_CONSTANT(VORONOI_COLOR_AREA_HEATMAP);
    BIND_ENUM_CONSTANT(VORONOI_COLOR_NEIGHBOR_VALENCE);
    BIND_ENUM_CONSTANT(VORONOI_COLOR_BOUNDARY_PROXIMITY);

    ClassDB::bind_method(D_METHOD("set_voronoi_resource", "voronoi"), &Voronoi2DCanvas::set_voronoi_resource);
    ClassDB::bind_method(D_METHOD("get_voronoi_resource"), &Voronoi2DCanvas::get_voronoi_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "voronoi_resource", PROPERTY_HINT_RESOURCE_TYPE, "VoronoiDiagram2DResource"),
        "set_voronoi_resource",
        "get_voronoi_resource"
    );

    ClassDB::bind_method(D_METHOD("set_color_mode", "mode"), &Voronoi2DCanvas::set_color_mode);
    ClassDB::bind_method(D_METHOD("get_color_mode"), &Voronoi2DCanvas::get_color_mode);

    ClassDB::bind_method(D_METHOD("set_show_voronoi_cells", "show"), &Voronoi2DCanvas::set_show_voronoi_cells);
    ClassDB::bind_method(D_METHOD("get_show_voronoi_cells"), &Voronoi2DCanvas::get_show_voronoi_cells);

    ClassDB::bind_method(D_METHOD("set_show_delaunay", "show"), &Voronoi2DCanvas::set_show_delaunay);
    ClassDB::bind_method(D_METHOD("get_show_delaunay"), &Voronoi2DCanvas::get_show_delaunay);

    ClassDB::bind_method(D_METHOD("set_show_csr_dual_graph", "show"), &Voronoi2DCanvas::set_show_csr_dual_graph);
    ClassDB::bind_method(D_METHOD("get_show_csr_dual_graph"), &Voronoi2DCanvas::get_show_csr_dual_graph);

    ClassDB::bind_method(D_METHOD("set_show_centroids", "show"), &Voronoi2DCanvas::set_show_centroids);
    ClassDB::bind_method(D_METHOD("get_show_centroids"), &Voronoi2DCanvas::get_show_centroids);

    ClassDB::bind_method(D_METHOD("set_show_clip_boundary", "show"), &Voronoi2DCanvas::set_show_clip_boundary);
    ClassDB::bind_method(D_METHOD("get_show_clip_boundary"), &Voronoi2DCanvas::get_show_clip_boundary);
}

Voronoi2DCanvas::Voronoi2DCanvas() {
    set_point_style(GEOM_POINT_STYLE_CIRCLE);
    set_anchor_drag_constraint(GEOM_DRAG_AXIS_FREE_2D);
    set_lock_aspect_ratio(true);
    set_pan_zoom_axes(true, true, true, true);
    set_view_bounds(godot::Vector2(-6.0f, -6.0f), godot::Vector2(6.0f, 6.0f));
    set_custom_minimum_size(godot::Vector2(0.0f, 260.0f));
}

void Voronoi2DCanvas::set_voronoi_resource(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi) {
    set_geometry_resource(p_voronoi);
}

godot::Ref<VoronoiDiagram2DResource> Voronoi2DCanvas::get_voronoi_resource() const {
    return geometry_resource;
}

void Voronoi2DCanvas::set_color_mode(VoronoiCanvasColorMode p_mode) {
    color_mode_ = p_mode;
    queue_redraw();
}

void Voronoi2DCanvas::set_show_voronoi_cells(bool p_show) {
    show_voronoi_cells_ = p_show;
    queue_redraw();
}

void Voronoi2DCanvas::set_show_delaunay(bool p_show) {
    show_delaunay_ = p_show;
    queue_redraw();
}

void Voronoi2DCanvas::set_show_csr_dual_graph(bool p_show) {
    show_csr_dual_graph_ = p_show;
    queue_redraw();
}

void Voronoi2DCanvas::set_show_centroids(bool p_show) {
    show_centroids_ = p_show;
    queue_redraw();
}

void Voronoi2DCanvas::set_show_clip_boundary(bool p_show) {
    show_clip_boundary_ = p_show;
    queue_redraw();
}

int Voronoi2DCanvas::_get_canvas_point_count() const {
    const godot::Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    return vor.is_valid() ? vor->get_site_count() : 0;
}

bool Voronoi2DCanvas::_has_non_point_content() const {
    return get_voronoi_resource().is_valid();
}

godot::Vector2 Voronoi2DCanvas::_get_point_world_pos(int p_index) const {
    const godot::Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    return vor.is_valid() ? vor->get_site(p_index) : godot::Vector2();
}

void Voronoi2DCanvas::_set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) {
    godot::Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    if (vor.is_valid()) {
        vor->set_site(p_index, p_world_pos);
    }
}

godot::PackedVector2Array Voronoi2DCanvas::_sample_curve_screen_points(const godot::Rect2 &p_plot) const {
    (void)p_plot;
    // Voronoi cells and dual edges are rendered in _draw_plot_background_overlay / _draw_plot_foreground_overlay.
    return godot::PackedVector2Array();
}

bool Voronoi2DCanvas::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    const godot::Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    if (vor.is_null()) return false;

    const godot::Ref<Polygon2DResource> clip_poly = vor->get_clip_polygon();
    if (clip_poly.is_valid() && clip_poly->get_vertex_count() >= 3) {
        r_bounds = clip_poly->get_bounds_2d();
        return r_bounds.size.x > 1e-4f || r_bounds.size.y > 1e-4f;
    }

    r_bounds = vor->get_clip_bounds();
    return r_bounds.size.x > 1e-4f || r_bounds.size.y > 1e-4f;
}

void Voronoi2DCanvas::_on_double_click_plot(
    const godot::Vector2 &p_world_pos,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    (void)p_screen_pos;
    (void)p_plot;
    godot::Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    if (vor.is_valid()) {
        vor->commit_add_site(p_world_pos);
        set_selected_point_index(vor->get_site_count() - 1);
        emit_signal("point_selected", selected_point_index);
    }
}

void Voronoi2DCanvas::_on_right_click_point(int p_index) {
    godot::Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    if (vor.is_valid()) {
        vor->commit_remove_site(p_index);
        if (selected_point_index >= vor->get_site_count()) {
            set_selected_point_index(vor->get_site_count() - 1);
            emit_signal("point_selected", selected_point_index);
        }
    }
}

godot::Color Voronoi2DCanvas::_compute_cell_fill_color(
    int p_site_idx,
    int p_site_count,
    real_t p_area,
    real_t p_max_area,
    int p_neighbor_count
) const {
    using namespace godot;
    const bool is_sel = (p_site_idx == selected_point_index);
    const bool is_hov = (p_site_idx == hovered_point_index);
    const float alpha = is_sel ? 0.46f : (is_hov ? 0.36f : 0.24f);

    switch (color_mode_) {
        case VORONOI_COLOR_AREA_HEATMAP: {
            const float t = (p_max_area > static_cast<real_t>(1e-5))
                ? std::clamp(static_cast<float>(p_area / p_max_area), 0.0f, 1.0f)
                : 0.5f;
            return Color::from_hsv(0.58f - t * 0.50f, 0.72f, 0.88f, alpha);
        }
        case VORONOI_COLOR_NEIGHBOR_VALENCE: {
            const float t = std::clamp(static_cast<float>(p_neighbor_count - 3) / 6.0f, 0.0f, 1.0f);
            return Color::from_hsv(0.38f + t * 0.42f, 0.68f, 0.86f, alpha);
        }
        case VORONOI_COLOR_BOUNDARY_PROXIMITY: {
            const Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
            float dist_norm = 0.5f;
            if (vor.is_valid()) {
                const Vector2 s = vor->get_site(p_site_idx);
                const Ref<Polygon2DResource> cp = vor->get_clip_polygon();
                if (cp.is_valid() && cp->get_vertex_count() >= 3) {
                    const float max_dim = std::max(1.0f, cp->get_bounds_2d().size.length() * 0.5f);
                    dist_norm = std::clamp(static_cast<float>(cp->distance_to_boundary(s)) / max_dim, 0.0f, 1.0f);
                } else {
                    const Rect2 cb = vor->get_clip_bounds();
                    const float dx = std::min(s.x - cb.position.x, (cb.position.x + cb.size.x) - s.x);
                    const float dy = std::min(s.y - cb.position.y, (cb.position.y + cb.size.y) - s.y);
                    const float max_dim = std::max(1.0f, std::min(cb.size.x, cb.size.y) * 0.5f);
                    dist_norm = std::clamp(std::min(dx, dy) / max_dim, 0.0f, 1.0f);
                }
            }
            return Color::from_hsv(0.08f + dist_norm * 0.48f, 0.74f, 0.88f, alpha);
        }
        case VORONOI_COLOR_INDEX_PALETTE:
        default: {
            const float hue = std::fmod(static_cast<float>(p_site_idx) * 0.61803398875f, 1.0f);
            (void)p_site_count;
            return Color::from_hsv(hue, is_sel ? 0.78f : 0.58f, is_sel ? 0.96f : 0.84f, alpha);
        }
    }
}

void Voronoi2DCanvas::_draw_plot_background_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    const Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    if (vor.is_null()) return;

    const int n = vor->get_site_count();

    if (show_voronoi_cells_ && n > 0) {
        real_t max_area = static_cast<real_t>(1e-4);
        for (int i = 0; i < n; ++i) {
            max_area = std::max(max_area, vor->get_cell_area(i));
        }

        for (int i = 0; i < n; ++i) {
            const PackedVector2Array cell_w = vor->get_cell_polygon(i);
            if (cell_w.size() < 3) continue;

            PackedVector2Array cell_scr;
            cell_scr.resize(cell_w.size());
            for (int64_t k = 0; k < cell_w.size(); ++k) {
                cell_scr.set(k, world_to_screen(cell_w[k], p_plot));
            }

            const int nbr_count = static_cast<int>(vor->get_site_neighbors(i).size());
            const Color fill = _compute_cell_fill_color(i, n, vor->get_cell_area(i), max_area, nbr_count);

            PackedColorArray cols;
            cols.resize(cell_scr.size());
            for (int64_t k = 0; k < cols.size(); ++k) {
                cols.set(k, fill);
            }
            draw_polygon(cell_scr, cols);

            // Cell border polyline
            PackedVector2Array border_scr = cell_scr;
            border_scr.push_back(cell_scr[0]);
            const bool is_sel = (i == selected_point_index);
            draw_polyline(
                border_scr,
                is_sel ? Color(1.0f, 0.84f, 0.28f, 0.95f) : Color(0.56f, 0.74f, 0.90f, 0.65f),
                is_sel ? 2.2f : 1.25f,
                true
            );
        }
    }

    if (show_clip_boundary_) {
        const Rect2 cb      = vor->get_clip_bounds();
        const Vector2 c_min = world_to_screen(cb.position, p_plot);
        const Vector2 c_max = world_to_screen(cb.position + cb.size, p_plot);
        const Rect2 scr_cb(
            Vector2(std::min(c_min.x, c_max.x), std::min(c_min.y, c_max.y)),
            Vector2(std::abs(c_max.x - c_min.x), std::abs(c_max.y - c_min.y))
        );
        draw_rect(scr_cb, Color(0.52f, 0.60f, 0.72f, 0.55f), false, 1.25f);

        const Ref<Polygon2DResource> clip_poly = vor->get_clip_polygon();
        if (clip_poly.is_valid() && clip_poly->get_vertex_count() >= 3) {
            const PackedVector2Array pv = clip_poly->get_vertices();
            PackedVector2Array pv_scr;
            pv_scr.resize(pv.size() + 1);
            for (int64_t k = 0; k < pv.size(); ++k) {
                pv_scr.set(k, world_to_screen(pv[k], p_plot));
            }
            pv_scr.set(pv.size(), world_to_screen(pv[0], p_plot));
            draw_polyline(pv_scr, Color(0.35f, 0.92f, 0.68f, 0.92f), 2.0f, true);
        }
    }
}

void Voronoi2DCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    const Ref<VoronoiDiagram2DResource> vor = get_voronoi_resource();
    if (vor.is_null()) return;

    const int n = vor->get_site_count();
    if (n <= 0) return;

    if (show_delaunay_ && n >= 3) {
        const PackedInt32Array tris = vor->get_delaunay_triangles();
        const Color del_col(0.96f, 0.58f, 0.28f, 0.52f);
        for (int64_t t = 0; t + 2 < tris.size(); t += 3) {
            const int32_t i0 = tris[t];
            const int32_t i1 = tris[t + 1];
            const int32_t i2 = tris[t + 2];
            if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= n || i1 >= n || i2 >= n) continue;
            const Vector2 s0 = world_to_screen(vor->get_site(i0), p_plot);
            const Vector2 s1 = world_to_screen(vor->get_site(i1), p_plot);
            const Vector2 s2 = world_to_screen(vor->get_site(i2), p_plot);
            draw_line(s0, s1, del_col, 1.15f);
            draw_line(s1, s2, del_col, 1.15f);
            draw_line(s2, s0, del_col, 1.15f);
        }
    }

    if (show_csr_dual_graph_) {
        const PackedInt32Array row_off = vor->get_csr_row_offsets();
        const PackedInt32Array col_nbr = vor->get_csr_col_neighbors();
        for (int i = 0; i < n && i + 1 < row_off.size(); ++i) {
            const Vector2 si = world_to_screen(vor->get_site(i), p_plot);
            const int32_t start = row_off[i];
            const int32_t end   = row_off[i + 1];
            for (int32_t e = start; e < end && e < col_nbr.size(); ++e) {
                const int32_t j = col_nbr[e];
                if (j <= i && i != selected_point_index && j != selected_point_index) continue;
                if (j < 0 || j >= n) continue;
                const Vector2 sj = world_to_screen(vor->get_site(j), p_plot);
                const bool touches_sel = (i == selected_point_index || j == selected_point_index);
                draw_line(
                    si,
                    sj,
                    touches_sel ? Color(1.0f, 0.86f, 0.30f, 0.92f) : Color(0.42f, 0.86f, 0.98f, 0.38f),
                    touches_sel ? 2.0f : 1.0f
                );
            }
        }
    }

    if (show_centroids_) {
        for (int i = 0; i < n; ++i) {
            const PackedVector2Array cell_w = vor->get_cell_polygon(i);
            if (cell_w.size() < 3) continue;
            const Vector2 site_scr = world_to_screen(vor->get_site(i), p_plot);
            const Vector2 cent_scr = world_to_screen(vor->get_cell_centroid(i), p_plot);
            draw_line(site_scr, cent_scr, Color(0.95f, 0.46f, 0.78f, 0.72f), 1.25f);
            draw_circle(cent_scr, 3.2f, Color(0.95f, 0.46f, 0.78f, 0.92f));
        }
    }
}

// ============================================================================
// Voronoi2DControl Implementation
// ============================================================================

void Voronoi2DControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_voronoi_resource", "voronoi"), &Voronoi2DControl::set_voronoi_resource);
    ClassDB::bind_method(D_METHOD("get_voronoi_resource"), &Voronoi2DControl::get_voronoi_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "voronoi_resource", PROPERTY_HINT_RESOURCE_TYPE, "VoronoiDiagram2DResource"),
        "set_voronoi_resource",
        "get_voronoi_resource"
    );

    ClassDB::bind_method(D_METHOD("get_voronoi_canvas"), &Voronoi2DControl::get_voronoi_canvas);
    ClassDB::bind_method(D_METHOD("refresh_ui"), &Voronoi2DControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &Voronoi2DControl::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_fit_view_pressed"), &Voronoi2DControl::_on_fit_view_pressed);
    ClassDB::bind_method(D_METHOD("_on_color_mode_selected", "idx"), &Voronoi2DControl::_on_color_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_cells_toggled", "show"), &Voronoi2DControl::_on_cells_toggled);
    ClassDB::bind_method(D_METHOD("_on_delaunay_toggled", "show"), &Voronoi2DControl::_on_delaunay_toggled);
    ClassDB::bind_method(D_METHOD("_on_csr_dual_toggled", "show"), &Voronoi2DControl::_on_csr_dual_toggled);
    ClassDB::bind_method(D_METHOD("_on_centroids_toggled", "show"), &Voronoi2DControl::_on_centroids_toggled);
    ClassDB::bind_method(D_METHOD("_on_clip_boundary_toggled", "show"), &Voronoi2DControl::_on_clip_boundary_toggled);

    ClassDB::bind_method(D_METHOD("_on_add_site_pressed"), &Voronoi2DControl::_on_add_site_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_selected_site_pressed"), &Voronoi2DControl::_on_remove_selected_site_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_sites_pressed"), &Voronoi2DControl::_on_clear_sites_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_grid_pressed"), &Voronoi2DControl::_on_generate_grid_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_poisson_pressed"), &Voronoi2DControl::_on_generate_poisson_pressed);
    ClassDB::bind_method(D_METHOD("_on_lloyd_1_step_pressed"), &Voronoi2DControl::_on_lloyd_1_step_pressed);
    ClassDB::bind_method(D_METHOD("_on_lloyd_5_steps_pressed"), &Voronoi2DControl::_on_lloyd_5_steps_pressed);

    ClassDB::bind_method(D_METHOD("_on_clip_bounds_edited", "unused"), &Voronoi2DControl::_on_clip_bounds_edited);
    ClassDB::bind_method(D_METHOD("_on_bind_octagon_clip_pressed"), &Voronoi2DControl::_on_bind_octagon_clip_pressed);
    ClassDB::bind_method(D_METHOD("_on_bind_circle_clip_pressed"), &Voronoi2DControl::_on_bind_circle_clip_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_clip_polygon_pressed"), &Voronoi2DControl::_on_clear_clip_polygon_pressed);

    ClassDB::bind_method(D_METHOD("_on_canvas_point_selected", "site_index"), &Voronoi2DControl::_on_canvas_point_selected);
    ClassDB::bind_method(D_METHOD("_on_selected_site_coords_edited", "unused"), &Voronoi2DControl::_on_selected_site_coords_edited);
}

Voronoi2DControl::Voronoi2DControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
}

Voronoi2DControl::~Voronoi2DControl() {
    if (voronoi_resource_.is_valid()) {
        const godot::Callable cb(this, "_on_resource_changed");
        if (voronoi_resource_->is_connected("changed", cb)) {
            voronoi_resource_->disconnect("changed", cb);
        }
    }
}

void Voronoi2DControl::_build_ui() {
    using namespace godot;

    // 1. Viewport & Overlay Toolbar
    HBoxContainer *top_bar = memnew(HBoxContainer);
    top_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(top_bar);

    color_mode_opt_ = memnew(OptionButton);
    color_mode_opt_->add_item("Color: Cell Index", VORONOI_COLOR_INDEX_PALETTE);
    color_mode_opt_->add_item("Color: Cell Area", VORONOI_COLOR_AREA_HEATMAP);
    color_mode_opt_->add_item("Color: Neighbor Valence", VORONOI_COLOR_NEIGHBOR_VALENCE);
    color_mode_opt_->add_item("Color: Boundary Dist", VORONOI_COLOR_BOUNDARY_PROXIMITY);
    color_mode_opt_->connect("item_selected", Callable(this, "_on_color_mode_selected"));
    top_bar->add_child(color_mode_opt_);

    cells_check_ = memnew(CheckBox);
    cells_check_->set_text("Cells");
    cells_check_->set_pressed(true);
    cells_check_->connect("toggled", Callable(this, "_on_cells_toggled"));
    top_bar->add_child(cells_check_);

    delaunay_check_ = memnew(CheckBox);
    delaunay_check_->set_text("Delaunay");
    delaunay_check_->set_pressed(false);
    delaunay_check_->connect("toggled", Callable(this, "_on_delaunay_toggled"));
    top_bar->add_child(delaunay_check_);

    csr_dual_check_ = memnew(CheckBox);
    csr_dual_check_->set_text("CSR Dual");
    csr_dual_check_->set_pressed(true);
    csr_dual_check_->connect("toggled", Callable(this, "_on_csr_dual_toggled"));
    top_bar->add_child(csr_dual_check_);

    centroids_check_ = memnew(CheckBox);
    centroids_check_->set_text("Centroids");
    centroids_check_->set_pressed(true);
    centroids_check_->connect("toggled", Callable(this, "_on_centroids_toggled"));
    top_bar->add_child(centroids_check_);

    clip_boundary_check_ = memnew(CheckBox);
    clip_boundary_check_->set_text("Clip Border");
    clip_boundary_check_->set_pressed(true);
    clip_boundary_check_->connect("toggled", Callable(this, "_on_clip_boundary_toggled"));
    top_bar->add_child(clip_boundary_check_);

    Button *fit_btn = memnew(Button);
    fit_btn->set_text("Fit View");
    fit_btn->connect("pressed", Callable(this, "_on_fit_view_pressed"));
    top_bar->add_child(fit_btn);

    // 2. Interactive 2D Voronoi Canvas
    voronoi_canvas_ = memnew(Voronoi2DCanvas);
    voronoi_canvas_->connect("point_selected", Callable(this, "_on_canvas_point_selected"));
    add_child(voronoi_canvas_);

    // 3. Site Authoring & Lloyd Relaxation Row
    HBoxContainer *lloyd_bar = memnew(HBoxContainer);
    lloyd_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(lloyd_bar);

    Button *add_site_btn = memnew(Button);
    add_site_btn->set_text("+ Site");
    add_site_btn->connect("pressed", Callable(this, "_on_add_site_pressed"));
    lloyd_bar->add_child(add_site_btn);

    Button *rem_site_btn = memnew(Button);
    rem_site_btn->set_text("- Selected");
    rem_site_btn->connect("pressed", Callable(this, "_on_remove_selected_site_pressed"));
    lloyd_bar->add_child(rem_site_btn);

    Button *clear_btn = memnew(Button);
    clear_btn->set_text("Clear");
    clear_btn->connect("pressed", Callable(this, "_on_clear_sites_pressed"));
    lloyd_bar->add_child(clear_btn);

    Label *lloyd_lbl = memnew(Label);
    lloyd_lbl->set_text("  Lloyd α:");
    lloyd_bar->add_child(lloyd_lbl);

    lloyd_strength_spin_ = memnew(SpinBox);
    lloyd_strength_spin_->set_min(0.05);
    lloyd_strength_spin_->set_max(1.0);
    lloyd_strength_spin_->set_step(0.05);
    lloyd_strength_spin_->set_value(0.85);
    lloyd_bar->add_child(lloyd_strength_spin_);

    Button *lloyd_1_btn = memnew(Button);
    lloyd_1_btn->set_text("Relax 1x");
    lloyd_1_btn->connect("pressed", Callable(this, "_on_lloyd_1_step_pressed"));
    lloyd_bar->add_child(lloyd_1_btn);

    Button *lloyd_5_btn = memnew(Button);
    lloyd_5_btn->set_text("Relax 5x");
    lloyd_5_btn->connect("pressed", Callable(this, "_on_lloyd_5_steps_pressed"));
    lloyd_bar->add_child(lloyd_5_btn);

    // 4. Procedural Grid & Blue-Noise / Poisson Site Generators
    HBoxContainer *gen_bar = memnew(HBoxContainer);
    gen_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(gen_bar);

    Label *grid_lbl = memnew(Label);
    grid_lbl->set_text("Grid:");
    gen_bar->add_child(grid_lbl);

    grid_cols_spin_ = memnew(SpinBox);
    grid_cols_spin_->set_min(1.0);
    grid_cols_spin_->set_max(32.0);
    grid_cols_spin_->set_step(1.0);
    grid_cols_spin_->set_value(4.0);
    gen_bar->add_child(grid_cols_spin_);

    grid_rows_spin_ = memnew(SpinBox);
    grid_rows_spin_->set_min(1.0);
    grid_rows_spin_->set_max(32.0);
    grid_rows_spin_->set_step(1.0);
    grid_rows_spin_->set_value(4.0);
    gen_bar->add_child(grid_rows_spin_);

    grid_jitter_spin_ = memnew(SpinBox);
    grid_jitter_spin_->set_min(0.0);
    grid_jitter_spin_->set_max(0.95);
    grid_jitter_spin_->set_step(0.05);
    grid_jitter_spin_->set_value(0.35);
    grid_jitter_spin_->set_prefix("J:");
    gen_bar->add_child(grid_jitter_spin_);

    Button *gen_grid_btn = memnew(Button);
    gen_grid_btn->set_text("Seed Grid");
    gen_grid_btn->connect("pressed", Callable(this, "_on_generate_grid_pressed"));
    gen_bar->add_child(gen_grid_btn);

    Label *pois_lbl = memnew(Label);
    pois_lbl->set_text("  Poisson:");
    gen_bar->add_child(pois_lbl);

    poisson_count_spin_ = memnew(SpinBox);
    poisson_count_spin_->set_min(1.0);
    poisson_count_spin_->set_max(512.0);
    poisson_count_spin_->set_step(1.0);
    poisson_count_spin_->set_value(18.0);
    poisson_count_spin_->set_prefix("N:");
    gen_bar->add_child(poisson_count_spin_);

    poisson_dist_spin_ = memnew(SpinBox);
    poisson_dist_spin_->set_min(0.1);
    poisson_dist_spin_->set_max(20.0);
    poisson_dist_spin_->set_step(0.1);
    poisson_dist_spin_->set_value(1.4);
    poisson_dist_spin_->set_prefix("r:");
    gen_bar->add_child(poisson_dist_spin_);

    Button *gen_pois_btn = memnew(Button);
    gen_pois_btn->set_text("Seed Blue-Noise");
    gen_pois_btn->connect("pressed", Callable(this, "_on_generate_poisson_pressed"));
    gen_bar->add_child(gen_pois_btn);

    // 5. Clipping Bounds & Polygon2D Boundary Bridge Row
    HBoxContainer *clip_bar = memnew(HBoxContainer);
    clip_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(clip_bar);

    Label *cb_lbl = memnew(Label);
    cb_lbl->set_text("Clip Box:");
    clip_bar->add_child(cb_lbl);

    bounds_x_spin_ = memnew(SpinBox);
    bounds_x_spin_->set_min(-1000.0);
    bounds_x_spin_->set_max(1000.0);
    bounds_x_spin_->set_step(0.5);
    bounds_x_spin_->set_prefix("X:");
    bounds_x_spin_->connect("value_changed", Callable(this, "_on_clip_bounds_edited"));
    clip_bar->add_child(bounds_x_spin_);

    bounds_y_spin_ = memnew(SpinBox);
    bounds_y_spin_->set_min(-1000.0);
    bounds_y_spin_->set_max(1000.0);
    bounds_y_spin_->set_step(0.5);
    bounds_y_spin_->set_prefix("Y:");
    bounds_y_spin_->connect("value_changed", Callable(this, "_on_clip_bounds_edited"));
    clip_bar->add_child(bounds_y_spin_);

    bounds_w_spin_ = memnew(SpinBox);
    bounds_w_spin_->set_min(0.5);
    bounds_w_spin_->set_max(2000.0);
    bounds_w_spin_->set_step(0.5);
    bounds_w_spin_->set_prefix("W:");
    bounds_w_spin_->connect("value_changed", Callable(this, "_on_clip_bounds_edited"));
    clip_bar->add_child(bounds_w_spin_);

    bounds_h_spin_ = memnew(SpinBox);
    bounds_h_spin_->set_min(0.5);
    bounds_h_spin_->set_max(2000.0);
    bounds_h_spin_->set_step(0.5);
    bounds_h_spin_->set_prefix("H:");
    bounds_h_spin_->connect("value_changed", Callable(this, "_on_clip_bounds_edited"));
    clip_bar->add_child(bounds_h_spin_);

    Button *oct_clip_btn = memnew(Button);
    oct_clip_btn->set_text("Octagon Clip");
    oct_clip_btn->connect("pressed", Callable(this, "_on_bind_octagon_clip_pressed"));
    clip_bar->add_child(oct_clip_btn);

    Button *cir_clip_btn = memnew(Button);
    cir_clip_btn->set_text("Circle Clip");
    cir_clip_btn->connect("pressed", Callable(this, "_on_bind_circle_clip_pressed"));
    clip_bar->add_child(cir_clip_btn);

    Button *clr_clip_btn = memnew(Button);
    clr_clip_btn->set_text("Clear Poly Clip");
    clr_clip_btn->connect("pressed", Callable(this, "_on_clear_clip_polygon_pressed"));
    clip_bar->add_child(clr_clip_btn);

    // 6. Selected Site Coordinates & CSR Telemetry Footer
    HBoxContainer *sel_bar = memnew(HBoxContainer);
    sel_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(sel_bar);

    Label *sel_lbl = memnew(Label);
    sel_lbl->set_text("Selected Site:");
    sel_bar->add_child(sel_lbl);

    sel_site_x_spin_ = memnew(SpinBox);
    sel_site_x_spin_->set_min(-1000.0);
    sel_site_x_spin_->set_max(1000.0);
    sel_site_x_spin_->set_step(0.05);
    sel_site_x_spin_->set_prefix("X:");
    sel_site_x_spin_->connect("value_changed", Callable(this, "_on_selected_site_coords_edited"));
    sel_bar->add_child(sel_site_x_spin_);

    sel_site_y_spin_ = memnew(SpinBox);
    sel_site_y_spin_->set_min(-1000.0);
    sel_site_y_spin_->set_max(1000.0);
    sel_site_y_spin_->set_step(0.05);
    sel_site_y_spin_->set_prefix("Y:");
    sel_site_y_spin_->connect("value_changed", Callable(this, "_on_selected_site_coords_edited"));
    sel_bar->add_child(sel_site_y_spin_);

    cell_inspector_label_ = memnew(Label);
    cell_inspector_label_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    sel_bar->add_child(cell_inspector_label_);

    telemetry_label_ = memnew(Label);
    telemetry_label_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(telemetry_label_);
}

void Voronoi2DControl::set_voronoi_resource(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi) {
    if (voronoi_resource_ == p_voronoi) return;

    const godot::Callable cb(this, "_on_resource_changed");
    if (voronoi_resource_.is_valid() && voronoi_resource_->is_connected("changed", cb)) {
        voronoi_resource_->disconnect("changed", cb);
    }

    voronoi_resource_ = p_voronoi;

    if (voronoi_canvas_) {
        voronoi_canvas_->set_voronoi_resource(voronoi_resource_);
        voronoi_canvas_->fit_view_to_data(true, true, 0.15f);
    }

    if (voronoi_resource_.is_valid() && !voronoi_resource_->is_connected("changed", cb)) {
        voronoi_resource_->connect("changed", cb);
    }

    refresh_ui();
}

void Voronoi2DControl::_on_resource_changed() {
    refresh_ui();
}

void Voronoi2DControl::refresh_ui() {
    is_updating_ui_ = true;

    if (voronoi_resource_.is_valid()) {
        const godot::Rect2 cb = voronoi_resource_->get_clip_bounds();
        if (bounds_x_spin_) bounds_x_spin_->set_value(cb.position.x);
        if (bounds_y_spin_) bounds_y_spin_->set_value(cb.position.y);
        if (bounds_w_spin_) bounds_w_spin_->set_value(cb.size.x);
        if (bounds_h_spin_) bounds_h_spin_->set_value(cb.size.y);

        const int sel = voronoi_canvas_ ? voronoi_canvas_->get_selected_point_index() : -1;
        if (sel >= 0 && sel < voronoi_resource_->get_site_count()) {
            const godot::Vector2 s = voronoi_resource_->get_site(sel);
            if (sel_site_x_spin_) {
                sel_site_x_spin_->set_editable(true);
                sel_site_x_spin_->set_value(s.x);
            }
            if (sel_site_y_spin_) {
                sel_site_y_spin_->set_editable(true);
                sel_site_y_spin_->set_value(s.y);
            }
        } else {
            if (sel_site_x_spin_) sel_site_x_spin_->set_editable(false);
            if (sel_site_y_spin_) sel_site_y_spin_->set_editable(false);
        }
    }

    _update_telemetry_labels();
    if (voronoi_canvas_) {
        voronoi_canvas_->queue_redraw();
    }
    is_updating_ui_ = false;
}

void Voronoi2DControl::_update_telemetry_labels() {
    using namespace godot;
    if (voronoi_resource_.is_null()) {
        if (telemetry_label_) telemetry_label_->set_text("No VoronoiDiagram2DResource bound.");
        if (cell_inspector_label_) cell_inspector_label_->set_text("");
        return;
    }

    const int n_sites    = voronoi_resource_->get_site_count();
    const int n_tris     = static_cast<int>(voronoi_resource_->get_delaunay_triangles().size() / 3);
    const int n_csr_arcs = static_cast<int>(voronoi_resource_->get_csr_col_neighbors().size());
    const bool has_poly  = voronoi_resource_->get_clip_polygon().is_valid();

    if (telemetry_label_) {
        telemetry_label_->set_text(
            "Sites: " + String::num_int64(n_sites) +
            "  |  Delaunay Triangles: " + String::num_int64(n_tris) +
            "  |  CSR Dual Directed Arcs: " + String::num_int64(n_csr_arcs) +
            "  |  Clip Mode: " + (has_poly ? String("Polygon2D + Box") : String("Axis-Aligned Box"))
        );
    }

    const int sel = voronoi_canvas_ ? voronoi_canvas_->get_selected_point_index() : -1;
    if (cell_inspector_label_) {
        if (sel >= 0 && sel < n_sites) {
            const real_t area            = voronoi_resource_->get_cell_area(sel);
            const PackedInt32Array nbrs  = voronoi_resource_->get_site_neighbors(sel);
            cell_inspector_label_->set_text(
                "  [S" + String::num_int64(sel) + "] Area: " + String::num(static_cast<double>(area), 3) +
                " | Neighbors: " + String::num_int64(nbrs.size())
            );
        } else {
            cell_inspector_label_->set_text("  (Double-click plot to add site, right-click to remove)");
        }
    }
}

void Voronoi2DControl::_on_fit_view_pressed() {
    if (voronoi_canvas_) {
        voronoi_canvas_->fit_view_to_data(true, true, 0.15f);
    }
}

void Voronoi2DControl::_on_color_mode_selected(int p_idx) {
    if (!voronoi_canvas_ || !color_mode_opt_) return;
    voronoi_canvas_->set_color_mode(static_cast<VoronoiCanvasColorMode>(color_mode_opt_->get_item_id(p_idx)));
}

void Voronoi2DControl::_on_cells_toggled(bool p_show) {
    if (voronoi_canvas_) voronoi_canvas_->set_show_voronoi_cells(p_show);
}

void Voronoi2DControl::_on_delaunay_toggled(bool p_show) {
    if (voronoi_canvas_) voronoi_canvas_->set_show_delaunay(p_show);
}

void Voronoi2DControl::_on_csr_dual_toggled(bool p_show) {
    if (voronoi_canvas_) voronoi_canvas_->set_show_csr_dual_graph(p_show);
}

void Voronoi2DControl::_on_centroids_toggled(bool p_show) {
    if (voronoi_canvas_) voronoi_canvas_->set_show_centroids(p_show);
}

void Voronoi2DControl::_on_clip_boundary_toggled(bool p_show) {
    if (voronoi_canvas_) voronoi_canvas_->set_show_clip_boundary(p_show);
}

void Voronoi2DControl::_on_add_site_pressed() {
    if (voronoi_resource_.is_null()) return;
    const godot::Rect2 cb = voronoi_resource_->get_clip_bounds();
    const godot::Vector2 center = cb.position + cb.size * 0.5f;
    voronoi_resource_->commit_add_site(center);
    if (voronoi_canvas_) {
        voronoi_canvas_->set_selected_point_index(voronoi_resource_->get_site_count() - 1);
    }
    refresh_ui();
}

void Voronoi2DControl::_on_remove_selected_site_pressed() {
    if (voronoi_resource_.is_null() || !voronoi_canvas_) return;
    const int sel = voronoi_canvas_->get_selected_point_index();
    if (sel >= 0 && sel < voronoi_resource_->get_site_count()) {
        voronoi_resource_->commit_remove_site(sel);
        voronoi_canvas_->set_selected_point_index(std::min(sel, voronoi_resource_->get_site_count() - 1));
        refresh_ui();
    }
}

void Voronoi2DControl::_on_clear_sites_pressed() {
    if (voronoi_resource_.is_null()) return;
    voronoi_resource_->commit_clear();
    if (voronoi_canvas_) voronoi_canvas_->set_selected_point_index(-1);
    refresh_ui();
}

void Voronoi2DControl::_on_generate_grid_pressed() {
    if (voronoi_resource_.is_null()) return;
    const int cols     = grid_cols_spin_ ? static_cast<int>(grid_cols_spin_->get_value()) : 4;
    const int rows     = grid_rows_spin_ ? static_cast<int>(grid_rows_spin_->get_value()) : 4;
    const real_t jit   = grid_jitter_spin_ ? static_cast<real_t>(grid_jitter_spin_->get_value()) : static_cast<real_t>(0.35);
    voronoi_resource_->commit_generate_grid_sites(cols, rows, jit);
    refresh_ui();
}

void Voronoi2DControl::_on_generate_poisson_pressed() {
    if (voronoi_resource_.is_null()) return;
    const int cnt     = poisson_count_spin_ ? static_cast<int>(poisson_count_spin_->get_value()) : 18;
    const real_t dist = poisson_dist_spin_ ? static_cast<real_t>(poisson_dist_spin_->get_value()) : static_cast<real_t>(1.4);
    voronoi_resource_->commit_generate_poisson_sites(cnt, dist);
    refresh_ui();
}

void Voronoi2DControl::_on_lloyd_1_step_pressed() {
    if (voronoi_resource_.is_null()) return;
    const real_t alpha = lloyd_strength_spin_ ? static_cast<real_t>(lloyd_strength_spin_->get_value()) : static_cast<real_t>(0.85);
    voronoi_resource_->commit_lloyd_relaxation_step(alpha);
    refresh_ui();
}

void Voronoi2DControl::_on_lloyd_5_steps_pressed() {
    if (voronoi_resource_.is_null() || voronoi_resource_->get_site_count() <= 0) return;
    const real_t alpha = lloyd_strength_spin_ ? static_cast<real_t>(lloyd_strength_spin_->get_value()) : static_cast<real_t>(0.85);
    const godot::Dictionary before = voronoi_resource_->get_snapshot_state();
    for (int i = 0; i < 5; ++i) {
        voronoi_resource_->apply_lloyd_relaxation_step(alpha);
    }
    voronoi_resource_->commit_snapshot_action("5x Lloyd Voronoi Relaxation", before, voronoi_resource_->get_snapshot_state());
    refresh_ui();
}

void Voronoi2DControl::_on_clip_bounds_edited(double p_unused) {
    (void)p_unused;
    if (is_updating_ui_ || voronoi_resource_.is_null()) return;
    if (!bounds_x_spin_ || !bounds_y_spin_ || !bounds_w_spin_ || !bounds_h_spin_) return;
    const godot::Rect2 new_cb(
        static_cast<float>(bounds_x_spin_->get_value()),
        static_cast<float>(bounds_y_spin_->get_value()),
        static_cast<float>(bounds_w_spin_->get_value()),
        static_cast<float>(bounds_h_spin_->get_value())
    );
    voronoi_resource_->commit_set_clip_bounds(new_cb);
}

void Voronoi2DControl::_on_bind_octagon_clip_pressed() {
    if (voronoi_resource_.is_null()) return;
    const godot::Rect2 cb = voronoi_resource_->get_clip_bounds();
    const real_t radius = static_cast<real_t>(std::min(cb.size.x, cb.size.y) * 0.48f);
    godot::Ref<Polygon2DResource> poly;
    poly.instantiate();
    poly->create_regular_polygon(8, radius, static_cast<real_t>(0.0), false);
    const godot::Vector2 center = cb.position + cb.size * 0.5f;
    for (int i = 0; i < poly->get_vertex_count(); ++i) {
        poly->set_vertex(i, poly->get_vertex(i) + center);
    }
    voronoi_resource_->commit_set_clip_polygon(poly);
    refresh_ui();
}

void Voronoi2DControl::_on_bind_circle_clip_pressed() {
    if (voronoi_resource_.is_null()) return;
    const godot::Rect2 cb = voronoi_resource_->get_clip_bounds();
    const real_t radius = static_cast<real_t>(std::min(cb.size.x, cb.size.y) * 0.48f);
    godot::Ref<Polygon2DResource> poly;
    poly.instantiate();
    poly->create_regular_polygon(24, radius, static_cast<real_t>(0.0), false);
    const godot::Vector2 center = cb.position + cb.size * 0.5f;
    for (int i = 0; i < poly->get_vertex_count(); ++i) {
        poly->set_vertex(i, poly->get_vertex(i) + center);
    }
    voronoi_resource_->commit_set_clip_polygon(poly);
    refresh_ui();
}

void Voronoi2DControl::_on_clear_clip_polygon_pressed() {
    if (voronoi_resource_.is_null()) return;
    voronoi_resource_->commit_set_clip_polygon(godot::Ref<Polygon2DResource>());
    refresh_ui();
}

void Voronoi2DControl::_on_canvas_point_selected(int p_site_index) {
    (void)p_site_index;
    refresh_ui();
}

void Voronoi2DControl::_on_selected_site_coords_edited(double p_unused) {
    (void)p_unused;
    if (is_updating_ui_ || voronoi_resource_.is_null() || !voronoi_canvas_) return;
    const int sel = voronoi_canvas_->get_selected_point_index();
    if (sel < 0 || sel >= voronoi_resource_->get_site_count()) return;
    if (!sel_site_x_spin_ || !sel_site_y_spin_) return;
    const godot::Vector2 pt(
        static_cast<float>(sel_site_x_spin_->get_value()),
        static_cast<float>(sel_site_y_spin_->get_value())
    );
    voronoi_resource_->commit_set_site(sel, pt);
}

} // namespace ideam::godot_ext
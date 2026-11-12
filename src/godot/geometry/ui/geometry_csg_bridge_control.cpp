#include "geometry_csg_bridge_control.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ideam::godot_ext {

// ============================================================================
// GeometryCSGPreviewCanvas Implementation
// ============================================================================

void GeometryCSGPreviewCanvas::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(CSG_PREVIEW_CONVEX_CLIP);
    BIND_ENUM_CONSTANT(CSG_PREVIEW_HALF_PLANE_CLIP);
    BIND_ENUM_CONSTANT(CSG_PREVIEW_SEAM_WELD_SPLIT);
    BIND_ENUM_CONSTANT(CSG_PREVIEW_WAVE2_CLASSIFY);

    ClassDB::bind_method(D_METHOD("set_subject_polygon", "subject"), &GeometryCSGPreviewCanvas::set_subject_polygon);
    ClassDB::bind_method(D_METHOD("get_subject_polygon"), &GeometryCSGPreviewCanvas::get_subject_polygon);

    ClassDB::bind_method(D_METHOD("set_boundary_polygon", "boundary"), &GeometryCSGPreviewCanvas::set_boundary_polygon);
    ClassDB::bind_method(D_METHOD("get_boundary_polygon"), &GeometryCSGPreviewCanvas::get_boundary_polygon);

    ClassDB::bind_method(D_METHOD("set_preview_mode", "mode"), &GeometryCSGPreviewCanvas::set_preview_mode);
    ClassDB::bind_method(D_METHOD("get_preview_mode"), &GeometryCSGPreviewCanvas::get_preview_mode);

    ClassDB::bind_method(D_METHOD("set_half_plane_origin", "origin"), &GeometryCSGPreviewCanvas::set_half_plane_origin);
    ClassDB::bind_method(D_METHOD("get_half_plane_origin"), &GeometryCSGPreviewCanvas::get_half_plane_origin);

    ClassDB::bind_method(D_METHOD("set_half_plane_angle_deg", "angle_deg"), &GeometryCSGPreviewCanvas::set_half_plane_angle_deg);
    ClassDB::bind_method(D_METHOD("get_half_plane_angle_deg"), &GeometryCSGPreviewCanvas::get_half_plane_angle_deg);
    ClassDB::bind_method(D_METHOD("get_half_plane_normal"), &GeometryCSGPreviewCanvas::get_half_plane_normal);

    ClassDB::bind_method(D_METHOD("set_weld_tolerance", "tol"), &GeometryCSGPreviewCanvas::set_weld_tolerance);
    ClassDB::bind_method(D_METHOD("get_weld_tolerance"), &GeometryCSGPreviewCanvas::get_weld_tolerance);

    ClassDB::bind_method(D_METHOD("set_show_intersections", "show"), &GeometryCSGPreviewCanvas::set_show_intersections);
    ClassDB::bind_method(D_METHOD("get_show_intersections"), &GeometryCSGPreviewCanvas::get_show_intersections);

    ClassDB::bind_method(D_METHOD("_on_boundary_changed"), &GeometryCSGPreviewCanvas::_on_boundary_changed);
}

GeometryCSGPreviewCanvas::GeometryCSGPreviewCanvas() {
    set_point_style(GEOM_POINT_STYLE_SQUARE);
    set_anchor_drag_constraint(GEOM_DRAG_AXIS_FREE_2D);
    set_lock_aspect_ratio(true);
    set_pan_zoom_axes(true, true, true, true);
    set_view_bounds(godot::Vector2(-1.6f, -1.6f), godot::Vector2(1.6f, 1.6f));
    set_custom_minimum_size(godot::Vector2(0.0f, 240.0f));
}

GeometryCSGPreviewCanvas::~GeometryCSGPreviewCanvas() {
    if (boundary_polygon_.is_valid()) {
        const godot::Callable cb(this, "_on_boundary_changed");
        if (boundary_polygon_->is_connected("changed", cb)) {
            boundary_polygon_->disconnect("changed", cb);
        }
    }
}

void GeometryCSGPreviewCanvas::set_subject_polygon(const godot::Ref<Polygon2DResource> &p_subject) {
    set_geometry_resource(p_subject);
}

godot::Ref<Polygon2DResource> GeometryCSGPreviewCanvas::get_subject_polygon() const {
    return geometry_resource;
}

void GeometryCSGPreviewCanvas::set_boundary_polygon(const godot::Ref<Polygon2DResource> &p_boundary) {
    if (boundary_polygon_ == p_boundary) return;

    const godot::Callable cb(this, "_on_boundary_changed");
    if (boundary_polygon_.is_valid() && boundary_polygon_->is_connected("changed", cb)) {
        boundary_polygon_->disconnect("changed", cb);
    }

    boundary_polygon_ = p_boundary;

    if (boundary_polygon_.is_valid() && !boundary_polygon_->is_connected("changed", cb)) {
        boundary_polygon_->connect("changed", cb);
    }
    queue_redraw();
}

void GeometryCSGPreviewCanvas::_on_boundary_changed() {
    queue_redraw();
}

void GeometryCSGPreviewCanvas::set_preview_mode(GeometryCSGPreviewMode p_mode) {
    preview_mode_ = p_mode;
    queue_redraw();
}

void GeometryCSGPreviewCanvas::set_half_plane_origin(const godot::Vector2 &p_origin) {
    half_plane_origin_ = p_origin;
    queue_redraw();
}

void GeometryCSGPreviewCanvas::set_half_plane_angle_deg(float p_angle_deg) {
    half_plane_angle_deg_ = p_angle_deg;
    queue_redraw();
}

godot::Vector2 GeometryCSGPreviewCanvas::get_half_plane_normal() const noexcept {
    const float rad = godot::Math::deg_to_rad(half_plane_angle_deg_);
    return godot::Vector2(-std::sin(rad), std::cos(rad));
}

void GeometryCSGPreviewCanvas::set_weld_tolerance(float p_tol) {
    weld_tolerance_ = std::max(1e-5f, p_tol);
    queue_redraw();
}

void GeometryCSGPreviewCanvas::set_show_intersections(bool p_show) {
    show_intersections_ = p_show;
    queue_redraw();
}

int GeometryCSGPreviewCanvas::_get_canvas_point_count() const {
    const godot::Ref<Polygon2DResource> subj = get_subject_polygon();
    return subj.is_valid() ? subj->get_vertex_count() : 0;
}

int GeometryCSGPreviewCanvas::_get_canvas_section_count() const {
    const godot::Ref<Polygon2DResource> subj = get_subject_polygon();
    if (subj.is_null() || subj->get_vertex_count() < 2) return 0;
    return subj->is_closed() ? subj->get_vertex_count() : (subj->get_vertex_count() - 1);
}

bool GeometryCSGPreviewCanvas::_is_canvas_wrapped() const {
    const godot::Ref<Polygon2DResource> subj = get_subject_polygon();
    return subj.is_valid() && subj->is_closed();
}

bool GeometryCSGPreviewCanvas::_has_non_point_content() const {
    return get_subject_polygon().is_valid() || boundary_polygon_.is_valid();
}

godot::Vector2 GeometryCSGPreviewCanvas::_get_point_world_pos(int p_index) const {
    const godot::Ref<Polygon2DResource> subj = get_subject_polygon();
    return subj.is_valid() ? subj->get_vertex(p_index) : godot::Vector2();
}

void GeometryCSGPreviewCanvas::_set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) {
    godot::Ref<Polygon2DResource> subj = get_subject_polygon();
    if (subj.is_valid()) {
        subj->set_vertex(p_index, p_world_pos);
    }
}

godot::PackedVector2Array GeometryCSGPreviewCanvas::_sample_curve_screen_points(const godot::Rect2 &p_plot) const {
    godot::PackedVector2Array out;
    const godot::Ref<Polygon2DResource> subj = get_subject_polygon();
    if (subj.is_null() || subj->get_vertex_count() < 2) return out;

    const int n = subj->get_vertex_count();
    out.resize(subj->is_closed() ? (n + 1) : n);
    for (int i = 0; i < n; ++i) {
        out.set(i, world_to_screen(subj->get_vertex(i), p_plot));
    }
    if (subj->is_closed()) {
        out.set(n, world_to_screen(subj->get_vertex(0), p_plot));
    }
    return out;
}

void GeometryCSGPreviewCanvas::_draw_plot_background_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    const Ref<Polygon2DResource> subj = get_subject_polygon();

    // Draw Boundary Polygon B if valid
    if (boundary_polygon_.is_valid() && boundary_polygon_->get_vertex_count() >= 3) {
        const PackedVector2Array bv = boundary_polygon_->get_vertices();
        PackedVector2Array b_scr;
        b_scr.resize(bv.size() + 1);
        for (int64_t i = 0; i < bv.size(); ++i) {
            b_scr.set(i, world_to_screen(bv[i], p_plot));
        }
        b_scr.set(bv.size(), world_to_screen(bv[0], p_plot));
        draw_polyline(b_scr, Color(0.68f, 0.52f, 0.96f, 0.85f), 2.0f, true);
    }

    if (subj.is_null() || subj->get_vertex_count() < 3) return;

    auto draw_filled_world_poly = [&](const PackedVector2Array &p_world_pts, const Color &p_fill, const Color &p_border) {
        if (p_world_pts.size() < 3) return;
        std::vector<glm::vec<2, real_t>> glm_pts;
        glm_pts.reserve(static_cast<size_t>(p_world_pts.size()));
        PackedVector2Array scr_pts;
        scr_pts.resize(p_world_pts.size());
        for (int64_t i = 0; i < p_world_pts.size(); ++i) {
            glm_pts.emplace_back(static_cast<real_t>(p_world_pts[i].x), static_cast<real_t>(p_world_pts[i].y));
            scr_pts.set(i, world_to_screen(p_world_pts[i], p_plot));
        }
        std::vector<uint32_t> tris;
        core::PolygonMath2D<real_t>::triangulate_into(glm_pts, tris);
        for (size_t t = 0; t + 2 < tris.size(); t += 3) {
            PackedVector2Array tri;
            tri.push_back(scr_pts[static_cast<int64_t>(tris[t])]);
            tri.push_back(scr_pts[static_cast<int64_t>(tris[t + 1])]);
            tri.push_back(scr_pts[static_cast<int64_t>(tris[t + 2])]);
            PackedColorArray cols;
            cols.push_back(p_fill);
            cols.push_back(p_fill);
            cols.push_back(p_fill);
            draw_polygon(tri, cols);
        }
        PackedVector2Array loop = scr_pts;
        loop.push_back(scr_pts[0]);
        draw_polyline(loop, p_border, 2.25f, true);
    };

    if (preview_mode_ == CSG_PREVIEW_CONVEX_CLIP && boundary_polygon_.is_valid() && boundary_polygon_->get_vertex_count() >= 3) {
        const PackedVector2Array clipped = subj->clip_convex(boundary_polygon_->get_vertices());
        draw_filled_world_poly(clipped, Color(0.28f, 0.88f, 0.56f, 0.36f), Color(0.35f, 0.96f, 0.65f, 0.96f));
    } else if (preview_mode_ == CSG_PREVIEW_HALF_PLANE_CLIP) {
        const Vector2 n = get_half_plane_normal();
        const PackedVector2Array clipped = subj->clip_half_plane(half_plane_origin_, n);
        draw_filled_world_poly(clipped, Color(0.28f, 0.88f, 0.56f, 0.36f), Color(0.35f, 0.96f, 0.65f, 0.96f));
    } else if (preview_mode_ == CSG_PREVIEW_SEAM_WELD_SPLIT) {
        const Vector2 n   = get_half_plane_normal();
        const Vector2 dir = Vector2(n.y, -n.x);
        using Vec2 = glm::vec<2, real_t>;
        std::vector<std::vector<Vec2>> left_tiles(1);
        std::vector<std::vector<Vec2>> right_tiles(1);
        const PackedVector2Array l_clip = subj->clip_half_plane(half_plane_origin_, n);
        const PackedVector2Array r_clip = subj->clip_half_plane(half_plane_origin_, -n);
        for (int64_t i = 0; i < l_clip.size(); ++i) left_tiles[0].emplace_back(l_clip[i].x, l_clip[i].y);
        for (int64_t i = 0; i < r_clip.size(); ++i) right_tiles[0].emplace_back(r_clip[i].x, r_clip[i].y);

        std::vector<Vec2> seam_line = {
            Vec2(half_plane_origin_.x - dir.x * 4.0f, half_plane_origin_.y - dir.y * 4.0f),
            Vec2(half_plane_origin_.x, half_plane_origin_.y),
            Vec2(half_plane_origin_.x + dir.x * 4.0f, half_plane_origin_.y + dir.y * 4.0f)
        };
        core::CSGMath2D<real_t>::weld_polygons_along_seam(left_tiles, right_tiles, seam_line, static_cast<real_t>(weld_tolerance_));

        PackedVector2Array l_out, r_out;
        for (const auto &v : left_tiles[0]) l_out.push_back(Vector2(static_cast<float>(v.x), static_cast<float>(v.y)));
        for (const auto &v : right_tiles[0]) r_out.push_back(Vector2(static_cast<float>(v.x), static_cast<float>(v.y)));

        draw_filled_world_poly(l_out, Color(0.24f, 0.78f, 0.96f, 0.34f), Color(0.32f, 0.86f, 1.0f, 0.92f));
        draw_filled_world_poly(r_out, Color(0.96f, 0.62f, 0.24f, 0.34f), Color(1.0f, 0.72f, 0.32f, 0.92f));
    } else if (preview_mode_ == CSG_PREVIEW_WAVE2_CLASSIFY) {
        GeometryClipClassification2D cls = CLIP_CLASS_STRADDLING_BOUNDARY;
        if (boundary_polygon_.is_valid() && boundary_polygon_->get_vertex_count() >= 3) {
            cls = subj->classify_against_boundary(boundary_polygon_->get_vertices());
        } else {
            cls = subj->classify_against_half_plane(half_plane_origin_, get_half_plane_normal());
        }
        const Color c_fill = (cls == CLIP_CLASS_FULLY_INSIDE)
            ? Color(0.24f, 0.90f, 0.52f, 0.36f)
            : ((cls == CLIP_CLASS_FULLY_OUTSIDE)
                ? Color(0.94f, 0.32f, 0.36f, 0.36f)
                : Color(0.98f, 0.76f, 0.24f, 0.36f));
        const Color c_bord = c_fill.lightened(0.25f);
        draw_filled_world_poly(subj->get_vertices(), c_fill, c_bord);
    }
}

void GeometryCSGPreviewCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    const Ref<Polygon2DResource> subj = get_subject_polygon();

    // Draw Half-Plane / Seam line and normal arrow when in Half-Plane or Seam Weld modes
    if (preview_mode_ == CSG_PREVIEW_HALF_PLANE_CLIP ||
        preview_mode_ == CSG_PREVIEW_SEAM_WELD_SPLIT ||
        (preview_mode_ == CSG_PREVIEW_WAVE2_CLASSIFY && boundary_polygon_.is_null())) {
        const Vector2 n   = get_half_plane_normal();
        const Vector2 dir = Vector2(n.y, -n.x);
        const Vector2 p0  = world_to_screen(half_plane_origin_ - dir * 5.0f, p_plot);
        const Vector2 p1  = world_to_screen(half_plane_origin_ + dir * 5.0f, p_plot);
        const Vector2 o_s = world_to_screen(half_plane_origin_, p_plot);
        const Vector2 n_s = world_to_screen(half_plane_origin_ + n * 0.45f, p_plot);

        draw_line(p0, p1, Color(0.98f, 0.82f, 0.28f, 0.9f), 1.8f, true);
        draw_line(o_s, n_s, Color(0.42f, 0.96f, 0.62f, 0.95f), 2.0f, true);
        draw_circle(o_s, 4.5f, Color(0.98f, 0.82f, 0.28f, 1.0f));
        draw_circle(n_s, 3.5f, Color(0.42f, 0.96f, 0.62f, 1.0f));
    }

    // Draw pairwise edge-edge intersection events (`CSGMath2D::find_polygon_intersections`)
    if (show_intersections_ &&
        subj.is_valid() && subj->get_vertex_count() >= 3 &&
        boundary_polygon_.is_valid() && boundary_polygon_->get_vertex_count() >= 3) {
        const auto events = core::CSGMath2D<real_t>::find_polygon_intersections(
            subj->get_vertices_span(),
            boundary_polygon_->get_vertices_span()
        );
        for (const auto &ev : events) {
            const Vector2 scr = world_to_screen(
                Vector2(static_cast<float>(ev.point.x), static_cast<float>(ev.point.y)),
                p_plot
            );
            draw_circle(scr, 5.5f, Color(0.12f, 0.14f, 0.18f, 1.0f));
            draw_circle(scr, 4.0f, Color(1.0f, 0.36f, 0.52f, 1.0f));
        }
    }
}

// ============================================================================
// GeometryCSGBridgeControl Implementation
// ============================================================================

void GeometryCSGBridgeControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_subject_polygon", "subject"), &GeometryCSGBridgeControl::set_subject_polygon);
    ClassDB::bind_method(D_METHOD("get_subject_polygon"), &GeometryCSGBridgeControl::get_subject_polygon);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "subject_polygon", PROPERTY_HINT_RESOURCE_TYPE, "Polygon2DResource"),
        "set_subject_polygon",
        "get_subject_polygon"
    );

    ClassDB::bind_method(D_METHOD("set_boundary_polygon", "boundary"), &GeometryCSGBridgeControl::set_boundary_polygon);
    ClassDB::bind_method(D_METHOD("get_boundary_polygon"), &GeometryCSGBridgeControl::get_boundary_polygon);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "boundary_polygon", PROPERTY_HINT_RESOURCE_TYPE, "Polygon2DResource"),
        "set_boundary_polygon",
        "get_boundary_polygon"
    );

    ClassDB::bind_method(D_METHOD("set_target_voronoi", "voronoi"), &GeometryCSGBridgeControl::set_target_voronoi);
    ClassDB::bind_method(D_METHOD("get_target_voronoi"), &GeometryCSGBridgeControl::get_target_voronoi);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "target_voronoi", PROPERTY_HINT_RESOURCE_TYPE, "VoronoiDiagram2DResource"),
        "set_target_voronoi",
        "get_target_voronoi"
    );

    ClassDB::bind_method(D_METHOD("set_target_sdf_grid", "sdf"), &GeometryCSGBridgeControl::set_target_sdf_grid);
    ClassDB::bind_method(D_METHOD("get_target_sdf_grid"), &GeometryCSGBridgeControl::get_target_sdf_grid);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "target_sdf_grid", PROPERTY_HINT_RESOURCE_TYPE, "SDFGrid2DResource"),
        "set_target_sdf_grid",
        "get_target_sdf_grid"
    );

    ClassDB::bind_method(D_METHOD("get_csg_canvas"), &GeometryCSGBridgeControl::get_csg_canvas);
    ClassDB::bind_method(D_METHOD("refresh_ui"), &GeometryCSGBridgeControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_operand_changed"), &GeometryCSGBridgeControl::_on_operand_changed);
    ClassDB::bind_method(D_METHOD("_on_mode_selected", "idx"), &GeometryCSGBridgeControl::_on_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_intersections_toggled", "show"), &GeometryCSGBridgeControl::_on_intersections_toggled);
    ClassDB::bind_method(D_METHOD("_on_plane_params_changed", "unused"), &GeometryCSGBridgeControl::_on_plane_params_changed);
    ClassDB::bind_method(D_METHOD("_on_fit_view_pressed"), &GeometryCSGBridgeControl::_on_fit_view_pressed);

    ClassDB::bind_method(D_METHOD("_on_commit_clip_to_subject_pressed"), &GeometryCSGBridgeControl::_on_commit_clip_to_subject_pressed);
    ClassDB::bind_method(D_METHOD("_on_swap_subject_and_boundary_pressed"), &GeometryCSGBridgeControl::_on_swap_subject_and_boundary_pressed);
    ClassDB::bind_method(D_METHOD("_on_bridge_to_voronoi_clip_pressed"), &GeometryCSGBridgeControl::_on_bridge_to_voronoi_clip_pressed);
    ClassDB::bind_method(D_METHOD("_on_bridge_stamp_to_sdf_pressed"), &GeometryCSGBridgeControl::_on_bridge_stamp_to_sdf_pressed);
}

GeometryCSGBridgeControl::GeometryCSGBridgeControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
    _ensure_default_operands();
}

GeometryCSGBridgeControl::~GeometryCSGBridgeControl() {
    const godot::Callable cb(this, "_on_operand_changed");
    if (subject_polygon_.is_valid() && subject_polygon_->is_connected("changed", cb)) {
        subject_polygon_->disconnect("changed", cb);
    }
    if (boundary_polygon_.is_valid() && boundary_polygon_->is_connected("changed", cb)) {
        boundary_polygon_->disconnect("changed", cb);
    }
}

void GeometryCSGBridgeControl::_build_ui() {
    using namespace godot;

    // 1. Top Mode & Seam / Half-Plane Parameter Toolbar
    HBoxContainer *top_bar = memnew(HBoxContainer);
    top_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(top_bar);

    mode_opt_ = memnew(OptionButton);
    mode_opt_->add_item("Mode: Convex Boundary Clip (Sutherland-Hodgman)", CSG_PREVIEW_CONVEX_CLIP);
    mode_opt_->add_item("Mode: Half-Plane Clip", CSG_PREVIEW_HALF_PLANE_CLIP);
    mode_opt_->add_item("Mode: Seam Weld Split (Watertight Left/Right)", CSG_PREVIEW_SEAM_WELD_SPLIT);
    mode_opt_->add_item("Mode: Wave-2 Boundary Classify", CSG_PREVIEW_WAVE2_CLASSIFY);
    mode_opt_->connect("item_selected", Callable(this, "_on_mode_selected"));
    top_bar->add_child(mode_opt_);

    intersections_check_ = memnew(CheckBox);
    intersections_check_->set_text("Show Intersections");
    intersections_check_->set_pressed(true);
    intersections_check_->connect("toggled", Callable(this, "_on_intersections_toggled"));
    top_bar->add_child(intersections_check_);

    Button *fit_btn = memnew(Button);
    fit_btn->set_text("Fit View");
    fit_btn->connect("pressed", Callable(this, "_on_fit_view_pressed"));
    top_bar->add_child(fit_btn);

    // Seam / Half-Plane Controls Row
    HBoxContainer *plane_bar = memnew(HBoxContainer);
    plane_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(plane_bar);

    Label *pl_lbl = memnew(Label);
    pl_lbl->set_text("Seam / Plane:");
    plane_bar->add_child(pl_lbl);

    plane_x_spin_ = memnew(SpinBox);
    plane_x_spin_->set_min(-100.0);
    plane_x_spin_->set_max(100.0);
    plane_x_spin_->set_step(0.05);
    plane_x_spin_->set_value(0.0);
    plane_x_spin_->set_prefix("X:");
    plane_x_spin_->connect("value_changed", Callable(this, "_on_plane_params_changed"));
    plane_bar->add_child(plane_x_spin_);

    plane_y_spin_ = memnew(SpinBox);
    plane_y_spin_->set_min(-100.0);
    plane_y_spin_->set_max(100.0);
    plane_y_spin_->set_step(0.05);
    plane_y_spin_->set_value(0.0);
    plane_y_spin_->set_prefix("Y:");
    plane_y_spin_->connect("value_changed", Callable(this, "_on_plane_params_changed"));
    plane_bar->add_child(plane_y_spin_);

    plane_angle_spin_ = memnew(SpinBox);
    plane_angle_spin_->set_min(-180.0);
    plane_angle_spin_->set_max(180.0);
    plane_angle_spin_->set_step(5.0);
    plane_angle_spin_->set_value(25.0);
    plane_angle_spin_->set_suffix("°");
    plane_angle_spin_->connect("value_changed", Callable(this, "_on_plane_params_changed"));
    plane_bar->add_child(plane_angle_spin_);

    weld_tol_spin_ = memnew(SpinBox);
    weld_tol_spin_->set_min(0.0001);
    weld_tol_spin_->set_max(1.0);
    weld_tol_spin_->set_step(0.005);
    weld_tol_spin_->set_value(0.02);
    weld_tol_spin_->set_prefix("Weld ε:");
    weld_tol_spin_->connect("value_changed", Callable(this, "_on_plane_params_changed"));
    plane_bar->add_child(weld_tol_spin_);

    // 2. Interactive CSG Preview Canvas
    csg_canvas_ = memnew(GeometryCSGPreviewCanvas);
    add_child(csg_canvas_);

    // 3. Cross-Plugin Action & Bridge Buttons Row
    HBoxContainer *bridge_bar = memnew(HBoxContainer);
    bridge_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(bridge_bar);

    Button *commit_clip_btn = memnew(Button);
    commit_clip_btn->set_text("Commit Clip -> Subject");
    commit_clip_btn->connect("pressed", Callable(this, "_on_commit_clip_to_subject_pressed"));
    bridge_bar->add_child(commit_clip_btn);

    Button *swap_btn = memnew(Button);
    swap_btn->set_text("Swap A <-> B");
    swap_btn->connect("pressed", Callable(this, "_on_swap_subject_and_boundary_pressed"));
    bridge_bar->add_child(swap_btn);

    Button *vor_bridge_btn = memnew(Button);
    vor_bridge_btn->set_text("Bind Subject -> Voronoi Clip");
    vor_bridge_btn->connect("pressed", Callable(this, "_on_bridge_to_voronoi_clip_pressed"));
    bridge_bar->add_child(vor_bridge_btn);

    Button *sdf_bridge_btn = memnew(Button);
    sdf_bridge_btn->set_text("Stamp Subject -> SDF Grid");
    sdf_bridge_btn->connect("pressed", Callable(this, "_on_bridge_stamp_to_sdf_pressed"));
    bridge_bar->add_child(sdf_bridge_btn);

    // 4. Telemetry & Classification Footer
    classification_label_ = memnew(Label);
    classification_label_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(classification_label_);
}

void GeometryCSGBridgeControl::_ensure_default_operands() {
    if (subject_polygon_.is_null()) {
        godot::Ref<Polygon2DResource> subj;
        subj.instantiate();
        subj->create_regular_polygon(6, static_cast<real_t>(0.85));
        set_subject_polygon(subj);
    }
    if (boundary_polygon_.is_null()) {
        godot::Ref<Polygon2DResource> bnd;
        bnd.instantiate();
        bnd->create_rectangle(godot::Vector2(0.25f, 0.1f), godot::Vector2(1.4f, 1.2f));
        set_boundary_polygon(bnd);
    }
}

void GeometryCSGBridgeControl::set_subject_polygon(const godot::Ref<Polygon2DResource> &p_subject) {
    if (subject_polygon_ == p_subject) return;

    const godot::Callable cb(this, "_on_operand_changed");
    if (subject_polygon_.is_valid() && subject_polygon_->is_connected("changed", cb)) {
        subject_polygon_->disconnect("changed", cb);
    }

    subject_polygon_ = p_subject;
    if (csg_canvas_) {
        csg_canvas_->set_subject_polygon(subject_polygon_);
    }

    if (subject_polygon_.is_valid() && !subject_polygon_->is_connected("changed", cb)) {
        subject_polygon_->connect("changed", cb);
    }
    refresh_ui();
}

void GeometryCSGBridgeControl::set_boundary_polygon(const godot::Ref<Polygon2DResource> &p_boundary) {
    if (boundary_polygon_ == p_boundary) return;

    const godot::Callable cb(this, "_on_operand_changed");
    if (boundary_polygon_.is_valid() && boundary_polygon_->is_connected("changed", cb)) {
        boundary_polygon_->disconnect("changed", cb);
    }

    boundary_polygon_ = p_boundary;
    if (csg_canvas_) {
        csg_canvas_->set_boundary_polygon(boundary_polygon_);
    }

    if (boundary_polygon_.is_valid() && !boundary_polygon_->is_connected("changed", cb)) {
        boundary_polygon_->connect("changed", cb);
    }
    refresh_ui();
}

void GeometryCSGBridgeControl::set_target_voronoi(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi) {
    target_voronoi_ = p_voronoi;
}

void GeometryCSGBridgeControl::set_target_sdf_grid(const godot::Ref<SDFGrid2DResource> &p_sdf) {
    target_sdf_grid_ = p_sdf;
}

void GeometryCSGBridgeControl::_on_operand_changed() {
    refresh_ui();
}

void GeometryCSGBridgeControl::refresh_ui() {
    _update_classification_telemetry();
    if (csg_canvas_) {
        csg_canvas_->queue_redraw();
    }
}

void GeometryCSGBridgeControl::_update_classification_telemetry() {
    using namespace godot;
    if (!classification_label_) return;
    if (subject_polygon_.is_null() || subject_polygon_->get_vertex_count() < 3) {
        classification_label_->set_text("Subject Polygon2DResource has fewer than 3 vertices.");
        return;
    }

    GeometryClipClassification2D cls = CLIP_CLASS_STRADDLING_BOUNDARY;
    size_t num_ix = 0u;

    if (boundary_polygon_.is_valid() && boundary_polygon_->get_vertex_count() >= 3) {
        cls = subject_polygon_->classify_against_boundary(boundary_polygon_->get_vertices());
        num_ix = core::CSGMath2D<real_t>::find_polygon_intersections(
            subject_polygon_->get_vertices_span(),
            boundary_polygon_->get_vertices_span()
        ).size();
    } else if (csg_canvas_) {
        cls = subject_polygon_->classify_against_half_plane(
            csg_canvas_->get_half_plane_origin(),
            csg_canvas_->get_half_plane_normal()
        );
    }

    const String cls_str = (cls == CLIP_CLASS_FULLY_INSIDE)
        ? "FULLY_INSIDE (Wave-2 Fast SIMD Path)"
        : ((cls == CLIP_CLASS_FULLY_OUTSIDE)
            ? "FULLY_OUTSIDE (Wave-2 Culled)"
            : "STRADDLING_BOUNDARY (Wave-3 Clip & Seam Weld)");

    classification_label_->set_text(
        "Wave-2 Classification: " + cls_str +
        "  |  Edge Intersections: " + String::num_int64(static_cast<int64_t>(num_ix)) +
        "  |  Subject Area: " + String::num(static_cast<double>(subject_polygon_->get_abs_area()), 3)
    );
}

void GeometryCSGBridgeControl::_on_mode_selected(int p_idx) {
    if (!csg_canvas_ || !mode_opt_) return;
    csg_canvas_->set_preview_mode(static_cast<GeometryCSGPreviewMode>(mode_opt_->get_item_id(p_idx)));
    _update_classification_telemetry();
}

void GeometryCSGBridgeControl::_on_intersections_toggled(bool p_show) {
    if (csg_canvas_) csg_canvas_->set_show_intersections(p_show);
}

void GeometryCSGBridgeControl::_on_plane_params_changed(double p_unused) {
    (void)p_unused;
    if (!csg_canvas_) return;
    if (plane_x_spin_ && plane_y_spin_) {
        csg_canvas_->set_half_plane_origin(godot::Vector2(
            static_cast<float>(plane_x_spin_->get_value()),
            static_cast<float>(plane_y_spin_->get_value())
        ));
    }
    if (plane_angle_spin_) {
        csg_canvas_->set_half_plane_angle_deg(static_cast<float>(plane_angle_spin_->get_value()));
    }
    if (weld_tol_spin_) {
        csg_canvas_->set_weld_tolerance(static_cast<float>(weld_tol_spin_->get_value()));
    }
    _update_classification_telemetry();
}

void GeometryCSGBridgeControl::_on_fit_view_pressed() {
    if (csg_canvas_) {
        csg_canvas_->fit_view_to_data(true, true, 0.25f);
    }
}

void GeometryCSGBridgeControl::_on_commit_clip_to_subject_pressed() {
    if (subject_polygon_.is_null() || !csg_canvas_) return;
    const GeometryCSGPreviewMode mode = csg_canvas_->get_preview_mode();
    if (mode == CSG_PREVIEW_HALF_PLANE_CLIP || mode == CSG_PREVIEW_SEAM_WELD_SPLIT) {
        subject_polygon_->commit_clip_half_plane(
            csg_canvas_->get_half_plane_origin(),
            csg_canvas_->get_half_plane_normal()
        );
    } else if (boundary_polygon_.is_valid() && boundary_polygon_->get_vertex_count() >= 3) {
        subject_polygon_->commit_clip_convex(boundary_polygon_->get_vertices());
    }
    refresh_ui();
}

void GeometryCSGBridgeControl::_on_swap_subject_and_boundary_pressed() {
    godot::Ref<Polygon2DResource> tmp = subject_polygon_;
    set_subject_polygon(boundary_polygon_);
    set_boundary_polygon(tmp);
}

void GeometryCSGBridgeControl::_on_bridge_to_voronoi_clip_pressed() {
    if (target_voronoi_.is_valid() && subject_polygon_.is_valid()) {
        target_voronoi_->commit_set_clip_polygon(subject_polygon_);
    }
}

void GeometryCSGBridgeControl::_on_bridge_stamp_to_sdf_pressed() {
    if (target_sdf_grid_.is_valid() && subject_polygon_.is_valid() && subject_polygon_->get_vertex_count() >= 3) {
        target_sdf_grid_->commit_stamp_polygon(subject_polygon_->get_vertices(), SDF_COMBINE_SMOOTH_UNION, 0.25f);
    }
}

} // namespace ideam::godot_ext
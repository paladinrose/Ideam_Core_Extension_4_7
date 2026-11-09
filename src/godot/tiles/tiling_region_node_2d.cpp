#include "tiling_region_node_2d.h"
#include "../splines/spline_2d/spline_polygon_2d.h"
#include "../../core/tiles/tiling_navigation_2d.h"

#include <godot_cpp/classes/collision_polygon2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>
#include <span>

namespace ideam::godot_ext {

void TilingRegionNode2D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tiling", "tiling"), &TilingRegionNode2D::set_tiling);
    ClassDB::bind_method(D_METHOD("get_tiling"), &TilingRegionNode2D::get_tiling);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tiling", PROPERTY_HINT_RESOURCE_TYPE, "TilingResource"),
        "set_tiling",
        "get_tiling"
    );

    ClassDB::bind_method(D_METHOD("set_boundary_spline", "spline"), &TilingRegionNode2D::set_boundary_spline);
    ClassDB::bind_method(D_METHOD("get_boundary_spline"), &TilingRegionNode2D::get_boundary_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "boundary_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_boundary_spline",
        "get_boundary_spline"
    );

    ClassDB::bind_method(D_METHOD("set_boundary_polygon_node_path", "path"), &TilingRegionNode2D::set_boundary_polygon_node_path);
    ClassDB::bind_method(D_METHOD("get_boundary_polygon_node_path"), &TilingRegionNode2D::get_boundary_polygon_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "boundary_polygon_node_path"),
        "set_boundary_polygon_node_path",
        "get_boundary_polygon_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_fallback_region_rect", "rect"), &TilingRegionNode2D::set_fallback_region_rect);
    ClassDB::bind_method(D_METHOD("get_fallback_region_rect"), &TilingRegionNode2D::get_fallback_region_rect);
    ADD_PROPERTY(
        PropertyInfo(Variant::RECT2, "fallback_region_rect"),
        "set_fallback_region_rect",
        "get_fallback_region_rect"
    );

    ClassDB::bind_method(D_METHOD("set_coverage_mode", "mode"), &TilingRegionNode2D::set_coverage_mode);
    ClassDB::bind_method(D_METHOD("get_coverage_mode"), &TilingRegionNode2D::get_coverage_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "coverage_mode",
            PROPERTY_HINT_ENUM,
            "Strict Interior:0,Centroid Inside:1,Include Straddling:2,Clip Exact to Spline:3"
        ),
        "set_coverage_mode",
        "get_coverage_mode"
    );

    ClassDB::bind_method(D_METHOD("set_edge_subdivisions", "subdivs"), &TilingRegionNode2D::set_edge_subdivisions);
    ClassDB::bind_method(D_METHOD("get_edge_subdivisions"), &TilingRegionNode2D::get_edge_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "edge_subdivisions", PROPERTY_HINT_RANGE, "1,64,1"),
        "set_edge_subdivisions",
        "get_edge_subdivisions"
    );

    ADD_GROUP("Visual Styling", "");
    ClassDB::bind_method(D_METHOD("set_draw_cells", "draw"), &TilingRegionNode2D::set_draw_cells);
    ClassDB::bind_method(D_METHOD("get_draw_cells"), &TilingRegionNode2D::get_draw_cells);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_cells"), "set_draw_cells", "get_draw_cells");

    ClassDB::bind_method(D_METHOD("set_draw_cell_borders", "draw"), &TilingRegionNode2D::set_draw_cell_borders);
    ClassDB::bind_method(D_METHOD("get_draw_cell_borders"), &TilingRegionNode2D::get_draw_cell_borders);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_cell_borders"), "set_draw_cell_borders", "get_draw_cell_borders");

    ClassDB::bind_method(D_METHOD("set_draw_hex_super_cells", "draw"), &TilingRegionNode2D::set_draw_hex_super_cells);
    ClassDB::bind_method(D_METHOD("get_draw_hex_super_cells"), &TilingRegionNode2D::get_draw_hex_super_cells);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_hex_super_cells"), "set_draw_hex_super_cells", "get_draw_hex_super_cells");

    ClassDB::bind_method(D_METHOD("set_draw_region_outlines", "draw"), &TilingRegionNode2D::set_draw_region_outlines);
    ClassDB::bind_method(D_METHOD("get_draw_region_outlines"), &TilingRegionNode2D::get_draw_region_outlines);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_region_outlines"), "set_draw_region_outlines", "get_draw_region_outlines");

    ClassDB::bind_method(D_METHOD("set_draw_symmetry_elements", "draw"), &TilingRegionNode2D::set_draw_symmetry_elements);
    ClassDB::bind_method(D_METHOD("get_draw_symmetry_elements"), &TilingRegionNode2D::get_draw_symmetry_elements);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_symmetry_elements"), "set_draw_symmetry_elements", "get_draw_symmetry_elements");

    ClassDB::bind_method(D_METHOD("set_color_0", "color"), &TilingRegionNode2D::set_color_0);
    ClassDB::bind_method(D_METHOD("get_color_0"), &TilingRegionNode2D::get_color_0);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_0"), "set_color_0", "get_color_0");

    ClassDB::bind_method(D_METHOD("set_color_1", "color"), &TilingRegionNode2D::set_color_1);
    ClassDB::bind_method(D_METHOD("get_color_1"), &TilingRegionNode2D::get_color_1);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_1"), "set_color_1", "get_color_1");

    ClassDB::bind_method(D_METHOD("set_color_2", "color"), &TilingRegionNode2D::set_color_2);
    ClassDB::bind_method(D_METHOD("get_color_2"), &TilingRegionNode2D::get_color_2);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_2"), "set_color_2", "get_color_2");

    ClassDB::bind_method(D_METHOD("set_bridge_quad_color", "color"), &TilingRegionNode2D::set_bridge_quad_color);
    ClassDB::bind_method(D_METHOD("get_bridge_quad_color"), &TilingRegionNode2D::get_bridge_quad_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "bridge_quad_color"), "set_bridge_quad_color", "get_bridge_quad_color");

    ClassDB::bind_method(D_METHOD("set_border_line_color", "color"), &TilingRegionNode2D::set_border_line_color);
    ClassDB::bind_method(D_METHOD("get_border_line_color"), &TilingRegionNode2D::get_border_line_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "border_line_color"), "set_border_line_color", "get_border_line_color");

    ClassDB::bind_method(D_METHOD("set_hex_line_color", "color"), &TilingRegionNode2D::set_hex_line_color);
    ClassDB::bind_method(D_METHOD("get_hex_line_color"), &TilingRegionNode2D::get_hex_line_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "hex_line_color"), "set_hex_line_color", "get_hex_line_color");

    ClassDB::bind_method(D_METHOD("set_outline_color", "color"), &TilingRegionNode2D::set_outline_color);
    ClassDB::bind_method(D_METHOD("get_outline_color"), &TilingRegionNode2D::get_outline_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "outline_color"), "set_outline_color", "get_outline_color");

    ClassDB::bind_method(D_METHOD("set_border_line_width", "width"), &TilingRegionNode2D::set_border_line_width);
    ClassDB::bind_method(D_METHOD("get_border_line_width"), &TilingRegionNode2D::get_border_line_width);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "border_line_width", PROPERTY_HINT_RANGE, "0.5,10.0,0.25"),
        "set_border_line_width",
        "get_border_line_width"
    );

    ClassDB::bind_method(D_METHOD("set_hex_line_width", "width"), &TilingRegionNode2D::set_hex_line_width);
    ClassDB::bind_method(D_METHOD("get_hex_line_width"), &TilingRegionNode2D::get_hex_line_width);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "hex_line_width", PROPERTY_HINT_RANGE, "0.5,10.0,0.25"),
        "set_hex_line_width",
        "get_hex_line_width"
    );

    ClassDB::bind_method(D_METHOD("set_outline_line_width", "width"), &TilingRegionNode2D::set_outline_line_width);
    ClassDB::bind_method(D_METHOD("get_outline_line_width"), &TilingRegionNode2D::get_outline_line_width);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "outline_line_width", PROPERTY_HINT_RANGE, "0.5,10.0,0.25"),
        "set_outline_line_width",
        "get_outline_line_width"
    );

    ADD_GROUP("Collision Sync", "");
    ClassDB::bind_method(D_METHOD("set_auto_sync_collision", "sync"), &TilingRegionNode2D::set_auto_sync_collision);
    ClassDB::bind_method(D_METHOD("get_auto_sync_collision"), &TilingRegionNode2D::get_auto_sync_collision);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_sync_collision"), "set_auto_sync_collision", "get_auto_sync_collision");

    ClassDB::bind_method(D_METHOD("set_collision_polygon_path", "path"), &TilingRegionNode2D::set_collision_polygon_path);
    ClassDB::bind_method(D_METHOD("get_collision_polygon_path"), &TilingRegionNode2D::get_collision_polygon_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "collision_polygon_path"),
        "set_collision_polygon_path",
        "get_collision_polygon_path"
    );

    // --- Queries ---
    ClassDB::bind_method(D_METHOD("rebuild_region"), &TilingRegionNode2D::rebuild_region);
    ClassDB::bind_method(D_METHOD("get_cell_count"), &TilingRegionNode2D::get_cell_count);
    ClassDB::bind_method(D_METHOD("get_cell_at_local_point", "local_point"), &TilingRegionNode2D::get_cell_at_local_point);
    ClassDB::bind_method(D_METHOD("get_cell_at_global_point", "global_point"), &TilingRegionNode2D::get_cell_at_global_point);
    ClassDB::bind_method(D_METHOD("get_hex_at_local_point", "local_point"), &TilingRegionNode2D::get_hex_at_local_point);
    ClassDB::bind_method(D_METHOD("get_hex_at_global_point", "global_point"), &TilingRegionNode2D::get_hex_at_global_point);
    ClassDB::bind_method(D_METHOD("find_local_path", "local_start", "local_goal"), &TilingRegionNode2D::find_local_path);
    ClassDB::bind_method(D_METHOD("find_global_path", "global_start", "global_goal"), &TilingRegionNode2D::find_global_path);

    ClassDB::bind_method(D_METHOD("_on_tiling_changed"), &TilingRegionNode2D::_on_tiling_changed);
    ClassDB::bind_method(D_METHOD("_on_boundary_changed"), &TilingRegionNode2D::_on_boundary_changed);
}

TilingRegionNode2D::TilingRegionNode2D() = default;

TilingRegionNode2D::~TilingRegionNode2D() {
    const godot::Callable cb_tiling(this, "_on_tiling_changed");
    if (tiling_.is_valid() && tiling_->is_connected("changed", cb_tiling)) {
        tiling_->disconnect("changed", cb_tiling);
    }
    const godot::Callable cb_boundary(this, "_on_boundary_changed");
    if (boundary_spline_.is_valid() && boundary_spline_->is_connected("changed", cb_boundary)) {
        boundary_spline_->disconnect("changed", cb_boundary);
    }
}

void TilingRegionNode2D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        local_cache_dirty_ = true;
        if (auto_sync_collision_) {
            _sync_collision_polygons();
        }
        queue_redraw();
    }
}

void TilingRegionNode2D::_on_tiling_changed() {
    local_cache_dirty_ = true;
    if (auto_sync_collision_) {
        _sync_collision_polygons();
    }
    queue_redraw();
}

void TilingRegionNode2D::_on_boundary_changed() {
    local_cache_dirty_ = true;
    if (auto_sync_collision_) {
        _sync_collision_polygons();
    }
    queue_redraw();
}

const TilingRegionNode2D::CoreRegion *TilingRegionNode2D::_resolve_active_region() const {
    if (!tiling_.is_valid()) {
        return nullptr;
    }

    godot::Ref<CompositeTilingResource> comp = tiling_;
    if (comp.is_valid()) {
        return &comp->get_core_region();
    }

    godot::Ref<WallpaperTilingResource> wp = tiling_;
    if (wp.is_valid()) {
        if (local_cache_dirty_) {
            local_cache_dirty_ = false;
            local_region_cache_.clear();
            local_region_cache_.set_tiling(wp->get_core_tiling());

            godot::Ref<Spline2DResource> active_spline = boundary_spline_;
            if (!active_spline.is_valid() && !boundary_polygon_node_path_.is_empty() && is_inside_tree()) {
                godot::Node *target = get_node_or_null(boundary_polygon_node_path_);
                if (auto *sp_poly = godot::Object::cast_to<SplinePolygon2D>(target)) {
                    active_spline = sp_poly->get_spline();
                }
            }

            const auto cov = static_cast<core::TilingCoverageMode>(coverage_mode_);

            if (active_spline.is_valid() && active_spline->get_point_count() >= 3) {
                const CoreSpline &cs = active_spline->get_core_spline();
                local_region_cache_.build_from_splines(
                    std::span<const CoreSpline>(&cs, 1),
                    Vec2(static_cast<real_t>(0.0)),
                    cov,
                    edge_subdivisions_,
                    draw_hex_super_cells_
                );
            } else {
                const core::AABB2D<real_t> bounds(
                    _to_glm(fallback_region_rect_.position),
                    _to_glm(fallback_region_rect_.position + fallback_region_rect_.size)
                );
                local_region_cache_.build_from_aabb(
                    bounds,
                    Vec2(static_cast<real_t>(0.0)),
                    draw_hex_super_cells_,
                    edge_subdivisions_
                );
            }
        }
        return &local_region_cache_;
    }

    return nullptr;
}

void TilingRegionNode2D::_sync_collision_polygons() {
    if (collision_polygon_path_.is_empty() || !is_inside_tree()) {
        return;
    }
    godot::Node *target = get_node_or_null(collision_polygon_path_);
    auto *col_poly = godot::Object::cast_to<godot::CollisionPolygon2D>(target);
    if (!col_poly) {
        return;
    }

    const CoreRegion *region = _resolve_active_region();
    if (!region || region->get_cell_count() == 0) {
        col_poly->set_polygon(godot::PackedVector2Array());
        return;
    }

    const auto outlines = region->get_border_outlines();
    if (outlines.empty()) {
        col_poly->set_polygon(godot::PackedVector2Array());
        return;
    }

    const auto outer = outlines[0].get_smoothed_points(edge_subdivisions_);
    godot::PackedVector2Array pts;
    pts.resize(static_cast<int>(outer.size()));
    for (size_t i = 0; i < outer.size(); ++i) {
        pts.set(static_cast<int>(i), _to_godot(outer[i]));
    }
    col_poly->set_polygon(pts);
}

void TilingRegionNode2D::_draw() {
    const CoreRegion *region = _resolve_active_region();
    if (!region || region->get_cell_count() == 0) {
        return;
    }

    const godot::Color palette[3] = { color_0_, color_1_, color_2_ };
    const auto cells = region->get_cells();
    const auto verts = region->get_vertices();

    // 1. Draw fundamental & stitched cells
    if (draw_cells_ || draw_cell_borders_) {
        for (uint32_t cid = 0; cid < static_cast<uint32_t>(cells.size()); ++cid) {
            const auto &cell = cells[cid];
            if ((cell.flags & core::CELL_FLAG_MASKED) != 0u) continue;

            const auto poly = region->get_cell_polygon_points(cid);
            if (poly.size() < 3) continue;

            godot::PackedVector2Array pts;
            pts.resize(static_cast<int>(poly.size()));
            for (size_t i = 0; i < poly.size(); ++i) {
                pts.set(static_cast<int>(i), _to_godot(poly[i]));
            }

            if (draw_cells_) {
                const godot::Color fill_col = ((cell.flags & core::CELL_FLAG_BRIDGE_QUAD) != 0u)
                    ? bridge_quad_color_
                    : palette[cell.colour_id % 3u];
                godot::PackedColorArray cols;
                cols.push_back(fill_col);
                draw_polygon(pts, cols);
            }

            if (draw_cell_borders_) {
                pts.push_back(pts[0]);
                draw_polyline(pts, border_line_color_, border_line_width_, true);
            }
        }
    }

    // 2. Draw Hexize super-cell overlay
    if (draw_hex_super_cells_) {
        const auto hexes = region->get_hex_super_cells();
        for (size_t hid = 0; hid < hexes.size(); ++hid) {
            const auto &hex = hexes[hid];
            godot::PackedVector2Array pts;
            for (uint32_t vid : hex.perimeter_vertex_ids) {
                if (vid < verts.size()) {
                    pts.push_back(_to_godot(verts[vid]));
                }
            }
            if (pts.size() >= 3) {
                pts.push_back(pts[0]);
                draw_polyline(pts, hex_line_color_, hex_line_width_, true);
            }
        }
    }

    // 3. Draw FindBorders region outlines
    if (draw_region_outlines_) {
        const auto outlines = region->get_border_outlines();
        for (const auto &loop : outlines) {
            const auto loop_pts = loop.get_smoothed_points(edge_subdivisions_);
            if (loop_pts.size() < 2) continue;

            godot::PackedVector2Array pts;
            pts.resize(static_cast<int>(loop_pts.size() + 1));
            for (size_t i = 0; i < loop_pts.size(); ++i) {
                pts.set(static_cast<int>(i), _to_godot(loop_pts[i]));
            }
            pts.set(static_cast<int>(loop_pts.size()), _to_godot(loop_pts[0]));
            draw_polyline(pts, outline_color_, outline_line_width_, true);
        }
    }

    // 4. Draw symmetry element gizmos if bound to a WallpaperTilingResource
    if (draw_symmetry_elements_) {
        godot::Ref<WallpaperTilingResource> wp = tiling_;
        if (wp.is_valid()) {
            const auto &sym = wp->get_core_tiling().get_symmetry_handles();
            for (uint8_t i = 0; i < sym.rotation_pivot_count; ++i) {
                const godot::Vector2 pos = _to_godot(sym.points_of_rotation[i]);
                draw_circle(pos, 0.12f, godot::Color(1.0f, 0.35f, 0.2f, 0.95f));
            }
            for (uint8_t i = 0; i < sym.axis_count; ++i) {
                const godot::Vector2 dir = _to_godot(sym.axes_of_symmetry[i]);
                draw_line(-dir * 2.0f, dir * 2.0f, godot::Color(0.25f, 0.85f, 1.0f, 0.9f), 2.0f, true);
            }
            for (uint8_t i = 0; i < sym.glide_count; ++i) {
                const godot::Vector2 g = _to_godot(sym.glides[i]);
                draw_line(-g * 1.5f, g * 1.5f, godot::Color(0.95f, 0.55f, 0.95f, 0.9f), 1.75f, true);
            }
        }
    }
}

void TilingRegionNode2D::set_tiling(const godot::Ref<TilingResource> &p_tiling) {
    const godot::Callable cb(this, "_on_tiling_changed");
    if (tiling_.is_valid() && tiling_->is_connected("changed", cb)) {
        tiling_->disconnect("changed", cb);
    }
    tiling_ = p_tiling;
    if (tiling_.is_valid() && !tiling_->is_connected("changed", cb)) {
        tiling_->connect("changed", cb);
    }
    local_cache_dirty_ = true;
    queue_redraw();
}

void TilingRegionNode2D::set_boundary_spline(const godot::Ref<Spline2DResource> &p_spline) {
    const godot::Callable cb(this, "_on_boundary_changed");
    if (boundary_spline_.is_valid() && boundary_spline_->is_connected("changed", cb)) {
        boundary_spline_->disconnect("changed", cb);
    }
    boundary_spline_ = p_spline;
    if (boundary_spline_.is_valid() && !boundary_spline_->is_connected("changed", cb)) {
        boundary_spline_->connect("changed", cb);
    }
    local_cache_dirty_ = true;
    queue_redraw();
}

void TilingRegionNode2D::set_boundary_polygon_node_path(const godot::NodePath &p_path) {
    boundary_polygon_node_path_ = p_path;
    local_cache_dirty_ = true;
    queue_redraw();
}

void TilingRegionNode2D::set_fallback_region_rect(const godot::Rect2 &p_rect) {
    fallback_region_rect_ = p_rect;
    local_cache_dirty_ = true;
    queue_redraw();
}

void TilingRegionNode2D::set_coverage_mode(TilingCoverageModeType p_mode) {
    coverage_mode_ = static_cast<TilingCoverageModeType>(std::clamp<int>(p_mode, 0, 3));
    local_cache_dirty_ = true;
    queue_redraw();
}

void TilingRegionNode2D::set_edge_subdivisions(int p_subdivs) {
    edge_subdivisions_ = std::clamp(p_subdivs, 1, 64);
    local_cache_dirty_ = true;
    queue_redraw();
}

void TilingRegionNode2D::set_draw_cells(bool p_draw) {
    draw_cells_ = p_draw;
    queue_redraw();
}

void TilingRegionNode2D::set_draw_cell_borders(bool p_draw) {
    draw_cell_borders_ = p_draw;
    queue_redraw();
}

void TilingRegionNode2D::set_draw_hex_super_cells(bool p_draw) {
    draw_hex_super_cells_ = p_draw;
    local_cache_dirty_ = true;
    queue_redraw();
}

void TilingRegionNode2D::set_draw_region_outlines(bool p_draw) {
    draw_region_outlines_ = p_draw;
    queue_redraw();
}

void TilingRegionNode2D::set_draw_symmetry_elements(bool p_draw) {
    draw_symmetry_elements_ = p_draw;
    queue_redraw();
}

void TilingRegionNode2D::set_color_0(const godot::Color &p_color) {
    color_0_ = p_color;
    queue_redraw();
}

void TilingRegionNode2D::set_color_1(const godot::Color &p_color) {
    color_1_ = p_color;
    queue_redraw();
}

void TilingRegionNode2D::set_color_2(const godot::Color &p_color) {
    color_2_ = p_color;
    queue_redraw();
}

void TilingRegionNode2D::set_bridge_quad_color(const godot::Color &p_color) {
    bridge_quad_color_ = p_color;
    queue_redraw();
}

void TilingRegionNode2D::set_border_line_color(const godot::Color &p_color) {
    border_line_color_ = p_color;
    queue_redraw();
}

void TilingRegionNode2D::set_hex_line_color(const godot::Color &p_color) {
    hex_line_color_ = p_color;
    queue_redraw();
}

void TilingRegionNode2D::set_outline_color(const godot::Color &p_color) {
    outline_color_ = p_color;
    queue_redraw();
}

void TilingRegionNode2D::set_border_line_width(real_t p_width) {
    border_line_width_ = p_width;
    queue_redraw();
}

void TilingRegionNode2D::set_hex_line_width(real_t p_width) {
    hex_line_width_ = p_width;
    queue_redraw();
}

void TilingRegionNode2D::set_outline_line_width(real_t p_width) {
    outline_line_width_ = p_width;
    queue_redraw();
}

void TilingRegionNode2D::set_auto_sync_collision(bool p_sync) {
    auto_sync_collision_ = p_sync;
    if (auto_sync_collision_) {
        _sync_collision_polygons();
    }
}

void TilingRegionNode2D::set_collision_polygon_path(const godot::NodePath &p_path) {
    collision_polygon_path_ = p_path;
    if (auto_sync_collision_) {
        _sync_collision_polygons();
    }
}

void TilingRegionNode2D::rebuild_region() {
    godot::Ref<CompositeTilingResource> comp = tiling_;
    if (comp.is_valid()) {
        comp->rebuild();
    } else {
        local_cache_dirty_ = true;
        (void)_resolve_active_region();
    }
    if (auto_sync_collision_) {
        _sync_collision_polygons();
    }
    queue_redraw();
}

int TilingRegionNode2D::get_cell_count() const {
    const CoreRegion *region = _resolve_active_region();
    return region ? static_cast<int>(region->get_cell_count()) : 0;
}

int TilingRegionNode2D::get_cell_at_local_point(const godot::Vector2 &p_local_point) const {
    const CoreRegion *region = _resolve_active_region();
    if (!region) return -1;
    int32_t cid = -1;
    if (region->contains_point(_to_glm(p_local_point), cid)) {
        return static_cast<int>(cid);
    }
    return -1;
}

int TilingRegionNode2D::get_cell_at_global_point(const godot::Vector2 &p_global_point) const {
    return get_cell_at_local_point(to_local(p_global_point));
}

int TilingRegionNode2D::get_hex_at_local_point(const godot::Vector2 &p_local_point) const {
    const CoreRegion *region = _resolve_active_region();
    if (!region) return -1;
    int32_t hid = -1;
    if (region->hex_contains_point(_to_glm(p_local_point), hid)) {
        return static_cast<int>(hid);
    }
    return -1;
}

int TilingRegionNode2D::get_hex_at_global_point(const godot::Vector2 &p_global_point) const {
    return get_hex_at_local_point(to_local(p_global_point));
}

godot::PackedVector2Array TilingRegionNode2D::find_local_path(
    const godot::Vector2 &p_local_start,
    const godot::Vector2 &p_local_goal
) const {
    godot::PackedVector2Array out;
    const CoreRegion *region = _resolve_active_region();
    if (!region) return out;

    int32_t s_cid = -1;
    int32_t g_cid = -1;
    if (!region->contains_point(_to_glm(p_local_start), s_cid) ||
        !region->contains_point(_to_glm(p_local_goal), g_cid)) {
        return out;
    }

    const auto path = core::TilingNavigation2D<real_t>::find_path_cells(
        *region,
        static_cast<uint32_t>(s_cid),
        static_cast<uint32_t>(g_cid)
    );
    if (path.empty()) return out;

    const auto cells = region->get_cells();
    out.resize(static_cast<int>(path.size()));
    for (size_t i = 0; i < path.size(); ++i) {
        const uint32_t cid = path[i];
        if (cid < cells.size()) {
            out.set(static_cast<int>(i), _to_godot_f(cells[cid].center));
        }
    }
    return out;
}

godot::PackedVector2Array TilingRegionNode2D::find_global_path(
    const godot::Vector2 &p_global_start,
    const godot::Vector2 &p_global_goal
) const {
    godot::PackedVector2Array local_pts = find_local_path(to_local(p_global_start), to_local(p_global_goal));
    for (int i = 0; i < local_pts.size(); ++i) {
        local_pts.set(i, to_global(local_pts[i]));
    }
    return local_pts;
}

} // namespace ideam::godot_ext
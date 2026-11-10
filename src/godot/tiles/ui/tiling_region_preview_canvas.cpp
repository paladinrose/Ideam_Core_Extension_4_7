#include "tiling_region_preview_canvas.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void TilingRegionPreviewCanvas::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(PREVIEW_COLOR_THREE_COLORING);
    BIND_ENUM_CONSTANT(PREVIEW_COLOR_LAYER_GROUP);
    BIND_ENUM_CONSTANT(PREVIEW_COLOR_GENERATION_RING);
    BIND_ENUM_CONSTANT(PREVIEW_COLOR_CELL_FLAGS);

    BIND_ENUM_CONSTANT(REGION_INTERACT_INSPECT);
    BIND_ENUM_CONSTANT(REGION_INTERACT_CARVE_CELL);
    BIND_ENUM_CONSTANT(REGION_INTERACT_PATHFIND_SANDBOX);
    BIND_ENUM_CONSTANT(REGION_INTERACT_CYCLE_EDGE_PORT);

    ADD_SIGNAL(MethodInfo(
        "cell_selected",
        PropertyInfo(Variant::INT, "cell_id")
    ));
    ADD_SIGNAL(MethodInfo(
        "path_updated",
        PropertyInfo(Variant::INT, "start_cell_id"),
        PropertyInfo(Variant::INT, "goal_cell_id"),
        PropertyInfo(Variant::INT, "path_length")
    ));

    ClassDB::bind_method(D_METHOD("set_composite_tiling", "composite"), &TilingRegionPreviewCanvas::set_composite_tiling);
    ClassDB::bind_method(D_METHOD("get_composite_tiling"), &TilingRegionPreviewCanvas::get_composite_tiling);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "composite_tiling", PROPERTY_HINT_RESOURCE_TYPE, "CompositeTilingResource"),
        "set_composite_tiling",
        "get_composite_tiling"
    );

    ClassDB::bind_method(D_METHOD("set_color_mode", "mode"), &TilingRegionPreviewCanvas::set_color_mode);
    ClassDB::bind_method(D_METHOD("get_color_mode"), &TilingRegionPreviewCanvas::get_color_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "color_mode", PROPERTY_HINT_ENUM, "Three Coloring:0,Layer Group:1,Generation Ring:2,Cell Flags:3"),
        "set_color_mode",
        "get_color_mode"
    );

    ClassDB::bind_method(D_METHOD("set_interaction_mode", "mode"), &TilingRegionPreviewCanvas::set_interaction_mode);
    ClassDB::bind_method(D_METHOD("get_interaction_mode"), &TilingRegionPreviewCanvas::get_interaction_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "interaction_mode", PROPERTY_HINT_ENUM, "Inspect Cell:0,Carve/Remove Cell:1,A* Pathfinding Sandbox:2,Cycle Edge Port:3"),
        "set_interaction_mode",
        "get_interaction_mode"
    );

    ClassDB::bind_method(D_METHOD("set_show_hex_super_cells", "show"), &TilingRegionPreviewCanvas::set_show_hex_super_cells);
    ClassDB::bind_method(D_METHOD("get_show_hex_super_cells"), &TilingRegionPreviewCanvas::get_show_hex_super_cells);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_hex_super_cells"), "set_show_hex_super_cells", "get_show_hex_super_cells");

    ClassDB::bind_method(D_METHOD("set_show_border_outlines", "show"), &TilingRegionPreviewCanvas::set_show_border_outlines);
    ClassDB::bind_method(D_METHOD("get_show_border_outlines"), &TilingRegionPreviewCanvas::get_show_border_outlines);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_border_outlines"), "set_show_border_outlines", "get_show_border_outlines");

    ClassDB::bind_method(D_METHOD("set_show_flow_field", "show"), &TilingRegionPreviewCanvas::set_show_flow_field);
    ClassDB::bind_method(D_METHOD("get_show_flow_field"), &TilingRegionPreviewCanvas::get_show_flow_field);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_flow_field"), "set_show_flow_field", "get_show_flow_field");

    ClassDB::bind_method(D_METHOD("set_use_manhattan_path", "manhattan"), &TilingRegionPreviewCanvas::set_use_manhattan_path);
    ClassDB::bind_method(D_METHOD("get_use_manhattan_path"), &TilingRegionPreviewCanvas::get_use_manhattan_path);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_manhattan_path"), "set_use_manhattan_path", "get_use_manhattan_path");

    ClassDB::bind_method(D_METHOD("set_selected_cell_id", "cell_id"), &TilingRegionPreviewCanvas::set_selected_cell_id);
    ClassDB::bind_method(D_METHOD("get_selected_cell_id"), &TilingRegionPreviewCanvas::get_selected_cell_id);

    ClassDB::bind_method(D_METHOD("set_path_endpoints", "start_cell", "goal_cell"), &TilingRegionPreviewCanvas::set_path_endpoints);
    ClassDB::bind_method(D_METHOD("get_path_start_cell_id"), &TilingRegionPreviewCanvas::get_path_start_cell_id);
    ClassDB::bind_method(D_METHOD("get_path_goal_cell_id"), &TilingRegionPreviewCanvas::get_path_goal_cell_id);
}

TilingRegionPreviewCanvas::TilingRegionPreviewCanvas() {
    set_custom_minimum_size(godot::Vector2(0.0f, 280.0f));
    set_point_style(GEOM_POINT_STYLE_CIRCLE);
    set_anchor_drag_constraint(GEOM_DRAG_AXIS_FREE_2D);
    set_lock_aspect_ratio(true);
    set_pan_zoom_axes(true, true, true, true);
    set_view_bounds(godot::Vector2(-6.0f, -6.0f), godot::Vector2(6.0f, 6.0f));
}

void TilingRegionPreviewCanvas::set_composite_tiling(const godot::Ref<CompositeTilingResource> &p_composite) {
    set_geometry_resource(p_composite);
}

godot::Ref<CompositeTilingResource> TilingRegionPreviewCanvas::get_composite_tiling() const {
    return godot::Ref<CompositeTilingResource>(
        godot::Object::cast_to<CompositeTilingResource>(get_geometry_resource().ptr())
    );
}

void TilingRegionPreviewCanvas::_on_resource_bound() {
    selected_cell_id_   = -1;
    hovered_cell_id_    = -1;
    path_start_cell_id_ = -1;
    path_goal_cell_id_  = -1;
    cached_path_cells_.clear();
    cached_path_points_.clear();
    cached_flow_vectors_.clear();

    godot::Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_valid()) {
        fit_view_to_data(true, true, static_cast<real_t>(0.15));
    }
}

void TilingRegionPreviewCanvas::set_color_mode(TilingPreviewColorMode p_mode) {
    color_mode_ = p_mode;
    queue_redraw();
}

void TilingRegionPreviewCanvas::set_interaction_mode(TilingRegionInteractionMode p_mode) {
    interact_mode_ = p_mode;
    queue_redraw();
}

void TilingRegionPreviewCanvas::set_show_hex_super_cells(bool p_show) {
    show_hex_super_cells_ = p_show;
    queue_redraw();
}

void TilingRegionPreviewCanvas::set_show_border_outlines(bool p_show) {
    show_border_outlines_ = p_show;
    queue_redraw();
}

void TilingRegionPreviewCanvas::set_show_flow_field(bool p_show) {
    show_flow_field_ = p_show;
    _recompute_navigation_overlay();
    queue_redraw();
}

void TilingRegionPreviewCanvas::set_use_manhattan_path(bool p_manhattan) {
    use_manhattan_path_ = p_manhattan;
    _recompute_navigation_overlay();
    queue_redraw();
}

void TilingRegionPreviewCanvas::set_selected_cell_id(int p_cell_id) {
    if (selected_cell_id_ == p_cell_id) return;
    selected_cell_id_ = p_cell_id;
    emit_signal("cell_selected", selected_cell_id_);
    queue_redraw();
}

void TilingRegionPreviewCanvas::set_path_endpoints(int p_start_cell, int p_goal_cell) {
    path_start_cell_id_ = p_start_cell;
    path_goal_cell_id_  = p_goal_cell;
    _recompute_navigation_overlay();
    emit_signal("path_updated", path_start_cell_id_, path_goal_cell_id_, static_cast<int>(cached_path_cells_.size()));
    queue_redraw();
}

void TilingRegionPreviewCanvas::_recompute_navigation_overlay() {
    cached_path_cells_.clear();
    cached_path_points_.clear();
    cached_flow_vectors_.clear();

    godot::Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null()) return;

    const int n_cells = comp->get_cell_count();
    if (path_start_cell_id_ >= 0 && path_start_cell_id_ < n_cells &&
        path_goal_cell_id_ >= 0 && path_goal_cell_id_ < n_cells) {
        cached_path_cells_ = comp->find_path_cells(path_start_cell_id_, path_goal_cell_id_, use_manhattan_path_);
        cached_path_points_.resize(cached_path_cells_.size());
        for (int i = 0; i < cached_path_cells_.size(); ++i) {
            cached_path_points_.set(i, comp->get_cell_center(cached_path_cells_[i]));
        }
    }

    if (show_flow_field_) {
        godot::PackedInt32Array goals;
        if (path_goal_cell_id_ >= 0 && path_goal_cell_id_ < n_cells) {
            goals.push_back(path_goal_cell_id_);
        } else if (selected_cell_id_ >= 0 && selected_cell_id_ < n_cells) {
            goals.push_back(selected_cell_id_);
        }
        if (goals.size() > 0) {
            cached_flow_vectors_ = comp->compute_flow_field_vectors(goals);
        }
    }
}

// ============================================================================
// GeometryCanvas2DBase Overrides (Layer Seed Points + Region Extents)
// ============================================================================

int TilingRegionPreviewCanvas::_get_canvas_point_count() const {
    godot::Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null()) return 0;
    return std::max(1, static_cast<int>(comp->get_layer_seed_points().size()));
}

godot::Vector2 TilingRegionPreviewCanvas::_get_point_world_pos(int p_index) const {
    godot::Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null()) return godot::Vector2();
    const godot::PackedVector2Array seeds = comp->get_layer_seed_points();
    if (p_index >= 0 && p_index < seeds.size()) {
        return seeds[p_index];
    }
    const godot::Rect2 ext = comp->get_extents_rect();
    return ext.position + ext.size * 0.5f;
}

void TilingRegionPreviewCanvas::_set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) {
    godot::Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null() || p_index < 0) return;

    godot::PackedVector2Array seeds = comp->get_layer_seed_points();
    if (p_index >= seeds.size()) {
        seeds.resize(p_index + 1);
    }
    seeds.set(p_index, p_world_pos);
    comp->set_layer_seed_points(seeds);
}

godot::PackedVector2Array TilingRegionPreviewCanvas::_sample_curve_screen_points(
    const godot::Rect2 &p_plot
) const {
    // Primary boundary contour is drawn in `_draw_plot_foreground_overlay` so multiple closed loops are supported.
    (void)p_plot;
    return {};
}

bool TilingRegionPreviewCanvas::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    godot::Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null()) return false;

    const godot::Rect2 ext = comp->get_extents_rect();
    if (ext.size.x > 1e-4f || ext.size.y > 1e-4f) {
        r_bounds = ext;
        return true;
    }
    r_bounds = comp->get_fallback_aabb();
    return true;
}

godot::Color TilingRegionPreviewCanvas::_compute_cell_fill_color(
    const godot::Ref<CompositeTilingResource> &p_comp,
    int p_cell_id,
    int p_max_gen
) const {
    using namespace godot;
    const bool is_sel = (p_cell_id == selected_cell_id_);
    const bool is_hov = (p_cell_id == hovered_cell_id_);
    const float alpha = is_sel ? 0.62f : (is_hov ? 0.45f : 0.28f);

    switch (color_mode_) {
        case PREVIEW_COLOR_LAYER_GROUP: {
            const int grp = std::abs(p_comp->get_cell_group_slot(p_cell_id)) % 6;
            static constexpr float HUES[6] = { 0.56f, 0.08f, 0.36f, 0.78f, 0.16f, 0.92f };
            return Color::from_hsv(HUES[grp], 0.68f, 0.90f, alpha);
        }
        case PREVIEW_COLOR_GENERATION_RING: {
            const int gen = std::max(0, p_comp->get_cell_generation(p_cell_id));
            const float t = (p_max_gen > 0)
                ? (static_cast<float>(gen) / static_cast<float>(p_max_gen))
                : 0.0f;
            return Color::from_hsv(0.58f - t * 0.48f, 0.75f, 0.92f, alpha);
        }
        case PREVIEW_COLOR_CELL_FLAGS: {
            const int flags = p_comp->get_cell_flags(p_cell_id);
            if ((flags & static_cast<int>(core::TILE_CELL_BRIDGE_STITCH)) != 0) {
                return Color(0.96f, 0.56f, 0.22f, alpha + 0.12f);
            }
            if ((flags & static_cast<int>(core::TILE_CELL_BOUNDARY_CLIPPED)) != 0) {
                return Color(0.92f, 0.82f, 0.28f, alpha + 0.06f);
            }
            if ((flags & static_cast<int>(core::TILE_CELL_BORDER)) != 0) {
                return Color(0.38f, 0.84f, 0.96f, alpha + 0.04f);
            }
            return Color(0.28f, 0.64f, 0.52f, alpha);
        }
        case PREVIEW_COLOR_THREE_COLORING:
        default: {
            switch (std::abs(p_comp->get_cell_colour(p_cell_id)) % 3) {
                case 0: return Color(0.26f, 0.64f, 0.90f, alpha);
                case 1: return Color(0.34f, 0.80f, 0.58f, alpha);
                default: return Color(0.92f, 0.66f, 0.30f, alpha);
            }
        }
    }
}

void TilingRegionPreviewCanvas::_draw_plot_background_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null()) return;

    const int n_cells = comp->get_cell_count();
    int max_gen = 1;
    if (color_mode_ == PREVIEW_COLOR_GENERATION_RING) {
        for (int i = 0; i < n_cells; ++i) {
            max_gen = std::max(max_gen, comp->get_cell_generation(i));
        }
    }

    // 1. Render all Fundamental & Seam-Stitched Cells
    for (int c = 0; c < n_cells; ++c) {
        const PackedVector2Array world_poly = comp->get_cell_polygon(c);
        if (world_poly.size() < 3) continue;

        PackedVector2Array scr_poly;
        scr_poly.resize(world_poly.size());
        for (int i = 0; i < world_poly.size(); ++i) {
            scr_poly.set(i, world_to_screen(world_poly[i], p_plot));
        }

        const Color fill_col = _compute_cell_fill_color(comp, c, max_gen);
        PackedColorArray cols;
        cols.push_back(fill_col);
        draw_polygon(scr_poly, cols);

        PackedVector2Array ring = scr_poly;
        ring.push_back(scr_poly[0]);
        const bool is_sel = (c == selected_cell_id_);
        const bool is_hov = (c == hovered_cell_id_);
        const Color wire_col = is_sel
            ? Color(1.0f, 0.88f, 0.30f, 0.98f)
            : (is_hov ? Color(0.65f, 0.92f, 1.0f, 0.88f) : Color(0.32f, 0.40f, 0.52f, 0.52f));
        draw_polyline(ring, wire_col, is_sel ? 2.2f : 1.0f, true);
    }

    // 2. Highlight Neighbors of Selected Cell
    if (selected_cell_id_ >= 0 && selected_cell_id_ < n_cells) {
        const Vector2 sel_ctr_s = world_to_screen(comp->get_cell_center(selected_cell_id_), p_plot);
        const PackedInt32Array nbrs = comp->get_cell_neighbors(selected_cell_id_);
        for (int i = 0; i < nbrs.size(); ++i) {
            const int nb = nbrs[i];
            if (nb >= 0 && nb < n_cells) {
                const Vector2 nb_ctr_s = world_to_screen(comp->get_cell_center(nb), p_plot);
                draw_line(sel_ctr_s, nb_ctr_s, Color(1.0f, 0.84f, 0.32f, 0.85f), 1.8f);
                draw_circle(nb_ctr_s, 3.5f, Color(1.0f, 0.84f, 0.32f, 0.95f));
            }
        }
    }
}

void TilingRegionPreviewCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null()) return;

    Ref<Font> font = get_theme_default_font();

    // 1. Render Hex Super-Cells (`Hexize`)
    if (show_hex_super_cells_) {
        const int hex_count = comp->get_hex_super_cell_count();
        for (int h = 0; h < hex_count; ++h) {
            const PackedVector2Array hex_poly = comp->get_hex_perimeter_polygon(h);
            if (hex_poly.size() < 3) continue;
            PackedVector2Array scr_hex;
            scr_hex.resize(hex_poly.size() + 1);
            for (int i = 0; i < hex_poly.size(); ++i) {
                scr_hex.set(i, world_to_screen(hex_poly[i], p_plot));
            }
            scr_hex.set(hex_poly.size(), world_to_screen(hex_poly[0], p_plot));
            draw_polyline(scr_hex, Color(0.95f, 0.56f, 0.98f, 0.72f), 1.8f, true);
        }
    }

    // 2. Render Extracted Border Outline `Spline2DResource` Contours
    if (show_border_outlines_) {
        const TypedArray<Spline2DResource> borders = comp->get_border_outline_splines();
        for (int b = 0; b < borders.size(); ++b) {
            Ref<Spline2DResource> sp = borders[b];
            if (sp.is_null() || sp->get_point_count() < 2) continue;
            const PackedVector2Array pts = sp->get_smoothed_points(6);
            if (pts.size() < 2) continue;
            PackedVector2Array scr_pts;
            scr_pts.resize(pts.size() + 1);
            for (int i = 0; i < pts.size(); ++i) {
                scr_pts.set(i, world_to_screen(pts[i], p_plot));
            }
            scr_pts.set(pts.size(), world_to_screen(pts[0], p_plot));
            draw_polyline(scr_pts, Color(0.36f, 0.96f, 0.84f, 0.92f), 2.2f, true);
        }
    }

    // 3. Render Flow-Field Vectors
    if (show_flow_field_ && cached_flow_vectors_.size() == comp->get_cell_count()) {
        const int n_cells = comp->get_cell_count();
        for (int c = 0; c < n_cells; ++c) {
            const Vector2 dir = cached_flow_vectors_[c];
            if (dir.length_squared() < 1e-4f) continue;
            const Vector2 ctr_s = world_to_screen(comp->get_cell_center(c), p_plot);
            const Vector2 tip_s = ctr_s + Vector2(dir.x, -dir.y).normalized() * 11.0f;
            draw_line(ctr_s, tip_s, Color(0.88f, 0.96f, 0.42f, 0.85f), 1.4f);
            draw_circle(tip_s, 2.2f, Color(0.88f, 0.96f, 0.42f, 0.95f));
        }
    }

    // 4. Render Active A* Path Ribbon
    if (cached_path_points_.size() >= 2) {
        PackedVector2Array scr_path;
        scr_path.resize(cached_path_points_.size());
        for (int i = 0; i < cached_path_points_.size(); ++i) {
            scr_path.set(i, world_to_screen(cached_path_points_[i], p_plot));
        }
        draw_polyline(scr_path, Color(1.0f, 0.36f, 0.36f, 0.98f), 3.2f, true);
    }
    if (path_start_cell_id_ >= 0 && path_start_cell_id_ < comp->get_cell_count()) {
        const Vector2 s_scr = world_to_screen(comp->get_cell_center(path_start_cell_id_), p_plot);
        draw_circle(s_scr, 6.0f, Color(0.32f, 0.96f, 0.48f, 1.0f));
    }
    if (path_goal_cell_id_ >= 0 && path_goal_cell_id_ < comp->get_cell_count()) {
        const Vector2 g_scr = world_to_screen(comp->get_cell_center(path_goal_cell_id_), p_plot);
        draw_circle(g_scr, 6.0f, Color(0.98f, 0.34f, 0.38f, 1.0f));
    }

    // 5. Render Top-Left Region Telemetry Banner
    if (font.is_valid()) {
        const String hud =
            "Cells: " + String::num_int64(comp->get_cell_count()) +
            "  |  Edges: " + String::num_int64(comp->get_edge_count()) +
            "  |  Hex Super-Cells: " + String::num_int64(comp->get_hex_super_cell_count()) +
            "  |  Bridge Quads: " + String::num_int64(comp->get_bridge_quad_count()) +
            (selected_cell_id_ >= 0 ? ("  |  Selected: #" + String::num_int64(selected_cell_id_)) : String(""));
        const Rect2 banner(p_plot.position.x + 6.0f, p_plot.position.y + 6.0f, std::min(480.0f, p_plot.size.x - 12.0f), 20.0f);
        draw_rect(banner, Color(0.08f, 0.10f, 0.14f, 0.82f), true);
        draw_string(
            font,
            banner.position + Vector2(6.0f, 14.0f),
            hud,
            HORIZONTAL_ALIGNMENT_LEFT,
            banner.size.x - 10.0f,
            11,
            Color(0.86f, 0.92f, 0.98f, 0.95f)
        );
    }
}

// ============================================================================
// Interactive Region Sandbox Modes (`_try_begin_custom_drag`)
// ============================================================================

bool TilingRegionPreviewCanvas::_try_begin_custom_drag(
    const godot::Ref<godot::InputEvent> &p_event,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_null() || !mb->is_pressed()) return false;

    Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null()) return false;

    const Vector2 mpos = mb->get_position();
    if (!p_plot.has_point(mpos)) return false;

    // Allow seed-point dragging to take priority if the user clicked directly on a layer seed grip
    const int seed_count = comp->get_layer_seed_points().size();
    for (int i = 0; i < seed_count; ++i) {
        const Vector2 seed_scr = world_to_screen(_get_point_world_pos(i), p_plot);
        if (mpos.distance_to(seed_scr) <= point_hit_radius) {
            return false; // Let GeometryCanvas2DBase handle seed point drag
        }
    }

    const Vector2 world_pt = screen_to_world(mpos, p_plot);
    const int hit_cell     = comp->contains_point(world_pt);
    if (hit_cell < 0) return false;

    const MouseButton bidx = mb->get_button_index();

    switch (interact_mode_) {
        case REGION_INTERACT_INSPECT: {
            if (bidx == MOUSE_BUTTON_LEFT) {
                set_selected_cell_id(hit_cell);
                if (show_flow_field_) {
                    _recompute_navigation_overlay();
                }
                return true;
            }
            break;
        }
        case REGION_INTERACT_CARVE_CELL: {
            if (bidx == MOUSE_BUTTON_LEFT) {
                PackedInt32Array to_remove;
                to_remove.push_back(hit_cell);
                if (selected_cell_id_ == hit_cell) {
                    selected_cell_id_ = -1;
                }
                comp->commit_remove_cells(to_remove);
                _recompute_navigation_overlay();
                queue_redraw();
                return true;
            }
            break;
        }
        case REGION_INTERACT_PATHFIND_SANDBOX: {
            if (bidx == MOUSE_BUTTON_LEFT && !mb->is_shift_pressed()) {
                set_selected_cell_id(hit_cell);
                set_path_endpoints(hit_cell, path_goal_cell_id_ >= 0 ? path_goal_cell_id_ : hit_cell);
                return true;
            } else if (bidx == MOUSE_BUTTON_RIGHT || (bidx == MOUSE_BUTTON_LEFT && mb->is_shift_pressed())) {
                set_path_endpoints(path_start_cell_id_ >= 0 ? path_start_cell_id_ : hit_cell, hit_cell);
                return true;
            }
            break;
        }
        case REGION_INTERACT_CYCLE_EDGE_PORT: {
            if (bidx == MOUSE_BUTTON_LEFT && hit_cell < comp->get_edge_count()) {
                const int cur_dir  = static_cast<int>(comp->get_edge_port_direction(hit_cell));
                const int next_dir = (cur_dir + 1) % 4;
                comp->commit_set_edge_port_direction(hit_cell, static_cast<TilingPortDirectionType>(next_dir));
                _recompute_navigation_overlay();
                queue_redraw();
                return true;
            }
            break;
        }
    }
    return false;
}

void TilingRegionPreviewCanvas::_update_custom_hover(
    const godot::Vector2 &p_mouse_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<CompositeTilingResource> comp = get_composite_tiling();
    if (comp.is_null() || !p_plot.has_point(p_mouse_pos)) {
        if (hovered_cell_id_ != -1) {
            hovered_cell_id_ = -1;
            queue_redraw();
        }
        return;
    }

    const int hit = comp->contains_point(screen_to_world(p_mouse_pos, p_plot));
    if (hit != hovered_cell_id_) {
        hovered_cell_id_ = hit;
        queue_redraw();
    }
}

} // namespace ideam::godot_ext

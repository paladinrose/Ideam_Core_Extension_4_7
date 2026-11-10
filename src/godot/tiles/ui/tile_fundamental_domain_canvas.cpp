#include "tile_fundamental_domain_canvas.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void TileFundamentalDomainCanvas::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "edge_shape_selected",
        PropertyInfo(Variant::INT, "edge_shape_id")
    ));

    ClassDB::bind_method(D_METHOD("set_wallpaper_tiling", "tiling"), &TileFundamentalDomainCanvas::set_wallpaper_tiling);
    ClassDB::bind_method(D_METHOD("get_wallpaper_tiling"), &TileFundamentalDomainCanvas::get_wallpaper_tiling);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "wallpaper_tiling", PROPERTY_HINT_RESOURCE_TYPE, "WallpaperTilingResource"),
        "set_wallpaper_tiling",
        "get_wallpaper_tiling"
    );

    ClassDB::bind_method(D_METHOD("set_show_lattice_neighborhood", "show"), &TileFundamentalDomainCanvas::set_show_lattice_neighborhood);
    ClassDB::bind_method(D_METHOD("get_show_lattice_neighborhood"), &TileFundamentalDomainCanvas::get_show_lattice_neighborhood);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_lattice_neighborhood"), "set_show_lattice_neighborhood", "get_show_lattice_neighborhood");

    ClassDB::bind_method(D_METHOD("set_show_three_coloring", "show"), &TileFundamentalDomainCanvas::set_show_three_coloring);
    ClassDB::bind_method(D_METHOD("get_show_three_coloring"), &TileFundamentalDomainCanvas::get_show_three_coloring);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_three_coloring"), "set_show_three_coloring", "get_show_three_coloring");

    ClassDB::bind_method(D_METHOD("set_show_lattice_vectors", "show"), &TileFundamentalDomainCanvas::set_show_lattice_vectors);
    ClassDB::bind_method(D_METHOD("get_show_lattice_vectors"), &TileFundamentalDomainCanvas::get_show_lattice_vectors);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_lattice_vectors"), "set_show_lattice_vectors", "get_show_lattice_vectors");

    ClassDB::bind_method(D_METHOD("set_show_symmetry_elements", "show"), &TileFundamentalDomainCanvas::set_show_symmetry_elements);
    ClassDB::bind_method(D_METHOD("get_show_symmetry_elements"), &TileFundamentalDomainCanvas::get_show_symmetry_elements);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_symmetry_elements"), "set_show_symmetry_elements", "get_show_symmetry_elements");

    ClassDB::bind_method(D_METHOD("set_show_edge_constraints", "show"), &TileFundamentalDomainCanvas::set_show_edge_constraints);
    ClassDB::bind_method(D_METHOD("get_show_edge_constraints"), &TileFundamentalDomainCanvas::get_show_edge_constraints);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_edge_constraints"), "set_show_edge_constraints", "get_show_edge_constraints");

    ClassDB::bind_method(D_METHOD("set_show_triangulation", "show"), &TileFundamentalDomainCanvas::set_show_triangulation);
    ClassDB::bind_method(D_METHOD("get_show_triangulation"), &TileFundamentalDomainCanvas::get_show_triangulation);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_triangulation"), "set_show_triangulation", "get_show_triangulation");

    ClassDB::bind_method(D_METHOD("set_neighborhood_radius", "radius"), &TileFundamentalDomainCanvas::set_neighborhood_radius);
    ClassDB::bind_method(D_METHOD("get_neighborhood_radius"), &TileFundamentalDomainCanvas::get_neighborhood_radius);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "neighborhood_radius", PROPERTY_HINT_RANGE, "0,3,1"),
        "set_neighborhood_radius",
        "get_neighborhood_radius"
    );

    ClassDB::bind_method(D_METHOD("set_edge_subdivisions", "subdivisions"), &TileFundamentalDomainCanvas::set_edge_subdivisions);
    ClassDB::bind_method(D_METHOD("get_edge_subdivisions"), &TileFundamentalDomainCanvas::get_edge_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "edge_subdivisions", PROPERTY_HINT_RANGE, "2,48,1"),
        "set_edge_subdivisions",
        "get_edge_subdivisions"
    );

    ClassDB::bind_method(D_METHOD("set_selected_edge_shape_id", "edge_shape_id"), &TileFundamentalDomainCanvas::set_selected_edge_shape_id);
    ClassDB::bind_method(D_METHOD("get_selected_edge_shape_id"), &TileFundamentalDomainCanvas::get_selected_edge_shape_id);
}

TileFundamentalDomainCanvas::TileFundamentalDomainCanvas() {
    set_custom_minimum_size(godot::Vector2(0.0f, 260.0f));
    set_point_style(GEOM_POINT_STYLE_SQUARE);
    set_anchor_drag_constraint(GEOM_DRAG_AXIS_FREE_2D);
    set_lock_aspect_ratio(true);
    set_pan_zoom_axes(true, true, true, true);
    set_view_bounds(godot::Vector2(-2.2f, -2.2f), godot::Vector2(2.2f, 2.2f));
}

void TileFundamentalDomainCanvas::set_wallpaper_tiling(const godot::Ref<WallpaperTilingResource> &p_tiling) {
    set_geometry_resource(p_tiling);
}

godot::Ref<WallpaperTilingResource> TileFundamentalDomainCanvas::get_wallpaper_tiling() const {
    return godot::Ref<WallpaperTilingResource>(
        godot::Object::cast_to<WallpaperTilingResource>(get_geometry_resource().ptr())
    );
}

void TileFundamentalDomainCanvas::_on_resource_bound() {
    godot::Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_valid()) {
        if (selected_edge_shape_id_ >= tiling->get_edge_shape_count()) {
            selected_edge_shape_id_ = 0;
        }
        fit_view_to_data(true, true, static_cast<real_t>(0.25));
    }
}

void TileFundamentalDomainCanvas::set_show_lattice_neighborhood(bool p_show) {
    show_lattice_neighborhood_ = p_show;
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_show_three_coloring(bool p_show) {
    show_three_coloring_ = p_show;
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_show_lattice_vectors(bool p_show) {
    show_lattice_vectors_ = p_show;
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_show_symmetry_elements(bool p_show) {
    show_symmetry_elements_ = p_show;
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_show_edge_constraints(bool p_show) {
    show_edge_constraints_ = p_show;
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_show_triangulation(bool p_show) {
    show_triangulation_ = p_show;
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_neighborhood_radius(int p_radius) {
    neighborhood_radius_ = std::clamp(p_radius, 0, 3);
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_edge_subdivisions(int p_subdivs) {
    edge_subdivisions_ = std::clamp(p_subdivs, 2, 48);
    queue_redraw();
}

void TileFundamentalDomainCanvas::set_selected_edge_shape_id(int p_edge_shape_id) {
    if (selected_edge_shape_id_ == p_edge_shape_id) return;
    selected_edge_shape_id_ = std::max(0, p_edge_shape_id);
    emit_signal("edge_shape_selected", selected_edge_shape_id_);
    queue_redraw();
}

// ============================================================================
// Palette & Badge Helpers
// ============================================================================

godot::Color TileFundamentalDomainCanvas::_palette_color_for_index(int p_colour_idx, float p_alpha) {
    switch (std::abs(p_colour_idx) % 3) {
        case 0: return godot::Color(0.24f, 0.62f, 0.88f, p_alpha); // Cerulean
        case 1: return godot::Color(0.32f, 0.78f, 0.56f, p_alpha); // Emerald
        default: return godot::Color(0.92f, 0.64f, 0.28f, p_alpha); // Amber
    }
}

godot::String TileFundamentalDomainCanvas::_edge_shape_badge_text(TilingEdgeShapeType p_shape) {
    switch (p_shape) {
        case EDGE_SHAPE_J: return "J (Free)";
        case EDGE_SHAPE_U: return "U (Mirror)";
        case EDGE_SHAPE_S: return "S (180° Rot)";
        case EDGE_SHAPE_I: return "I (Rigid)";
        default: return "J";
    }
}

godot::Color TileFundamentalDomainCanvas::_edge_shape_badge_color(TilingEdgeShapeType p_shape) {
    switch (p_shape) {
        case EDGE_SHAPE_J: return godot::Color(0.42f, 0.86f, 0.54f, 0.95f);
        case EDGE_SHAPE_U: return godot::Color(0.38f, 0.78f, 0.98f, 0.95f);
        case EDGE_SHAPE_S: return godot::Color(0.98f, 0.74f, 0.30f, 0.95f);
        case EDGE_SHAPE_I: return godot::Color(0.94f, 0.44f, 0.48f, 0.95f);
        default: return godot::Color(0.80f, 0.84f, 0.90f, 0.95f);
    }
}

// ============================================================================
// GeometryCanvas2DBase Overrides
// ============================================================================

int TileFundamentalDomainCanvas::_get_canvas_point_count() const {
    godot::Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return 0;
    return tiling->get_base_prototile_vertices().size();
}

int TileFundamentalDomainCanvas::_get_canvas_section_count() const {
    godot::Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return 0;
    return tiling->get_vertex_count();
}

godot::Vector2 TileFundamentalDomainCanvas::_get_point_world_pos(int p_index) const {
    godot::Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return godot::Vector2();
    const godot::PackedVector2Array verts = tiling->get_base_prototile_vertices();
    if (p_index < 0 || p_index >= verts.size()) return godot::Vector2();
    const godot::Transform2D xform = tiling->get_cell_transform_2d(0, 0, 0);
    return xform.xform(verts[p_index]);
}

void TileFundamentalDomainCanvas::_set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) {
    godot::Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return;

    const int param_count = tiling->get_parameter_count();
    if (param_count > 0) {
        const godot::Vector2 prev_world = _get_point_world_pos(p_index);
        const godot::Vector2 delta      = p_world_pos - prev_world;
        const int p0_idx                = (p_index * 2) % param_count;
        const int p1_idx                = (p_index * 2 + 1) % param_count;

        const real_t cur_p0 = tiling->get_tactile_parameter(p0_idx);
        tiling->set_tactile_parameter(p0_idx, std::clamp<real_t>(cur_p0 + delta.x * static_cast<real_t>(0.35), static_cast<real_t>(0.05), static_cast<real_t>(0.95)));
        if (p1_idx != p0_idx) {
            const real_t cur_p1 = tiling->get_tactile_parameter(p1_idx);
            tiling->set_tactile_parameter(p1_idx, std::clamp<real_t>(cur_p1 + delta.y * static_cast<real_t>(0.35), static_cast<real_t>(0.05), static_cast<real_t>(0.95)));
        }
    } else {
        // When the active IH type has 0 free vertex parameters, dragging a corner scales the lattice basis smoothly.
        const godot::Vector2 origin = tiling->get_lattice_origin();
        const godot::Vector2 rel    = p_world_pos - origin;
        const float dist            = std::max(0.15f, rel.length());
        tiling->set_lattice_scale(godot::Vector2(dist, dist));
    }
}

godot::PackedVector2Array TileFundamentalDomainCanvas::_sample_curve_screen_points(
    const godot::Rect2 &p_plot
) const {
    godot::PackedVector2Array scr_pts;
    godot::Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return scr_pts;

    const godot::PackedVector2Array world_poly =
        tiling->get_cell_world_polygon(0, 0, 0, true, edge_subdivisions_);
    if (world_poly.size() == 0) return scr_pts;

    scr_pts.resize(world_poly.size() + 1);
    for (int i = 0; i < world_poly.size(); ++i) {
        scr_pts.set(i, world_to_screen(world_poly[i], p_plot));
    }
    scr_pts.set(world_poly.size(), world_to_screen(world_poly[0], p_plot));
    return scr_pts;
}

bool TileFundamentalDomainCanvas::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    godot::Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return false;

    const int r = show_lattice_neighborhood_ ? std::min(1, neighborhood_radius_) : 0;
    const int aspects = std::max(1, tiling->get_aspect_count());
    bool initialized = false;
    godot::Vector2 min_pt;
    godot::Vector2 max_pt;

    for (int t1 = -r; t1 <= r; ++t1) {
        for (int t2 = -r; t2 <= r; ++t2) {
            for (int asp = 0; asp < aspects; ++asp) {
                const godot::PackedVector2Array poly =
                    tiling->get_cell_world_polygon(t1, t2, asp, true, 6);
                for (int i = 0; i < poly.size(); ++i) {
                    const godot::Vector2 p = poly[i];
                    if (!initialized) {
                        min_pt = max_pt = p;
                        initialized = true;
                    } else {
                        min_pt.x = std::min(min_pt.x, p.x);
                        min_pt.y = std::min(min_pt.y, p.y);
                        max_pt.x = std::max(max_pt.x, p.x);
                        max_pt.y = std::max(max_pt.y, p.y);
                    }
                }
            }
        }
    }

    if (!initialized) return false;
    r_bounds = godot::Rect2(min_pt, max_pt - min_pt);
    return true;
}

// ============================================================================
// Custom Overlay Rendering (`_draw_plot_background_overlay` / `_draw_plot_foreground_overlay`)
// ============================================================================

void TileFundamentalDomainCanvas::_draw_plot_background_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return;

    const int aspects = std::max(1, tiling->get_aspect_count());
    const int radius  = show_lattice_neighborhood_ ? neighborhood_radius_ : 0;

    // 1. Draw surrounding neighborhood & unit-cell instances
    for (int t1 = -radius; t1 <= radius; ++t1) {
        for (int t2 = -radius; t2 <= radius; ++t2) {
            for (int asp = 0; asp < aspects; ++asp) {
                const bool is_primary = (t1 == 0 && t2 == 0 && asp == 0);
                const bool is_unit_cell = (t1 == 0 && t2 == 0);
                if (!show_lattice_neighborhood_ && !is_unit_cell) continue;

                const PackedVector2Array world_poly =
                    tiling->get_cell_world_polygon(t1, t2, asp, true, edge_subdivisions_);
                if (world_poly.size() < 3) continue;

                PackedVector2Array scr_poly;
                scr_poly.resize(world_poly.size());
                for (int i = 0; i < world_poly.size(); ++i) {
                    scr_poly.set(i, world_to_screen(world_poly[i], p_plot));
                }

                const int col_idx = tiling->get_cell_colour(t1, t2, asp);
                const float fill_alpha = is_primary ? 0.34f : (is_unit_cell ? 0.22f : 0.11f);
                const Color fill_col = show_three_coloring_
                    ? _palette_color_for_index(col_idx, fill_alpha)
                    : Color(0.32f, 0.58f, 0.82f, fill_alpha);

                PackedColorArray colors;
                colors.push_back(fill_col);
                draw_polygon(scr_poly, colors);

                PackedVector2Array closed_ring = scr_poly;
                closed_ring.push_back(scr_poly[0]);
                const Color wire_col = is_primary
                    ? Color(0.50f, 0.88f, 1.0f, 0.95f)
                    : (is_unit_cell
                        ? Color(0.45f, 0.64f, 0.82f, 0.65f)
                        : Color(0.32f, 0.42f, 0.54f, 0.40f));
                draw_polyline(closed_ring, wire_col, is_primary ? 2.0f : 1.1f, true);
            }
        }
    }

    // 2. Optional ear-clipped triangulation wireframe on primary prototile (0, 0, 0)
    if (show_triangulation_) {
        const PackedVector2Array outline = tiling->get_prototile_outline(edge_subdivisions_);
        const PackedInt32Array tris      = tiling->get_prototile_triangles(edge_subdivisions_);
        const Transform2D xform0         = tiling->get_cell_transform_2d(0, 0, 0);
        for (int i = 0; i + 2 < tris.size(); i += 3) {
            const int i0 = tris[i];
            const int i1 = tris[i + 1];
            const int i2 = tris[i + 2];
            if (i0 >= 0 && i0 < outline.size() &&
                i1 >= 0 && i1 < outline.size() &&
                i2 >= 0 && i2 < outline.size()) {
                const Vector2 s0 = world_to_screen(xform0.xform(outline[i0]), p_plot);
                const Vector2 s1 = world_to_screen(xform0.xform(outline[i1]), p_plot);
                const Vector2 s2 = world_to_screen(xform0.xform(outline[i2]), p_plot);
                const Color tri_col(0.95f, 0.84f, 0.36f, 0.35f);
                draw_line(s0, s1, tri_col, 1.0f);
                draw_line(s1, s2, tri_col, 1.0f);
                draw_line(s2, s0, tri_col, 1.0f);
            }
        }
    }
}

void TileFundamentalDomainCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return;

    Ref<Font> font = get_theme_default_font();
    const Vector2 origin_w = tiling->get_lattice_origin();
    const Vector2 origin_s = world_to_screen(origin_w, p_plot);

    // 1. Draw Lattice Basis Vectors T1 & T2 + Unit Parallelogram
    if (show_lattice_vectors_) {
        const Vector2 t1_w = origin_w + tiling->get_translation_vector_t1();
        const Vector2 t2_w = origin_w + tiling->get_translation_vector_t2();
        const Vector2 t12_w = t1_w + tiling->get_translation_vector_t2();

        const Vector2 t1_s  = world_to_screen(t1_w, p_plot);
        const Vector2 t2_s  = world_to_screen(t2_w, p_plot);
        const Vector2 t12_s = world_to_screen(t12_w, p_plot);

        const Color cell_box_col(0.88f, 0.72f, 0.30f, 0.38f);
        draw_dashed_line(t1_s, t12_s, cell_box_col, 1.25f, 5.0f);
        draw_dashed_line(t2_s, t12_s, cell_box_col, 1.25f, 5.0f);

        const Color t1_col(0.96f, 0.42f, 0.42f, 0.95f);
        const Color t2_col(0.42f, 0.92f, 0.52f, 0.95f);
        draw_line(origin_s, t1_s, t1_col, 2.2f);
        draw_line(origin_s, t2_s, t2_col, 2.2f);
        draw_circle(t1_s, 5.5f, t1_col);
        draw_circle(t2_s, 5.5f, t2_col);
        draw_circle(origin_s, 5.5f, Color(0.98f, 0.86f, 0.32f, 1.0f));

        if (font.is_valid()) {
            draw_string(font, t1_s + Vector2(6.0f, -4.0f), "T1", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, t1_col);
            draw_string(font, t2_s + Vector2(6.0f, -4.0f), "T2", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, t2_col);
            draw_string(font, origin_s + Vector2(6.0f, 12.0f), "O", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 10, Color(0.98f, 0.86f, 0.32f, 0.95f));
        }
    }

    // 2. Draw Symmetry Elements (Mirror Axes, Rotational Pivots, Glide Axes)
    if (show_symmetry_elements_) {
        const PackedVector2Array axes   = tiling->get_axes_of_symmetry();
        const PackedVector2Array pivots = tiling->get_points_of_rotation();
        const PackedVector2Array glides = tiling->get_glides();

        // Mirror reflection axes (pairs of points or direction vectors)
        for (int i = 0; i < axes.size(); ++i) {
            const Vector2 a_w = origin_w + axes[i];
            const Vector2 a_s = world_to_screen(a_w, p_plot);
            draw_dashed_line(origin_s, a_s, Color(0.35f, 0.92f, 0.96f, 0.85f), 1.6f, 6.0f);
            draw_circle(a_s, 4.5f, Color(0.35f, 0.92f, 0.96f, 1.0f));
        }

        // Glide reflection vectors
        for (int i = 0; i < glides.size(); ++i) {
            const Vector2 g_w = origin_w + glides[i];
            const Vector2 g_s = world_to_screen(g_w, p_plot);
            draw_dashed_line(origin_s, g_s, Color(0.96f, 0.62f, 0.28f, 0.85f), 1.6f, 4.0f);
            draw_rect(Rect2(g_s - Vector2(4.0f, 4.0f), Vector2(8.0f, 8.0f)), Color(0.96f, 0.62f, 0.28f, 1.0f), true);
        }

        // Rotational symmetry pivots (diamonds)
        for (int i = 0; i < pivots.size(); ++i) {
            const Vector2 p_w = origin_w + pivots[i];
            const Vector2 p_s = world_to_screen(p_w, p_plot);
            PackedVector2Array diamond;
            diamond.push_back(p_s + Vector2(0.0f, -6.0f));
            diamond.push_back(p_s + Vector2(6.0f, 0.0f));
            diamond.push_back(p_s + Vector2(0.0f, 6.0f));
            diamond.push_back(p_s + Vector2(-6.0f, 0.0f));
            PackedColorArray d_cols;
            d_cols.push_back(Color(0.86f, 0.48f, 0.98f, 0.95f));
            draw_polygon(diamond, d_cols);
        }
    }

    // 3. Draw Per-Edge J / U / S / I Constraint Badges on Primary Prototile
    if (show_edge_constraints_ && font.is_valid()) {
        const PackedVector2Array base_verts = tiling->get_base_prototile_vertices();
        const Transform2D xform0            = tiling->get_cell_transform_2d(0, 0, 0);
        const int v_count                   = base_verts.size();
        const auto &core_tiling             = tiling->get_core_tiling();

        for (int e = 0; e < v_count; ++e) {
            const Vector2 v0_s = world_to_screen(xform0.xform(base_verts[e]), p_plot);
            const Vector2 v1_s = world_to_screen(xform0.xform(base_verts[(e + 1) % v_count]), p_plot);
            const Vector2 mid_s = (v0_s + v1_s) * 0.5f;

            const int edge_shape_id = static_cast<int>(core_tiling.get_edge_shape_id(static_cast<size_t>(e)));
            const TilingEdgeShapeType shape_type = tiling->get_edge_shape_constraint(edge_shape_id);
            const bool is_sel_edge = (edge_shape_id == selected_edge_shape_id_);
            const bool is_hov_edge = (edge_shape_id == hovered_edge_shape_id_);

            if (is_sel_edge || is_hov_edge) {
                draw_line(
                    v0_s,
                    v1_s,
                    is_sel_edge ? Color(1.0f, 0.86f, 0.30f, 0.98f) : Color(0.65f, 0.90f, 1.0f, 0.90f),
                    is_sel_edge ? 3.4f : 2.6f
                );
            }

            const String badge_txt = String("E") + String::num_int64(edge_shape_id) + ":" + _edge_shape_badge_text(shape_type);
            const Color badge_col  = _edge_shape_badge_color(shape_type);
            const Rect2 pill_rect(mid_s.x - 28.0f, mid_s.y - 9.0f, 56.0f, 16.0f);
            draw_rect(
                pill_rect,
                is_sel_edge ? Color(0.22f, 0.20f, 0.10f, 0.92f) : Color(0.10f, 0.12f, 0.16f, 0.86f),
                true
            );
            draw_rect(
                pill_rect,
                is_sel_edge ? Color(1.0f, 0.86f, 0.30f, 1.0f) : badge_col,
                false,
                is_sel_edge ? 1.8f : 1.1f
            );
            draw_string(
                font,
                Vector2(pill_rect.position.x + 4.0f, pill_rect.position.y + 12.0f),
                badge_txt,
                HORIZONTAL_ALIGNMENT_LEFT,
                pill_rect.size.x - 6.0f,
                9,
                badge_col
            );
        }
    }

    // 4. Draw Top-Left Crystallographic HUD Banner
    if (font.is_valid()) {
        const String hud =
            tiling->get_iucr_symbol().to_upper() + " (" + tiling->get_orbifold_symbol() + ")" +
            "  |  IH" + String::num_int64(tiling->get_tactile_ih_type()).pad_zeros(2) +
            "  |  Aspects: " + String::num_int64(tiling->get_aspect_count()) +
            "  |  Edges: " + String::num_int64(tiling->get_edge_shape_count()) +
            "  |  Params: " + String::num_int64(tiling->get_parameter_count());
        const Rect2 banner(p_plot.position.x + 6.0f, p_plot.position.y + 6.0f, std::min(420.0f, p_plot.size.x - 12.0f), 20.0f);
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
// Interactive Custom Grips (`_try_begin_custom_drag` / `_update_custom_drag`)
// ============================================================================

bool TileFundamentalDomainCanvas::_try_begin_custom_drag(
    const godot::Ref<godot::InputEvent> &p_event,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_null() || !mb->is_pressed() || mb->get_button_index() != MOUSE_BUTTON_LEFT) {
        return false;
    }

    Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return false;

    const Vector2 mpos     = mb->get_position();
    const Vector2 origin_w = tiling->get_lattice_origin();
    const Vector2 origin_s = world_to_screen(origin_w, p_plot);

    // 1. Hit-test Lattice Basis Handles (T1, T2, Origin)
    if (show_lattice_vectors_) {
        const Vector2 t1_s = world_to_screen(origin_w + tiling->get_translation_vector_t1(), p_plot);
        if (mpos.distance_to(t1_s) <= handle_hit_radius + 2.0f) {
            _begin_snapshot_drag(BASE_DRAG_CUSTOM, 0, mpos, "Drag T1 Lattice Vector", GRIP_BASIS_T1);
            return true;
        }
        const Vector2 t2_s = world_to_screen(origin_w + tiling->get_translation_vector_t2(), p_plot);
        if (mpos.distance_to(t2_s) <= handle_hit_radius + 2.0f) {
            _begin_snapshot_drag(BASE_DRAG_CUSTOM, 0, mpos, "Drag T2 Lattice Vector", GRIP_BASIS_T2);
            return true;
        }
        if (mpos.distance_to(origin_s) <= handle_hit_radius + 2.0f) {
            _begin_snapshot_drag(BASE_DRAG_CUSTOM, 0, mpos, "Move Lattice Origin", GRIP_LATTICE_ORIGIN);
            return true;
        }
    }

    // 2. Hit-test Symmetry Handles (Rotational Pivots, Mirror Axes, Glides)
    if (show_symmetry_elements_) {
        const PackedVector2Array pivots = tiling->get_points_of_rotation();
        for (int i = 0; i < pivots.size(); ++i) {
            const Vector2 p_s = world_to_screen(origin_w + pivots[i], p_plot);
            if (mpos.distance_to(p_s) <= handle_hit_radius + 2.0f) {
                _begin_snapshot_drag(BASE_DRAG_CUSTOM, i, mpos, "Move Rotation Pivot", GRIP_ROTATION_PIVOT);
                return true;
            }
        }
        const PackedVector2Array axes = tiling->get_axes_of_symmetry();
        for (int i = 0; i < axes.size(); ++i) {
            const Vector2 a_s = world_to_screen(origin_w + axes[i], p_plot);
            if (mpos.distance_to(a_s) <= handle_hit_radius + 2.0f) {
                _begin_snapshot_drag(BASE_DRAG_CUSTOM, i, mpos, "Adjust Symmetry Axis", GRIP_SYMMETRY_AXIS);
                return true;
            }
        }
        const PackedVector2Array glides = tiling->get_glides();
        for (int i = 0; i < glides.size(); ++i) {
            const Vector2 g_s = world_to_screen(origin_w + glides[i], p_plot);
            if (mpos.distance_to(g_s) <= handle_hit_radius + 2.0f) {
                _begin_snapshot_drag(BASE_DRAG_CUSTOM, i, mpos, "Adjust Glide Reflection", GRIP_GLIDE_AXIS);
                return true;
            }
        }
    }

    // 3. Hit-test Edge Constraint Pills to select an `edge_shape_id`
    if (show_edge_constraints_) {
        const PackedVector2Array base_verts = tiling->get_base_prototile_vertices();
        const Transform2D xform0            = tiling->get_cell_transform_2d(0, 0, 0);
        const int v_count                   = base_verts.size();
        const auto &core_tiling             = tiling->get_core_tiling();

        for (int e = 0; e < v_count; ++e) {
            const Vector2 v0_s  = world_to_screen(xform0.xform(base_verts[e]), p_plot);
            const Vector2 v1_s  = world_to_screen(xform0.xform(base_verts[(e + 1) % v_count]), p_plot);
            const Vector2 mid_s = (v0_s + v1_s) * 0.5f;
            const Rect2 pill_rect(mid_s.x - 28.0f, mid_s.y - 9.0f, 56.0f, 16.0f);
            if (pill_rect.has_point(mpos)) {
                const int edge_shape_id = static_cast<int>(core_tiling.get_edge_shape_id(static_cast<size_t>(e)));
                set_selected_edge_shape_id(edge_shape_id);
                set_selected_section_index(e);
                emit_signal("section_selected", e);
                return true;
            }
        }
    }

    return false;
}

bool TileFundamentalDomainCanvas::_update_custom_drag(
    const godot::Ref<godot::InputEvent> &p_event,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_null()) return false;

    Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null()) return false;

    Vector2 target_w = screen_to_world(mm->get_position(), p_plot);
    if (mm->is_shift_pressed() && snap_step > 1e-5f) {
        target_w.x = std::round(target_w.x / snap_step) * snap_step;
        target_w.y = std::round(target_w.y / snap_step) * snap_step;
    }

    const Vector2 origin_w = tiling->get_lattice_origin();
    const Vector2 rel_w    = target_w - origin_w;

    switch (custom_drag_submode) {
        case GRIP_LATTICE_ORIGIN: {
            tiling->set_lattice_origin(target_w);
            drag_modified = true;
            return true;
        }
        case GRIP_BASIS_T1: {
            const float new_len = std::max(0.1f, rel_w.length());
            const float new_rot = std::atan2(rel_w.y, rel_w.x);
            Vector2 sc          = tiling->get_lattice_scale();
            sc.x                = new_len;
            tiling->set_lattice_scale(sc);
            tiling->set_lattice_rotation_radians(new_rot);
            drag_modified = true;
            return true;
        }
        case GRIP_BASIS_T2: {
            const float new_len = std::max(0.1f, rel_w.length());
            Vector2 sc          = tiling->get_lattice_scale();
            sc.y                = new_len;
            tiling->set_lattice_scale(sc);
            drag_modified = true;
            return true;
        }
        case GRIP_ROTATION_PIVOT: {
            PackedVector2Array pivots = tiling->get_points_of_rotation();
            if (drag_target_index >= 0 && drag_target_index < pivots.size()) {
                pivots.set(drag_target_index, rel_w);
                tiling->set_points_of_rotation(pivots);
                drag_modified = true;
                return true;
            }
            break;
        }
        case GRIP_SYMMETRY_AXIS: {
            PackedVector2Array axes = tiling->get_axes_of_symmetry();
            if (drag_target_index >= 0 && drag_target_index < axes.size()) {
                axes.set(drag_target_index, rel_w);
                tiling->set_axes_of_symmetry(axes);
                drag_modified = true;
                return true;
            }
            break;
        }
        case GRIP_GLIDE_AXIS: {
            PackedVector2Array glides = tiling->get_glides();
            if (drag_target_index >= 0 && drag_target_index < glides.size()) {
                glides.set(drag_target_index, rel_w);
                tiling->set_glides(glides);
                drag_modified = true;
                return true;
            }
            break;
        }
        default:
            break;
    }
    return false;
}

void TileFundamentalDomainCanvas::_update_custom_hover(
    const godot::Vector2 &p_mouse_pos,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<WallpaperTilingResource> tiling = get_wallpaper_tiling();
    if (tiling.is_null() || !show_edge_constraints_) {
        if (hovered_edge_shape_id_ != -1) {
            hovered_edge_shape_id_ = -1;
            queue_redraw();
        }
        return;
    }

    const PackedVector2Array base_verts = tiling->get_base_prototile_vertices();
    const Transform2D xform0            = tiling->get_cell_transform_2d(0, 0, 0);
    const int v_count                   = base_verts.size();
    const auto &core_tiling             = tiling->get_core_tiling();
    int new_hov                         = -1;

    for (int e = 0; e < v_count; ++e) {
        const Vector2 v0_s  = world_to_screen(xform0.xform(base_verts[e]), p_plot);
        const Vector2 v1_s  = world_to_screen(xform0.xform(base_verts[(e + 1) % v_count]), p_plot);
        const Vector2 mid_s = (v0_s + v1_s) * 0.5f;
        if (Rect2(mid_s.x - 28.0f, mid_s.y - 9.0f, 56.0f, 16.0f).has_point(p_mouse_pos)) {
            new_hov = static_cast<int>(core_tiling.get_edge_shape_id(static_cast<size_t>(e)));
            break;
        }
    }

    if (new_hov != hovered_edge_shape_id_) {
        hovered_edge_shape_id_ = new_hov;
        queue_redraw();
    }
}

} // namespace ideam::godot_ext

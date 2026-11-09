#pragma once

/**
 * spline_polygon_2d.h
 * SceneTree Closed-Loop 2D Spline Polygon Renderer, Offsetter, & Region Host (`godot::Node2D`).
 *
 * Consumes a `Ref<Spline2DResource>` and provides:
 *   1. Cached ear-clipping triangulation (`PolygonMath2D::triangulate_into`), optional inward/outward
 *      polygon buffering (`polygon_offset`, `offset_join_style` via `PolygonMath2D::offset_polygon`),
 *      textured/filled `_draw()` rendering, and antialiased perimeter stroke drawing.
 *   2. Automatic synchronization with a target `CollisionPolygon2D` (`collision_polygon_path`)
 *      so curved zones and territories automatically generate physics/area hitboxes.
 *   3. Lateral Handshake Slot (`tiling_resource`) so `IdeamTilesPlugin` / `TilingRegionNode2D`
 *      can fill the spline polygon's interior with any of the 17 Wallpaper Groups or 81 Isohedral Tilings.
 */

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "spline_2d_resource.h"

namespace ideam::godot_ext {

enum SplinePolygonJoinStyle : int32_t {
    POLYGON_JOIN_MITER = static_cast<int32_t>(core::JoinStyle2D::MITER),
    POLYGON_JOIN_BEVEL = static_cast<int32_t>(core::JoinStyle2D::BEVEL),
    POLYGON_JOIN_ROUND = static_cast<int32_t>(core::JoinStyle2D::ROUND)
};

class SplinePolygon2D : public godot::Node2D {
    GDCLASS(SplinePolygon2D, godot::Node2D)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<Spline2DResource> spline;

    // Lateral Handshake Slot for IdeamTiles (WallpaperTilingResource / CompositeTilingResource)
    godot::Ref<godot::Resource> tiling_resource;

    // --- Tesselation & Offset Settings ---
    int subdivisions                      = 12;
    real_t polygon_offset                 = static_cast<real_t>(0.0);
    SplinePolygonJoinStyle join_style     = POLYGON_JOIN_MITER;
    real_t miter_limit                    = static_cast<real_t>(2.0);

    // --- Fill, Texture & Border Appearance ---
    bool draw_fill                        = true;
    godot::Color fill_color               = godot::Color(0.24f, 0.56f, 0.88f, 0.45f);
    godot::Ref<godot::Texture2D> texture;
    godot::Vector2 uv_scale               = godot::Vector2(1.0f, 1.0f);
    godot::Vector2 uv_offset              = godot::Vector2(0.0f, 0.0f);
    real_t uv_rotation_radians            = static_cast<real_t>(0.0);

    bool draw_border                      = true;
    float border_width                    = 2.0f;
    godot::Color border_color             = godot::Color(0.38f, 0.82f, 1.0f, 0.95f);

    // --- SceneTree Collision Sync ---
    bool auto_sync_collision              = true;
    godot::NodePath collision_polygon_path;

    // --- Cached Geometry State ---
    mutable godot::PackedVector2Array cached_polygon;
    mutable godot::PackedVector2Array cached_uvs;
    mutable godot::PackedInt32Array cached_triangles;
    mutable godot::Rect2 cached_bounds;
    mutable real_t cached_signed_area     = static_cast<real_t>(0.0);
    mutable real_t cached_perimeter       = static_cast<real_t>(0.0);
    mutable godot::Vector2 cached_centroid;
    mutable bool cache_dirty              = true;

    void _on_spline_changed();
    void _on_tiling_changed();
    void _rebuild_geometry_cache() const;
    void _sync_collision_polygon();

public:
    SplinePolygon2D();
    ~SplinePolygon2D() override;

    void _ready() override;
    void _draw() override;

    // ========================================================================
    // Resource & Lateral Handshake Configuration
    // ========================================================================

    void set_spline(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_spline() const { return spline; }

    void set_tiling_resource(const godot::Ref<godot::Resource> &p_tiling);
    [[nodiscard]] godot::Ref<godot::Resource> get_tiling_resource() const { return tiling_resource; }

    // ========================================================================
    // Tesselation & Offset Properties
    // ========================================================================

    void set_subdivisions(int p_subdivisions);
    [[nodiscard]] int get_subdivisions() const noexcept { return subdivisions; }

    void set_polygon_offset(real_t p_offset);
    [[nodiscard]] real_t get_polygon_offset() const noexcept { return polygon_offset; }

    void set_join_style(SplinePolygonJoinStyle p_style);
    [[nodiscard]] SplinePolygonJoinStyle get_join_style() const noexcept { return join_style; }

    void set_miter_limit(real_t p_limit);
    [[nodiscard]] real_t get_miter_limit() const noexcept { return miter_limit; }

    // ========================================================================
    // Visual Appearance Properties
    // ========================================================================

    void set_draw_fill(bool p_draw);
    [[nodiscard]] bool get_draw_fill() const noexcept { return draw_fill; }

    void set_fill_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_fill_color() const noexcept { return fill_color; }

    void set_texture(const godot::Ref<godot::Texture2D> &p_texture);
    [[nodiscard]] godot::Ref<godot::Texture2D> get_texture() const { return texture; }

    void set_uv_scale(const godot::Vector2 &p_scale);
    [[nodiscard]] godot::Vector2 get_uv_scale() const noexcept { return uv_scale; }

    void set_uv_offset(const godot::Vector2 &p_offset);
    [[nodiscard]] godot::Vector2 get_uv_offset() const noexcept { return uv_offset; }

    void set_uv_rotation_radians(real_t p_rad);
    [[nodiscard]] real_t get_uv_rotation_radians() const noexcept { return uv_rotation_radians; }

    void set_draw_border(bool p_draw);
    [[nodiscard]] bool get_draw_border() const noexcept { return draw_border; }

    void set_border_width(float p_width);
    [[nodiscard]] float get_border_width() const noexcept { return border_width; }

    void set_border_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_border_color() const noexcept { return border_color; }

    // ========================================================================
    // Collision Sync Configuration
    // ========================================================================

    void set_auto_sync_collision(bool p_sync);
    [[nodiscard]] bool get_auto_sync_collision() const noexcept { return auto_sync_collision; }

    void set_collision_polygon_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_collision_polygon_path() const { return collision_polygon_path; }

    // ========================================================================
    // Cached Geometry & Spatial Query API
    // ========================================================================

    [[nodiscard]] godot::PackedVector2Array get_cached_polygon() const;
    [[nodiscard]] godot::PackedInt32Array get_cached_triangles() const;
    [[nodiscard]] godot::Rect2 get_bounding_rect() const;
    [[nodiscard]] real_t get_signed_area() const;
    [[nodiscard]] real_t get_abs_area() const;
    [[nodiscard]] real_t get_perimeter() const;
    [[nodiscard]] godot::Vector2 get_centroid() const;

    [[nodiscard]] bool contains_local_point(const godot::Vector2 &p_local_pt) const;
    [[nodiscard]] bool contains_global_point(const godot::Vector2 &p_global_pt) const;
    [[nodiscard]] godot::PackedVector2Array clip_convex_polygon(const godot::PackedVector2Array &p_convex_clip) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplinePolygonJoinStyle);
#pragma once

/**
 * polygon_2d_resource.h
 * Concrete Godot Resource wrapping `ideam::core::PolygonMath2D<real_t>` and
 * `ideam::core::CSGMath2D<real_t>` (`src/godot/geometry/resources/`).
 *
 * Provides a shared, curve-agnostic 2D polygon/polyline data container that can be:
 *   - Produced by flattening a `Spline2DResource` or extracting a `TilingRegion2D` border.
 *   - Used as a boundary clip polygon or CSG operand in `IdeamSplines`, `IdeamTiles`, and `GeometryDOD`.
 *   - Edited directly in the Godot Inspector via `Polygon2DControl` / `Polygon2DCanvas` (`GeometryCanvas2DBase`).
 */

#include "../geometry_resource.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace ideam::godot_ext {

class Polygon2DResource : public GeometryResource {
    GDCLASS(Polygon2DResource, GeometryResource)

private:
    using PolyMath = core::PolygonMath2D<real_t>;
    using CSGMath  = core::CSGMath2D<real_t>;
    using Vec2     = typename PolyMath::Vec2;

    std::vector<Vec2> vertices_;
    bool closed_ = true;

    mutable std::vector<uint32_t> cached_triangles_;
    mutable bool triangles_dirty_ = true;

    void _mark_dirty();
    void _ensure_triangulated() const;

    [[nodiscard]] static inline Vec2 _to_glm(const godot::Vector2 &p_v) noexcept {
        return Vec2(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y));
    }
    [[nodiscard]] static inline godot::Vector2 _to_godot(const Vec2 &p_v) noexcept {
        return godot::Vector2(static_cast<float>(p_v.x), static_cast<float>(p_v.y));
    }
    [[nodiscard]] static std::vector<Vec2> _to_glm_vector(const godot::PackedVector2Array &p_arr);
    [[nodiscard]] static godot::PackedVector2Array _to_godot_array(std::span<const Vec2> p_span);

protected:
    static void _bind_methods();

public:
    Polygon2DResource() = default;
    ~Polygon2DResource() override = default;

    // --- Direct C++ Core Accessors (Zero-Copy Span for GeometryDOD / SplineDOD / TileDOD) ---
    [[nodiscard]] std::span<const Vec2> get_vertices_span() const noexcept { return vertices_; }
    [[nodiscard]] std::span<const uint32_t> get_triangles_span() const {
        _ensure_triangulated();
        return cached_triangles_;
    }

    // --- GeometryResource Overrides ---
    [[nodiscard]] int get_geometry_dimension() const override { return 2; }
    [[nodiscard]] godot::Rect2 get_bounds_2d() const override;
    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // --- Vertex & Topology Mutators / Accessors ---
    void set_vertices(const godot::PackedVector2Array &p_vertices);
    [[nodiscard]] godot::PackedVector2Array get_vertices() const;

    void set_closed(bool p_closed);
    [[nodiscard]] bool is_closed() const noexcept { return closed_; }

    [[nodiscard]] int get_vertex_count() const noexcept { return static_cast<int>(vertices_.size()); }
    void add_vertex(const godot::Vector2 &p_vertex);
    void insert_vertex(int p_index, const godot::Vector2 &p_vertex);
    void set_vertex(int p_index, const godot::Vector2 &p_vertex);
    [[nodiscard]] godot::Vector2 get_vertex(int p_index) const;
    void remove_vertex(int p_index);
    void clear();

    // --- PolygonMath2D Queries & Operations ---
    [[nodiscard]] bool contains_point(const godot::Vector2 &p_point) const;
    [[nodiscard]] real_t get_signed_area() const;
    [[nodiscard]] real_t get_abs_area() const;
    [[nodiscard]] real_t get_perimeter() const;
    [[nodiscard]] godot::Vector2 get_centroid() const;
    [[nodiscard]] GeometryWindingOrder get_winding_order() const;
    void ensure_winding(GeometryWindingOrder p_desired = WINDING_COUNTER_CLOCKWISE);

    [[nodiscard]] real_t distance_to_boundary(const godot::Vector2 &p_point) const;
    [[nodiscard]] godot::Vector2 closest_point_on_boundary(const godot::Vector2 &p_point) const;

    [[nodiscard]] godot::PackedInt32Array get_triangles() const;
    [[nodiscard]] godot::PackedVector2Array compute_convex_hull() const;
    void replace_with_convex_hull();
    [[nodiscard]] godot::PackedVector2Array compute_offset_polygon(
        real_t p_delta,
        GeometryJoinStyle2D p_join = JOIN_STYLE_MITER,
        real_t p_miter_limit = static_cast<real_t>(2.0)
    ) const;
    void apply_offset(
        real_t p_delta,
        GeometryJoinStyle2D p_join = JOIN_STYLE_MITER,
        real_t p_miter_limit = static_cast<real_t>(2.0)
    );

    // --- CSGMath2D Clipping & Classification ---
    [[nodiscard]] GeometryClipClassification2D classify_against_half_plane(
        const godot::Vector2 &p_plane_origin,
        const godot::Vector2 &p_inside_normal
    ) const;
    [[nodiscard]] GeometryClipClassification2D classify_against_boundary(
        const godot::PackedVector2Array &p_boundary_polygon
    ) const;
    [[nodiscard]] godot::PackedVector2Array clip_half_plane(
        const godot::Vector2 &p_plane_origin,
        const godot::Vector2 &p_inside_normal
    ) const;
    [[nodiscard]] godot::PackedVector2Array clip_convex(
        const godot::PackedVector2Array &p_convex_clip
    ) const;

    // --- Parametric Polygon Shape Generators ---
    void create_rectangle(const godot::Vector2 &p_center, const godot::Vector2 &p_size);
    void create_regular_polygon(
        int p_sides = 6,
        real_t p_radius = static_cast<real_t>(0.5),
        real_t p_angle_offset = static_cast<real_t>(0.0),
        bool p_reverse_winding = true
    );
    void create_star(
        int p_sides = 5,
        real_t p_outer_radius = static_cast<real_t>(0.5),
        real_t p_inner_radius = static_cast<real_t>(0.25),
        real_t p_angle_offset = static_cast<real_t>(0.0)
    );

    // --- IdeamUndoRedo Commit Wrappers ---
    void commit_set_closed(bool p_closed);
    void commit_clear();
    void commit_add_vertex(const godot::Vector2 &p_vertex);
    void commit_insert_vertex(int p_index, const godot::Vector2 &p_vertex);
    void commit_set_vertex(int p_index, const godot::Vector2 &p_vertex);
    void commit_remove_vertex(int p_index);
    void commit_apply_offset(
        real_t p_delta,
        GeometryJoinStyle2D p_join = JOIN_STYLE_MITER,
        real_t p_miter_limit = static_cast<real_t>(2.0)
    );
    void commit_ensure_winding(GeometryWindingOrder p_desired = WINDING_COUNTER_CLOCKWISE);
    void commit_replace_with_convex_hull();
    void commit_create_rectangle(const godot::Vector2 &p_center, const godot::Vector2 &p_size);
    void commit_create_regular_polygon(
        int p_sides = 6,
        real_t p_radius = static_cast<real_t>(0.5),
        real_t p_angle_offset = static_cast<real_t>(0.0),
        bool p_reverse_winding = true
    );
    void commit_create_star(
        int p_sides = 5,
        real_t p_outer_radius = static_cast<real_t>(0.5),
        real_t p_inner_radius = static_cast<real_t>(0.25),
        real_t p_angle_offset = static_cast<real_t>(0.0)
    );
    void commit_clip_half_plane(
        const godot::Vector2 &p_plane_origin,
        const godot::Vector2 &p_inside_normal
    );
    void commit_clip_convex(const godot::PackedVector2Array &p_convex_clip);
};

} // namespace ideam::godot_ext
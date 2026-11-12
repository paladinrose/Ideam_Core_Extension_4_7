#pragma once

/**
 * voronoi_diagram_2d_resource.h
 * Concrete Godot Resource wrapping `ideam::core::VoronoiMath2D<real_t>`
 * (`src/godot/geometry/resources/`).
 *
 * Bridges 2D point-set Delaunay triangulation, clipped Voronoi cells (clipped against
 * `clip_bounds` and an optional `Polygon2DResource` `clip_polygon`), Lloyd centroidal
 * relaxation, procedural grid/Poisson site generators, and Compressed Sparse Row (CSR)
 * dual adjacency extraction directly to:
 *   - `NarrativeDOD::rebuild_location_adjacency_csr` (procedural territory graphs for `Location` narremes)
 *   - `IdeamTiles` / `IdeamGames` (cellular biome generation, tactical zones, mesh shattering)
 *   - `GeometryDOD` (`GEOM_PLANAR_HALF_EDGE_POD` & `GEOM_POLYGON_MESH_SLICE_POD` buffers)
 */

#include "../geometry_resource.h"
#include "polygon_2d_resource.h"
#include "../../../core/geometry/voronoi_2d.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace ideam::godot_ext {

class VoronoiDiagram2DResource : public GeometryResource {
    GDCLASS(VoronoiDiagram2DResource, GeometryResource)

private:
    using VorMath  = core::VoronoiMath2D<real_t>;
    using PolyMath = core::PolygonMath2D<real_t>;
    using CSGMath  = core::CSGMath2D<real_t>;
    using Vec2     = typename VorMath::Vec2;

    std::vector<Vec2> sites_;
    godot::Rect2 clip_bounds_ = godot::Rect2(-5.0f, -5.0f, 10.0f, 10.0f);
    godot::Ref<Polygon2DResource> clip_polygon_;

    mutable std::vector<uint32_t> cached_delaunay_indices_;
    mutable std::vector<core::VoronoiCell2D<real_t>> cached_cells_;
    mutable std::vector<uint32_t> cached_csr_row_offsets_;
    mutable std::vector<uint32_t> cached_csr_col_neighbors_;
    mutable bool cache_dirty_ = true;

    void _mark_dirty();
    void _on_clip_polygon_changed();
    void _ensure_built() const;

    [[nodiscard]] static inline Vec2 _to_glm(const godot::Vector2 &p_v) noexcept {
        return Vec2(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y));
    }
    [[nodiscard]] static inline godot::Vector2 _to_godot(const Vec2 &p_v) noexcept {
        return godot::Vector2(static_cast<float>(p_v.x), static_cast<float>(p_v.y));
    }

protected:
    static void _bind_methods();

public:
    VoronoiDiagram2DResource() = default;
    ~VoronoiDiagram2DResource() override;

    // --- Direct C++ Core Accessors (For GeometryDOD & NarrativeDOD CSR Bridging) ---
    [[nodiscard]] std::span<const Vec2> get_sites_span() const noexcept { return sites_; }
    [[nodiscard]] std::span<const core::VoronoiCell2D<real_t>> get_cells_span() const {
        _ensure_built();
        return cached_cells_;
    }
    [[nodiscard]] std::span<const uint32_t> get_csr_row_offsets_span() const {
        _ensure_built();
        return cached_csr_row_offsets_;
    }
    [[nodiscard]] std::span<const uint32_t> get_csr_col_neighbors_span() const {
        _ensure_built();
        return cached_csr_col_neighbors_;
    }

    // --- GeometryResource Overrides ---
    [[nodiscard]] int get_geometry_dimension() const override { return 2; }
    [[nodiscard]] godot::Rect2 get_bounds_2d() const override { return clip_bounds_; }
    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // --- Site, Clip Bounds & Optional Polygon2D Clipping Mutators / Accessors ---
    void set_sites(const godot::PackedVector2Array &p_sites);
    [[nodiscard]] godot::PackedVector2Array get_sites() const;

    void set_clip_bounds(const godot::Rect2 &p_bounds);
    [[nodiscard]] godot::Rect2 get_clip_bounds() const noexcept { return clip_bounds_; }

    void set_clip_polygon(const godot::Ref<Polygon2DResource> &p_polygon);
    [[nodiscard]] godot::Ref<Polygon2DResource> get_clip_polygon() const { return clip_polygon_; }

    [[nodiscard]] int get_site_count() const noexcept { return static_cast<int>(sites_.size()); }
    void add_site(const godot::Vector2 &p_site);
    void set_site(int p_index, const godot::Vector2 &p_site);
    [[nodiscard]] godot::Vector2 get_site(int p_index) const;
    void remove_site(int p_index);
    void clear();

    // --- Procedural Site Generators (Grid + Jitter & Blue-Noise / Poisson) ---
    void generate_grid_sites(
        int p_columns = 4,
        int p_rows = 4,
        real_t p_jitter = static_cast<real_t>(0.35),
        int64_t p_seed = 1337
    );
    void generate_poisson_sites(
        int p_target_count = 16,
        real_t p_min_distance = static_cast<real_t>(1.2),
        int64_t p_seed = 1337
    );

    // --- Delaunay, Voronoi & CSR Adjacency Queries ---
    [[nodiscard]] godot::PackedInt32Array get_delaunay_triangles() const;
    [[nodiscard]] godot::PackedVector2Array get_cell_polygon(int p_site_index) const;
    [[nodiscard]] godot::Vector2 get_cell_centroid(int p_site_index) const;
    [[nodiscard]] real_t get_cell_area(int p_site_index) const;
    [[nodiscard]] int find_containing_cell(const godot::Vector2 &p_point) const;
    [[nodiscard]] godot::Ref<Polygon2DResource> export_cell_as_polygon_resource(int p_site_index) const;

    [[nodiscard]] godot::PackedInt32Array get_site_neighbors(int p_site_index) const;
    [[nodiscard]] godot::PackedInt32Array get_csr_row_offsets() const;
    [[nodiscard]] godot::PackedInt32Array get_csr_col_neighbors() const;

    // --- Lloyd Centroidal Voronoi Relaxation ---
    void apply_lloyd_relaxation_step(real_t p_step_strength = static_cast<real_t>(1.0));

    // --- IdeamUndoRedo Commit Wrappers ---
    void commit_set_clip_bounds(const godot::Rect2 &p_bounds);
    void commit_set_clip_polygon(const godot::Ref<Polygon2DResource> &p_polygon);
    void commit_clear();
    void commit_add_site(const godot::Vector2 &p_site);
    void commit_set_site(int p_index, const godot::Vector2 &p_site);
    void commit_remove_site(int p_index);
    void commit_lloyd_relaxation_step(real_t p_step_strength = static_cast<real_t>(1.0));
    void commit_generate_grid_sites(
        int p_columns = 4,
        int p_rows = 4,
        real_t p_jitter = static_cast<real_t>(0.35),
        int64_t p_seed = 1337
    );
    void commit_generate_poisson_sites(
        int p_target_count = 16,
        real_t p_min_distance = static_cast<real_t>(1.2),
        int64_t p_seed = 1337
    );
};

} // namespace ideam::godot_ext
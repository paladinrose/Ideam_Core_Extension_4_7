#pragma once

#include "godot/geometry/geometry_resource.h"
#include "godot/geometry/resources/polygon_2d_resource.h"
#include "godot/geometry/resources/sdf_grid_2d_resource.h"
#include "godot/controls/heatmap_grid.h"
#include "core/geometry/tensor_field_2d.h"

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <span>
#include <vector>

namespace ideam::godot_ext {

// ============================================================================
// TensorField2DResource
// Godot-native 2D Symmetric Tensor Field Resource with double-angle basis math,
// topological singularity detection, RK4 hyperstreamline tracing, and closed
// DCEL city-block/parcel region extraction.
// ============================================================================
class TensorField2DResource : public GeometryResource {
    GDCLASS(TensorField2DResource, GeometryResource)

public:
    enum BasisType : int {
        BASIS_GRIDLINE          = 0,
        BASIS_RADIAL            = 1,
        BASIS_VORTEX            = 2,
        BASIS_POLYLINE          = 3,
        BASIS_SDF_GRADIENT      = 4,
        BASIS_WEDGE_SINGULARITY = 5,
        BASIS_TRISECTOR         = 6
    };

    enum BlendMode : int {
        BLEND_WEIGHTED_AVERAGE = 0,
        BLEND_ADDITIVE         = 1
    };

private:
    int grid_width_ = 64;
    int grid_height_ = 64;
    godot::Rect2 domain_bounds_ = godot::Rect2(0.0f, 0.0f, 512.0f, 512.0f);
    BlendMode blend_mode_ = BLEND_WEIGHTED_AVERAGE;

    float step_size_ = 4.0f;
    float min_streamline_separation_ = 16.0f;
    float simplify_tolerance_ = 0.75f;
    int max_streamline_steps_ = 256;
    bool trace_minor_streamlines_ = true;
    bool extract_closed_parcels_ = true;

    // Contiguous DOD Basis & Evaluated Grid Storage
    std::vector<core::TensorBasisElementPod> basis_elements_;
    std::vector<core::PolylineSlicePOD> basis_polyline_slices_;
    std::vector<glm::vec2> basis_polyline_vertices_;
    godot::Ref<SDFGrid2DResource> sdf_source_;

    std::vector<core::SymmetricTensor2DPod> tensor_cells_;
    std::vector<glm::vec2> major_eigenvectors_;
    std::vector<glm::vec2> minor_eigenvectors_;
    std::vector<core::TensorSingularityPod> singularities_;
    std::vector<core::TensorStreamlineSeedPod> seeds_;

    core::TensorNetworkResult2D traced_network_;
    bool network_dirty_ = true;

    void _rebuild_grid();
    void _rebuild_network_if_needed();

protected:
    static void _bind_methods();

public:
    TensorField2DResource();
    ~TensorField2DResource() override = default;

    // --- Grid & Tracing Configuration ---
    void set_grid_width(int p_width);
    int get_grid_width() const { return grid_width_; }

    void set_grid_height(int p_height);
    int get_grid_height() const { return grid_height_; }

    void set_domain_bounds(const godot::Rect2 &p_bounds);
    godot::Rect2 get_domain_bounds() const { return domain_bounds_; }

    void set_blend_mode(BlendMode p_mode);
    BlendMode get_blend_mode() const { return blend_mode_; }

    void set_step_size(float p_step);
    float get_step_size() const { return step_size_; }

    void set_min_streamline_separation(float p_sep);
    float get_min_streamline_separation() const { return min_streamline_separation_; }

    void set_simplify_tolerance(float p_tol);
    float get_simplify_tolerance() const { return simplify_tolerance_; }

    void set_max_streamline_steps(int p_steps);
    int get_max_streamline_steps() const { return max_streamline_steps_; }

    void set_trace_minor_streamlines(bool p_enable);
    bool get_trace_minor_streamlines() const { return trace_minor_streamlines_; }

    void set_extract_closed_parcels(bool p_enable);
    bool get_extract_closed_parcels() const { return extract_closed_parcels_; }

    void set_sdf_source(const godot::Ref<SDFGrid2DResource> &p_sdf);
    godot::Ref<SDFGrid2DResource> get_sdf_source() const { return sdf_source_; }

    // --- Godot-Native Basis Element Builders ---
    void clear_basis_elements();
    int get_basis_element_count() const { return static_cast<int>(basis_elements_.size()); }

    void add_gridline_basis(const godot::Vector2 &p_origin, const godot::Vector2 &p_direction, float p_weight = 1.0f, float p_decay = 0.005f, float p_radius = 0.0f);
    void add_transform_basis(const godot::Transform2D &p_transform, float p_weight = 1.0f, float p_decay = 0.005f, float p_radius = 0.0f);
    void add_radial_basis(const godot::Vector2 &p_center, float p_weight = 1.0f, float p_decay = 0.008f, float p_radius = 0.0f);
    void add_vortex_basis(const godot::Vector2 &p_center, float p_weight = 1.0f, float p_decay = 0.008f, float p_radius = 0.0f);
    void add_polyline_basis(const godot::PackedVector2Array &p_polyline, float p_weight = 1.5f, float p_decay = 0.01f, float p_radius = 0.0f);
    void add_sdf_gradient_basis(float p_weight = 1.0f, float p_decay = 0.0f);
    void add_wedge_singularity_basis(const godot::Vector2 &p_center, float p_orientation_rad = 0.0f, float p_weight = 1.0f, float p_decay = 0.008f);
    void add_trisector_singularity_basis(const godot::Vector2 &p_center, float p_orientation_rad = 0.0f, float p_weight = 1.0f, float p_decay = 0.008f);

    // --- Godot-Native Field Queries ---
    // Returns Vector4(a, b, trace, weight) where a = R*cos(2*theta), b = R*sin(2*theta)
    godot::Vector4 sample_tensor_vec4(const godot::Vector2 &p_world_pos) const;
    godot::Vector2 sample_major_eigenvector(const godot::Vector2 &p_world_pos) const;
    godot::Vector2 sample_minor_eigenvector(const godot::Vector2 &p_world_pos) const;
    float sample_anisotropy(const godot::Vector2 &p_world_pos) const;
    // Returns a Transform2D whose X basis is the major eigenvector and Y basis is the minor eigenvector at p_world_pos
    godot::Transform2D sample_eigen_transform(const godot::Vector2 &p_world_pos) const;

    // --- Topological Singularities & Hyperstreamlines ---
    void bake_field_and_streamlines();
    godot::TypedArray<godot::Dictionary> get_singularities() const;
    godot::TypedArray<godot::PackedVector2Array> get_major_streamlines();
    godot::TypedArray<godot::PackedVector2Array> get_minor_streamlines();
    godot::TypedArray<godot::PackedVector2Array> get_closed_parcel_polygons();
    godot::TypedArray<Polygon2DResource> bake_parcel_polygon_resources();
    godot::Ref<HeatmapGrid> to_anisotropy_heatmap() const;

    // --- GeometryResource Overrides & Helpers ---
    [[nodiscard]] int get_geometry_dimension() const override { return 2; }
    [[nodiscard]] godot::Rect2 get_bounds_2d() const override { return domain_bounds_; }
    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    godot::Rect2 get_bounding_box() const { return domain_bounds_; }
    bool intersects_point(const godot::Vector2 &p_point) const { return domain_bounds_.has_point(p_point); }
    godot::Vector2 get_closest_point(const godot::Vector2 &p_to_point) const;
    godot::PackedVector2Array get_render_Wireframe() const;

    // --- Direct Core DOD Accessors for DataOrientedGeometry ---
    core::TensorGridHeaderPod get_core_grid_header() const;
    const std::vector<core::TensorBasisElementPod> &get_core_basis_elements() const { return basis_elements_; }
    const std::vector<glm::vec2> &get_core_basis_polyline_vertices() const { return basis_polyline_vertices_; }
    const std::vector<core::SymmetricTensor2DPod> &get_core_tensor_cells() const { return tensor_cells_; }
    const std::vector<glm::vec2> &get_core_major_eigenvectors() const { return major_eigenvectors_; }
    const std::vector<glm::vec2> &get_core_minor_eigenvectors() const { return minor_eigenvectors_; }
    const std::vector<core::TensorSingularityPod> &get_core_singularities() const { return singularities_; }
    const std::vector<core::TensorStreamlineSeedPod> &get_core_seeds() const { return seeds_; }
    const core::TensorNetworkResult2D &get_core_traced_network() const { return traced_network_; }

    void sync_from_dod_results(
        const core::TensorGridHeaderPod &p_header,
        std::span<const core::SymmetricTensor2DPod> p_cells,
        std::span<const glm::vec2> p_major,
        std::span<const glm::vec2> p_minor,
        std::span<const core::TensorSingularityPod> p_singularities
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TensorField2DResource::BasisType);
VARIANT_ENUM_CAST(ideam::godot_ext::TensorField2DResource::BlendMode);

#include "godot/geometry/resources/tensor_field_2d_resource.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>
#include <cmath>

namespace godot {

TensorField2DResource::TensorField2DResource() {
    // Default demonstration basis: an urban gridline + radial plaza center
    add_gridline_basis(Vector2(256.0f, 256.0f), Vector2(1.0f, 0.15f), 1.0f, 0.002f, 0.0f);
    add_radial_basis(Vector2(200.0f, 220.0f), 1.35f, 0.009f, 240.0f);
}

ideam::geometry::TensorGridHeaderPod TensorField2DResource::get_core_grid_header() const {
    ideam::geometry::TensorGridHeaderPod hdr{};
    hdr.width = static_cast<uint32_t>(std::max(2, grid_width_));
    hdr.height = static_cast<uint32_t>(std::max(2, grid_height_));
    hdr.origin_x = domain_bounds_.position.x;
    hdr.origin_y = domain_bounds_.position.y;
    const float w = std::max(1.0f, domain_bounds_.size.x);
    const float h = std::max(1.0f, domain_bounds_.size.y);
    hdr.cell_size_x = w / static_cast<float>(hdr.width - 1);
    hdr.cell_size_y = h / static_cast<float>(hdr.height - 1);
    hdr.inv_cell_size_x = 1.0f / hdr.cell_size_x;
    hdr.inv_cell_size_y = 1.0f / hdr.cell_size_y;
    return hdr;
}

void TensorField2DResource::_rebuild_grid() {
    const auto hdr = get_core_grid_header();
    const size_t total = static_cast<size_t>(hdr.width) * static_cast<size_t>(hdr.height);
    tensor_cells_.resize(total);
    major_eigenvectors_.resize(total);
    minor_eigenvectors_.resize(total);

    std::span<const float> sdf_span{};
    if (sdf_source_.is_valid() && sdf_source_->get_width() == static_cast<int>(hdr.width) &&
        sdf_source_->get_height() == static_cast<int>(hdr.height)) {
        const auto &raw_sdf = sdf_source_->get_core_sdf_grid().distances;
        sdf_span = std::span<const float>(raw_sdf.data(), raw_sdf.size());
    }

    std::vector<ideam::geometry::PolylineSlicePOD> poly_slices;
    for (const auto &b : basis_elements_) {
        if (b.basis_type == static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_POLYLINE_ATTRACTOR) &&
            b.polyline_vertex_count >= 2) {
            poly_slices.push_back(ideam::geometry::PolylineSlicePOD{
                b.polyline_Start_idx,
                b.polyline_vertex_count,
                0u,
                0.0f
            });
        }
    }

    ideam::geometry::TensorFieldMath2D::evaluate_grid(
        hdr,
        basis_elements_,
        poly_slices,
        basis_polyline_vertices_,
        sdf_span,
        blend_mode_ == BLEND_WEIGHTED_AVERAGE,
        tensor_cells_,
        major_eigenvectors_,
        minor_eigenvectors_
    );

    singularities_.clear();
    ideam::geometry::TensorFieldMath2D::detect_singularities(
        hdr,
        tensor_cells_,
        1e-4f,
        singularities_
    );

    seeds_.clear();
    const uint32_t stride = std::max(1u, static_cast<uint32_t>(std::round(min_streamline_separation_ * hdr.inv_cell_size_x)));
    for (const auto &s : singularities_) {
        if (s.singularity_type == static_cast<uint32_t>(ideam::geometry::TensorSingularityType::SINGULARITY_NONE)) continue;
        seeds_.push_back(ideam::geometry::TensorStreamlineSeedPod{
            s.world_x, s.world_y, 2.0f, 1u
        });
    }
    for (uint32_t gy = stride / 2; gy < hdr.height; gy += stride) {
        for (uint32_t gx = stride / 2; gx < hdr.width; gx += stride) {
            const auto &cell = tensor_cells_[static_cast<size_t>(gy) * hdr.width + gx];
            const glm::vec2 wp = hdr.grid_to_world(static_cast<float>(gx), static_cast<float>(gy));
            seeds_.push_back(ideam::geometry::TensorStreamlineSeedPod{
                wp.x, wp.y, cell.anisotropy(), 1u
            });
        }
    }

    network_dirty_ = true;
}

void TensorField2DResource::_rebuild_network_if_needed() {
    if (tensor_cells_.empty()) {
        _rebuild_grid();
    }
    if (!network_dirty_) {
        return;
    }

    const auto hdr = get_core_grid_header();
    ideam::geometry::TensorTracingConfigPod cfg{};
    cfg.step_size = std::max(0.5f, step_size_);
    cfg.min_separation_distance = std::max(2.0f, min_streamline_separation_);
    cfg.collinear_Cos_threshold = std::clamp(1.0f - (simplify_tolerance_ * 0.01f), 0.90f, 0.9999f);
    cfg.dangling_edge_prune_length = min_streamline_separation_ * 0.75f;
    cfg.max_steps = static_cast<uint32_t>(std::max(8, max_streamline_steps_));
    cfg.trace_minor_field = trace_minor_streamlines_ ? 1u : 0u;
    cfg.extract_closed_regions = extract_closed_parcels_ ? 1u : 0u;
    cfg.min_region_vertices = 3u;

    ideam::geometry::TensorFieldMath2D::build_hyperstreamline_network(
        hdr,
        tensor_cells_,
        seeds_,
        cfg,
        traced_network_
    );
    network_dirty_ = false;
}

void TensorField2DResource::set_grid_width(int p_width) {
    grid_width_ = std::clamp(p_width, 4, 1024);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::set_grid_height(int p_height) {
    grid_height_ = std::clamp(p_height, 4, 1024);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::set_domain_bounds(const Rect2 &p_bounds) {
    domain_bounds_ = p_bounds;
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::set_blend_mode(BlendMode p_mode) {
    blend_mode_ = p_mode;
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::set_step_size(float p_step) {
    step_size_ = std::max(0.25f, p_step);
    network_dirty_ = true;
    emit_changed();
}

void TensorField2DResource::set_min_streamline_separation(float p_sep) {
    min_streamline_separation_ = std::max(2.0f, p_sep);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::set_simplify_tolerance(float p_tol) {
    simplify_tolerance_ = std::max(0.0f, p_tol);
    network_dirty_ = true;
    emit_changed();
}

void TensorField2DResource::set_max_streamline_steps(int p_steps) {
    max_streamline_steps_ = std::clamp(p_steps, 8, 4096);
    network_dirty_ = true;
    emit_changed();
}

void TensorField2DResource::set_trace_minor_streamlines(bool p_enable) {
    trace_minor_streamlines_ = p_enable;
    network_dirty_ = true;
    emit_changed();
}

void TensorField2DResource::set_extract_closed_parcels(bool p_enable) {
    extract_closed_parcels_ = p_enable;
    network_dirty_ = true;
    emit_changed();
}

void TensorField2DResource::set_sdf_source(const Ref<SDFGrid2DResource> &p_sdf) {
    sdf_source_ = p_sdf;
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::clear_basis_elements() {
    basis_elements_.clear();
    basis_polyline_vertices_.clear();
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_gridline_basis(const Vector2 &p_origin, const Vector2 &p_direction, float p_weight, float p_decay, float p_radius) {
    const Vector2 dir = p_direction.length_squared() > 1e-8f ? p_direction.normalized() : Vector2(1.0f, 0.0f);
    ideam::geometry::TensorBasisElementPod pod{};
    pod.pos_x = p_origin.x;
    pod.pos_y = p_origin.y;
    pod.dir_x = dir.x;
    pod.dir_y = dir.y;
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.radius = p_radius;
    pod.basis_type = static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_GRIDLINE);
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_transform_basis(const Transform2D &p_transform, float p_weight, float p_decay, float p_radius) {
    add_gridline_basis(p_transform.get_origin(), p_transform.columns[0], p_weight, p_decay, p_radius);
}

void TensorField2DResource::add_radial_basis(const Vector2 &p_center, float p_weight, float p_decay, float p_radius) {
    ideam::geometry::TensorBasisElementPod pod{};
    pod.pos_x = p_center.x;
    pod.pos_y = p_center.y;
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.radius = p_radius;
    pod.basis_type = static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_RADIAL);
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_vortex_basis(const Vector2 &p_center, float p_weight, float p_decay, float p_radius) {
    ideam::geometry::TensorBasisElementPod pod{};
    pod.pos_x = p_center.x;
    pod.pos_y = p_center.y;
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.radius = p_radius;
    pod.basis_type = static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_VORTEX);
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_polyline_basis(const PackedVector2Array &p_polyline, float p_weight, float p_decay, float p_radius) {
    if (p_polyline.size() < 2) return;
    const uint32_t start_idx = static_cast<uint32_t>(basis_polyline_vertices_.size());
    for (int i = 0; i < p_polyline.size(); ++i) {
        basis_polyline_vertices_.push_back(glm::vec2(p_polyline[i].x, p_polyline[i].y));
    }
    ideam::geometry::TensorBasisElementPod pod{};
    pod.pos_x = p_polyline[0].x;
    pod.pos_y = p_polyline[0].y;
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.radius = p_radius;
    pod.basis_type = static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_POLYLINE_ATTRACTOR);
    pod.polyline_Start_idx = start_idx;
    pod.polyline_vertex_count = static_cast<uint32_t>(p_polyline.size());
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_sdf_gradient_basis(float p_weight, float p_decay) {
    ideam::geometry::TensorBasisElementPod pod{};
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.basis_type = static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_SDF_GRADIENT);
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_wedge_singularity_basis(const Vector2 &p_center, float p_orientation_rad, float p_weight, float p_decay) {
    ideam::geometry::TensorBasisElementPod pod{};
    pod.pos_x = p_center.x;
    pod.pos_y = p_center.y;
    pod.dir_x = std::cos(p_orientation_rad);
    pod.dir_y = std::sin(p_orientation_rad);
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.basis_type = static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_WEDGE_SINGULARITY);
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_trisector_singularity_basis(const Vector2 &p_center, float p_orientation_rad, float p_weight, float p_decay) {
    ideam::geometry::TensorBasisElementPod pod{};
    pod.pos_x = p_center.x;
    pod.pos_y = p_center.y;
    pod.dir_x = std::cos(p_orientation_rad);
    pod.dir_y = std::sin(p_orientation_rad);
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.basis_type = static_cast<uint32_t>(ideam::geometry::TensorBasisType::TENSOR_BASIS_TRISECTOR_SINGULARITY);
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

Vector4 TensorField2DResource::sample_tensor_vec4(const Vector2 &p_world_pos) const {
    if (tensor_cells_.empty()) return Vector4(1.0f, 0.0f, 0.0f, 1.0f);
    const auto hdr = get_core_grid_header();
    const auto t = ideam::geometry::TensorFieldMath2D::sample_bilinear(hdr, tensor_cells_, glm::vec2(p_world_pos.x, p_world_pos.y));
    return Vector4(t.a, t.b, t.trace, t.weight);
}

Vector2 TensorField2DResource::sample_major_eigenvector(const Vector2 &p_world_pos) const {
    if (tensor_cells_.empty()) return Vector2(1.0f, 0.0f);
    const auto hdr = get_core_grid_header();
    const auto t = ideam::geometry::TensorFieldMath2D::sample_bilinear(hdr, tensor_cells_, glm::vec2(p_world_pos.x, p_world_pos.y));
    const glm::vec2 maj = t.major_eigenvector();
    return Vector2(maj.x, maj.y);
}

Vector2 TensorField2DResource::sample_minor_eigenvector(const Vector2 &p_world_pos) const {
    if (tensor_cells_.empty()) return Vector2(0.0f, 1.0f);
    const auto hdr = get_core_grid_header();
    const auto t = ideam::geometry::TensorFieldMath2D::sample_bilinear(hdr, tensor_cells_, glm::vec2(p_world_pos.x, p_world_pos.y));
    const glm::vec2 min_v = t.minor_eigenvector();
    return Vector2(min_v.x, min_v.y);
}

float TensorField2DResource::sample_anisotropy(const Vector2 &p_world_pos) const {
    if (tensor_cells_.empty()) return 0.0f;
    const auto hdr = get_core_grid_header();
    const auto t = ideam::geometry::TensorFieldMath2D::sample_bilinear(hdr, tensor_cells_, glm::vec2(p_world_pos.x, p_world_pos.y));
    return t.anisotropy();
}

Transform2D TensorField2DResource::sample_eigen_transform(const Vector2 &p_world_pos) const {
    const Vector2 maj = sample_major_eigenvector(p_world_pos);
    const Vector2 min_v = sample_minor_eigenvector(p_world_pos);
    return Transform2D(maj, min_v, p_world_pos);
}

void TensorField2DResource::bake_field_and_streamlines() {
    _rebuild_grid();
    _rebuild_network_if_needed();
    emit_changed();
}

TypedArray<Dictionary> TensorField2DResource::get_singularities() const {
    TypedArray<Dictionary> out;
    for (const auto &s : singularities_) {
        Dictionary d;
        d["position"] = Vector2(s.world_x, s.world_y);
        d["poincare_index"] = s.poincare_index;
        d["type"] = static_cast<int>(s.singularity_type);
        out.push_back(d);
    }
    return out;
}

TypedArray<PackedVector2Array> TensorField2DResource::get_major_streamlines() {
    _rebuild_network_if_needed();
    TypedArray<PackedVector2Array> out;
    for (size_t i = 0; i < traced_network_.polyline_slices.size(); ++i) {
        if (i < traced_network_.streamline_is_minor.size() && traced_network_.streamline_is_minor[i] != 0u) continue;
        const auto &sl = traced_network_.polyline_slices[i];
        PackedVector2Array arr;
        arr.resize(static_cast<int>(sl.vertex_count));
        for (uint32_t k = 0; k < sl.vertex_count; ++k) {
            const glm::vec2 &pt = traced_network_.polyline_vertices[sl.vertex_Start_idx + k];
            arr.set(static_cast<int>(k), Vector2(pt.x, pt.y));
        }
        out.push_back(arr);
    }
    return out;
}

TypedArray<PackedVector2Array> TensorField2DResource::get_minor_streamlines() {
    _rebuild_network_if_needed();
    TypedArray<PackedVector2Array> out;
    for (size_t i = 0; i < traced_network_.polyline_slices.size(); ++i) {
        if (i >= traced_network_.streamline_is_minor.size() || traced_network_.streamline_is_minor[i] == 0u) continue;
        const auto &sl = traced_network_.polyline_slices[i];
        PackedVector2Array arr;
        arr.resize(static_cast<int>(sl.vertex_count));
        for (uint32_t k = 0; k < sl.vertex_count; ++k) {
            const glm::vec2 &pt = traced_network_.polyline_vertices[sl.vertex_Start_idx + k];
            arr.set(static_cast<int>(k), Vector2(pt.x, pt.y));
        }
        out.push_back(arr);
    }
    return out;
}

TypedArray<PackedVector2Array> TensorField2DResource::get_closed_parcel_polygons() {
    _rebuild_network_if_needed();
    TypedArray<PackedVector2Array> out;
    for (const auto &poly : traced_network_.region_polygon_slices) {
        PackedVector2Array arr;
        arr.resize(static_cast<int>(poly.vertex_count));
        for (uint32_t k = 0; k < poly.vertex_count; ++k) {
            const glm::vec2 &pt = traced_network_.region_polygon_vertices[poly.vertex_Start_idx + k];
            arr.set(static_cast<int>(k), Vector2(pt.x, pt.y));
        }
        out.push_back(arr);
    }
    return out;
}

TypedArray<Polygon2DResource> TensorField2DResource::bake_parcel_polygon_resources() {
    _rebuild_network_if_needed();
    TypedArray<Polygon2DResource> out;
    for (const auto &poly : traced_network_.region_polygon_slices) {
        PackedVector2Array arr;
        arr.resize(static_cast<int>(poly.vertex_count));
        for (uint32_t k = 0; k < poly.vertex_count; ++k) {
            const glm::vec2 &pt = traced_network_.region_polygon_vertices[poly.vertex_Start_idx + k];
            arr.set(static_cast<int>(k), Vector2(pt.x, pt.y));
        }
        Ref<Polygon2DResource> res;
        res.instantiate();
        res->set_outer_boundary(arr);
        out.push_back(res);
    }
    return out;
}

Ref<HeatmapGrid> TensorField2DResource::to_anisotropy_heatmap() const {
    Ref<HeatmapGrid> hm;
    hm.instantiate();
    const auto hdr = get_core_grid_header();
    hm->initialize(
        static_cast<int>(hdr.width),
        static_cast<int>(hdr.height),
        std::max(hdr.cell_size_x, hdr.cell_size_y),
        Vector2(hdr.origin_x, hdr.origin_y),
        0.0f
    );
    PackedFloat32Array data;
    data.resize(static_cast<int>(tensor_cells_.size()));
    for (size_t i = 0; i < tensor_cells_.size(); ++i) {
        data.set(static_cast<int>(i), tensor_cells_[i].anisotropy());
    }
    hm->set_grid_data(data);
    return hm;
}

Vector2 TensorField2DResource::get_closest_point(const Vector2 &p_to_point) const {
    return Vector2(
        std::clamp(p_to_point.x, domain_bounds_.position.x, domain_bounds_.position.x + domain_bounds_.size.x),
        std::clamp(p_to_point.y, domain_bounds_.position.y, domain_bounds_.position.y + domain_bounds_.size.y)
    );
}

PackedVector2Array TensorField2DResource::get_render_Wireframe() const {
    PackedVector2Array lines;
    const Vector2 p0 = domain_bounds_.position;
    const Vector2 p1 = p0 + Vector2(domain_bounds_.size.x, 0.0f);
    const Vector2 p2 = p0 + domain_bounds_.size;
    const Vector2 p3 = p0 + Vector2(0.0f, domain_bounds_.size.y);
    lines.push_back(p0); lines.push_back(p1);
    lines.push_back(p1); lines.push_back(p2);
    lines.push_back(p2); lines.push_back(p3);
    lines.push_back(p3); lines.push_back(p0);

    for (const auto &sl : traced_network_.polyline_slices) {
        for (uint32_t k = 0; k + 1 < sl.vertex_count; ++k) {
            const glm::vec2 &a = traced_network_.polyline_vertices[sl.vertex_Start_idx + k];
            const glm::vec2 &b = traced_network_.polyline_vertices[sl.vertex_Start_idx + k + 1];
            lines.push_back(Vector2(a.x, a.y));
            lines.push_back(Vector2(b.x, b.y));
        }
    }
    return lines;
}

void TensorField2DResource::sync_from_dod_results(
    const ideam::geometry::TensorGridHeaderPod &p_header,
    std::span<const ideam::geometry::SymmetricTensor2DPod> p_cells,
    std::span<const glm::vec2> p_major,
    std::span<const glm::vec2> p_minor,
    std::span<const ideam::geometry::TensorSingularityPod> p_singularities
) {
    grid_width_ = static_cast<int>(p_header.width);
    grid_height_ = static_cast<int>(p_header.height);
    tensor_cells_.assign(p_cells.begin(), p_cells.end());
    major_eigenvectors_.assign(p_major.begin(), p_major.end());
    minor_eigenvectors_.assign(p_minor.begin(), p_minor.end());
    singularities_.assign(p_singularities.begin(), p_singularities.end());
    network_dirty_ = true;
    emit_changed();
}

void TensorField2DResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &TensorField2DResource::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &TensorField2DResource::get_grid_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_width", "get_grid_width");

    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &TensorField2DResource::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &TensorField2DResource::get_grid_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_height", "get_grid_height");

    ClassDB::bind_method(D_METHOD("set_domain_bounds", "bounds"), &TensorField2DResource::set_domain_bounds);
    ClassDB::bind_method(D_METHOD("get_domain_bounds"), &TensorField2DResource::get_domain_bounds);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "domain_bounds"), "set_domain_bounds", "get_domain_bounds");

    ClassDB::bind_method(D_METHOD("set_blend_mode", "mode"), &TensorField2DResource::set_blend_mode);
    ClassDB::bind_method(D_METHOD("get_blend_mode"), &TensorField2DResource::get_blend_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "blend_mode", PROPERTY_HINT_ENUM, "WeightedAverage,Additive"), "set_blend_mode", "get_blend_mode");

    ClassDB::bind_method(D_METHOD("set_step_size", "step"), &TensorField2DResource::set_step_size);
    ClassDB::bind_method(D_METHOD("get_step_size"), &TensorField2DResource::get_step_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "step_size", PROPERTY_HINT_RANGE, "0.5,32.0,0.25"), "set_step_size", "get_step_size");

    ClassDB::bind_method(D_METHOD("set_min_streamline_separation", "separation"), &TensorField2DResource::set_min_streamline_separation);
    ClassDB::bind_method(D_METHOD("get_min_streamline_separation"), &TensorField2DResource::get_min_streamline_separation);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_streamline_separation", PROPERTY_HINT_RANGE, "4.0,128.0,1.0"), "set_min_streamline_separation", "get_min_streamline_separation");

    ClassDB::bind_method(D_METHOD("set_simplify_tolerance", "tolerance"), &TensorField2DResource::set_simplify_tolerance);
    ClassDB::bind_method(D_METHOD("get_simplify_tolerance"), &TensorField2DResource::get_simplify_tolerance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "simplify_tolerance", PROPERTY_HINT_RANGE, "0.0,10.0,0.05"), "set_simplify_tolerance", "get_simplify_tolerance");

    ClassDB::bind_method(D_METHOD("set_max_streamline_steps", "steps"), &TensorField2DResource::set_max_streamline_steps);
    ClassDB::bind_method(D_METHOD("get_max_streamline_steps"), &TensorField2DResource::get_max_streamline_steps);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_streamline_steps", PROPERTY_HINT_RANGE, "16,2048,8"), "set_max_streamline_steps", "get_max_streamline_steps");

    ClassDB::bind_method(D_METHOD("set_trace_minor_streamlines", "enable"), &TensorField2DResource::set_trace_minor_streamlines);
    ClassDB::bind_method(D_METHOD("get_trace_minor_streamlines"), &TensorField2DResource::get_trace_minor_streamlines);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "trace_minor_streamlines"), "set_trace_minor_streamlines", "get_trace_minor_streamlines");

    ClassDB::bind_method(D_METHOD("set_extract_closed_parcels", "enable"), &TensorField2DResource::set_extract_closed_parcels);
    ClassDB::bind_method(D_METHOD("get_extract_closed_parcels"), &TensorField2DResource::get_extract_closed_parcels);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "extract_closed_parcels"), "set_extract_closed_parcels", "get_extract_closed_parcels");

    ClassDB::bind_method(D_METHOD("set_sdf_source", "sdf"), &TensorField2DResource::set_sdf_source);
    ClassDB::bind_method(D_METHOD("get_sdf_source"), &TensorField2DResource::get_sdf_source);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sdf_source", PROPERTY_HINT_RESOURCE_TYPE, "SDFGrid2DResource"), "set_sdf_source", "get_sdf_source");

    ClassDB::bind_method(D_METHOD("clear_basis_elements"), &TensorField2DResource::clear_basis_elements);
    ClassDB::bind_method(D_METHOD("get_basis_element_count"), &TensorField2DResource::get_basis_element_count);
    ClassDB::bind_method(D_METHOD("add_gridline_basis", "origin", "direction", "weight", "decay", "radius"), &TensorField2DResource::add_gridline_basis, DEFVAL(1.0f), DEFVAL(0.005f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("add_transform_basis", "transform", "weight", "decay", "radius"), &TensorField2DResource::add_transform_basis, DEFVAL(1.0f), DEFVAL(0.005f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("add_radial_basis", "center", "weight", "decay", "radius"), &TensorField2DResource::add_radial_basis, DEFVAL(1.0f), DEFVAL(0.008f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("add_vortex_basis", "center", "weight", "decay", "radius"), &TensorField2DResource::add_vortex_basis, DEFVAL(1.0f), DEFVAL(0.008f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("add_polyline_basis", "polyline", "weight", "decay", "radius"), &TensorField2DResource::add_polyline_basis, DEFVAL(1.5f), DEFVAL(0.01f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("add_sdf_gradient_basis", "weight", "decay"), &TensorField2DResource::add_sdf_gradient_basis, DEFVAL(1.0f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("add_wedge_singularity_basis", "center", "orientation_rad", "weight", "decay"), &TensorField2DResource::add_wedge_singularity_basis, DEFVAL(0.0f), DEFVAL(1.0f), DEFVAL(0.008f));
    ClassDB::bind_method(D_METHOD("add_trisector_singularity_basis", "center", "orientation_rad", "weight", "decay"), &TensorField2DResource::add_trisector_singularity_basis, DEFVAL(0.0f), DEFVAL(1.0f), DEFVAL(0.008f));

    ClassDB::bind_method(D_METHOD("sample_tensor_vec4", "world_pos"), &TensorField2DResource::sample_tensor_vec4);
    ClassDB::bind_method(D_METHOD("sample_major_eigenvector", "world_pos"), &TensorField2DResource::sample_major_eigenvector);
    ClassDB::bind_method(D_METHOD("sample_minor_eigenvector", "world_pos"), &TensorField2DResource::sample_minor_eigenvector);
    ClassDB::bind_method(D_METHOD("sample_anisotropy", "world_pos"), &TensorField2DResource::sample_anisotropy);
    ClassDB::bind_method(D_METHOD("sample_eigen_transform", "world_pos"), &TensorField2DResource::sample_eigen_transform);

    ClassDB::bind_method(D_METHOD("bake_field_and_streamlines"), &TensorField2DResource::bake_field_and_streamlines);
    ClassDB::bind_method(D_METHOD("get_singularities"), &TensorField2DResource::get_singularities);
    ClassDB::bind_method(D_METHOD("get_major_streamlines"), &TensorField2DResource::get_major_streamlines);
    ClassDB::bind_method(D_METHOD("get_minor_streamlines"), &TensorField2DResource::get_minor_streamlines);
    ClassDB::bind_method(D_METHOD("get_closed_parcel_polygons"), &TensorField2DResource::get_closed_parcel_polygons);
    ClassDB::bind_method(D_METHOD("bake_parcel_polygon_resources"), &TensorField2DResource::bake_parcel_polygon_resources);
    ClassDB::bind_method(D_METHOD("to_anisotropy_heatmap"), &TensorField2DResource::to_anisotropy_heatmap);

    BIND_ENUM_CONSTANT(BASIS_GRIDLINE);
    BIND_ENUM_CONSTANT(BASIS_RADIAL);
    BIND_ENUM_CONSTANT(BASIS_VORTEX);
    BIND_ENUM_CONSTANT(BASIS_POLYLINE);
    BIND_ENUM_CONSTANT(BASIS_SDF_GRADIENT);
    BIND_ENUM_CONSTANT(BASIS_WEDGE_SINGULARITY);
    BIND_ENUM_CONSTANT(BASIS_TRISECTOR);

    BIND_ENUM_CONSTANT(BLEND_WEIGHTED_AVERAGE);
    BIND_ENUM_CONSTANT(BLEND_ADDITIVE);
}

} // namespace godot

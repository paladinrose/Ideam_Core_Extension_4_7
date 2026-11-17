#include "godot/geometry/resources/tensor_field_2d_resource.h"
#include "core/tasks/domains/geometry/geometry_dod_common.h"

#include <godot_cpp/core/class_db.hpp>
#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

using namespace godot;

TensorField2DResource::TensorField2DResource() {
    // Default demonstration basis: an urban gridline + radial plaza center
    add_gridline_basis(Vector2(256.0f, 256.0f), Vector2(1.0f, 0.15f), 1.0f, 0.002f, 0.0f);
    add_radial_basis(Vector2(200.0f, 220.0f), 1.35f, 0.009f, 240.0f);
}

core::TensorGridHeaderPod TensorField2DResource::get_core_grid_header() const {
    core::TensorGridHeaderPod hdr{};
    hdr.width = static_cast<decltype(hdr.width)>(std::max(2, grid_width_));
    hdr.height = static_cast<decltype(hdr.height)>(std::max(2, grid_height_));
    hdr.basis_offset = 0u;
    hdr.basis_count = static_cast<uint16_t>(basis_elements_.size());
    hdr.origin = glm::vec2(domain_bounds_.position.x, domain_bounds_.position.y);
    const float w = std::max(1.0f, domain_bounds_.size.x);
    const float h = std::max(1.0f, domain_bounds_.size.y);
    hdr.cell_size = glm::vec2(
        w / static_cast<float>(std::max<uint32_t>(1u, static_cast<uint32_t>(hdr.width) - 1u)),
        h / static_cast<float>(std::max<uint32_t>(1u, static_cast<uint32_t>(hdr.height) - 1u))
    );
    return hdr;
}

void TensorField2DResource::_rebuild_grid() {
    const auto hdr = get_core_grid_header();
    const size_t total = static_cast<size_t>(hdr.width) * static_cast<size_t>(hdr.height);
    tensor_cells_.resize(total);
    major_eigenvectors_.resize(total);
    minor_eigenvectors_.resize(total);

    core::SDFGridHeaderPOD sdf_hdr{};
    const core::SDFGridHeaderPOD *sdf_hdr_ptr = nullptr;
    std::span<const float> sdf_span{};
    if (sdf_source_.is_valid()) {
        const Vector2i res = sdf_source_->get_resolution();
        const Rect2 bnd = sdf_source_->get_world_bounds();
        sdf_span = sdf_source_->get_grid_span();
        if (!sdf_span.empty() && res.x > 1 && res.y > 1) {
            sdf_hdr.cell_offset = 0u;
            sdf_hdr.width = static_cast<uint32_t>(res.x);
            sdf_hdr.height = static_cast<uint32_t>(res.y);
            sdf_hdr.iso_value = sdf_source_->get_iso_value();
            sdf_hdr.origin = glm::vec2(bnd.position.x, bnd.position.y);
            sdf_hdr.cell_size = glm::vec2(
                bnd.size.x / static_cast<float>(res.x - 1),
                bnd.size.y / static_cast<float>(res.y - 1)
            );
            sdf_hdr_ptr = &sdf_hdr;
        }
    }

    core::TensorFieldMath2D::evaluate_tensor_grid(
        hdr,
        basis_elements_,
        tensor_cells_,
        major_eigenvectors_,
        minor_eigenvectors_,
        basis_polyline_slices_,
        basis_polyline_vertices_,
        sdf_hdr_ptr,
        sdf_span
    );

    singularities_.resize(std::max<size_t>(16u, total / 4u));
    const uint32_t sing_count = core::TensorFieldMath2D::detect_grid_singularities(
        hdr,
        tensor_cells_,
        singularities_
    );
    singularities_.resize(sing_count);

    seeds_.clear();
    const float inv_cell_x = (hdr.cell_size.x > 1e-6f) ? (1.0f / hdr.cell_size.x) : 1.0f;
    const uint32_t stride = std::max(1u, static_cast<uint32_t>(std::round(min_streamline_separation_ * inv_cell_x)));
    for (const auto &s : singularities_) {
        if (s.singularity_type == core::TENSOR_SINGULARITY_NONE) continue;
        core::TensorStreamlineSeedPod seed{};
        seed.position = s.position;
        seed.priority = 2.0f;
        seed.follow_minor = 0u;
        seed.hierarchy_tier = 0u;
        seed.active = 1u;
        seeds_.push_back(seed);
    }
    for (uint32_t gy = stride / 2; gy < static_cast<uint32_t>(hdr.height); gy += stride) {
        for (uint32_t gx = stride / 2; gx < static_cast<uint32_t>(hdr.width); gx += stride) {
            const auto &cell = tensor_cells_[static_cast<size_t>(gy) * static_cast<size_t>(hdr.width) + gx];
            const glm::vec2 wp = hdr.origin + glm::vec2(static_cast<float>(gx), static_cast<float>(gy)) * hdr.cell_size;
            core::TensorStreamlineSeedPod seed{};
            seed.position = wp;
            seed.priority = cell.anisotropy();
            seed.follow_minor = 0u;
            seed.hierarchy_tier = 1u;
            seed.active = 1u;
            seeds_.push_back(seed);
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
    core::TensorTracingConfig2D cfg{};
    cfg.world_bounds = core::AABB2D<float>{
        glm::vec2(domain_bounds_.position.x, domain_bounds_.position.y),
        glm::vec2(domain_bounds_.position.x + domain_bounds_.size.x, domain_bounds_.position.y + domain_bounds_.size.y)
    };
    cfg.step_size = std::max(0.5f, step_size_);
    cfg.major_separation = std::max(2.0f, min_streamline_separation_);
    cfg.minor_separation = std::max(2.0f, min_streamline_separation_);
    cfg.collinear_cos_threshold = std::clamp(1.0f - (simplify_tolerance_ * 0.01f), 0.90f, 0.9999f);
    cfg.merge_distance = min_streamline_separation_ * 0.35f;
    cfg.max_steps_per_stream = static_cast<uint32_t>(std::max(8, max_streamline_steps_));
    cfg.extract_closed_regions = extract_closed_parcels_;

    traced_network_ = core::TensorFieldMath2D::build_hyperstreamline_network(
        hdr,
        tensor_cells_,
        basis_elements_,
        seeds_,
        cfg
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
    basis_polyline_slices_.clear();
    basis_polyline_vertices_.clear();
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_gridline_basis(const Vector2 &p_origin, const Vector2 &p_direction, float p_weight, float p_decay, float p_radius) {
    const Vector2 dir = p_direction.length_squared() > 1e-8f ? p_direction.normalized() : Vector2(1.0f, 0.0f);
    core::TensorBasisElementPod pod{};
    pod.position = glm::vec2(p_origin.x, p_origin.y);
    pod.direction = glm::vec2(dir.x, dir.y);
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.aux_param = p_radius;
    pod.basis_type = core::TENSOR_BASIS_GRIDLINE;
    pod.enabled = 1u;
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_transform_basis(const Transform2D &p_transform, float p_weight, float p_decay, float p_radius) {
    add_gridline_basis(p_transform.get_origin(), p_transform.columns[0], p_weight, p_decay, p_radius);
}

void TensorField2DResource::add_radial_basis(const Vector2 &p_center, float p_weight, float p_decay, float p_radius) {
    core::TensorBasisElementPod pod{};
    pod.position = glm::vec2(p_center.x, p_center.y);
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.aux_param = p_radius;
    pod.basis_type = core::TENSOR_BASIS_RADIAL;
    pod.enabled = 1u;
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_vortex_basis(const Vector2 &p_center, float p_weight, float p_decay, float p_radius) {
    core::TensorBasisElementPod pod{};
    pod.position = glm::vec2(p_center.x, p_center.y);
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.aux_param = p_radius;
    pod.basis_type = core::TENSOR_BASIS_VORTEX;
    pod.enabled = 1u;
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
    const uint16_t slice_id = static_cast<uint16_t>(basis_polyline_slices_.size());
    core::PolylineSlicePOD slice{};
    slice.vertex_offset = start_idx;
    slice.vertex_count = static_cast<uint32_t>(p_polyline.size());
    basis_polyline_slices_.push_back(slice);

    core::TensorBasisElementPod pod{};
    pod.position = glm::vec2(p_polyline[0].x, p_polyline[0].y);
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.aux_param = p_radius;
    pod.source_slice_id = slice_id;
    pod.basis_type = core::TENSOR_BASIS_POLYLINE_ATTRACTOR;
    pod.enabled = 1u;
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_sdf_gradient_basis(float p_weight, float p_decay) {
    core::TensorBasisElementPod pod{};
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.basis_type = core::TENSOR_BASIS_SDF_GRADIENT;
    pod.enabled = 1u;
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_wedge_singularity_basis(const Vector2 &p_center, float p_orientation_rad, float p_weight, float p_decay) {
    core::TensorBasisElementPod pod{};
    pod.position = glm::vec2(p_center.x, p_center.y);
    pod.direction = glm::vec2(std::cos(p_orientation_rad), std::sin(p_orientation_rad));
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.aux_param = p_orientation_rad;
    pod.basis_type = core::TENSOR_BASIS_WEDGE_SINGULARITY;
    pod.enabled = 1u;
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

void TensorField2DResource::add_trisector_singularity_basis(const Vector2 &p_center, float p_orientation_rad, float p_weight, float p_decay) {
    core::TensorBasisElementPod pod{};
    pod.position = glm::vec2(p_center.x, p_center.y);
    pod.direction = glm::vec2(std::cos(p_orientation_rad), std::sin(p_orientation_rad));
    pod.decay = p_decay;
    pod.weight = p_weight;
    pod.aux_param = p_orientation_rad;
    pod.basis_type = core::TENSOR_BASIS_TRISECTOR_SINGULARITY;
    pod.enabled = 1u;
    basis_elements_.push_back(pod);
    _rebuild_grid();
    emit_changed();
}

Vector4 TensorField2DResource::sample_tensor_vec4(const Vector2 &p_world_pos) const {
    const auto t = core::TensorFieldMath2D::sample_tensor_field(
        glm::vec2(p_world_pos.x, p_world_pos.y),
        basis_elements_,
        basis_polyline_slices_,
        basis_polyline_vertices_
    );
    return Vector4(t.a, t.b, t.trace, t.weight);
}

Vector2 TensorField2DResource::sample_major_eigenvector(const Vector2 &p_world_pos) const {
    const auto t = core::TensorFieldMath2D::sample_tensor_field(
        glm::vec2(p_world_pos.x, p_world_pos.y),
        basis_elements_,
        basis_polyline_slices_,
        basis_polyline_vertices_
    );
    glm::vec2 maj{}, min_v{};
    t.eigenvectors(maj, min_v);
    return Vector2(maj.x, maj.y);
}

Vector2 TensorField2DResource::sample_minor_eigenvector(const Vector2 &p_world_pos) const {
    const auto t = core::TensorFieldMath2D::sample_tensor_field(
        glm::vec2(p_world_pos.x, p_world_pos.y),
        basis_elements_,
        basis_polyline_slices_,
        basis_polyline_vertices_
    );
    glm::vec2 maj{}, min_v{};
    t.eigenvectors(maj, min_v);
    return Vector2(min_v.x, min_v.y);
}

float TensorField2DResource::sample_anisotropy(const Vector2 &p_world_pos) const {
    const auto t = core::TensorFieldMath2D::sample_tensor_field(
        glm::vec2(p_world_pos.x, p_world_pos.y),
        basis_elements_,
        basis_polyline_slices_,
        basis_polyline_vertices_
    );
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
        d["position"] = Vector2(s.position.x, s.position.y);
        d["poincare_index"] = s.poincare_index;
        d["type"] = static_cast<int>(s.singularity_type);
        out.push_back(d);
    }
    return out;
}

TypedArray<PackedVector2Array> TensorField2DResource::get_major_streamlines() {
    _rebuild_network_if_needed();
    TypedArray<PackedVector2Array> out;
    for (size_t i = 0; i < traced_network_.streamlines.size(); ++i) {
        if (i < traced_network_.streamline_is_minor.size() && traced_network_.streamline_is_minor[i] != 0u) continue;
        const auto &sl = traced_network_.streamlines[i];
        PackedVector2Array arr;
        arr.resize(static_cast<int>(sl.size()));
        for (size_t k = 0; k < sl.size(); ++k) {
            arr.set(static_cast<int>(k), Vector2(sl[k].x, sl[k].y));
        }
        out.push_back(arr);
    }
    return out;
}

TypedArray<PackedVector2Array> TensorField2DResource::get_minor_streamlines() {
    _rebuild_network_if_needed();
    TypedArray<PackedVector2Array> out;
    for (size_t i = 0; i < traced_network_.streamlines.size(); ++i) {
        if (i >= traced_network_.streamline_is_minor.size() || traced_network_.streamline_is_minor[i] == 0u) continue;
        const auto &sl = traced_network_.streamlines[i];
        PackedVector2Array arr;
        arr.resize(static_cast<int>(sl.size()));
        for (size_t k = 0; k < sl.size(); ++k) {
            arr.set(static_cast<int>(k), Vector2(sl[k].x, sl[k].y));
        }
        out.push_back(arr);
    }
    return out;
}

TypedArray<PackedVector2Array> TensorField2DResource::get_closed_parcel_polygons() {
    _rebuild_network_if_needed();
    TypedArray<PackedVector2Array> out;
    for (const auto &poly : traced_network_.closed_regions) {
        PackedVector2Array arr;
        arr.resize(static_cast<int>(poly.size()));
        for (size_t k = 0; k < poly.size(); ++k) {
            arr.set(static_cast<int>(k), Vector2(poly[k].x, poly[k].y));
        }
        out.push_back(arr);
    }
    return out;
}

TypedArray<Polygon2DResource> TensorField2DResource::bake_parcel_polygon_resources() {
    _rebuild_network_if_needed();
    TypedArray<Polygon2DResource> out;
    for (const auto &poly : traced_network_.closed_regions) {
        PackedVector2Array arr;
        arr.resize(static_cast<int>(poly.size()));
        for (size_t k = 0; k < poly.size(); ++k) {
            arr.set(static_cast<int>(k), Vector2(poly[k].x, poly[k].y));
        }
        Ref<Polygon2DResource> res;
        res.instantiate();
        res->set_vertices(arr);
        out.push_back(res);
    }
    return out;
}

HeatmapGrid *TensorField2DResource::to_anisotropy_heatmap() const {
    HeatmapGrid *hm = memnew(HeatmapGrid);
    return hm;
}

Dictionary TensorField2DResource::get_snapshot_state() const {
    Dictionary d;
    d["grid_width"] = grid_width_;
    d["grid_height"] = grid_height_;
    d["domain_bounds"] = domain_bounds_;
    return d;
}

void TensorField2DResource::apply_snapshot_state(const Dictionary &p_state) {
    if (p_state.has("grid_width")) grid_width_ = static_cast<int>(p_state["grid_width"]);
    if (p_state.has("grid_height")) grid_height_ = static_cast<int>(p_state["grid_height"]);
    if (p_state.has("domain_bounds")) domain_bounds_ = p_state["domain_bounds"];
    _rebuild_grid();
    emit_changed();
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

    for (const auto &sl : traced_network_.streamlines) {
        for (size_t k = 0; k + 1 < sl.size(); ++k) {
            lines.push_back(Vector2(sl[k].x, sl[k].y));
            lines.push_back(Vector2(sl[k + 1].x, sl[k + 1].y));
        }
    }
    return lines;
}

void TensorField2DResource::sync_from_dod_results(
    const core::TensorGridHeaderPod &p_header,
    std::span<const core::SymmetricTensor2DPod> p_cells,
    std::span<const glm::vec2> p_major,
    std::span<const glm::vec2> p_minor,
    std::span<const core::TensorSingularityPod> p_singularities
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

} // namespace ideam::godot_ext

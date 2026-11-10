#include "sdf_grid_2d_resource.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void SDFGrid2DResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("SDF Grid 2D", "");

    ClassDB::bind_method(D_METHOD("set_resolution", "resolution"), &SDFGrid2DResource::set_resolution);
    ClassDB::bind_method(D_METHOD("get_resolution"), &SDFGrid2DResource::get_resolution);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "resolution"), "set_resolution", "get_resolution");

    ClassDB::bind_method(D_METHOD("set_world_bounds", "bounds"), &SDFGrid2DResource::set_world_bounds);
    ClassDB::bind_method(D_METHOD("get_world_bounds"), &SDFGrid2DResource::get_world_bounds);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "world_bounds"), "set_world_bounds", "get_world_bounds");

    ClassDB::bind_method(D_METHOD("set_grid_values", "values"), &SDFGrid2DResource::set_grid_values);
    ClassDB::bind_method(D_METHOD("get_grid_values"), &SDFGrid2DResource::get_grid_values);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "grid_values", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_grid_values",
        "get_grid_values"
    );

    ClassDB::bind_method(D_METHOD("clear_field", "fill_value"), &SDFGrid2DResource::clear_field, DEFVAL(1000.0f));

    ClassDB::bind_method(
        D_METHOD("stamp_circle", "center", "radius", "mode", "smooth_k"),
        &SDFGrid2DResource::stamp_circle,
        DEFVAL(SDF_COMBINE_UNION),
        DEFVAL(0.0f)
    );
    ClassDB::bind_method(
        D_METHOD("stamp_box", "center", "half_extents", "mode", "smooth_k"),
        &SDFGrid2DResource::stamp_box,
        DEFVAL(SDF_COMBINE_UNION),
        DEFVAL(0.0f)
    );
    ClassDB::bind_method(
        D_METHOD("stamp_segment", "a", "b", "radius", "mode", "smooth_k"),
        &SDFGrid2DResource::stamp_segment,
        DEFVAL(0.1f),
        DEFVAL(SDF_COMBINE_UNION),
        DEFVAL(0.0f)
    );
    ClassDB::bind_method(
        D_METHOD("stamp_polygon", "polygon", "mode", "smooth_k"),
        &SDFGrid2DResource::stamp_polygon,
        DEFVAL(SDF_COMBINE_UNION),
        DEFVAL(0.0f)
    );
    ClassDB::bind_method(
        D_METHOD("evaluate_chladni_field", "mode_n", "mode_m", "amp_a", "amp_b"),
        &SDFGrid2DResource::evaluate_chladni_field,
        DEFVAL(1.0f),
        DEFVAL(-1.0f)
    );

    ClassDB::bind_method(D_METHOD("sample_distance", "world_pos"), &SDFGrid2DResource::sample_distance);
    ClassDB::bind_method(D_METHOD("sample_gradient", "world_pos"), &SDFGrid2DResource::sample_gradient);
    ClassDB::bind_method(
        D_METHOD("extract_iso_contour_segments", "iso_value"),
        &SDFGrid2DResource::extract_iso_contour_segments,
        DEFVAL(0.0f)
    );
}

SDFGrid2DResource::SDFGrid2DResource() {
    _ensure_allocated();
}

void SDFGrid2DResource::_ensure_allocated() {
    const int w = std::max(2, resolution_.x);
    const int h = std::max(2, resolution_.y);
    resolution_ = godot::Vector2i(w, h);
    const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
    if (grid_values_.size() != total) {
        grid_values_.assign(total, 1000.0f);
    }
}

SDFGrid2DResource::Vec2 SDFGrid2DResource::_cell_to_world(int p_x, int p_y) const noexcept {
    const float u = (resolution_.x > 1)
        ? (static_cast<float>(p_x) / static_cast<float>(resolution_.x - 1))
        : 0.0f;
    const float v = (resolution_.y > 1)
        ? (static_cast<float>(p_y) / static_cast<float>(resolution_.y - 1))
        : 0.0f;
    return Vec2(
        world_bounds_.position.x + u * world_bounds_.size.x,
        world_bounds_.position.y + v * world_bounds_.size.y
    );
}

SDFGrid2DResource::Vec2 SDFGrid2DResource::_world_to_grid_coord(const godot::Vector2 &p_world) const noexcept {
    const float sx = std::max(1e-5f, world_bounds_.size.x);
    const float sy = std::max(1e-5f, world_bounds_.size.y);
    const float u  = (p_world.x - world_bounds_.position.x) / sx;
    const float v  = (p_world.y - world_bounds_.position.y) / sy;
    return Vec2(
        u * static_cast<float>( std::max(1, resolution_.x - 1) ),
        v * static_cast<float>( std::max(1, resolution_.y - 1) )
    );
}

float SDFGrid2DResource::_combine_sample(
    float p_existing,
    float p_candidate,
    GeometrySDFCombineMode p_mode,
    float p_smooth_k
) noexcept {
    switch (p_mode) {
        case SDF_COMBINE_REPLACE:
            return p_candidate;
        case SDF_COMBINE_UNION:
            return SDF::op_union(p_existing, p_candidate);
        case SDF_COMBINE_SUBTRACT:
            return SDF::op_subtraction(p_existing, p_candidate);
        case SDF_COMBINE_INTERSECT:
            return SDF::op_intersection(p_existing, p_candidate);
        case SDF_COMBINE_SMOOTH_UNION:
            return SDF::op_smooth_union(p_existing, p_candidate, p_smooth_k);
        case SDF_COMBINE_SMOOTH_SUBTRACT:
            return SDF::op_smooth_subtraction(p_existing, p_candidate, p_smooth_k);
        case SDF_COMBINE_SMOOTH_INTERSECT:
            return SDF::op_smooth_intersection(p_existing, p_candidate, p_smooth_k);
        default:
            return p_candidate;
    }
}

godot::Dictionary SDFGrid2DResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["resolution"]   = resolution_;
    d["world_bounds"] = world_bounds_;
    d["grid_values"]  = get_grid_values();
    return d;
}

void SDFGrid2DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("resolution")) {
        resolution_ = p_state["resolution"];
    }
    if (p_state.has("world_bounds")) {
        world_bounds_ = p_state["world_bounds"];
    }
    _ensure_allocated();
    if (p_state.has("grid_values")) {
        set_grid_values(p_state["grid_values"]);
    }
}

void SDFGrid2DResource::set_resolution(const godot::Vector2i &p_resolution) {
    const godot::Vector2i clamped(std::max(2, p_resolution.x), std::max(2, p_resolution.y));
    if (resolution_ == clamped && !grid_values_.empty()) return;
    resolution_ = clamped;
    _ensure_allocated();
    emit_signal("geometry_changed");
    emit_changed();
}

void SDFGrid2DResource::set_world_bounds(const godot::Rect2 &p_bounds) {
    if (world_bounds_ == p_bounds) return;
    world_bounds_ = p_bounds;
    emit_signal("geometry_changed");
    emit_changed();
}

void SDFGrid2DResource::set_grid_values(const godot::PackedFloat32Array &p_values) {
    _ensure_allocated();
    const size_t count = std::min(grid_values_.size(), static_cast<size_t>(p_values.size()));
    for (size_t i = 0; i < count; ++i) {
        grid_values_[i] = p_values[static_cast<int64_t>(i)];
    }
    emit_signal("geometry_changed");
    emit_changed();
}

godot::PackedFloat32Array SDFGrid2DResource::get_grid_values() const {
    godot::PackedFloat32Array out;
    out.resize(static_cast<int64_t>(grid_values_.size()));
    for (size_t i = 0; i < grid_values_.size(); ++i) {
        out.set(static_cast<int64_t>(i), grid_values_[i]);
    }
    return out;
}

void SDFGrid2DResource::clear_field(float p_fill_value) {
    _ensure_allocated();
    std::fill(grid_values_.begin(), grid_values_.end(), p_fill_value);
    emit_signal("geometry_changed");
    emit_changed();
}

void SDFGrid2DResource::stamp_circle(
    const godot::Vector2 &p_center,
    float p_radius,
    GeometrySDFCombineMode p_mode,
    float p_smooth_k
) {
    _ensure_allocated();
    const Vec2 c(p_center.x, p_center.y);
    for (int y = 0; y < resolution_.y; ++y) {
        for (int x = 0; x < resolution_.x; ++x) {
            const size_t idx = static_cast<size_t>(y * resolution_.x + x);
            const float d    = SDF::sd_circle(_cell_to_world(x, y), c, p_radius);
            grid_values_[idx] = _combine_sample(grid_values_[idx], d, p_mode, p_smooth_k);
        }
    }
    emit_signal("geometry_changed");
    emit_changed();
}

void SDFGrid2DResource::stamp_box(
    const godot::Vector2 &p_center,
    const godot::Vector2 &p_half_extents,
    GeometrySDFCombineMode p_mode,
    float p_smooth_k
) {
    _ensure_allocated();
    const Vec2 c(p_center.x, p_center.y);
    const Vec2 he(p_half_extents.x, p_half_extents.y);
    for (int y = 0; y < resolution_.y; ++y) {
        for (int x = 0; x < resolution_.x; ++x) {
            const size_t idx = static_cast<size_t>(y * resolution_.x + x);
            const float d    = SDF::sd_box_2d(_cell_to_world(x, y), c, he);
            grid_values_[idx] = _combine_sample(grid_values_[idx], d, p_mode, p_smooth_k);
        }
    }
    emit_signal("geometry_changed");
    emit_changed();
}

void SDFGrid2DResource::stamp_segment(
    const godot::Vector2 &p_a,
    const godot::Vector2 &p_b,
    float p_radius,
    GeometrySDFCombineMode p_mode,
    float p_smooth_k
) {
    _ensure_allocated();
    const Vec2 a(p_a.x, p_a.y);
    const Vec2 b(p_b.x, p_b.y);
    for (int y = 0; y < resolution_.y; ++y) {
        for (int x = 0; x < resolution_.x; ++x) {
            const size_t idx = static_cast<size_t>(y * resolution_.x + x);
            const float d    = SDF::sd_segment_2d(_cell_to_world(x, y), a, b, p_radius);
            grid_values_[idx] = _combine_sample(grid_values_[idx], d, p_mode, p_smooth_k);
        }
    }
    emit_signal("geometry_changed");
    emit_changed();
}

void SDFGrid2DResource::stamp_polygon(
    const godot::PackedVector2Array &p_polygon,
    GeometrySDFCombineMode p_mode,
    float p_smooth_k
) {
    if (p_polygon.size() < 3) return;
    _ensure_allocated();
    std::vector<Vec2> poly;
    poly.reserve(static_cast<size_t>(p_polygon.size()));
    for (int64_t i = 0; i < p_polygon.size(); ++i) {
        poly.emplace_back(p_polygon[i].x, p_polygon[i].y);
    }

    for (int y = 0; y < resolution_.y; ++y) {
        for (int x = 0; x < resolution_.x; ++x) {
            const size_t idx = static_cast<size_t>(y * resolution_.x + x);
            const float d    = SDF::sd_polygon_2d(poly, _cell_to_world(x, y));
            grid_values_[idx] = _combine_sample(grid_values_[idx], d, p_mode, p_smooth_k);
        }
    }
    emit_signal("geometry_changed");
    emit_changed();
}

void SDFGrid2DResource::evaluate_chladni_field(
    float p_mode_n,
    float p_mode_m,
    float p_amp_a,
    float p_amp_b
) {
    _ensure_allocated();
    for (int y = 0; y < resolution_.y; ++y) {
        const float v = (resolution_.y > 1)
            ? (static_cast<float>(y) / static_cast<float>(resolution_.y - 1))
            : 0.0f;
        for (int x = 0; x < resolution_.x; ++x) {
            const float u = (resolution_.x > 1)
                ? (static_cast<float>(x) / static_cast<float>(resolution_.x - 1))
                : 0.0f;
            const size_t idx = static_cast<size_t>(y * resolution_.x + x);
            grid_values_[idx] = SDF::evaluate_chladni_wave_2d(
                Vec2(u, v),
                p_mode_n,
                p_mode_m,
                p_amp_a,
                p_amp_b
            );
        }
    }
    emit_signal("geometry_changed");
    emit_changed();
}

float SDFGrid2DResource::sample_distance(const godot::Vector2 &p_world_pos) const {
    if (grid_values_.empty()) return 0.0f;
    return SDF::sample_grid_bilinear_2d(
        grid_values_,
        static_cast<uint32_t>(resolution_.x),
        static_cast<uint32_t>(resolution_.y),
        _world_to_grid_coord(p_world_pos)
    );
}

godot::Vector2 SDFGrid2DResource::sample_gradient(const godot::Vector2 &p_world_pos) const {
    if (grid_values_.empty()) return godot::Vector2();
    const Vec2 gc = _world_to_grid_coord(p_world_pos);
    const uint32_t cx = static_cast<uint32_t>(
        std::clamp(static_cast<int>(std::round(gc.x)), 0, std::max(0, resolution_.x - 1))
    );
    const uint32_t cy = static_cast<uint32_t>(
        std::clamp(static_cast<int>(std::round(gc.y)), 0, std::max(0, resolution_.y - 1))
    );
    const Vec2 cell_sz(
        world_bounds_.size.x / static_cast<float>(std::max(1, resolution_.x - 1)),
        world_bounds_.size.y / static_cast<float>(std::max(1, resolution_.y - 1))
    );
    const Vec2 g = SDF::compute_grid_gradient_2d(
        grid_values_,
        static_cast<uint32_t>(resolution_.x),
        static_cast<uint32_t>(resolution_.y),
        cx,
        cy,
        cell_sz
    );
    return godot::Vector2(g.x, g.y);
}

godot::PackedVector2Array SDFGrid2DResource::extract_iso_contour_segments(float p_iso_value) const {
    godot::PackedVector2Array out;
    if (grid_values_.empty()) return out;

    const Vec2 origin(world_bounds_.position.x, world_bounds_.position.y);
    const Vec2 cell_sz(
        world_bounds_.size.x / static_cast<float>(std::max(1, resolution_.x - 1)),
        world_bounds_.size.y / static_cast<float>(std::max(1, resolution_.y - 1))
    );

    const auto segs = SDF::extract_marching_squares_contours(
        grid_values_,
        static_cast<uint32_t>(resolution_.x),
        static_cast<uint32_t>(resolution_.y),
        p_iso_value,
        origin,
        cell_sz
    );

    out.resize(static_cast<int64_t>(segs.size() * 2u));
    for (size_t i = 0; i < segs.size(); ++i) {
        out.set(static_cast<int64_t>(i * 2u), godot::Vector2(segs[i].start.x, segs[i].start.y));
        out.set(static_cast<int64_t>(i * 2u + 1u), godot::Vector2(segs[i].end.x, segs[i].end.y));
    }
    return out;
}

} // namespace ideam::godot_ext

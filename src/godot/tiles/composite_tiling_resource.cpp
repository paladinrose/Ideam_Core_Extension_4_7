#include "composite_tiling_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

namespace ideam::godot_ext {

void CompositeTilingResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Layers & Boundary Regions", "");

    ClassDB::bind_method(D_METHOD("set_tiling_layers", "layers"), &CompositeTilingResource::set_tiling_layers);
    ClassDB::bind_method(D_METHOD("get_tiling_layers"), &CompositeTilingResource::get_tiling_layers);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::ARRAY,
            "tiling_layers",
            PROPERTY_HINT_ARRAY_TYPE,
            String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":WallpaperTilingResource"
        ),
        "set_tiling_layers",
        "get_tiling_layers"
    );

    ClassDB::bind_method(D_METHOD("set_region_splines", "splines"), &CompositeTilingResource::set_region_splines);
    ClassDB::bind_method(D_METHOD("get_region_splines"), &CompositeTilingResource::get_region_splines);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::ARRAY,
            "region_splines",
            PROPERTY_HINT_ARRAY_TYPE,
            String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":Spline2DResource"
        ),
        "set_region_splines",
        "get_region_splines"
    );

    ClassDB::bind_method(D_METHOD("set_layer_priorities", "priorities"), &CompositeTilingResource::set_layer_priorities);
    ClassDB::bind_method(D_METHOD("get_layer_priorities"), &CompositeTilingResource::get_layer_priorities);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "layer_priorities"), "set_layer_priorities", "get_layer_priorities");

    ClassDB::bind_method(D_METHOD("set_layer_seed_points", "seeds"), &CompositeTilingResource::set_layer_seed_points);
    ClassDB::bind_method(D_METHOD("get_layer_seed_points"), &CompositeTilingResource::get_layer_seed_points);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "layer_seed_points"), "set_layer_seed_points", "get_layer_seed_points");

    ADD_GROUP("Coverage & Seam Stitching", "");

    ClassDB::bind_method(D_METHOD("set_coverage_mode", "mode"), &CompositeTilingResource::set_coverage_mode);
    ClassDB::bind_method(D_METHOD("get_coverage_mode"), &CompositeTilingResource::get_coverage_mode);
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

    ClassDB::bind_method(D_METHOD("set_stitch_mode", "mode"), &CompositeTilingResource::set_stitch_mode);
    ClassDB::bind_method(D_METHOD("get_stitch_mode"), &CompositeTilingResource::get_stitch_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "stitch_mode",
            PROPERTY_HINT_ENUM,
            "Quad Bridge:0,Topological Seam Weld:1,Curvilinear Metric Morph:2"
        ),
        "set_stitch_mode",
        "get_stitch_mode"
    );

    ClassDB::bind_method(D_METHOD("set_max_bridge_span", "span"), &CompositeTilingResource::set_max_bridge_span);
    ClassDB::bind_method(D_METHOD("get_max_bridge_span"), &CompositeTilingResource::get_max_bridge_span);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "max_bridge_span", PROPERTY_HINT_RANGE, "0.0,50.0,0.05"),
        "set_max_bridge_span",
        "get_max_bridge_span"
    );

    ClassDB::bind_method(D_METHOD("set_fallback_aabb", "rect"), &CompositeTilingResource::set_fallback_aabb);
    ClassDB::bind_method(D_METHOD("get_fallback_aabb"), &CompositeTilingResource::get_fallback_aabb);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "fallback_aabb"), "set_fallback_aabb", "get_fallback_aabb");

    ClassDB::bind_method(D_METHOD("set_auto_hexize", "hexize"), &CompositeTilingResource::set_auto_hexize);
    ClassDB::bind_method(D_METHOD("get_auto_hexize"), &CompositeTilingResource::get_auto_hexize);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_hexize"), "set_auto_hexize", "get_auto_hexize");

    ClassDB::bind_method(D_METHOD("set_edge_subdivisions", "subdivs"), &CompositeTilingResource::set_edge_subdivisions);
    ClassDB::bind_method(D_METHOD("get_edge_subdivisions"), &CompositeTilingResource::get_edge_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "edge_subdivisions", PROPERTY_HINT_RANGE, "1,64,1"),
        "set_edge_subdivisions",
        "get_edge_subdivisions"
    );

    ClassDB::bind_method(D_METHOD("set_auto_rebuild", "auto_rebuild"), &CompositeTilingResource::set_auto_rebuild);
    ClassDB::bind_method(D_METHOD("get_auto_rebuild"), &CompositeTilingResource::get_auto_rebuild);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_rebuild"), "set_auto_rebuild", "get_auto_rebuild");

    // --- Topology & Queries ---
    ClassDB::bind_method(D_METHOD("rebuild"), &CompositeTilingResource::rebuild);
    ClassDB::bind_method(D_METHOD("get_cell_count"), &CompositeTilingResource::get_cell_count);
    ClassDB::bind_method(D_METHOD("get_edge_count"), &CompositeTilingResource::get_edge_count);
    ClassDB::bind_method(D_METHOD("get_hex_super_cell_count"), &CompositeTilingResource::get_hex_super_cell_count);
    ClassDB::bind_method(D_METHOD("get_bridge_quad_count"), &CompositeTilingResource::get_bridge_quad_count);

    ClassDB::bind_method(D_METHOD("get_shared_vertices"), &CompositeTilingResource::get_shared_vertices);
    ClassDB::bind_method(D_METHOD("get_triangle_indices"), &CompositeTilingResource::get_triangle_indices);
    ClassDB::bind_method(D_METHOD("get_extents_rect"), &CompositeTilingResource::get_extents_rect);

    ClassDB::bind_method(D_METHOD("get_cell_polygon", "cell_id"), &CompositeTilingResource::get_cell_polygon);
    ClassDB::bind_method(D_METHOD("get_cell_center", "cell_id"), &CompositeTilingResource::get_cell_center);
    ClassDB::bind_method(D_METHOD("get_cell_transform_2d", "cell_id"), &CompositeTilingResource::get_cell_transform_2d);
    ClassDB::bind_method(D_METHOD("get_cell_colour", "cell_id"), &CompositeTilingResource::get_cell_colour);
    ClassDB::bind_method(D_METHOD("get_cell_generation", "cell_id"), &CompositeTilingResource::get_cell_generation);
    ClassDB::bind_method(D_METHOD("get_cell_flags", "cell_id"), &CompositeTilingResource::get_cell_flags);
    ClassDB::bind_method(D_METHOD("get_cell_group_slot", "cell_id"), &CompositeTilingResource::get_cell_group_slot);
    ClassDB::bind_method(D_METHOD("get_cell_neighbors", "cell_id"), &CompositeTilingResource::get_cell_neighbors);
    ClassDB::bind_method(
        D_METHOD("get_cells_by_generation", "generation"),
        &CompositeTilingResource::get_cells_by_generation,
        DEFVAL(-1)
    );

    ClassDB::bind_method(D_METHOD("get_border_cell_ids"), &CompositeTilingResource::get_border_cell_ids);
    ClassDB::bind_method(D_METHOD("get_border_edge_ids"), &CompositeTilingResource::get_border_edge_ids);
    ClassDB::bind_method(D_METHOD("get_border_vertex_ids"), &CompositeTilingResource::get_border_vertex_ids);
    ClassDB::bind_method(D_METHOD("get_border_outline_splines"), &CompositeTilingResource::get_border_outline_splines);

    ClassDB::bind_method(D_METHOD("build_hex_super_cells"), &CompositeTilingResource::build_hex_super_cells);
    ClassDB::bind_method(D_METHOD("get_hex_center_points"), &CompositeTilingResource::get_hex_center_points);
    ClassDB::bind_method(D_METHOD("get_hex_perimeter_polygon", "hex_id"), &CompositeTilingResource::get_hex_perimeter_polygon);
    ClassDB::bind_method(D_METHOD("get_hex_child_cells", "hex_id"), &CompositeTilingResource::get_hex_child_cells);
    ClassDB::bind_method(D_METHOD("get_hex_neighbors", "hex_id"), &CompositeTilingResource::get_hex_neighbors);

    ClassDB::bind_method(D_METHOD("remove_cells", "cell_ids"), &CompositeTilingResource::remove_cells);
    ClassDB::bind_method(D_METHOD("commit_remove_cells", "cell_ids"), &CompositeTilingResource::commit_remove_cells);
    ClassDB::bind_method(D_METHOD("set_edge_port_direction", "edge_id", "direction"), &CompositeTilingResource::set_edge_port_direction);
    ClassDB::bind_method(D_METHOD("commit_set_edge_port_direction", "edge_id", "direction"), &CompositeTilingResource::commit_set_edge_port_direction);
    ClassDB::bind_method(D_METHOD("get_edge_port_direction", "edge_id"), &CompositeTilingResource::get_edge_port_direction);
    ClassDB::bind_method(D_METHOD("set_edge_cross_distance", "edge_id", "cost"), &CompositeTilingResource::set_edge_cross_distance);
    ClassDB::bind_method(D_METHOD("get_edge_cross_distance", "edge_id"), &CompositeTilingResource::get_edge_cross_distance);

    ClassDB::bind_method(D_METHOD("contains_point", "point"), &CompositeTilingResource::contains_point);
    ClassDB::bind_method(D_METHOD("hex_contains_point", "point"), &CompositeTilingResource::hex_contains_point);
    ClassDB::bind_method(
        D_METHOD("find_path_cells", "start_cell", "goal_cell", "manhattan_heuristic"),
        &CompositeTilingResource::find_path_cells,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("find_path_world_points", "start", "goal", "manhattan_heuristic"),
        &CompositeTilingResource::find_path_world_points,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("find_path_hex_super_cells", "start_hex", "goal_hex", "manhattan_heuristic"),
        &CompositeTilingResource::find_path_hex_super_cells,
        DEFVAL(false)
    );
    ClassDB::bind_method(D_METHOD("compute_flow_field_vectors", "goal_cells"), &CompositeTilingResource::compute_flow_field_vectors);

    ClassDB::bind_method(D_METHOD("_on_dependency_changed"), &CompositeTilingResource::_on_dependency_changed);
}

CompositeTilingResource::CompositeTilingResource() {
    is_dirty_ = true;
}

CompositeTilingResource::~CompositeTilingResource() {
    _disconnect_dependencies();
}

void CompositeTilingResource::_disconnect_dependencies() {
    const godot::Callable cb(this, "_on_dependency_changed");
    for (int i = 0; i < tiling_layers_.size(); ++i) {
        godot::Ref<WallpaperTilingResource> layer = tiling_layers_[i];
        if (layer.is_valid() && layer->is_connected("changed", cb)) {
            layer->disconnect("changed", cb);
        }
    }
    for (int i = 0; i < region_splines_.size(); ++i) {
        godot::Ref<Spline2DResource> spline = region_splines_[i];
        if (spline.is_valid() && spline->is_connected("changed", cb)) {
            spline->disconnect("changed", cb);
        }
    }
}

void CompositeTilingResource::_connect_dependencies() {
    const godot::Callable cb(this, "_on_dependency_changed");
    for (int i = 0; i < tiling_layers_.size(); ++i) {
        godot::Ref<WallpaperTilingResource> layer = tiling_layers_[i];
        if (layer.is_valid() && !layer->is_connected("changed", cb)) {
            layer->connect("changed", cb);
        }
    }
    for (int i = 0; i < region_splines_.size(); ++i) {
        godot::Ref<Spline2DResource> spline = region_splines_[i];
        if (spline.is_valid() && !spline->is_connected("changed", cb)) {
            spline->connect("changed", cb);
        }
    }
}

void CompositeTilingResource::_on_dependency_changed() {
    is_dirty_ = true;
    if (auto_rebuild_) {
        rebuild();
    } else {
        emit_signal("topology_changed");
        emit_changed();
    }
}

void CompositeTilingResource::_ensure_built() const {
    if (is_dirty_) {
        const_cast<CompositeTilingResource *>(this)->rebuild();
    }
}

// ============================================================================
// TilingResource Virtual Overrides
// ============================================================================

int CompositeTilingResource::get_vertex_count() const {
    _ensure_built();
    return static_cast<int>(core_region_.get_vertex_count());
}

int CompositeTilingResource::get_aspect_count() const {
    if (tiling_layers_.size() > 0) {
        godot::Ref<WallpaperTilingResource> primary = tiling_layers_[0];
        if (primary.is_valid()) {
            return primary->get_aspect_count();
        }
    }
    return 0;
}

int CompositeTilingResource::get_edge_shape_count() const {
    if (tiling_layers_.size() > 0) {
        godot::Ref<WallpaperTilingResource> primary = tiling_layers_[0];
        if (primary.is_valid()) {
            return primary->get_edge_shape_count();
        }
    }
    return 0;
}

godot::PackedVector2Array CompositeTilingResource::get_prototile_outline(int p_subdivisions) const {
    if (tiling_layers_.size() > 0) {
        godot::Ref<WallpaperTilingResource> primary = tiling_layers_[0];
        if (primary.is_valid()) {
            return primary->get_prototile_outline(p_subdivisions);
        }
    }
    return {};
}

godot::PackedInt32Array CompositeTilingResource::get_prototile_triangles(int p_subdivisions) const {
    if (tiling_layers_.size() > 0) {
        godot::Ref<WallpaperTilingResource> primary = tiling_layers_[0];
        if (primary.is_valid()) {
            return primary->get_prototile_triangles(p_subdivisions);
        }
    }
    return {};
}

godot::Dictionary CompositeTilingResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["layer_priorities"]  = layer_priorities_;
    d["layer_seed_points"] = layer_seed_points_;
    d["coverage_mode"]     = static_cast<int>(coverage_mode_);
    d["stitch_mode"]       = static_cast<int>(stitch_mode_);
    d["max_bridge_span"]   = max_bridge_span_;
    d["fallback_aabb"]     = fallback_aabb_;
    d["auto_hexize"]       = auto_hexize_;
    d["edge_subdivisions"] = edge_subdivisions_;
    return d;
}

void CompositeTilingResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.is_empty()) return;
    if (p_state.has("layer_priorities"))  layer_priorities_  = p_state["layer_priorities"];
    if (p_state.has("layer_seed_points")) layer_seed_points_ = p_state["layer_seed_points"];
    if (p_state.has("coverage_mode"))     coverage_mode_     = static_cast<TilingCoverageModeType>(int(p_state["coverage_mode"]));
    if (p_state.has("stitch_mode"))       stitch_mode_       = static_cast<TilingStitchModeType>(int(p_state["stitch_mode"]));
    if (p_state.has("max_bridge_span"))   max_bridge_span_   = static_cast<real_t>(double(p_state["max_bridge_span"]));
    if (p_state.has("fallback_aabb"))     fallback_aabb_     = p_state["fallback_aabb"];
    if (p_state.has("auto_hexize"))       auto_hexize_       = bool(p_state["auto_hexize"]);
    if (p_state.has("edge_subdivisions")) edge_subdivisions_ = std::clamp(int(p_state["edge_subdivisions"]), 1, 64);

    is_dirty_ = true;
    rebuild();
}

// ============================================================================
// Layer, Boundary Spline & Seam Stitching Properties
// ============================================================================

void CompositeTilingResource::set_tiling_layers(const godot::TypedArray<WallpaperTilingResource> &p_layers) {
    _disconnect_dependencies();
    tiling_layers_ = p_layers;
    _connect_dependencies();
    _on_dependency_changed();
}

void CompositeTilingResource::set_region_splines(const godot::TypedArray<Spline2DResource> &p_splines) {
    _disconnect_dependencies();
    region_splines_ = p_splines;
    _connect_dependencies();
    _on_dependency_changed();
}

void CompositeTilingResource::set_layer_priorities(const godot::PackedInt32Array &p_priorities) {
    layer_priorities_ = p_priorities;
    _on_dependency_changed();
}

void CompositeTilingResource::set_layer_seed_points(const godot::PackedVector2Array &p_seeds) {
    layer_seed_points_ = p_seeds;
    _on_dependency_changed();
}

void CompositeTilingResource::set_coverage_mode(TilingCoverageModeType p_mode) {
    const auto clamped = static_cast<TilingCoverageModeType>(std::clamp<int>(p_mode, 0, 3));
    if (coverage_mode_ == clamped) return;
    coverage_mode_ = clamped;
    _on_dependency_changed();
}

void CompositeTilingResource::set_stitch_mode(TilingStitchModeType p_mode) {
    const auto clamped = static_cast<TilingStitchModeType>(std::clamp<int>(p_mode, 0, 2));
    if (stitch_mode_ == clamped) return;
    stitch_mode_ = clamped;
    _on_dependency_changed();
}

void CompositeTilingResource::set_max_bridge_span(real_t p_span) {
    const real_t clamped = std::max<real_t>(static_cast<real_t>(0.0), p_span);
    if (std::abs(max_bridge_span_ - clamped) <= 1e-6f) return;
    max_bridge_span_ = clamped;
    _on_dependency_changed();
}

void CompositeTilingResource::set_fallback_aabb(const godot::Rect2 &p_rect) {
    if (fallback_aabb_ == p_rect) return;
    fallback_aabb_ = p_rect;
    _on_dependency_changed();
}

void CompositeTilingResource::set_auto_hexize(bool p_hexize) {
    if (auto_hexize_ == p_hexize) return;
    auto_hexize_ = p_hexize;
    _on_dependency_changed();
}

void CompositeTilingResource::set_edge_subdivisions(int p_subdivs) {
    const int clamped = std::clamp(p_subdivs, 1, 64);
    if (edge_subdivisions_ == clamped) return;
    edge_subdivisions_ = clamped;
    _on_dependency_changed();
}

void CompositeTilingResource::set_auto_rebuild(bool p_auto) {
    auto_rebuild_ = p_auto;
    if (auto_rebuild_ && is_dirty_) {
        rebuild();
    }
}

// ============================================================================
// Rebuild & Topological Queries
// ============================================================================

void CompositeTilingResource::rebuild() {
    is_dirty_ = false;
    core_region_.clear();

    const int layer_count = tiling_layers_.size();
    if (layer_count == 0) {
        emit_signal("topology_changed");
        emit_changed();
        return;
    }

    const auto cov_mode = static_cast<core::TilingCoverageMode>(coverage_mode_);
    const auto st_mode  = static_cast<core::TilingStitchMode>(stitch_mode_);
    const core::AABB2D<real_t> fallback_bounds(
        _to_glm(fallback_aabb_.position),
        _to_glm(fallback_aabb_.position + fallback_aabb_.size)
    );

    if (layer_count == 1) {
        godot::Ref<WallpaperTilingResource> wp = tiling_layers_[0];
        if (!wp.is_valid()) {
            emit_signal("topology_changed");
            emit_changed();
            return;
        }

        const int32_t prio = (layer_priorities_.size() > 0) ? layer_priorities_[0] : 0;
        const Vec2 seed = (layer_seed_points_.size() > 0)
            ? _to_glm(layer_seed_points_[0])
            : Vec2(static_cast<real_t>(0.0));

        core_region_.set_tiling(wp->get_core_tiling());
        core_region_.set_priority(prio);

        std::vector<CoreSpline> valid_splines;
        for (int i = 0; i < region_splines_.size(); ++i) {
            godot::Ref<Spline2DResource> sp = region_splines_[i];
            if (sp.is_valid() && sp->get_point_count() >= 3) {
                valid_splines.push_back(sp->get_core_spline());
            }
        }

        if (!valid_splines.empty()) {
            core_region_.build_from_splines(
                valid_splines,
                seed,
                cov_mode,
                edge_subdivisions_,
                auto_hexize_
            );
        } else {
            core_region_.build_from_aabb(
                fallback_bounds,
                seed,
                auto_hexize_,
                edge_subdivisions_
            );
        }

        emit_signal("topology_changed");
        emit_changed();
        return;
    }

    // Multi-layer build: construct each layer's TilingRegion2D and merge via TilingSeamStitcher2D
    std::vector<CoreRegion> built_regions;
    std::vector<int32_t> built_priorities;
    built_regions.reserve(static_cast<size_t>(layer_count));
    built_priorities.reserve(static_cast<size_t>(layer_count));

    for (int i = 0; i < layer_count; ++i) {
        godot::Ref<WallpaperTilingResource> wp = tiling_layers_[i];
        if (!wp.is_valid()) continue;

        const int32_t prio = (i < layer_priorities_.size())
            ? layer_priorities_[i]
            : static_cast<int32_t>(i);
        const Vec2 seed = (i < layer_seed_points_.size())
            ? _to_glm(layer_seed_points_[i])
            : Vec2(static_cast<real_t>(0.0));

        CoreRegion layer_reg(wp->get_core_tiling(), prio);

        godot::Ref<Spline2DResource> layer_spline;
        if (i < region_splines_.size()) {
            layer_spline = region_splines_[i];
        } else if (region_splines_.size() == 1) {
            layer_spline = region_splines_[0];
        }

        if (layer_spline.is_valid() && layer_spline->get_point_count() >= 3) {
            const CoreSpline &cs = layer_spline->get_core_spline();
            layer_reg.build_from_splines(
                std::span<const CoreSpline>(&cs, 1),
                seed,
                cov_mode,
                edge_subdivisions_,
                false
            );
        } else {
            layer_reg.build_from_aabb(
                fallback_bounds,
                seed,
                false,
                edge_subdivisions_
            );
        }

        if (!layer_reg.is_empty()) {
            built_regions.push_back(std::move(layer_reg));
            built_priorities.push_back(prio);
        }
    }

    if (built_regions.size() == 1) {
        core_region_ = std::move(built_regions[0]);
        if (auto_hexize_) {
            core_region_.hexize();
        }
    } else if (built_regions.size() > 1) {
        core_region_ = CoreStitcher::populate_composite_region(
            built_regions,
            built_priorities,
            st_mode,
            max_bridge_span_
        );
        if (auto_hexize_) {
            core_region_.hexize();
        }
    }

    emit_signal("topology_changed");
    emit_changed();
}

int CompositeTilingResource::get_cell_count() const {
    _ensure_built();
    return static_cast<int>(core_region_.get_cell_count());
}

int CompositeTilingResource::get_edge_count() const {
    _ensure_built();
    return static_cast<int>(core_region_.get_edge_count());
}

int CompositeTilingResource::get_hex_super_cell_count() const {
    _ensure_built();
    return static_cast<int>(core_region_.get_hex_super_cell_count());
}

int CompositeTilingResource::get_bridge_quad_count() const {
    _ensure_built();
    return static_cast<int>(core_region_.get_bridge_quads().size());
}

godot::PackedVector2Array CompositeTilingResource::get_shared_vertices() const {
    _ensure_built();
    const auto verts = core_region_.get_vertices();
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(verts.size()));
    for (size_t i = 0; i < verts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(verts[i]));
    }
    return out;
}

godot::PackedInt32Array CompositeTilingResource::get_triangle_indices() const {
    _ensure_built();
    const auto tris = core_region_.get_triangle_indices();
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(tris.size()));
    for (size_t i = 0; i < tris.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(tris[i]));
    }
    return out;
}

godot::Rect2 CompositeTilingResource::get_extents_rect() const {
    _ensure_built();
    const auto &ext = core_region_.get_extents();
    if (!ext.is_valid()) {
        return godot::Rect2();
    }
    return godot::Rect2(_to_godot(ext.min_bounds), _to_godot(ext.get_size()));
}

// ============================================================================
// Per-Cell Queries
// ============================================================================

godot::PackedVector2Array CompositeTilingResource::get_cell_polygon(int p_cell_id) const {
    _ensure_built();
    godot::PackedVector2Array out;
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_region_.get_cell_count()) {
        return out;
    }
    const auto poly = core_region_.get_cell_polygon_points(static_cast<uint32_t>(p_cell_id));
    out.resize(static_cast<int>(poly.size()));
    for (size_t i = 0; i < poly.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(poly[i]));
    }
    return out;
}

godot::Vector2 CompositeTilingResource::get_cell_center(int p_cell_id) const {
    _ensure_built();
    const auto cells = core_region_.get_cells();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= cells.size()) {
        return godot::Vector2();
    }
    return _to_godot_f(cells[static_cast<size_t>(p_cell_id)].center);
}

godot::Transform2D CompositeTilingResource::get_cell_transform_2d(int p_cell_id) const {
    _ensure_built();
    const auto xforms = core_region_.get_cell_transforms();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= xforms.size()) {
        return godot::Transform2D();
    }
    const auto &tr = xforms[static_cast<size_t>(p_cell_id)];
    const float c = std::cos(tr.angle_rad);
    const float s = std::sin(tr.angle_rad);
    return godot::Transform2D(
        c * tr.scale.x, s * tr.scale.x,
        -s * tr.scale.y, c * tr.scale.y,
        tr.position.x, tr.position.y
    );
}

int CompositeTilingResource::get_cell_colour(int p_cell_id) const {
    _ensure_built();
    const auto cells = core_region_.get_cells();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= cells.size()) {
        return 0;
    }
    return static_cast<int>(cells[static_cast<size_t>(p_cell_id)].colour_id);
}

int CompositeTilingResource::get_cell_generation(int p_cell_id) const {
    _ensure_built();
    const auto cells = core_region_.get_cells();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= cells.size()) {
        return 0;
    }
    return static_cast<int>(cells[static_cast<size_t>(p_cell_id)].generation);
}

int CompositeTilingResource::get_cell_flags(int p_cell_id) const {
    _ensure_built();
    const auto cells = core_region_.get_cells();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= cells.size()) {
        return 0;
    }
    return static_cast<int>(cells[static_cast<size_t>(p_cell_id)].flags);
}

int CompositeTilingResource::get_cell_group_slot(int p_cell_id) const {
    _ensure_built();
    const auto cells = core_region_.get_cells();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= cells.size()) {
        return 0;
    }
    return static_cast<int>(cells[static_cast<size_t>(p_cell_id)].group_slot);
}

godot::PackedInt32Array CompositeTilingResource::get_cell_neighbors(int p_cell_id) const {
    _ensure_built();
    godot::PackedInt32Array out;
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_region_.get_cell_count()) {
        return out;
    }
    const auto nbs = core_region_.get_cell_neighbors(static_cast<uint32_t>(p_cell_id));
    out.resize(static_cast<int>(nbs.size()));
    for (size_t i = 0; i < nbs.size(); ++i) {
        out.set(static_cast<int>(i), nbs[i]);
    }
    return out;
}

godot::PackedInt32Array CompositeTilingResource::get_cells_by_generation(int p_generation) const {
    _ensure_built();
    godot::PackedInt32Array out;
    if (p_generation < 0) {
        const auto sorted = core_region_.get_cell_ids_by_generation();
        out.resize(static_cast<int>(sorted.size()));
        for (size_t i = 0; i < sorted.size(); ++i) {
            out.set(static_cast<int>(i), static_cast<int32_t>(sorted[i]));
        }
        return out;
    }

    const auto cells = core_region_.get_cells();
    const uint16_t target_gen = static_cast<uint16_t>(p_generation);
    for (size_t i = 0; i < cells.size(); ++i) {
        if (cells[i].generation == target_gen) {
            out.push_back(static_cast<int32_t>(i));
        }
    }
    return out;
}

// ============================================================================
// Border Queries (`FindBorders`)
// ============================================================================

godot::PackedInt32Array CompositeTilingResource::get_border_cell_ids() const {
    _ensure_built();
    const auto ids = core_region_.get_border_cell_ids();
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(ids.size()));
    for (size_t i = 0; i < ids.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(ids[i]));
    }
    return out;
}

godot::PackedInt32Array CompositeTilingResource::get_border_edge_ids() const {
    _ensure_built();
    const auto ids = core_region_.get_border_edge_ids();
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(ids.size()));
    for (size_t i = 0; i < ids.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(ids[i]));
    }
    return out;
}

godot::PackedInt32Array CompositeTilingResource::get_border_vertex_ids() const {
    _ensure_built();
    const auto ids = core_region_.get_border_vertex_ids();
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(ids.size()));
    for (size_t i = 0; i < ids.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(ids[i]));
    }
    return out;
}

godot::TypedArray<Spline2DResource> CompositeTilingResource::get_border_outline_splines() const {
    _ensure_built();
    godot::TypedArray<Spline2DResource> out;
    const auto outlines = core_region_.get_border_outlines();
    for (const auto &cs : outlines) {
        godot::Ref<Spline2DResource> res;
        res.instantiate();
        res->get_core_spline_mut() = cs;
        out.push_back(res);
    }
    return out;
}

// ============================================================================
// Hex Super-Cell (`Hexize`) Queries
// ============================================================================

int CompositeTilingResource::build_hex_super_cells() {
    _ensure_built();
    core_region_.hexize();
    emit_signal("topology_changed");
    emit_changed();
    return static_cast<int>(core_region_.get_hex_super_cell_count());
}

godot::PackedVector2Array CompositeTilingResource::get_hex_center_points() const {
    _ensure_built();
    const auto pts = core_region_.get_hexagon_center_points();
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i]));
    }
    return out;
}

godot::PackedVector2Array CompositeTilingResource::get_hex_perimeter_polygon(int p_hex_id) const {
    _ensure_built();
    godot::PackedVector2Array out;
    const auto hexes = core_region_.get_hex_super_cells();
    if (p_hex_id < 0 || static_cast<size_t>(p_hex_id) >= hexes.size()) {
        return out;
    }
    const auto &hex = hexes[static_cast<size_t>(p_hex_id)];
    const auto verts = core_region_.get_vertices();
    for (uint32_t vid : hex.perimeter_vertex_ids) {
        if (vid < verts.size()) {
            out.push_back(_to_godot(verts[vid]));
        }
    }
    return out;
}

godot::PackedInt32Array CompositeTilingResource::get_hex_child_cells(int p_hex_id) const {
    _ensure_built();
    godot::PackedInt32Array out;
    const auto hexes = core_region_.get_hex_super_cells();
    if (p_hex_id < 0 || static_cast<size_t>(p_hex_id) >= hexes.size()) {
        return out;
    }
    const auto &hex = hexes[static_cast<size_t>(p_hex_id)];
    const auto children = core_region_.get_hex_child_cell_indices();
    const uint32_t offset = hex.get_child_offset();
    const uint32_t count  = hex.get_child_count();
    for (uint32_t i = 0; i < count && (offset + i) < children.size(); ++i) {
        out.push_back(static_cast<int32_t>(children[offset + i]));
    }
    return out;
}

godot::PackedInt32Array CompositeTilingResource::get_hex_neighbors(int p_hex_id) const {
    _ensure_built();
    godot::PackedInt32Array out;
    const auto hexes = core_region_.get_hex_super_cells();
    if (p_hex_id < 0 || static_cast<size_t>(p_hex_id) >= hexes.size()) {
        return out;
    }
    const auto &hex = hexes[static_cast<size_t>(p_hex_id)];
    for (int32_t nb : hex.neighbor_hex_ids) {
        if (nb >= 0) {
            out.push_back(nb);
        }
    }
    return out;
}

// ============================================================================
// Topological Mutations & Undo/Redo Counterparts
// ============================================================================

void CompositeTilingResource::remove_cells(const godot::PackedInt32Array &p_cell_ids) {
    _ensure_built();
    std::vector<uint32_t> ids;
    ids.reserve(static_cast<size_t>(p_cell_ids.size()));
    for (int i = 0; i < p_cell_ids.size(); ++i) {
        if (p_cell_ids[i] >= 0) {
            ids.push_back(static_cast<uint32_t>(p_cell_ids[i]));
        }
    }
    if (ids.empty()) return;

    core_region_.remove_cells(ids);
    if (auto_hexize_) {
        core_region_.hexize();
    }
    emit_signal("topology_changed");
    emit_changed();
}

void CompositeTilingResource::commit_remove_cells(const godot::PackedInt32Array &p_cell_ids) {
    if (p_cell_ids.size() == 0) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Remove Tiling Cells", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "remove_cells").bind(p_cell_ids));
    ur->add_undo_method(godot::Callable(this, "rebuild"));
    ur->commit_action(true);
}

void CompositeTilingResource::set_edge_port_direction(int p_edge_id, TilingPortDirectionType p_direction) {
    _ensure_built();
    auto meta = core_region_.get_edge_metadata_mut();
    if (p_edge_id < 0 || static_cast<size_t>(p_edge_id) >= meta.size()) {
        return;
    }
    const auto dir = static_cast<core::TilePortDirection>(std::clamp<int>(p_direction, 0, 3));
    if (meta[static_cast<size_t>(p_edge_id)].port_dir == dir) {
        return;
    }
    meta[static_cast<size_t>(p_edge_id)].port_dir = dir;
    emit_changed();
}

void CompositeTilingResource::commit_set_edge_port_direction(int p_edge_id, TilingPortDirectionType p_direction) {
    const TilingPortDirectionType prev = get_edge_port_direction(p_edge_id);
    if (prev == p_direction) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Set Edge Port Direction", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_edge_port_direction").bind(p_edge_id, static_cast<int>(p_direction)));
    ur->add_undo_method(godot::Callable(this, "set_edge_port_direction").bind(p_edge_id, static_cast<int>(prev)));
    ur->commit_action(true);
}

TilingPortDirectionType CompositeTilingResource::get_edge_port_direction(int p_edge_id) const {
    _ensure_built();
    const auto meta = core_region_.get_edge_metadata();
    if (p_edge_id < 0 || static_cast<size_t>(p_edge_id) >= meta.size()) {
        return PORT_DIR_FREE;
    }
    return static_cast<TilingPortDirectionType>(meta[static_cast<size_t>(p_edge_id)].port_dir);
}

void CompositeTilingResource::set_edge_cross_distance(int p_edge_id, real_t p_cost) {
    _ensure_built();
    auto meta = core_region_.get_edge_metadata_mut();
    if (p_edge_id < 0 || static_cast<size_t>(p_edge_id) >= meta.size()) {
        return;
    }
    meta[static_cast<size_t>(p_edge_id)].cross_distance = std::max(0.0f, static_cast<float>(p_cost));
    emit_changed();
}

real_t CompositeTilingResource::get_edge_cross_distance(int p_edge_id) const {
    _ensure_built();
    const auto meta = core_region_.get_edge_metadata();
    if (p_edge_id < 0 || static_cast<size_t>(p_edge_id) >= meta.size()) {
        return static_cast<real_t>(0.0);
    }
    return static_cast<real_t>(meta[static_cast<size_t>(p_edge_id)].cross_distance);
}

// ============================================================================
// Spatial Containment & Navigation Queries (`TilingNavigation2D`)
// ============================================================================

int CompositeTilingResource::contains_point(const godot::Vector2 &p_point) const {
    _ensure_built();
    int32_t cell_id = -1;
    if (core_region_.contains_point(_to_glm(p_point), cell_id)) {
        return static_cast<int>(cell_id);
    }
    return -1;
}

int CompositeTilingResource::hex_contains_point(const godot::Vector2 &p_point) const {
    _ensure_built();
    int32_t hex_id = -1;
    if (core_region_.hex_contains_point(_to_glm(p_point), hex_id)) {
        return static_cast<int>(hex_id);
    }
    return -1;
}

godot::PackedInt32Array CompositeTilingResource::find_path_cells(
    int p_start_cell,
    int p_goal_cell,
    bool p_manhattan_heuristic
) const {
    _ensure_built();
    godot::PackedInt32Array out;
    if (p_start_cell < 0 || p_goal_cell < 0) {
        return out;
    }

    const auto path = CoreNav::find_path_cells(
        core_region_,
        static_cast<uint32_t>(p_start_cell),
        static_cast<uint32_t>(p_goal_cell),
        {},
        p_manhattan_heuristic
    );
    out.resize(static_cast<int>(path.size()));
    for (size_t i = 0; i < path.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(path[i]));
    }
    return out;
}

godot::PackedVector2Array CompositeTilingResource::find_path_world_points(
    const godot::Vector2 &p_start,
    const godot::Vector2 &p_goal,
    bool p_manhattan_heuristic
) const {
    _ensure_built();
    godot::PackedVector2Array out;

    int32_t start_cid = -1;
    int32_t goal_cid  = -1;
    if (!core_region_.contains_point(_to_glm(p_start), start_cid) ||
        !core_region_.contains_point(_to_glm(p_goal), goal_cid)) {
        return out;
    }

    const auto path = CoreNav::find_path_cells(
        core_region_,
        static_cast<uint32_t>(start_cid),
        static_cast<uint32_t>(goal_cid),
        {},
        p_manhattan_heuristic
    );
    if (path.empty()) {
        return out;
    }

    const auto cells = core_region_.get_cells();
    out.resize(static_cast<int>(path.size()));
    for (size_t i = 0; i < path.size(); ++i) {
        const uint32_t cid = path[i];
        if (cid < cells.size()) {
            out.set(static_cast<int>(i), _to_godot_f(cells[cid].center));
        }
    }
    return out;
}

godot::PackedInt32Array CompositeTilingResource::find_path_hex_super_cells(
    int p_start_hex,
    int p_goal_hex,
    bool p_manhattan_heuristic
) const {
    _ensure_built();
    godot::PackedInt32Array out;
    if (p_start_hex < 0 || p_goal_hex < 0) {
        return out;
    }

    const auto path = CoreNav::find_path_hex_super_cells(
        core_region_,
        static_cast<uint32_t>(p_start_hex),
        static_cast<uint32_t>(p_goal_hex),
        p_manhattan_heuristic
    );
    out.resize(static_cast<int>(path.size()));
    for (size_t i = 0; i < path.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(path[i]));
    }
    return out;
}

godot::PackedVector2Array CompositeTilingResource::compute_flow_field_vectors(
    const godot::PackedInt32Array &p_goal_cells
) const {
    _ensure_built();
    godot::PackedVector2Array out;
    std::vector<uint32_t> goals;
    goals.reserve(static_cast<size_t>(p_goal_cells.size()));
    for (int i = 0; i < p_goal_cells.size(); ++i) {
        if (p_goal_cells[i] >= 0) {
            goals.push_back(static_cast<uint32_t>(p_goal_cells[i]));
        }
    }

    const auto ff = CoreNav::compute_flow_field(core_region_, goals);
    out.resize(static_cast<int>(ff.flow_vectors.size()));
    for (size_t i = 0; i < ff.flow_vectors.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(ff.flow_vectors[i]));
    }
    return out;
}

} // namespace ideam::godot_ext
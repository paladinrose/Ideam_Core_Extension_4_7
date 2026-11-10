#include "voronoi_diagram_2d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void VoronoiDiagram2DResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Voronoi Diagram", "");

    ClassDB::bind_method(D_METHOD("set_clip_bounds", "bounds"), &VoronoiDiagram2DResource::set_clip_bounds);
    ClassDB::bind_method(D_METHOD("get_clip_bounds"), &VoronoiDiagram2DResource::get_clip_bounds);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "clip_bounds"), "set_clip_bounds", "get_clip_bounds");

    ClassDB::bind_method(D_METHOD("set_sites", "sites"), &VoronoiDiagram2DResource::set_sites);
    ClassDB::bind_method(D_METHOD("get_sites"), &VoronoiDiagram2DResource::get_sites);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "sites"), "set_sites", "get_sites");

    // --- Site Mutators ---
    ClassDB::bind_method(D_METHOD("get_site_count"), &VoronoiDiagram2DResource::get_site_count);
    ClassDB::bind_method(D_METHOD("add_site", "site"), &VoronoiDiagram2DResource::add_site);
    ClassDB::bind_method(D_METHOD("set_site", "index", "site"), &VoronoiDiagram2DResource::set_site);
    ClassDB::bind_method(D_METHOD("get_site", "index"), &VoronoiDiagram2DResource::get_site);
    ClassDB::bind_method(D_METHOD("remove_site", "index"), &VoronoiDiagram2DResource::remove_site);
    ClassDB::bind_method(D_METHOD("clear"), &VoronoiDiagram2DResource::clear);

    // --- Delaunay, Voronoi & CSR Queries ---
    ClassDB::bind_method(D_METHOD("get_delaunay_triangles"), &VoronoiDiagram2DResource::get_delaunay_triangles);
    ClassDB::bind_method(D_METHOD("get_cell_polygon", "site_index"), &VoronoiDiagram2DResource::get_cell_polygon);
    ClassDB::bind_method(D_METHOD("get_cell_centroid", "site_index"), &VoronoiDiagram2DResource::get_cell_centroid);
    ClassDB::bind_method(D_METHOD("get_cell_area", "site_index"), &VoronoiDiagram2DResource::get_cell_area);
    ClassDB::bind_method(D_METHOD("find_containing_cell", "point"), &VoronoiDiagram2DResource::find_containing_cell);

    ClassDB::bind_method(D_METHOD("get_site_neighbors", "site_index"), &VoronoiDiagram2DResource::get_site_neighbors);
    ClassDB::bind_method(D_METHOD("get_csr_row_offsets"), &VoronoiDiagram2DResource::get_csr_row_offsets);
    ClassDB::bind_method(D_METHOD("get_csr_col_neighbors"), &VoronoiDiagram2DResource::get_csr_col_neighbors);

    ClassDB::bind_method(
        D_METHOD("apply_lloyd_relaxation_step", "step_strength"),
        &VoronoiDiagram2DResource::apply_lloyd_relaxation_step,
        DEFVAL(1.0)
    );

    // --- Undo/Redo Commit Wrappers ---
    ClassDB::bind_method(D_METHOD("commit_add_site", "site"), &VoronoiDiagram2DResource::commit_add_site);
    ClassDB::bind_method(D_METHOD("commit_set_site", "index", "site"), &VoronoiDiagram2DResource::commit_set_site);
    ClassDB::bind_method(D_METHOD("commit_remove_site", "index"), &VoronoiDiagram2DResource::commit_remove_site);
    ClassDB::bind_method(
        D_METHOD("commit_lloyd_relaxation_step", "step_strength"),
        &VoronoiDiagram2DResource::commit_lloyd_relaxation_step,
        DEFVAL(1.0)
    );
}

void VoronoiDiagram2DResource::_mark_dirty() {
    cache_dirty_ = true;
    emit_signal("geometry_changed");
    emit_changed();
}

void VoronoiDiagram2DResource::_ensure_built() const {
    if (!cache_dirty_) return;

    cached_delaunay_indices_.clear();
    cached_cells_.clear();
    cached_csr_row_offsets_.clear();
    cached_csr_col_neighbors_.clear();

    const core::AABB2D<real_t> bounds(
        _to_glm(clip_bounds_.position),
        _to_glm(clip_bounds_.position + clip_bounds_.size)
    );

    if (!sites_.empty()) {
        cached_cells_ = VorMath::build_clipped_voronoi_cells(sites_, bounds);
    }
    if (sites_.size() >= 3) {
        cached_delaunay_indices_ = VorMath::triangulate_delaunay(sites_);
        VorMath::extract_delaunay_adjacency_csr(
            sites_.size(),
            cached_delaunay_indices_,
            cached_csr_row_offsets_,
            cached_csr_col_neighbors_
        );
    } else {
        cached_csr_row_offsets_.assign(sites_.size() + 1u, 0u);
    }

    cache_dirty_ = false;
}

godot::Dictionary VoronoiDiagram2DResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["clip_bounds"] = clip_bounds_;
    d["sites"]       = get_sites();
    return d;
}

void VoronoiDiagram2DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("clip_bounds")) {
        clip_bounds_ = p_state["clip_bounds"];
    }
    if (p_state.has("sites")) {
        const godot::PackedVector2Array arr = p_state["sites"];
        sites_.clear();
        sites_.reserve(static_cast<size_t>(arr.size()));
        for (int64_t i = 0; i < arr.size(); ++i) {
            sites_.push_back(_to_glm(arr[i]));
        }
    }
    _mark_dirty();
}

void VoronoiDiagram2DResource::set_sites(const godot::PackedVector2Array &p_sites) {
    sites_.clear();
    sites_.reserve(static_cast<size_t>(p_sites.size()));
    for (int64_t i = 0; i < p_sites.size(); ++i) {
        sites_.push_back(_to_glm(p_sites[i]));
    }
    _mark_dirty();
}

godot::PackedVector2Array VoronoiDiagram2DResource::get_sites() const {
    godot::PackedVector2Array out;
    out.resize(static_cast<int64_t>(sites_.size()));
    for (size_t i = 0; i < sites_.size(); ++i) {
        out.set(static_cast<int64_t>(i), _to_godot(sites_[i]));
    }
    return out;
}

void VoronoiDiagram2DResource::set_clip_bounds(const godot::Rect2 &p_bounds) {
    if (clip_bounds_ == p_bounds) return;
    clip_bounds_ = p_bounds;
    _mark_dirty();
}

void VoronoiDiagram2DResource::add_site(const godot::Vector2 &p_site) {
    sites_.push_back(_to_glm(p_site));
    _mark_dirty();
}

void VoronoiDiagram2DResource::set_site(int p_index, const godot::Vector2 &p_site) {
    if (p_index < 0 || p_index >= static_cast<int>(sites_.size())) return;
    sites_[static_cast<size_t>(p_index)] = _to_glm(p_site);
    _mark_dirty();
}

godot::Vector2 VoronoiDiagram2DResource::get_site(int p_index) const {
    if (p_index < 0 || p_index >= static_cast<int>(sites_.size())) return godot::Vector2();
    return _to_godot(sites_[static_cast<size_t>(p_index)]);
}

void VoronoiDiagram2DResource::remove_site(int p_index) {
    if (p_index < 0 || p_index >= static_cast<int>(sites_.size())) return;
    sites_.erase(sites_.begin() + p_index);
    _mark_dirty();
}

void VoronoiDiagram2DResource::clear() {
    if (sites_.empty()) return;
    sites_.clear();
    _mark_dirty();
}

godot::PackedInt32Array VoronoiDiagram2DResource::get_delaunay_triangles() const {
    _ensure_built();
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(cached_delaunay_indices_.size()));
    for (size_t i = 0; i < cached_delaunay_indices_.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(cached_delaunay_indices_[i]));
    }
    return out;
}

godot::PackedVector2Array VoronoiDiagram2DResource::get_cell_polygon(int p_site_index) const {
    _ensure_built();
    godot::PackedVector2Array out;
    if (p_site_index < 0 || p_site_index >= static_cast<int>(cached_cells_.size())) return out;
    const auto &verts = cached_cells_[static_cast<size_t>(p_site_index)].polygon_vertices;
    out.resize(static_cast<int64_t>(verts.size()));
    for (size_t i = 0; i < verts.size(); ++i) {
        out.set(static_cast<int64_t>(i), _to_godot(verts[i]));
    }
    return out;
}

godot::Vector2 VoronoiDiagram2DResource::get_cell_centroid(int p_site_index) const {
    _ensure_built();
    if (p_site_index < 0 || p_site_index >= static_cast<int>(cached_cells_.size())) return godot::Vector2();
    return _to_godot(PolyMath::centroid(cached_cells_[static_cast<size_t>(p_site_index)].polygon_vertices));
}

real_t VoronoiDiagram2DResource::get_cell_area(int p_site_index) const {
    _ensure_built();
    if (p_site_index < 0 || p_site_index >= static_cast<int>(cached_cells_.size())) return static_cast<real_t>(0.0);
    return PolyMath::abs_area(cached_cells_[static_cast<size_t>(p_site_index)].polygon_vertices);
}

int VoronoiDiagram2DResource::find_containing_cell(const godot::Vector2 &p_point) const {
    _ensure_built();
    const Vec2 pt = _to_glm(p_point);
    for (size_t i = 0; i < cached_cells_.size(); ++i) {
        if (PolyMath::contains_point(cached_cells_[i].polygon_vertices, pt)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

godot::PackedInt32Array VoronoiDiagram2DResource::get_site_neighbors(int p_site_index) const {
    _ensure_built();
    godot::PackedInt32Array out;
    if (p_site_index < 0 || p_site_index >= static_cast<int>(cached_cells_.size())) return out;
    const auto &nbrs = cached_cells_[static_cast<size_t>(p_site_index)].neighbor_site_indices;
    out.resize(static_cast<int64_t>(nbrs.size()));
    for (size_t i = 0; i < nbrs.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(nbrs[i]));
    }
    return out;
}

godot::PackedInt32Array VoronoiDiagram2DResource::get_csr_row_offsets() const {
    _ensure_built();
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(cached_csr_row_offsets_.size()));
    for (size_t i = 0; i < cached_csr_row_offsets_.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(cached_csr_row_offsets_[i]));
    }
    return out;
}

godot::PackedInt32Array VoronoiDiagram2DResource::get_csr_col_neighbors() const {
    _ensure_built();
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(cached_csr_col_neighbors_.size()));
    for (size_t i = 0; i < cached_csr_col_neighbors_.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(cached_csr_col_neighbors_[i]));
    }
    return out;
}

void VoronoiDiagram2DResource::apply_lloyd_relaxation_step(real_t p_step_strength) {
    if (sites_.empty()) return;
    _ensure_built();
    const real_t alpha = std::clamp(p_step_strength, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    for (size_t i = 0; i < sites_.size() && i < cached_cells_.size(); ++i) {
        const auto &poly = cached_cells_[i].polygon_vertices;
        if (poly.size() >= 3) {
            const Vec2 c = PolyMath::centroid(poly);
            sites_[i] = glm::mix(sites_[i], c, alpha);
        }
    }
    _mark_dirty();
}

void VoronoiDiagram2DResource::commit_add_site(const godot::Vector2 &p_site) {
    const godot::Dictionary before = get_snapshot_state();
    add_site(p_site);
    commit_snapshot_action("Add Voronoi Site", before, get_snapshot_state());
}

void VoronoiDiagram2DResource::commit_set_site(int p_index, const godot::Vector2 &p_site) {
    if (p_index < 0 || p_index >= get_site_count()) return;
    const godot::Vector2 prev = get_site(p_index);
    if (prev == p_site) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Move Voronoi Site", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_site").bind(p_index, p_site));
    ur->add_undo_method(godot::Callable(this, "set_site").bind(p_index, prev));
    ur->commit_action(true);
}

void VoronoiDiagram2DResource::commit_remove_site(int p_index) {
    if (p_index < 0 || p_index >= get_site_count()) return;
    const godot::Dictionary before = get_snapshot_state();
    remove_site(p_index);
    commit_snapshot_action("Remove Voronoi Site", before, get_snapshot_state());
}

void VoronoiDiagram2DResource::commit_lloyd_relaxation_step(real_t p_step_strength) {
    if (sites_.empty()) return;
    const godot::Dictionary before = get_snapshot_state();
    apply_lloyd_relaxation_step(p_step_strength);
    commit_snapshot_action("Lloyd Voronoi Relaxation", before, get_snapshot_state());
}

} // namespace ideam::godot_ext

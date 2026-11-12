#include "voronoi_diagram_2d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ideam::godot_ext {

namespace {

[[nodiscard]] inline uint64_t splitmix64_next(uint64_t &r_state) noexcept {
    uint64_t z = (r_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31U);
}

[[nodiscard]] inline float rand_uniform_01(uint64_t &r_state) noexcept {
    return static_cast<float>((splitmix64_next(r_state) >> 11U) * (1.0 / 9007199254740992.0));
}

} // namespace

void VoronoiDiagram2DResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Voronoi Diagram", "");

    ClassDB::bind_method(D_METHOD("set_clip_bounds", "bounds"), &VoronoiDiagram2DResource::set_clip_bounds);
    ClassDB::bind_method(D_METHOD("get_clip_bounds"), &VoronoiDiagram2DResource::get_clip_bounds);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "clip_bounds"), "set_clip_bounds", "get_clip_bounds");

    ClassDB::bind_method(D_METHOD("set_clip_polygon", "polygon"), &VoronoiDiagram2DResource::set_clip_polygon);
    ClassDB::bind_method(D_METHOD("get_clip_polygon"), &VoronoiDiagram2DResource::get_clip_polygon);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "clip_polygon", PROPERTY_HINT_RESOURCE_TYPE, "Polygon2DResource"),
        "set_clip_polygon",
        "get_clip_polygon"
    );

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

    // --- Procedural Site Generators ---
    ClassDB::bind_method(
        D_METHOD("generate_grid_sites", "columns", "rows", "jitter", "seed"),
        &VoronoiDiagram2DResource::generate_grid_sites,
        DEFVAL(4),
        DEFVAL(4),
        DEFVAL(0.35),
        DEFVAL(1337)
    );
    ClassDB::bind_method(
        D_METHOD("generate_poisson_sites", "target_count", "min_distance", "seed"),
        &VoronoiDiagram2DResource::generate_poisson_sites,
        DEFVAL(16),
        DEFVAL(1.2),
        DEFVAL(1337)
    );

    // --- Delaunay, Voronoi & CSR Queries ---
    ClassDB::bind_method(D_METHOD("get_delaunay_triangles"), &VoronoiDiagram2DResource::get_delaunay_triangles);
    ClassDB::bind_method(D_METHOD("get_cell_polygon", "site_index"), &VoronoiDiagram2DResource::get_cell_polygon);
    ClassDB::bind_method(D_METHOD("get_cell_centroid", "site_index"), &VoronoiDiagram2DResource::get_cell_centroid);
    ClassDB::bind_method(D_METHOD("get_cell_area", "site_index"), &VoronoiDiagram2DResource::get_cell_area);
    ClassDB::bind_method(D_METHOD("find_containing_cell", "point"), &VoronoiDiagram2DResource::find_containing_cell);
    ClassDB::bind_method(
        D_METHOD("export_cell_as_polygon_resource", "site_index"),
        &VoronoiDiagram2DResource::export_cell_as_polygon_resource
    );

    ClassDB::bind_method(D_METHOD("get_site_neighbors", "site_index"), &VoronoiDiagram2DResource::get_site_neighbors);
    ClassDB::bind_method(D_METHOD("get_csr_row_offsets"), &VoronoiDiagram2DResource::get_csr_row_offsets);
    ClassDB::bind_method(D_METHOD("get_csr_col_neighbors"), &VoronoiDiagram2DResource::get_csr_col_neighbors);

    ClassDB::bind_method(
        D_METHOD("apply_lloyd_relaxation_step", "step_strength"),
        &VoronoiDiagram2DResource::apply_lloyd_relaxation_step,
        DEFVAL(1.0)
    );

    // --- Undo/Redo Commit Wrappers ---
    ClassDB::bind_method(D_METHOD("commit_set_clip_bounds", "bounds"), &VoronoiDiagram2DResource::commit_set_clip_bounds);
    ClassDB::bind_method(D_METHOD("commit_set_clip_polygon", "polygon"), &VoronoiDiagram2DResource::commit_set_clip_polygon);
    ClassDB::bind_method(D_METHOD("commit_clear"), &VoronoiDiagram2DResource::commit_clear);
    ClassDB::bind_method(D_METHOD("commit_add_site", "site"), &VoronoiDiagram2DResource::commit_add_site);
    ClassDB::bind_method(D_METHOD("commit_set_site", "index", "site"), &VoronoiDiagram2DResource::commit_set_site);
    ClassDB::bind_method(D_METHOD("commit_remove_site", "index"), &VoronoiDiagram2DResource::commit_remove_site);
    ClassDB::bind_method(
        D_METHOD("commit_lloyd_relaxation_step", "step_strength"),
        &VoronoiDiagram2DResource::commit_lloyd_relaxation_step,
        DEFVAL(1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_generate_grid_sites", "columns", "rows", "jitter", "seed"),
        &VoronoiDiagram2DResource::commit_generate_grid_sites,
        DEFVAL(4),
        DEFVAL(4),
        DEFVAL(0.35),
        DEFVAL(1337)
    );
    ClassDB::bind_method(
        D_METHOD("commit_generate_poisson_sites", "target_count", "min_distance", "seed"),
        &VoronoiDiagram2DResource::commit_generate_poisson_sites,
        DEFVAL(16),
        DEFVAL(1.2),
        DEFVAL(1337)
    );

    ClassDB::bind_method(D_METHOD("_on_clip_polygon_changed"), &VoronoiDiagram2DResource::_on_clip_polygon_changed);
}

VoronoiDiagram2DResource::~VoronoiDiagram2DResource() {
    if (clip_polygon_.is_valid()) {
        const godot::Callable cb(this, "_on_clip_polygon_changed");
        if (clip_polygon_->is_connected("changed", cb)) {
            clip_polygon_->disconnect("changed", cb);
        }
    }
}

void VoronoiDiagram2DResource::_mark_dirty() {
    cache_dirty_ = true;
    emit_signal("geometry_changed");
    emit_changed();
}

void VoronoiDiagram2DResource::_on_clip_polygon_changed() {
    _mark_dirty();
}

void VoronoiDiagram2DResource::_ensure_built() const {
    if (!cache_dirty_) return;

    cached_delaunay_indices_.clear();
    cached_cells_.clear();
    cached_csr_row_offsets_.clear();
    cached_csr_col_neighbors_.clear();

    core::AABB2D<real_t> bounds(
        _to_glm(clip_bounds_.position),
        _to_glm(clip_bounds_.position + clip_bounds_.size)
    );

    const bool has_clip_poly = clip_polygon_.is_valid() && clip_polygon_->get_vertex_count() >= 3;
    if (has_clip_poly) {
        const godot::Rect2 poly_bounds = clip_polygon_->get_bounds_2d();
        if (poly_bounds.size.x > 1e-4f && poly_bounds.size.y > 1e-4f) {
            const Vec2 pmin = _to_glm(poly_bounds.position);
            const Vec2 pmax = _to_glm(poly_bounds.position + poly_bounds.size);
            bounds.min_bounds = glm::min(bounds.min_bounds, pmin);
            bounds.max_bounds = glm::max(bounds.max_bounds, pmax);
        }
    }

    if (!sites_.empty()) {
        cached_cells_ = VorMath::build_clipped_voronoi_cells(sites_, bounds);
        if (has_clip_poly) {
            std::span<const Vec2> clip_verts = clip_polygon_->get_vertices_span();
            for (auto &cell : cached_cells_) {
                if (cell.polygon_vertices.size() >= 3) {
                    cell.polygon_vertices = CSGMath::clip_polygon_convex(cell.polygon_vertices, clip_verts);
                }
            }
        }
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
    d["clip_bounds"]  = clip_bounds_;
    d["clip_polygon"] = clip_polygon_;
    d["sites"]        = get_sites();
    return d;
}

void VoronoiDiagram2DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("clip_bounds")) {
        clip_bounds_ = p_state["clip_bounds"];
    }
    if (p_state.has("clip_polygon")) {
        set_clip_polygon(p_state["clip_polygon"]);
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

void VoronoiDiagram2DResource::set_clip_polygon(const godot::Ref<Polygon2DResource> &p_polygon) {
    if (clip_polygon_ == p_polygon) return;

    const godot::Callable cb(this, "_on_clip_polygon_changed");
    if (clip_polygon_.is_valid() && clip_polygon_->is_connected("changed", cb)) {
        clip_polygon_->disconnect("changed", cb);
    }

    clip_polygon_ = p_polygon;

    if (clip_polygon_.is_valid() && !clip_polygon_->is_connected("changed", cb)) {
        clip_polygon_->connect("changed", cb);
    }
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

void VoronoiDiagram2DResource::generate_grid_sites(
    int p_columns,
    int p_rows,
    real_t p_jitter,
    int64_t p_seed
) {
    const int cols = std::max(1, p_columns);
    const int rows = std::max(1, p_rows);
    const real_t jitter = std::clamp(p_jitter, static_cast<real_t>(0.0), static_cast<real_t>(0.95));
    uint64_t rng = static_cast<uint64_t>(p_seed) ^ 0xA5A5A5A55A5A5A5AULL;

    const bool has_poly = clip_polygon_.is_valid() && clip_polygon_->get_vertex_count() >= 3;
    const godot::Rect2 active_bounds = has_poly ? clip_polygon_->get_bounds_2d() : clip_bounds_;
    const float cell_w = active_bounds.size.x / static_cast<float>(cols);
    const float cell_h = active_bounds.size.y / static_cast<float>(rows);

    sites_.clear();
    sites_.reserve(static_cast<size_t>(cols * rows));

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const float jx = (rand_uniform_01(rng) - 0.5f) * static_cast<float>(jitter) * cell_w;
            const float jy = (rand_uniform_01(rng) - 0.5f) * static_cast<float>(jitter) * cell_h;
            const godot::Vector2 pt(
                active_bounds.position.x + (static_cast<float>(c) + 0.5f) * cell_w + jx,
                active_bounds.position.y + (static_cast<float>(r) + 0.5f) * cell_h + jy
            );
            if (!has_poly || clip_polygon_->contains_point(pt)) {
                sites_.push_back(_to_glm(pt));
            }
        }
    }
    _mark_dirty();
}

void VoronoiDiagram2DResource::generate_poisson_sites(
    int p_target_count,
    real_t p_min_distance,
    int64_t p_seed
) {
    const int target = std::clamp(p_target_count, 1, 2048);
    const float min_d = std::max(0.05f, static_cast<float>(p_min_distance));
    const float min_d_sq = min_d * min_d;
    uint64_t rng = static_cast<uint64_t>(p_seed) ^ 0x5F3759DF1337BEEFULL;

    const bool has_poly = clip_polygon_.is_valid() && clip_polygon_->get_vertex_count() >= 3;
    const godot::Rect2 active_bounds = has_poly ? clip_polygon_->get_bounds_2d() : clip_bounds_;

    sites_.clear();
    sites_.reserve(static_cast<size_t>(target));

    const int max_attempts = target * 36;
    for (int attempt = 0; attempt < max_attempts && static_cast<int>(sites_.size()) < target; ++attempt) {
        const godot::Vector2 candidate(
            active_bounds.position.x + rand_uniform_01(rng) * active_bounds.size.x,
            active_bounds.position.y + rand_uniform_01(rng) * active_bounds.size.y
        );
        if (has_poly && !clip_polygon_->contains_point(candidate)) {
            continue;
        }
        const Vec2 cand_glm = _to_glm(candidate);
        bool too_close = false;
        for (const Vec2 &existing : sites_) {
            const Vec2 diff = existing - cand_glm;
            if (glm::dot(diff, diff) < static_cast<real_t>(min_d_sq)) {
                too_close = true;
                break;
            }
        }
        if (!too_close) {
            sites_.push_back(cand_glm);
        }
    }
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

godot::Ref<Polygon2DResource> VoronoiDiagram2DResource::export_cell_as_polygon_resource(int p_site_index) const {
    godot::Ref<Polygon2DResource> poly;
    const godot::PackedVector2Array cell_verts = get_cell_polygon(p_site_index);
    if (cell_verts.size() < 3) return poly;
    poly.instantiate();
    poly->set_closed(true);
    poly->set_vertices(cell_verts);
    return poly;
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

void VoronoiDiagram2DResource::commit_set_clip_bounds(const godot::Rect2 &p_bounds) {
    if (clip_bounds_ == p_bounds) return;
    const godot::Dictionary before = get_snapshot_state();
    set_clip_bounds(p_bounds);
    commit_snapshot_action("Set Voronoi Clip Bounds", before, get_snapshot_state());
}

void VoronoiDiagram2DResource::commit_set_clip_polygon(const godot::Ref<Polygon2DResource> &p_polygon) {
    if (clip_polygon_ == p_polygon) return;
    const godot::Dictionary before = get_snapshot_state();
    set_clip_polygon(p_polygon);
    commit_snapshot_action("Bind Voronoi Clip Polygon", before, get_snapshot_state());
}

void VoronoiDiagram2DResource::commit_clear() {
    if (sites_.empty()) return;
    const godot::Dictionary before = get_snapshot_state();
    clear();
    commit_snapshot_action("Clear Voronoi Sites", before, get_snapshot_state());
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

void VoronoiDiagram2DResource::commit_generate_grid_sites(
    int p_columns,
    int p_rows,
    real_t p_jitter,
    int64_t p_seed
) {
    const godot::Dictionary before = get_snapshot_state();
    generate_grid_sites(p_columns, p_rows, p_jitter, p_seed);
    commit_snapshot_action("Generate Grid Voronoi Sites", before, get_snapshot_state());
}

void VoronoiDiagram2DResource::commit_generate_poisson_sites(
    int p_target_count,
    real_t p_min_distance,
    int64_t p_seed
) {
    const godot::Dictionary before = get_snapshot_state();
    generate_poisson_sites(p_target_count, p_min_distance, p_seed);
    commit_snapshot_action("Generate Blue-Noise Voronoi Sites", before, get_snapshot_state());
}

} // namespace ideam::godot_ext
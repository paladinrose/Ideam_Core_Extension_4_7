#include "polygon_2d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void Polygon2DResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Polygon Data", "");

    ClassDB::bind_method(D_METHOD("set_closed", "closed"), &Polygon2DResource::set_closed);
    ClassDB::bind_method(D_METHOD("is_closed"), &Polygon2DResource::is_closed);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "closed"), "set_closed", "is_closed");

    ClassDB::bind_method(D_METHOD("set_vertices", "vertices"), &Polygon2DResource::set_vertices);
    ClassDB::bind_method(D_METHOD("get_vertices"), &Polygon2DResource::get_vertices);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "vertices"), "set_vertices", "get_vertices");

    // --- Vertex Mutators ---
    ClassDB::bind_method(D_METHOD("get_vertex_count"), &Polygon2DResource::get_vertex_count);
    ClassDB::bind_method(D_METHOD("add_vertex", "vertex"), &Polygon2DResource::add_vertex);
    ClassDB::bind_method(D_METHOD("insert_vertex", "index", "vertex"), &Polygon2DResource::insert_vertex);
    ClassDB::bind_method(D_METHOD("set_vertex", "index", "vertex"), &Polygon2DResource::set_vertex);
    ClassDB::bind_method(D_METHOD("get_vertex", "index"), &Polygon2DResource::get_vertex);
    ClassDB::bind_method(D_METHOD("remove_vertex", "index"), &Polygon2DResource::remove_vertex);
    ClassDB::bind_method(D_METHOD("clear"), &Polygon2DResource::clear);

    // --- Geometric Queries ---
    ClassDB::bind_method(D_METHOD("contains_point", "point"), &Polygon2DResource::contains_point);
    ClassDB::bind_method(D_METHOD("get_signed_area"), &Polygon2DResource::get_signed_area);
    ClassDB::bind_method(D_METHOD("get_abs_area"), &Polygon2DResource::get_abs_area);
    ClassDB::bind_method(D_METHOD("get_perimeter"), &Polygon2DResource::get_perimeter);
    ClassDB::bind_method(D_METHOD("get_centroid"), &Polygon2DResource::get_centroid);
    ClassDB::bind_method(D_METHOD("get_winding_order"), &Polygon2DResource::get_winding_order);
    ClassDB::bind_method(
        D_METHOD("ensure_winding", "desired"),
        &Polygon2DResource::ensure_winding,
        DEFVAL(WINDING_COUNTER_CLOCKWISE)
    );

    ClassDB::bind_method(D_METHOD("distance_to_boundary", "point"), &Polygon2DResource::distance_to_boundary);
    ClassDB::bind_method(D_METHOD("closest_point_on_boundary", "point"), &Polygon2DResource::closest_point_on_boundary);
    ClassDB::bind_method(D_METHOD("get_triangles"), &Polygon2DResource::get_triangles);
    ClassDB::bind_method(D_METHOD("compute_convex_hull"), &Polygon2DResource::compute_convex_hull);
    ClassDB::bind_method(
        D_METHOD("compute_offset_polygon", "delta", "join_style", "miter_limit"),
        &Polygon2DResource::compute_offset_polygon,
        DEFVAL(JOIN_STYLE_MITER),
        DEFVAL(2.0)
    );
    ClassDB::bind_method(
        D_METHOD("apply_offset", "delta", "join_style", "miter_limit"),
        &Polygon2DResource::apply_offset,
        DEFVAL(JOIN_STYLE_MITER),
        DEFVAL(2.0)
    );

    // --- CSG & Clipping ---
    ClassDB::bind_method(
        D_METHOD("classify_against_half_plane", "plane_origin", "inside_normal"),
        &Polygon2DResource::classify_against_half_plane
    );
    ClassDB::bind_method(
        D_METHOD("classify_against_boundary", "boundary_polygon"),
        &Polygon2DResource::classify_against_boundary
    );
    ClassDB::bind_method(
        D_METHOD("clip_half_plane", "plane_origin", "inside_normal"),
        &Polygon2DResource::clip_half_plane
    );
    ClassDB::bind_method(D_METHOD("clip_convex", "convex_clip"), &Polygon2DResource::clip_convex);

    // --- Shape Generators ---
    ClassDB::bind_method(D_METHOD("create_rectangle", "center", "size"), &Polygon2DResource::create_rectangle);
    ClassDB::bind_method(
        D_METHOD("create_regular_polygon", "sides", "radius", "angle_offset", "reverse_winding"),
        &Polygon2DResource::create_regular_polygon,
        DEFVAL(6),
        DEFVAL(0.5),
        DEFVAL(0.0),
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("create_star", "sides", "outer_radius", "inner_radius", "angle_offset"),
        &Polygon2DResource::create_star,
        DEFVAL(5),
        DEFVAL(0.5),
        DEFVAL(0.25),
        DEFVAL(0.0)
    );

    // --- Undo/Redo Commit Wrappers ---
    ClassDB::bind_method(D_METHOD("commit_add_vertex", "vertex"), &Polygon2DResource::commit_add_vertex);
    ClassDB::bind_method(D_METHOD("commit_insert_vertex", "index", "vertex"), &Polygon2DResource::commit_insert_vertex);
    ClassDB::bind_method(D_METHOD("commit_set_vertex", "index", "vertex"), &Polygon2DResource::commit_set_vertex);
    ClassDB::bind_method(D_METHOD("commit_remove_vertex", "index"), &Polygon2DResource::commit_remove_vertex);
    ClassDB::bind_method(
        D_METHOD("commit_apply_offset", "delta", "join_style", "miter_limit"),
        &Polygon2DResource::commit_apply_offset,
        DEFVAL(JOIN_STYLE_MITER),
        DEFVAL(2.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_ensure_winding", "desired"),
        &Polygon2DResource::commit_ensure_winding,
        DEFVAL(WINDING_COUNTER_CLOCKWISE)
    );
}

std::vector<Polygon2DResource::Vec2> Polygon2DResource::_to_glm_vector(const godot::PackedVector2Array &p_arr) {
    std::vector<Vec2> out;
    out.reserve(static_cast<size_t>(p_arr.size()));
    for (int64_t i = 0; i < p_arr.size(); ++i) {
        out.push_back(_to_glm(p_arr[i]));
    }
    return out;
}

godot::PackedVector2Array Polygon2DResource::_to_godot_array(std::span<const Vec2> p_span) {
    godot::PackedVector2Array out;
    out.resize(static_cast<int64_t>(p_span.size()));
    for (size_t i = 0; i < p_span.size(); ++i) {
        out.set(static_cast<int64_t>(i), _to_godot(p_span[i]));
    }
    return out;
}

void Polygon2DResource::_mark_dirty() {
    triangles_dirty_ = true;
    emit_signal("geometry_changed");
    emit_changed();
}

void Polygon2DResource::_ensure_triangulated() const {
    if (!triangles_dirty_) return;
    cached_triangles_.clear();
    if (closed_ && vertices_.size() >= 3) {
        PolyMath::triangulate_into(vertices_, cached_triangles_);
    }
    triangles_dirty_ = false;
}

godot::Rect2 Polygon2DResource::get_bounds_2d() const {
    if (vertices_.empty()) return godot::Rect2();
    const core::AABB2D<real_t> aabb = PolyMath::compute_aabb(vertices_);
    return godot::Rect2(_to_godot(aabb.min_bounds), _to_godot(aabb.get_size()));
}

godot::Dictionary Polygon2DResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["closed"]   = closed_;
    d["vertices"] = get_vertices();
    return d;
}

void Polygon2DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("closed")) {
        closed_ = static_cast<bool>(p_state["closed"]);
    }
    if (p_state.has("vertices")) {
        vertices_ = _to_glm_vector(p_state["vertices"]);
    }
    _mark_dirty();
}

void Polygon2DResource::set_vertices(const godot::PackedVector2Array &p_vertices) {
    vertices_ = _to_glm_vector(p_vertices);
    _mark_dirty();
}

godot::PackedVector2Array Polygon2DResource::get_vertices() const {
    return _to_godot_array(vertices_);
}

void Polygon2DResource::set_closed(bool p_closed) {
    if (closed_ == p_closed) return;
    closed_ = p_closed;
    _mark_dirty();
}

void Polygon2DResource::add_vertex(const godot::Vector2 &p_vertex) {
    vertices_.push_back(_to_glm(p_vertex));
    _mark_dirty();
}

void Polygon2DResource::insert_vertex(int p_index, const godot::Vector2 &p_vertex) {
    const int clamped = std::clamp(p_index, 0, static_cast<int>(vertices_.size()));
    vertices_.insert(vertices_.begin() + clamped, _to_glm(p_vertex));
    _mark_dirty();
}

void Polygon2DResource::set_vertex(int p_index, const godot::Vector2 &p_vertex) {
    if (p_index < 0 || p_index >= static_cast<int>(vertices_.size())) return;
    vertices_[static_cast<size_t>(p_index)] = _to_glm(p_vertex);
    _mark_dirty();
}

godot::Vector2 Polygon2DResource::get_vertex(int p_index) const {
    if (p_index < 0 || p_index >= static_cast<int>(vertices_.size())) return godot::Vector2();
    return _to_godot(vertices_[static_cast<size_t>(p_index)]);
}

void Polygon2DResource::remove_vertex(int p_index) {
    if (p_index < 0 || p_index >= static_cast<int>(vertices_.size())) return;
    vertices_.erase(vertices_.begin() + p_index);
    _mark_dirty();
}

void Polygon2DResource::clear() {
    if (vertices_.empty()) return;
    vertices_.clear();
    _mark_dirty();
}

bool Polygon2DResource::contains_point(const godot::Vector2 &p_point) const {
    return closed_ && PolyMath::contains_point(vertices_, _to_glm(p_point));
}

real_t Polygon2DResource::get_signed_area() const {
    return closed_ ? PolyMath::signed_area(vertices_) : static_cast<real_t>(0.0);
}

real_t Polygon2DResource::get_abs_area() const {
    return closed_ ? PolyMath::abs_area(vertices_) : static_cast<real_t>(0.0);
}

real_t Polygon2DResource::get_perimeter() const {
    return PolyMath::perimeter(vertices_, closed_);
}

godot::Vector2 Polygon2DResource::get_centroid() const {
    return _to_godot(PolyMath::centroid(vertices_));
}

GeometryWindingOrder Polygon2DResource::get_winding_order() const {
    return static_cast<GeometryWindingOrder>(PolyMath::get_winding_order(vertices_));
}

void Polygon2DResource::ensure_winding(GeometryWindingOrder p_desired) {
    PolyMath::ensure_winding(vertices_, static_cast<core::WindingOrder>(p_desired));
    _mark_dirty();
}

real_t Polygon2DResource::distance_to_boundary(const godot::Vector2 &p_point) const {
    return PolyMath::distance_to_boundary(vertices_, _to_glm(p_point), nullptr, nullptr, closed_);
}

godot::Vector2 Polygon2DResource::closest_point_on_boundary(const godot::Vector2 &p_point) const {
    Vec2 closest(static_cast<real_t>(0.0));
    (void)PolyMath::distance_to_boundary(vertices_, _to_glm(p_point), &closest, nullptr, closed_);
    return _to_godot(closest);
}

godot::PackedInt32Array Polygon2DResource::get_triangles() const {
    _ensure_triangulated();
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(cached_triangles_.size()));
    for (size_t i = 0; i < cached_triangles_.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(cached_triangles_[i]));
    }
    return out;
}

godot::PackedVector2Array Polygon2DResource::compute_convex_hull() const {
    const std::vector<Vec2> hull = PolyMath::convex_hull(vertices_);
    return _to_godot_array(hull);
}

godot::PackedVector2Array Polygon2DResource::compute_offset_polygon(
    real_t p_delta,
    GeometryJoinStyle2D p_join,
    real_t p_miter_limit
) const {
    const std::vector<Vec2> offset = PolyMath::offset_polygon(
        vertices_,
        p_delta,
        static_cast<core::JoinStyle2D>(p_join),
        p_miter_limit
    );
    return _to_godot_array(offset);
}

void Polygon2DResource::apply_offset(
    real_t p_delta,
    GeometryJoinStyle2D p_join,
    real_t p_miter_limit
) {
    vertices_ = PolyMath::offset_polygon(
        vertices_,
        p_delta,
        static_cast<core::JoinStyle2D>(p_join),
        p_miter_limit
    );
    _mark_dirty();
}

GeometryClipClassification2D Polygon2DResource::classify_against_half_plane(
    const godot::Vector2 &p_plane_origin,
    const godot::Vector2 &p_inside_normal
) const {
    return static_cast<GeometryClipClassification2D>(
        CSGMath::classify_polygon_half_plane(vertices_, _to_glm(p_plane_origin), _to_glm(p_inside_normal))
    );
}

GeometryClipClassification2D Polygon2DResource::classify_against_boundary(
    const godot::PackedVector2Array &p_boundary_polygon
) const {
    const std::vector<Vec2> boundary = _to_glm_vector(p_boundary_polygon);
    const core::AABB2D<real_t> b_aabb = PolyMath::compute_aabb(boundary);
    return static_cast<GeometryClipClassification2D>(
        CSGMath::classify_polygon_against_boundary(vertices_, boundary, b_aabb)
    );
}

godot::PackedVector2Array Polygon2DResource::clip_half_plane(
    const godot::Vector2 &p_plane_origin,
    const godot::Vector2 &p_inside_normal
) const {
    const std::vector<Vec2> clipped = CSGMath::clip_polygon_half_plane(
        vertices_,
        _to_glm(p_plane_origin),
        _to_glm(p_inside_normal)
    );
    return _to_godot_array(clipped);
}

godot::PackedVector2Array Polygon2DResource::clip_convex(
    const godot::PackedVector2Array &p_convex_clip
) const {
    const std::vector<Vec2> clip_poly = _to_glm_vector(p_convex_clip);
    const std::vector<Vec2> clipped   = CSGMath::clip_polygon_convex(vertices_, clip_poly);
    return _to_godot_array(clipped);
}

void Polygon2DResource::create_rectangle(const godot::Vector2 &p_center, const godot::Vector2 &p_size) {
    vertices_ = PolyMath::generate_rectangle_vertices(_to_glm(p_center), _to_glm(p_size));
    closed_   = true;
    _mark_dirty();
}

void Polygon2DResource::create_regular_polygon(
    int p_sides,
    real_t p_radius,
    real_t p_angle_offset,
    bool p_reverse_winding
) {
    vertices_ = PolyMath::generate_regular_polygon_vertices(p_sides, p_radius, p_angle_offset, p_reverse_winding);
    closed_   = true;
    _mark_dirty();
}

void Polygon2DResource::create_star(
    int p_sides,
    real_t p_outer_radius,
    real_t p_inner_radius,
    real_t p_angle_offset
) {
    vertices_ = PolyMath::generate_star_vertices(p_sides, p_outer_radius, p_inner_radius, p_angle_offset);
    closed_   = true;
    _mark_dirty();
}

void Polygon2DResource::commit_add_vertex(const godot::Vector2 &p_vertex) {
    const godot::Dictionary before = get_snapshot_state();
    add_vertex(p_vertex);
    commit_snapshot_action("Add Polygon Vertex", before, get_snapshot_state());
}

void Polygon2DResource::commit_insert_vertex(int p_index, const godot::Vector2 &p_vertex) {
    const godot::Dictionary before = get_snapshot_state();
    insert_vertex(p_index, p_vertex);
    commit_snapshot_action("Insert Polygon Vertex", before, get_snapshot_state());
}

void Polygon2DResource::commit_set_vertex(int p_index, const godot::Vector2 &p_vertex) {
    if (p_index < 0 || p_index >= get_vertex_count()) return;
    const godot::Vector2 prev = get_vertex(p_index);
    if (prev == p_vertex) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Move Polygon Vertex", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_vertex").bind(p_index, p_vertex));
    ur->add_undo_method(godot::Callable(this, "set_vertex").bind(p_index, prev));
    ur->commit_action(true);
}

void Polygon2DResource::commit_remove_vertex(int p_index) {
    if (p_index < 0 || p_index >= get_vertex_count()) return;
    const godot::Dictionary before = get_snapshot_state();
    remove_vertex(p_index);
    commit_snapshot_action("Remove Polygon Vertex", before, get_snapshot_state());
}

void Polygon2DResource::commit_apply_offset(
    real_t p_delta,
    GeometryJoinStyle2D p_join,
    real_t p_miter_limit
) {
    const godot::Dictionary before = get_snapshot_state();
    apply_offset(p_delta, p_join, p_miter_limit);
    commit_snapshot_action("Offset Polygon 2D", before, get_snapshot_state());
}

void Polygon2DResource::commit_ensure_winding(GeometryWindingOrder p_desired) {
    const godot::Dictionary before = get_snapshot_state();
    ensure_winding(p_desired);
    commit_snapshot_action("Ensure Polygon Winding", before, get_snapshot_state());
}

} // namespace ideam::godot_ext

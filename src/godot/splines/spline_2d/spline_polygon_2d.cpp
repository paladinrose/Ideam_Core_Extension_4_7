#include "spline_polygon_2d.h"

#include "../../../core/geometry/csg_2d.h"
#include "../../../core/geometry/polygon_math_2d.h"

#include <godot_cpp/classes/collision_polygon2d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ideam::godot_ext {

void SplinePolygon2D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(POLYGON_JOIN_MITER);
    BIND_ENUM_CONSTANT(POLYGON_JOIN_BEVEL);
    BIND_ENUM_CONSTANT(POLYGON_JOIN_ROUND);

    ADD_SIGNAL(MethodInfo("polygon_rebuilt", PropertyInfo(Variant::INT, "vertex_count"), PropertyInfo(Variant::FLOAT, "area")));

    // --- Resource & Lateral Handshake Binding ---
    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &SplinePolygon2D::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &SplinePolygon2D::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_spline",
        "get_spline"
    );

    ClassDB::bind_method(D_METHOD("set_tiling_resource", "tiling"), &SplinePolygon2D::set_tiling_resource);
    ClassDB::bind_method(D_METHOD("get_tiling_resource"), &SplinePolygon2D::get_tiling_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tiling_resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource"),
        "set_tiling_resource",
        "get_tiling_resource"
    );

    // --- Tesselation & Offset Group ---
    ADD_GROUP("Tesselation & Offset", "");

    ClassDB::bind_method(D_METHOD("set_subdivisions", "subdivisions"), &SplinePolygon2D::set_subdivisions);
    ClassDB::bind_method(D_METHOD("get_subdivisions"), &SplinePolygon2D::get_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "subdivisions", PROPERTY_HINT_RANGE, "1,64,1"),
        "set_subdivisions",
        "get_subdivisions"
    );

    ClassDB::bind_method(D_METHOD("set_polygon_offset", "offset"), &SplinePolygon2D::set_polygon_offset);
    ClassDB::bind_method(D_METHOD("get_polygon_offset"), &SplinePolygon2D::get_polygon_offset);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "polygon_offset", PROPERTY_HINT_RANGE, "-200.0,200.0,0.5,or_less,or_greater"),
        "set_polygon_offset",
        "get_polygon_offset"
    );

    ClassDB::bind_method(D_METHOD("set_join_style", "style"), &SplinePolygon2D::set_join_style);
    ClassDB::bind_method(D_METHOD("get_join_style"), &SplinePolygon2D::get_join_style);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "join_style", PROPERTY_HINT_ENUM, "Miter:0,Bevel:1,Round:2"),
        "set_join_style",
        "get_join_style"
    );

    ClassDB::bind_method(D_METHOD("set_miter_limit", "limit"), &SplinePolygon2D::set_miter_limit);
    ClassDB::bind_method(D_METHOD("get_miter_limit"), &SplinePolygon2D::get_miter_limit);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "miter_limit", PROPERTY_HINT_RANGE, "1.0,10.0,0.1"),
        "set_miter_limit",
        "get_miter_limit"
    );

    // --- Appearance Group ---
    ADD_GROUP("Appearance", "");

    ClassDB::bind_method(D_METHOD("set_draw_fill", "draw"), &SplinePolygon2D::set_draw_fill);
    ClassDB::bind_method(D_METHOD("get_draw_fill"), &SplinePolygon2D::get_draw_fill);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_fill"), "set_draw_fill", "get_draw_fill");

    ClassDB::bind_method(D_METHOD("set_fill_color", "color"), &SplinePolygon2D::set_fill_color);
    ClassDB::bind_method(D_METHOD("get_fill_color"), &SplinePolygon2D::get_fill_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "fill_color"), "set_fill_color", "get_fill_color");

    ClassDB::bind_method(D_METHOD("set_texture", "texture"), &SplinePolygon2D::set_texture);
    ClassDB::bind_method(D_METHOD("get_texture"), &SplinePolygon2D::get_texture);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"),
        "set_texture",
        "get_texture"
    );

    ClassDB::bind_method(D_METHOD("set_uv_scale", "scale"), &SplinePolygon2D::set_uv_scale);
    ClassDB::bind_method(D_METHOD("get_uv_scale"), &SplinePolygon2D::get_uv_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "uv_scale"), "set_uv_scale", "get_uv_scale");

    ClassDB::bind_method(D_METHOD("set_uv_offset", "offset"), &SplinePolygon2D::set_uv_offset);
    ClassDB::bind_method(D_METHOD("get_uv_offset"), &SplinePolygon2D::get_uv_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "uv_offset"), "set_uv_offset", "get_uv_offset");

    ClassDB::bind_method(D_METHOD("set_uv_rotation_radians", "radians"), &SplinePolygon2D::set_uv_rotation_radians);
    ClassDB::bind_method(D_METHOD("get_uv_rotation_radians"), &SplinePolygon2D::get_uv_rotation_radians);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "uv_rotation_radians", PROPERTY_HINT_RANGE, "-6.28319,6.28319,0.01,radians_as_degrees"),
        "set_uv_rotation_radians",
        "get_uv_rotation_radians"
    );

    ClassDB::bind_method(D_METHOD("set_draw_border", "draw"), &SplinePolygon2D::set_draw_border);
    ClassDB::bind_method(D_METHOD("get_draw_border"), &SplinePolygon2D::get_draw_border);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_border"), "set_draw_border", "get_draw_border");

    ClassDB::bind_method(D_METHOD("set_border_width", "width"), &SplinePolygon2D::set_border_width);
    ClassDB::bind_method(D_METHOD("get_border_width"), &SplinePolygon2D::get_border_width);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "border_width", PROPERTY_HINT_RANGE, "0.5,32.0,0.5"),
        "set_border_width",
        "get_border_width"
    );

    ClassDB::bind_method(D_METHOD("set_border_color", "color"), &SplinePolygon2D::set_border_color);
    ClassDB::bind_method(D_METHOD("get_border_color"), &SplinePolygon2D::get_border_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "border_color"), "set_border_color", "get_border_color");

    // --- Collision Sync Group ---
    ADD_GROUP("Collision Sync", "");

    ClassDB::bind_method(D_METHOD("set_auto_sync_collision", "sync"), &SplinePolygon2D::set_auto_sync_collision);
    ClassDB::bind_method(D_METHOD("get_auto_sync_collision"), &SplinePolygon2D::get_auto_sync_collision);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_sync_collision"), "set_auto_sync_collision", "get_auto_sync_collision");

    ClassDB::bind_method(D_METHOD("set_collision_polygon_path", "path"), &SplinePolygon2D::set_collision_polygon_path);
    ClassDB::bind_method(D_METHOD("get_collision_polygon_path"), &SplinePolygon2D::get_collision_polygon_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "collision_polygon_path"),
        "set_collision_polygon_path",
        "get_collision_polygon_path"
    );

    // --- Spatial Queries & Cache Accessors ---
    ClassDB::bind_method(D_METHOD("get_cached_polygon"), &SplinePolygon2D::get_cached_polygon);
    ClassDB::bind_method(D_METHOD("get_cached_triangles"), &SplinePolygon2D::get_cached_triangles);
    ClassDB::bind_method(D_METHOD("get_bounding_rect"), &SplinePolygon2D::get_bounding_rect);
    ClassDB::bind_method(D_METHOD("get_signed_area"), &SplinePolygon2D::get_signed_area);
    ClassDB::bind_method(D_METHOD("get_abs_area"), &SplinePolygon2D::get_abs_area);
    ClassDB::bind_method(D_METHOD("get_perimeter"), &SplinePolygon2D::get_perimeter);
    ClassDB::bind_method(D_METHOD("get_centroid"), &SplinePolygon2D::get_centroid);

    ClassDB::bind_method(D_METHOD("contains_local_point", "local_pt"), &SplinePolygon2D::contains_local_point);
    ClassDB::bind_method(D_METHOD("contains_global_point", "global_pt"), &SplinePolygon2D::contains_global_point);
    ClassDB::bind_method(D_METHOD("clip_convex_polygon", "convex_clip"), &SplinePolygon2D::clip_convex_polygon);

    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &SplinePolygon2D::_on_spline_changed);
    ClassDB::bind_method(D_METHOD("_on_tiling_changed"), &SplinePolygon2D::_on_tiling_changed);
}

SplinePolygon2D::SplinePolygon2D() = default;

SplinePolygon2D::~SplinePolygon2D() {
    if (spline.is_valid()) {
        const godot::Callable cb(this, "_on_spline_changed");
        if (spline->is_connected("changed", cb)) {
            spline->disconnect("changed", cb);
        }
    }
    if (tiling_resource.is_valid()) {
        const godot::Callable cb(this, "_on_tiling_changed");
        if (tiling_resource->is_connected("changed", cb)) {
            tiling_resource->disconnect("changed", cb);
        }
    }
}

void SplinePolygon2D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _ready();
    } else if (p_what == NOTIFICATION_DRAW) {
        _draw();
    }
}

void SplinePolygon2D::_ready() {
    _rebuild_geometry_cache();
    _sync_collision_polygon();
    queue_redraw();
}

void SplinePolygon2D::_on_spline_changed() {
    cache_dirty = true;
    _rebuild_geometry_cache();
    _sync_collision_polygon();
    emit_signal("polygon_rebuilt", cached_polygon.size(), cached_signed_area);
    queue_redraw();
}

void SplinePolygon2D::_on_tiling_changed() {
    queue_redraw();
}

void SplinePolygon2D::_rebuild_geometry_cache() const {
    if (!cache_dirty) return;

    cached_polygon.clear();
    cached_uvs.clear();
    cached_triangles.clear();
    cached_bounds      = godot::Rect2();
    cached_signed_area = static_cast<real_t>(0.0);
    cached_perimeter   = static_cast<real_t>(0.0);
    cached_centroid    = godot::Vector2();
    cache_dirty        = false;

    if (spline.is_null() || spline->get_point_count() < 3) {
        return;
    }

    using Vec2 = glm::vec<2, real_t>;
    std::vector<Vec2> raw_pts = spline->get_core_spline().get_smoothed_points(subdivisions);
    if (raw_pts.size() > 3 && glm::distance(raw_pts.front(), raw_pts.back()) < core::GeometryConstants<real_t>::WELD_EPSILON) {
        raw_pts.pop_back();
    }
    if (raw_pts.size() < 3) return;

    if (std::abs(polygon_offset) > core::GeometryConstants<real_t>::EPSILON) {
        raw_pts = core::PolygonMath2D<real_t>::offset_polygon(
            raw_pts,
            polygon_offset,
            static_cast<core::JoinStyle2D>(join_style),
            miter_limit
        );
    }
    if (raw_pts.size() < 3) return;

    std::vector<uint32_t> tri_indices;
    core::PolygonMath2D<real_t>::triangulate_into(raw_pts, tri_indices);

    const core::AABB2D<real_t> aabb = core::PolygonMath2D<real_t>::compute_aabb(raw_pts);
    cached_signed_area              = core::PolygonMath2D<real_t>::area(raw_pts);
    cached_perimeter                = core::PolygonMath2D<real_t>::perimeter(raw_pts, true);
    const Vec2 cent                 = core::PolygonMath2D<real_t>::centroid(raw_pts);
    cached_centroid                 = godot::Vector2(static_cast<float>(cent.x), static_cast<float>(cent.y));

    if (aabb.is_valid()) {
        cached_bounds = godot::Rect2(
            static_cast<float>(aabb.min_bounds.x),
            static_cast<float>(aabb.min_bounds.y),
            static_cast<float>(aabb.get_size().x),
            static_cast<float>(aabb.get_size().y)
        );
    }

    const Vec2 box_min  = aabb.is_valid() ? aabb.min_bounds : Vec2(static_cast<real_t>(0.0));
    const Vec2 box_size = aabb.is_valid() ? glm::max(aabb.get_size(), Vec2(static_cast<real_t>(1e-5))) : Vec2(static_cast<real_t>(1.0));
    const real_t cos_uv = std::cos(uv_rotation_radians);
    const real_t sin_uv = std::sin(uv_rotation_radians);

    cached_polygon.resize(static_cast<int>(raw_pts.size()));
    cached_uvs.resize(static_cast<int>(raw_pts.size()));
    for (size_t i = 0; i < raw_pts.size(); ++i) {
        const Vec2& p = raw_pts[i];
        cached_polygon.set(static_cast<int>(i), godot::Vector2(static_cast<float>(p.x), static_cast<float>(p.y)));

        const Vec2 norm_uv = (p - box_min) / box_size;
        const Vec2 rot_uv(
            norm_uv.x * cos_uv - norm_uv.y * sin_uv,
            norm_uv.x * sin_uv + norm_uv.y * cos_uv
        );
        cached_uvs.set(
            static_cast<int>(i),
            godot::Vector2(
                static_cast<float>(rot_uv.x) * uv_scale.x + uv_offset.x,
                static_cast<float>(rot_uv.y) * uv_scale.y + uv_offset.y
            )
        );
    }

    cached_triangles.resize(static_cast<int>(tri_indices.size()));
    for (size_t i = 0; i < tri_indices.size(); ++i) {
        cached_triangles.set(static_cast<int>(i), static_cast<int32_t>(tri_indices[i]));
    }
}

void SplinePolygon2D::_sync_collision_polygon() {
    if (!auto_sync_collision || collision_polygon_path.is_empty() || !is_inside_tree()) {
        return;
    }
    if (godot::Node *target = get_node_or_null(collision_polygon_path)) {
        if (auto *col_poly = godot::Object::cast_to<godot::CollisionPolygon2D>(target)) {
            _rebuild_geometry_cache();
            col_poly->set_polygon(cached_polygon);
        }
    }
}

void SplinePolygon2D::_draw() {
    using namespace godot;
    _rebuild_geometry_cache();
    if (cached_polygon.size() < 3) return;

    if (draw_fill && cached_triangles.size() >= 3) {
        PackedColorArray colors;
        colors.resize(cached_polygon.size());
        colors.fill(fill_color);

        RenderingServer::get_singleton()->canvas_item_add_triangle_array(
            get_canvas_item(),
            cached_triangles,
            cached_polygon,
            colors,
            cached_uvs,
            PackedInt32Array(),
            PackedFloat32Array(),
            texture.is_valid() ? texture->get_rid() : RID()
        );
    }

    if (draw_border && border_width > 0.0f) {
        PackedVector2Array loop = cached_polygon;
        loop.push_back(cached_polygon[0]);
        draw_polyline(loop, border_color, border_width, true);
    }
}

// ============================================================================
// Setters & Getters
// ============================================================================

void SplinePolygon2D::set_spline(const godot::Ref<Spline2DResource> &p_spline) {
    if (spline == p_spline) return;

    const godot::Callable cb(this, "_on_spline_changed");
    if (spline.is_valid() && spline->is_connected("changed", cb)) {
        spline->disconnect("changed", cb);
    }

    spline = p_spline;
    if (spline.is_valid() && !spline->is_connected("changed", cb)) {
        spline->connect("changed", cb);
    }

    cache_dirty = true;
    _rebuild_geometry_cache();
    _sync_collision_polygon();
    queue_redraw();
}

void SplinePolygon2D::set_tiling_resource(const godot::Ref<godot::Resource> &p_tiling) {
    if (tiling_resource == p_tiling) return;

    const godot::Callable cb(this, "_on_tiling_changed");
    if (tiling_resource.is_valid() && tiling_resource->is_connected("changed", cb)) {
        tiling_resource->disconnect("changed", cb);
    }

    tiling_resource = p_tiling;
    if (tiling_resource.is_valid() && !tiling_resource->is_connected("changed", cb)) {
        tiling_resource->connect("changed", cb);
    }
    queue_redraw();
}

void SplinePolygon2D::set_subdivisions(int p_subdivisions) {
    const int clamped = std::clamp(p_subdivisions, 1, 64);
    if (subdivisions == clamped) return;
    subdivisions = clamped;
    cache_dirty  = true;
    _sync_collision_polygon();
    queue_redraw();
}

void SplinePolygon2D::set_polygon_offset(real_t p_offset) {
    if (std::abs(polygon_offset - p_offset) <= 1e-6f) return;
    polygon_offset = p_offset;
    cache_dirty    = true;
    _sync_collision_polygon();
    queue_redraw();
}

void SplinePolygon2D::set_join_style(SplinePolygonJoinStyle p_style) {
    if (join_style == p_style) return;
    join_style  = p_style;
    cache_dirty = true;
    _sync_collision_polygon();
    queue_redraw();
}

void SplinePolygon2D::set_miter_limit(real_t p_limit) {
    const real_t clamped = std::max(static_cast<real_t>(1.0), p_limit);
    if (std::abs(miter_limit - clamped) <= 1e-6f) return;
    miter_limit = clamped;
    cache_dirty = true;
    _sync_collision_polygon();
    queue_redraw();
}

void SplinePolygon2D::set_draw_fill(bool p_draw) {
    draw_fill = p_draw;
    queue_redraw();
}

void SplinePolygon2D::set_fill_color(const godot::Color &p_color) {
    fill_color = p_color;
    queue_redraw();
}

void SplinePolygon2D::set_texture(const godot::Ref<godot::Texture2D> &p_texture) {
    texture = p_texture;
    queue_redraw();
}

void SplinePolygon2D::set_uv_scale(const godot::Vector2 &p_scale) {
    uv_scale    = p_scale;
    cache_dirty = true;
    queue_redraw();
}

void SplinePolygon2D::set_uv_offset(const godot::Vector2 &p_offset) {
    uv_offset   = p_offset;
    cache_dirty = true;
    queue_redraw();
}

void SplinePolygon2D::set_uv_rotation_radians(real_t p_rad) {
    uv_rotation_radians = p_rad;
    cache_dirty         = true;
    queue_redraw();
}

void SplinePolygon2D::set_draw_border(bool p_draw) {
    draw_border = p_draw;
    queue_redraw();
}

void SplinePolygon2D::set_border_width(float p_width) {
    border_width = std::max(0.5f, p_width);
    queue_redraw();
}

void SplinePolygon2D::set_border_color(const godot::Color &p_color) {
    border_color = p_color;
    queue_redraw();
}

void SplinePolygon2D::set_auto_sync_collision(bool p_sync) {
    auto_sync_collision = p_sync;
    if (auto_sync_collision) {
        _sync_collision_polygon();
    }
}

void SplinePolygon2D::set_collision_polygon_path(const godot::NodePath &p_path) {
    collision_polygon_path = p_path;
    _sync_collision_polygon();
}

// ============================================================================
// Cached Geometry & Spatial Query API
// ============================================================================

godot::PackedVector2Array SplinePolygon2D::get_cached_polygon() const {
    _rebuild_geometry_cache();
    return cached_polygon;
}

godot::PackedInt32Array SplinePolygon2D::get_cached_triangles() const {
    _rebuild_geometry_cache();
    return cached_triangles;
}

godot::Rect2 SplinePolygon2D::get_bounding_rect() const {
    _rebuild_geometry_cache();
    return cached_bounds;
}

real_t SplinePolygon2D::get_signed_area() const {
    _rebuild_geometry_cache();
    return cached_signed_area;
}

real_t SplinePolygon2D::get_abs_area() const {
    _rebuild_geometry_cache();
    return std::abs(cached_signed_area);
}

real_t SplinePolygon2D::get_perimeter() const {
    _rebuild_geometry_cache();
    return cached_perimeter;
}

godot::Vector2 SplinePolygon2D::get_centroid() const {
    _rebuild_geometry_cache();
    return cached_centroid;
}

bool SplinePolygon2D::contains_local_point(const godot::Vector2 &p_local_pt) const {
    _rebuild_geometry_cache();
    if (cached_polygon.size() < 3 || !cached_bounds.has_point(p_local_pt)) {
        return false;
    }

    using Vec2 = glm::vec<2, real_t>;
    std::vector<Vec2> poly(static_cast<size_t>(cached_polygon.size()));
    for (int i = 0; i < cached_polygon.size(); ++i) {
        poly[static_cast<size_t>(i)] = Vec2(cached_polygon[i].x, cached_polygon[i].y);
    }
    return core::PolygonMath2D<real_t>::contains_point(poly, Vec2(p_local_pt.x, p_local_pt.y));
}

bool SplinePolygon2D::contains_global_point(const godot::Vector2 &p_global_pt) const {
    return contains_local_point(to_local(p_global_pt));
}

godot::PackedVector2Array SplinePolygon2D::clip_convex_polygon(const godot::PackedVector2Array &p_convex_clip) const {
    _rebuild_geometry_cache();
    if (cached_polygon.size() < 3 || p_convex_clip.size() < 3) {
        return {};
    }

    using Vec2 = glm::vec<2, real_t>;
    std::vector<Vec2> subject(static_cast<size_t>(cached_polygon.size()));
    for (int i = 0; i < cached_polygon.size(); ++i) {
        subject[static_cast<size_t>(i)] = Vec2(cached_polygon[i].x, cached_polygon[i].y);
    }

    std::vector<Vec2> clip(static_cast<size_t>(p_convex_clip.size()));
    for (int i = 0; i < p_convex_clip.size(); ++i) {
        clip[static_cast<size_t>(i)] = Vec2(p_convex_clip[i].x, p_convex_clip[i].y);
    }

    const std::vector<Vec2> clipped = core::CSGMath2D<real_t>::clip_polygon_convex(subject, clip);
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(clipped.size()));
    for (size_t i = 0; i < clipped.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector2(static_cast<float>(clipped[i].x), static_cast<float>(clipped[i].y)));
    }
    return out;
}

} // namespace ideam::godot_ext
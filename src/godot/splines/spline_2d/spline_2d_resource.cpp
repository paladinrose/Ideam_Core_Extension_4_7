#include "spline_2d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

void Spline2DResource::_bind_methods() {
    using namespace godot;

    // --- 2D Point & Handle API ---
    ClassDB::bind_method(
        D_METHOD("add_point", "position", "type", "weight"),
        &Spline2DResource::add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("insert_point", "index", "position", "type", "weight"),
        &Spline2DResource::insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );

    ClassDB::bind_method(D_METHOD("set_point_position", "index", "position"), &Spline2DResource::set_point_position);
    ClassDB::bind_method(D_METHOD("get_point_position", "index"), &Spline2DResource::get_point_position);

    ClassDB::bind_method(
        D_METHOD("set_point_back_handle", "index", "handle", "enable"),
        &Spline2DResource::set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_back_handle", "index"), &Spline2DResource::get_point_back_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_fore_handle", "index", "handle", "enable"),
        &Spline2DResource::set_point_fore_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_fore_handle", "index"), &Spline2DResource::get_point_fore_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_handles", "index", "back_handle", "use_back", "fore_handle", "use_fore"),
        &Spline2DResource::set_point_handles
    );

    // --- 2D Undo/Redo Commit Counterparts ---
    ClassDB::bind_method(
        D_METHOD("commit_add_point", "position", "type", "weight"),
        &Spline2DResource::commit_add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_insert_point", "index", "position", "type", "weight"),
        &Spline2DResource::commit_insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_position", "index", "position"),
        &Spline2DResource::commit_set_point_position
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_back_handle", "index", "handle", "enable"),
        &Spline2DResource::commit_set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_fore_handle", "index", "handle", "enable"),
        &Spline2DResource::commit_set_point_fore_handle,
        DEFVAL(true)
    );

    // --- 2D Serialized Packed Arrays ---
    ADD_GROUP("Serialized Arrays", "");

    ClassDB::bind_method(D_METHOD("set_points_array", "points"), &Spline2DResource::set_points_array);
    ClassDB::bind_method(D_METHOD("get_points_array"), &Spline2DResource::get_points_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT),
        "set_points_array",
        "get_points_array"
    );

    ClassDB::bind_method(D_METHOD("set_back_handles_array", "handles"), &Spline2DResource::set_back_handles_array);
    ClassDB::bind_method(D_METHOD("get_back_handles_array"), &Spline2DResource::get_back_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "back_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_back_handles_array",
        "get_back_handles_array"
    );

    ClassDB::bind_method(D_METHOD("set_fore_handles_array", "handles"), &Spline2DResource::set_fore_handles_array);
    ClassDB::bind_method(D_METHOD("get_fore_handles_array"), &Spline2DResource::get_fore_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "fore_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_fore_handles_array",
        "get_fore_handles_array"
    );

    // --- 2D Parametric Evaluation & Frenet Frame ---
    ClassDB::bind_method(D_METHOD("sample", "percent", "weighted"), &Spline2DResource::sample, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_point", "percent", "weighted"), &Spline2DResource::get_point, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section", "section_index", "local_t"), &Spline2DResource::sample_section);
    ClassDB::bind_method(D_METHOD("sample_derivative", "percent", "weighted"), &Spline2DResource::sample_derivative, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section_derivative", "section_index", "local_t"), &Spline2DResource::sample_section_derivative);
    ClassDB::bind_method(D_METHOD("sample_section_second_derivative", "section_index", "local_t"), &Spline2DResource::sample_section_second_derivative);

    ClassDB::bind_method(D_METHOD("get_tangent", "percent", "weighted"), &Spline2DResource::get_tangent, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_normal", "percent", "weighted"), &Spline2DResource::get_normal, DEFVAL(true));
    ClassDB::bind_method(
        D_METHOD("get_transform_at_percent", "percent", "weighted"),
        &Spline2DResource::get_transform_at_percent,
        DEFVAL(true)
    );

    ClassDB::bind_method(
        D_METHOD("get_smoothed_points", "max_subdivisions"),
        &Spline2DResource::get_smoothed_points,
        DEFVAL(10)
    );
    ClassDB::bind_method(
        D_METHOD("get_bounding_rect", "use_smoothed"),
        &Spline2DResource::get_bounding_rect,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("get_closest_point_on_spline", "target", "samples"),
        &Spline2DResource::get_closest_point_on_spline,
        DEFVAL(10)
    );
    ClassDB::bind_method(
        D_METHOD("get_closest_percent_on_spline", "target", "samples", "weighted"),
        &Spline2DResource::get_closest_percent_on_spline,
        DEFVAL(10),
        DEFVAL(false)
    );

    // --- 2D Closed-Polygon & Geometry Queries ---
    ClassDB::bind_method(
        D_METHOD("contains_point", "point", "subdivisions"),
        &Spline2DResource::contains_point,
        DEFVAL(10)
    );
    ClassDB::bind_method(D_METHOD("get_area", "subdivisions"), &Spline2DResource::get_area, DEFVAL(10));
    ClassDB::bind_method(D_METHOD("get_abs_area", "subdivisions"), &Spline2DResource::get_abs_area, DEFVAL(10));
    ClassDB::bind_method(D_METHOD("get_perimeter", "subdivisions"), &Spline2DResource::get_perimeter, DEFVAL(10));
    ClassDB::bind_method(D_METHOD("get_centroid", "subdivisions"), &Spline2DResource::get_centroid, DEFVAL(10));
    ClassDB::bind_method(D_METHOD("triangulate", "subdivisions"), &Spline2DResource::triangulate, DEFVAL(10));
    ClassDB::bind_method(
        D_METHOD("clip_with_convex_polygon", "convex_clip", "subdivisions"),
        &Spline2DResource::clip_with_convex_polygon,
        DEFVAL(10)
    );

    ClassDB::bind_method(D_METHOD("rotate_spline", "angle_rad"), &Spline2DResource::rotate_spline);
    ClassDB::bind_method(D_METHOD("scale_spline", "scale"), &Spline2DResource::scale_spline);
    ClassDB::bind_method(D_METHOD("commit_rotate_spline", "angle_rad"), &Spline2DResource::commit_rotate_spline);
    ClassDB::bind_method(D_METHOD("commit_scale_spline", "scale"), &Spline2DResource::commit_scale_spline);

    // --- 2D Shape Presets & Static Factory Builders ---
    ClassDB::bind_method(
        D_METHOD("setup_from_vertices", "vertices", "wrap", "type"),
        &Spline2DResource::setup_from_vertices,
        DEFVAL(true),
        DEFVAL(CURVE_LINEAR)
    );
    ClassDB::bind_method(
        D_METHOD("setup_rectangle", "center", "size"),
        &Spline2DResource::setup_rectangle,
        DEFVAL(Vector2(0.0f, 0.0f)),
        DEFVAL(Vector2(100.0f, 100.0f))
    );
    ClassDB::bind_method(
        D_METHOD("setup_regular_polygon", "sides", "radius", "angle_offset"),
        &Spline2DResource::setup_regular_polygon,
        DEFVAL(6),
        DEFVAL(50.0),
        DEFVAL(0.0)
    );
    ClassDB::bind_method(
        D_METHOD("setup_star", "sides", "outer_radius", "inner_radius", "angle_offset"),
        &Spline2DResource::setup_star,
        DEFVAL(5),
        DEFVAL(50.0),
        DEFVAL(25.0),
        DEFVAL(0.0)
    );
    ClassDB::bind_method(
        D_METHOD("setup_circle", "radius", "points"),
        &Spline2DResource::setup_circle,
        DEFVAL(50.0),
        DEFVAL(8)
    );

    ClassDB::bind_static_method(
        "Spline2DResource",
        D_METHOD("create_rectangle", "center", "size"),
        &Spline2DResource::create_rectangle,
        DEFVAL(Vector2(0.0f, 0.0f)),
        DEFVAL(Vector2(100.0f, 100.0f))
    );
    ClassDB::bind_static_method(
        "Spline2DResource",
        D_METHOD("create_regular_polygon", "sides", "radius", "angle_offset"),
        &Spline2DResource::create_regular_polygon,
        DEFVAL(6),
        DEFVAL(50.0),
        DEFVAL(0.0)
    );
    ClassDB::bind_static_method(
        "Spline2DResource",
        D_METHOD("create_star", "sides", "outer_radius", "inner_radius", "angle_offset"),
        &Spline2DResource::create_star,
        DEFVAL(5),
        DEFVAL(50.0),
        DEFVAL(25.0),
        DEFVAL(0.0)
    );
    ClassDB::bind_static_method(
        "Spline2DResource",
        D_METHOD("create_circle", "radius", "points"),
        &Spline2DResource::create_circle,
        DEFVAL(50.0),
        DEFVAL(8)
    );
}

Spline2DResource::Spline2DResource() {
    // Initialize with a default 4-point open curve or rectangle so new resources
    // render immediately when inspected.
    core_spline.add_point(Vec2(static_cast<real_t>(-40.0), static_cast<real_t>(-20.0)), core::CurveType::CATMULL_ROM);
    core_spline.add_point(Vec2(static_cast<real_t>(-15.0), static_cast<real_t>(30.0)), core::CurveType::CATMULL_ROM);
    core_spline.add_point(Vec2(static_cast<real_t>(15.0), static_cast<real_t>(-30.0)), core::CurveType::CATMULL_ROM);
    core_spline.add_point(Vec2(static_cast<real_t>(40.0), static_cast<real_t>(20.0)), core::CurveType::CATMULL_ROM);
    core_spline.set_default_curve_type(core::CurveType::CATMULL_ROM);
}

Spline2DResource::~Spline2DResource() = default;

// ============================================================================
// SplineResource Virtual Overrides
// ============================================================================

int Spline2DResource::get_point_count() const {
    return static_cast<int>(core_spline.get_point_count());
}

int Spline2DResource::get_length() const {
    return static_cast<int>(core_spline.get_length());
}

int Spline2DResource::get_section_count() const {
    return static_cast<int>(core_spline.get_section_count());
}

void Spline2DResource::set_wrap(bool p_wrap) {
    if (core_spline.get_wrap() == p_wrap) return;
    core_spline.set_wrap(p_wrap);
    emit_signal("structure_changed");
    emit_changed();
}

bool Spline2DResource::get_wrap() const {
    return core_spline.get_wrap();
}

void Spline2DResource::set_default_curve_type(SplineCurveType p_type) {
    core_spline.set_default_curve_type(static_cast<core::CurveType>(p_type));
}

SplineCurveType Spline2DResource::get_default_curve_type() const {
    return static_cast<SplineCurveType>(core_spline.get_default_curve_type());
}

void Spline2DResource::set_default_weight(real_t p_weight) {
    core_spline.set_default_weight(p_weight);
}

real_t Spline2DResource::get_default_weight() const {
    return core_spline.get_default_weight();
}

real_t Spline2DResource::get_total_weight() const {
    return core_spline.get_total_weight();
}

real_t Spline2DResource::get_total_spline_length(real_t p_sample_percent, bool p_force_recalculate) const {
    return core_spline.get_total_spline_length(p_sample_percent, p_force_recalculate);
}

void Spline2DResource::remove_point(int p_index) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.remove_point(static_cast<size_t>(p_index));
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::clear() {
    if (core_spline.get_point_count() == 0) return;
    core_spline.clear();
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::set_point_use_back_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_back_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline2DResource::get_point_use_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_back_handle;
}

void Spline2DResource::set_point_use_fore_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_fore_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline2DResource::get_point_use_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_fore_handle;
}

void Spline2DResource::set_section_curve_type(int p_section_index, SplineCurveType p_type) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_curve_type(
        static_cast<size_t>(p_section_index),
        static_cast<core::CurveType>(p_type)
    );
    emit_changed();
}

SplineCurveType Spline2DResource::get_section_curve_type(int p_section_index) const {
    return static_cast<SplineCurveType>(
        core_spline.get_section_curve_type(static_cast<size_t>(p_section_index))
    );
}

void Spline2DResource::set_section_weight(int p_section_index, real_t p_weight) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_weight(static_cast<size_t>(p_section_index), p_weight);
    emit_signal("section_weights_changed");
    emit_changed();
}

real_t Spline2DResource::get_section_weight(int p_section_index) const {
    return core_spline.get_section_weight(static_cast<size_t>(p_section_index));
}

real_t Spline2DResource::get_section_length(int p_section_index) const {
    return core_spline.get_section_length(static_cast<size_t>(p_section_index));
}

real_t Spline2DResource::section_local_to_global_percent(
    int p_section_index,
    real_t p_local_t,
    bool p_weighted
) const {
    if (p_section_index < 0) return static_cast<real_t>(0.0);
    return core_spline.section_local_to_global_percent(
        static_cast<size_t>(p_section_index),
        p_local_t,
        p_weighted
    );
}

godot::Vector2 Spline2DResource::get_section_at_percent(real_t p_percent, bool p_weighted) const {
    real_t local_t = static_cast<real_t>(0.0);
    const size_t sec_idx = core_spline.get_section_at_percent(p_percent, p_weighted, local_t);
    return godot::Vector2(static_cast<float>(sec_idx), static_cast<float>(local_t));
}

godot::Variant Spline2DResource::get_point_variant(real_t p_percent, bool p_weighted) const {
    return sample(p_percent, p_weighted);
}

godot::Variant Spline2DResource::get_derivative_variant(real_t p_percent, bool p_weighted) const {
    return sample_derivative(p_percent, p_weighted);
}

godot::Variant Spline2DResource::get_tangent_variant(real_t p_percent, bool p_weighted) const {
    return get_tangent(p_percent, p_weighted);
}

// ============================================================================
// 2D Point & Handle API
// ============================================================================

void Spline2DResource::add_point(
    const godot::Vector2 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    core_spline.add_point(_to_glm(p_position), static_cast<core::CurveType>(p_type), p_weight);
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::insert_point(
    int p_index,
    const godot::Vector2 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    if (p_index < 0) p_index = 0;
    core_spline.insert_point(
        static_cast<size_t>(p_index),
        _to_glm(p_position),
        static_cast<core::CurveType>(p_type),
        p_weight
    );
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::set_point_position(int p_index, const godot::Vector2 &p_position) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_position(static_cast<size_t>(p_index), _to_glm(p_position));
    emit_changed();
}

godot::Vector2 Spline2DResource::get_point_position(int p_index) const {
    if (p_index < 0 || p_index >= get_point_count()) return godot::Vector2();
    return _to_godot(core_spline.get_point_position(static_cast<size_t>(p_index)));
}

void Spline2DResource::set_point_back_handle(
    int p_index,
    const godot::Vector2 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_back_handle(static_cast<size_t>(p_index), _to_glm(p_handle), p_enable);
    emit_changed();
}

godot::Vector2 Spline2DResource::get_point_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return godot::Vector2();
    return _to_godot(pts[static_cast<size_t>(p_index)].back_handle);
}

void Spline2DResource::set_point_fore_handle(
    int p_index,
    const godot::Vector2 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_fore_handle(static_cast<size_t>(p_index), _to_glm(p_handle), p_enable);
    emit_changed();
}

godot::Vector2 Spline2DResource::get_point_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return godot::Vector2();
    return _to_godot(pts[static_cast<size_t>(p_index)].fore_handle);
}

void Spline2DResource::set_point_handles(
    int p_index,
    const godot::Vector2 &p_back_handle,
    bool p_use_back,
    const godot::Vector2 &p_fore_handle,
    bool p_use_fore
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_handles(
        static_cast<size_t>(p_index),
        _to_glm(p_back_handle),
        p_use_back,
        _to_glm(p_fore_handle),
        p_use_fore
    );
    emit_changed();
}

void Spline2DResource::commit_add_point(
    const godot::Vector2 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    const godot::Dictionary before = get_snapshot_state();
    add_point(p_position, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Add Spline2D Point", before, after);
}

void Spline2DResource::commit_insert_point(
    int p_index,
    const godot::Vector2 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    const godot::Dictionary before = get_snapshot_state();
    insert_point(p_index, p_position, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Insert Spline2D Point", before, after);
}

void Spline2DResource::commit_set_point_position(int p_index, const godot::Vector2 &p_position) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector2 prev_pos = get_point_position(p_index);
    if (prev_pos == p_position) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Move Spline2D Point", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_position").bind(p_index, p_position));
    ur->add_undo_method(godot::Callable(this, "set_point_position").bind(p_index, prev_pos));
    ur->commit_action(true);
}

void Spline2DResource::commit_set_point_back_handle(
    int p_index,
    const godot::Vector2 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector2 prev_h = get_point_back_handle(p_index);
    const bool prev_en          = get_point_use_back_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline2D Back Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_back_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_back_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

void Spline2DResource::commit_set_point_fore_handle(
    int p_index,
    const godot::Vector2 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector2 prev_h = get_point_fore_handle(p_index);
    const bool prev_en          = get_point_use_fore_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline2D Fore Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

// ============================================================================
// Order-Independent Packed Array Serialization
// ============================================================================

void Spline2DResource::_apply_pending_arrays() {
    const int pt_count  = get_point_count();
    const int sec_count = get_section_count();

    if (pending_back_handles.size() == pt_count) {
        for (int i = 0; i < pt_count; ++i) {
            const bool en = get_point_use_back_handle(i);
            core_spline.set_point_back_handle(static_cast<size_t>(i), _to_glm(pending_back_handles[i]), en);
        }
    }
    if (pending_fore_handles.size() == pt_count) {
        for (int i = 0; i < pt_count; ++i) {
            const bool en = get_point_use_fore_handle(i);
            core_spline.set_point_fore_handle(static_cast<size_t>(i), _to_glm(pending_fore_handles[i]), en);
        }
    }
    if (pending_handle_flags.size() == pt_count) {
        for (int i = 0; i < pt_count; ++i) {
            const uint8_t fl = pending_handle_flags[i];
            core_spline.set_use_back_handle(static_cast<size_t>(i), (fl & HANDLE_USE_BACK) != 0);
            core_spline.set_use_fore_handle(static_cast<size_t>(i), (fl & HANDLE_USE_FORE) != 0);
        }
    }
    if (pending_section_types.size() == sec_count) {
        for (int i = 0; i < sec_count; ++i) {
            core_spline.set_section_curve_type(
                static_cast<size_t>(i),
                static_cast<core::CurveType>(pending_section_types[i])
            );
        }
    }
    if (pending_section_weights.size() == sec_count) {
        for (int i = 0; i < sec_count; ++i) {
            core_spline.set_section_weight(static_cast<size_t>(i), pending_section_weights[i]);
        }
    }
}

void Spline2DResource::set_points_array(const godot::PackedVector2Array &p_points) {
    if (pending_back_handles.is_empty())    pending_back_handles    = get_back_handles_array();
    if (pending_fore_handles.is_empty())    pending_fore_handles    = get_fore_handles_array();
    if (pending_handle_flags.is_empty())    pending_handle_flags    = get_handle_flags_array();
    if (pending_section_types.is_empty())   pending_section_types   = get_section_types_array();
    if (pending_section_weights.is_empty()) pending_section_weights = get_section_weights_array();

    const bool was_wrap = core_spline.get_wrap();
    core_spline.set_wrap(false);
    core_spline.clear();

    for (int i = 0; i < p_points.size(); ++i) {
        core_spline.add_point(_to_glm(p_points[i]), core_spline.get_default_curve_type());
    }
    if (was_wrap) {
        core_spline.set_wrap(true);
    }

    _apply_pending_arrays();
    emit_signal("structure_changed");
    emit_changed();
}

godot::PackedVector2Array Spline2DResource::get_points_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].point));
    }
    return out;
}

void Spline2DResource::set_back_handles_array(const godot::PackedVector2Array &p_handles) {
    pending_back_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_back_handle(i);
        core_spline.set_point_back_handle(static_cast<size_t>(i), _to_glm(p_handles[i]), en);
    }
    emit_changed();
}

godot::PackedVector2Array Spline2DResource::get_back_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].back_handle));
    }
    return out;
}

void Spline2DResource::set_fore_handles_array(const godot::PackedVector2Array &p_handles) {
    pending_fore_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_fore_handle(i);
        core_spline.set_point_fore_handle(static_cast<size_t>(i), _to_glm(p_handles[i]), en);
    }
    emit_changed();
}

godot::PackedVector2Array Spline2DResource::get_fore_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].fore_handle));
    }
    return out;
}

godot::PackedByteArray Spline2DResource::get_handle_flags_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedByteArray out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        uint8_t fl = HANDLE_NONE;
        if (pts[i].use_back_handle) fl |= HANDLE_USE_BACK;
        if (pts[i].use_fore_handle) fl |= HANDLE_USE_FORE;
        out.set(static_cast<int>(i), fl);
    }
    return out;
}

void Spline2DResource::set_handle_flags_array(const godot::PackedByteArray &p_flags) {
    pending_handle_flags = p_flags;
    const int count = std::min(static_cast<int>(p_flags.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const uint8_t fl = p_flags[i];
        core_spline.set_use_back_handle(static_cast<size_t>(i), (fl & HANDLE_USE_BACK) != 0);
        core_spline.set_use_fore_handle(static_cast<size_t>(i), (fl & HANDLE_USE_FORE) != 0);
    }
    emit_changed();
}

godot::PackedByteArray Spline2DResource::get_section_types_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedByteArray out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<uint8_t>(secs[i].curve_type));
    }
    return out;
}

void Spline2DResource::set_section_types_array(const godot::PackedByteArray &p_types) {
    pending_section_types = p_types;
    const int count = std::min(static_cast<int>(p_types.size()), get_section_count());
    for (int i = 0; i < count; ++i) {
        core_spline.set_section_curve_type(
            static_cast<size_t>(i),
            static_cast<core::CurveType>(p_types[i])
        );
    }
    emit_changed();
}

godot::PackedFloat32Array Spline2DResource::get_section_weights_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(secs[i].weight));
    }
    return out;
}

void Spline2DResource::set_section_weights_array(const godot::PackedFloat32Array &p_weights) {
    pending_section_weights = p_weights;
    const int count = std::min(static_cast<int>(p_weights.size()), get_section_count());
    for (int i = 0; i < count; ++i) {
        core_spline.set_section_weight(static_cast<size_t>(i), static_cast<real_t>(p_weights[i]));
    }
    emit_signal("section_weights_changed");
    emit_changed();
}

godot::Dictionary Spline2DResource::get_snapshot_state() const {
    godot::Dictionary state;
    state["wrap"]               = get_wrap();
    state["default_curve_type"] = static_cast<int>(get_default_curve_type());
    state["default_weight"]     = get_default_weight();
    state["points"]             = get_points_array();
    state["back_handles"]       = get_back_handles_array();
    state["fore_handles"]       = get_fore_handles_array();
    state["handle_flags"]       = get_handle_flags_array();
    state["section_types"]      = get_section_types_array();
    state["section_weights"]    = get_section_weights_array();
    return state;
}

void Spline2DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("default_curve_type")) {
        set_default_curve_type(static_cast<SplineCurveType>(static_cast<int>(p_state["default_curve_type"])));
    }
    if (p_state.has("default_weight")) {
        set_default_weight(static_cast<real_t>(p_state["default_weight"]));
    }

    pending_back_handles    = p_state.get("back_handles", godot::PackedVector2Array());
    pending_fore_handles    = p_state.get("fore_handles", godot::PackedVector2Array());
    pending_handle_flags    = p_state.get("handle_flags", godot::PackedByteArray());
    pending_section_types   = p_state.get("section_types", godot::PackedByteArray());
    pending_section_weights = p_state.get("section_weights", godot::PackedFloat32Array());

    if (p_state.has("points")) {
        set_points_array(p_state["points"]);
    }
    if (p_state.has("wrap")) {
        set_wrap(static_cast<bool>(p_state["wrap"]));
    }
    _apply_pending_arrays();
    emit_signal("structure_changed");
    emit_changed();
}

// ============================================================================
// 2D Parametric Evaluation, Frenet Frame & Proximity Solvers
// ============================================================================

godot::Vector2 Spline2DResource::sample(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_point(p_percent, p_weighted));
}

godot::Vector2 Spline2DResource::sample_section(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector2();
    return _to_godot(core_spline.evaluate_section(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector2 Spline2DResource::sample_derivative(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_derivative(p_percent, p_weighted));
}

godot::Vector2 Spline2DResource::sample_section_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector2();
    return _to_godot(core_spline.evaluate_section_derivative(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector2 Spline2DResource::sample_section_second_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector2();
    return _to_godot(core_spline.evaluate_section_second_derivative(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector2 Spline2DResource::get_tangent(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_tangent(p_percent, p_weighted));
}

godot::Vector2 Spline2DResource::get_normal(real_t p_percent, bool p_weighted) const {
    const godot::Vector2 t = get_tangent(p_percent, p_weighted);
    return godot::Vector2(-t.y, t.x);
}

godot::Transform2D Spline2DResource::get_transform_at_percent(real_t p_percent, bool p_weighted) const {
    const godot::Vector2 pos = sample(p_percent, p_weighted);
    godot::Vector2 tan       = get_tangent(p_percent, p_weighted);
    if (tan.length_squared() <= 1e-6f) {
        tan = godot::Vector2(1.0f, 0.0f);
    }
    const godot::Vector2 nrm(-tan.y, tan.x);
    return godot::Transform2D(tan, nrm, pos);
}

godot::PackedVector2Array Spline2DResource::get_smoothed_points(int p_max_subdivisions) const {
    const auto pts = core_spline.get_smoothed_points(p_max_subdivisions);
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i]));
    }
    return out;
}

godot::Rect2 Spline2DResource::get_bounding_rect(bool p_use_smoothed) const {
    if (core_spline.get_point_count() == 0) {
        return godot::Rect2();
    }
    Vec2 min_v(static_cast<real_t>(0.0));
    Vec2 max_v(static_cast<real_t>(0.0));
    core_spline.get_min_max(min_v, max_v, p_use_smoothed);
    const godot::Vector2 g_min = _to_godot(min_v);
    const godot::Vector2 g_max = _to_godot(max_v);
    return godot::Rect2(g_min, g_max - g_min);
}

godot::Vector2 Spline2DResource::get_closest_point_on_spline(
    const godot::Vector2 &p_target,
    int p_samples
) const {
    return _to_godot(core_spline.get_closest_point_on_spline(_to_glm(p_target), p_samples));
}

real_t Spline2DResource::get_closest_percent_on_spline(
    const godot::Vector2 &p_target,
    int p_samples,
    bool p_weighted
) const {
    real_t best_pct = static_cast<real_t>(0.0);
    (void)core_spline.get_closest_point_on_spline(
        _to_glm(p_target),
        p_samples,
        nullptr,
        &best_pct,
        p_weighted
    );
    return best_pct;
}

// ============================================================================
// 2D Closed-Polygon & Computational Geometry Operations
// ============================================================================

bool Spline2DResource::contains_point(const godot::Vector2 &p_point, int p_subdivisions) const {
    return core_spline.contains_point(_to_glm(p_point), p_subdivisions);
}

real_t Spline2DResource::get_area(int p_subdivisions) const {
    return core_spline.get_area(p_subdivisions);
}

real_t Spline2DResource::get_abs_area(int p_subdivisions) const {
    return core_spline.get_abs_area(p_subdivisions);
}

real_t Spline2DResource::get_perimeter(int p_subdivisions) const {
    return core_spline.get_perimeter(p_subdivisions);
}

godot::Vector2 Spline2DResource::get_centroid(int p_subdivisions) const {
    return _to_godot(core_spline.get_centroid(p_subdivisions));
}

godot::PackedInt32Array Spline2DResource::triangulate(int p_subdivisions) const {
    const std::vector<uint32_t> indices = core_spline.triangulate(p_subdivisions);
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(indices.size()));
    for (size_t i = 0; i < indices.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(indices[i]));
    }
    return out;
}

godot::PackedVector2Array Spline2DResource::clip_with_convex_polygon(
    const godot::PackedVector2Array &p_convex_clip,
    int p_subdivisions
) const {
    std::vector<Vec2> clip_verts;
    clip_verts.reserve(static_cast<size_t>(p_convex_clip.size()));
    for (int i = 0; i < p_convex_clip.size(); ++i) {
        clip_verts.push_back(_to_glm(p_convex_clip[i]));
    }

    const std::vector<Vec2> clipped = core_spline.clip_with_convex_polygon(clip_verts, p_subdivisions);
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(clipped.size()));
    for (size_t i = 0; i < clipped.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(clipped[i]));
    }
    return out;
}

void Spline2DResource::rotate_spline(real_t p_angle_rad) {
    if (core_spline.get_point_count() == 0) return;
    core_spline.rotate_spline(p_angle_rad);
    emit_changed();
}

void Spline2DResource::scale_spline(const godot::Vector2 &p_scale) {
    if (core_spline.get_point_count() == 0) return;
    core_spline.scale_spline(_to_glm(p_scale));
    emit_changed();
}

void Spline2DResource::commit_rotate_spline(real_t p_angle_rad) {
    if (core_spline.get_point_count() == 0) return;
    const godot::Dictionary before = get_snapshot_state();
    rotate_spline(p_angle_rad);
    commit_snapshot_action("Rotate 2D Spline", before, get_snapshot_state());
}

void Spline2DResource::commit_scale_spline(const godot::Vector2 &p_scale) {
    if (core_spline.get_point_count() == 0) return;
    const godot::Dictionary before = get_snapshot_state();
    scale_spline(p_scale);
    commit_snapshot_action("Scale 2D Spline", before, get_snapshot_state());
}

// ============================================================================
// 2D Shape Presets & Static Factory Builders
// ============================================================================

void Spline2DResource::setup_from_vertices(
    const godot::PackedVector2Array &p_vertices,
    bool p_wrap,
    SplineCurveType p_type
) {
    std::vector<Vec2> verts;
    verts.reserve(static_cast<size_t>(p_vertices.size()));
    for (int i = 0; i < p_vertices.size(); ++i) {
        verts.push_back(_to_glm(p_vertices[i]));
    }
    core_spline = CoreSpline::from_vertices(verts, p_wrap, static_cast<core::CurveType>(p_type));
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::setup_rectangle(const godot::Vector2 &p_center, const godot::Vector2 &p_size) {
    core_spline = CoreSpline::create_rectangle(_to_glm(p_center), _to_glm(p_size));
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::setup_regular_polygon(int p_sides, real_t p_radius, real_t p_angle_offset) {
    core_spline = CoreSpline::create_regular_polygon(std::max(3, p_sides), p_radius, p_angle_offset);
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::setup_star(
    int p_sides,
    real_t p_outer_radius,
    real_t p_inner_radius,
    real_t p_angle_offset
) {
    core_spline = CoreSpline::create_star(
        std::max(3, p_sides),
        p_outer_radius,
        p_inner_radius,
        p_angle_offset
    );
    emit_signal("structure_changed");
    emit_changed();
}

void Spline2DResource::setup_circle(real_t p_radius, int p_points) {
    const int n = std::max(4, p_points);
    core_spline = CoreSpline::create_regular_polygon(n, p_radius, static_cast<real_t>(0.0));
    core_spline.set_default_curve_type(core::CurveType::CATMULL_ROM);
    for (int s = 0; s < static_cast<int>(core_spline.get_section_count()); ++s) {
        core_spline.set_section_curve_type(static_cast<size_t>(s), core::CurveType::CATMULL_ROM);
    }
    emit_signal("structure_changed");
    emit_changed();
}

godot::Ref<Spline2DResource> Spline2DResource::create_rectangle(
    const godot::Vector2 &p_center,
    const godot::Vector2 &p_size
) {
    godot::Ref<Spline2DResource> res;
    res.instantiate();
    res->setup_rectangle(p_center, p_size);
    return res;
}

godot::Ref<Spline2DResource> Spline2DResource::create_regular_polygon(
    int p_sides,
    real_t p_radius,
    real_t p_angle_offset
) {
    godot::Ref<Spline2DResource> res;
    res.instantiate();
    res->setup_regular_polygon(p_sides, p_radius, p_angle_offset);
    return res;
}

godot::Ref<Spline2DResource> Spline2DResource::create_star(
    int p_sides,
    real_t p_outer_radius,
    real_t p_inner_radius,
    real_t p_angle_offset
) {
    godot::Ref<Spline2DResource> res;
    res.instantiate();
    res->setup_star(p_sides, p_outer_radius, p_inner_radius, p_angle_offset);
    return res;
}

godot::Ref<Spline2DResource> Spline2DResource::create_circle(real_t p_radius, int p_points) {
    godot::Ref<Spline2DResource> res;
    res.instantiate();
    res->setup_circle(p_radius, p_points);
    return res;
}

} // namespace ideam::godot_ext
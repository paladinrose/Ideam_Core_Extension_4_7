#include "spline_3d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

void Spline3DResource::_bind_methods() {
    using namespace godot;

    // --- 3D Point & Handle API ---
    ClassDB::bind_method(
        D_METHOD("add_point", "position", "type", "weight"),
        &Spline3DResource::add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("insert_point", "index", "position", "type", "weight"),
        &Spline3DResource::insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );

    ClassDB::bind_method(D_METHOD("set_point_position", "index", "position"), &Spline3DResource::set_point_position);
    ClassDB::bind_method(D_METHOD("get_point_position", "index"), &Spline3DResource::get_point_position);

    ClassDB::bind_method(
        D_METHOD("set_point_back_handle", "index", "handle", "enable"),
        &Spline3DResource::set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_back_handle", "index"), &Spline3DResource::get_point_back_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_fore_handle", "index", "handle", "enable"),
        &Spline3DResource::set_point_fore_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_fore_handle", "index"), &Spline3DResource::get_point_fore_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_handles", "index", "back_handle", "use_back", "fore_handle", "use_fore"),
        &Spline3DResource::set_point_handles
    );

    // --- 3D Undo/Redo Commit Counterparts ---
    ClassDB::bind_method(
        D_METHOD("commit_add_point", "position", "type", "weight"),
        &Spline3DResource::commit_add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_insert_point", "index", "position", "type", "weight"),
        &Spline3DResource::commit_insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_position", "index", "position"),
        &Spline3DResource::commit_set_point_position
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_back_handle", "index", "handle", "enable"),
        &Spline3DResource::commit_set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_fore_handle", "index", "handle", "enable"),
        &Spline3DResource::commit_set_point_fore_handle,
        DEFVAL(true)
    );

    // --- 3D Serialized Packed Arrays ---
    ADD_GROUP("Serialized Arrays", "");

    ClassDB::bind_method(D_METHOD("set_points_array", "points"), &Spline3DResource::set_points_array);
    ClassDB::bind_method(D_METHOD("get_points_array"), &Spline3DResource::get_points_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT),
        "set_points_array",
        "get_points_array"
    );

    ClassDB::bind_method(D_METHOD("set_back_handles_array", "handles"), &Spline3DResource::set_back_handles_array);
    ClassDB::bind_method(D_METHOD("get_back_handles_array"), &Spline3DResource::get_back_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "back_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_back_handles_array",
        "get_back_handles_array"
    );

    ClassDB::bind_method(D_METHOD("set_fore_handles_array", "handles"), &Spline3DResource::set_fore_handles_array);
    ClassDB::bind_method(D_METHOD("get_fore_handles_array"), &Spline3DResource::get_fore_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "fore_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_fore_handles_array",
        "get_fore_handles_array"
    );

    // --- 3D Parametric Evaluation, Bishop RMF & Proximity Solvers ---
    ClassDB::bind_method(D_METHOD("sample", "percent", "weighted"), &Spline3DResource::sample, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_point", "percent", "weighted"), &Spline3DResource::get_point, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section", "section_index", "local_t"), &Spline3DResource::sample_section);
    ClassDB::bind_method(D_METHOD("sample_derivative", "percent", "weighted"), &Spline3DResource::sample_derivative, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section_derivative", "section_index", "local_t"), &Spline3DResource::sample_section_derivative);
    ClassDB::bind_method(D_METHOD("sample_section_second_derivative", "section_index", "local_t"), &Spline3DResource::sample_section_second_derivative);

    ClassDB::bind_method(D_METHOD("get_tangent", "percent", "weighted"), &Spline3DResource::get_tangent, DEFVAL(true));
    ClassDB::bind_method(
        D_METHOD("get_transform_at_percent", "percent", "weighted", "up_hint"),
        &Spline3DResource::get_transform_at_percent,
        DEFVAL(true),
        DEFVAL(Vector3(0.0f, 1.0f, 0.0f))
    );
    ClassDB::bind_method(
        D_METHOD("compute_bishop_transforms", "subdivisions_per_section", "initial_up"),
        &Spline3DResource::compute_bishop_transforms,
        DEFVAL(10),
        DEFVAL(Vector3(0.0f, 1.0f, 0.0f))
    );

    ClassDB::bind_method(
        D_METHOD("get_smoothed_points", "max_subdivisions"),
        &Spline3DResource::get_smoothed_points,
        DEFVAL(10)
    );
    ClassDB::bind_method(D_METHOD("get_aabb", "use_smoothed"), &Spline3DResource::get_aabb, DEFVAL(true));
    ClassDB::bind_method(
        D_METHOD("get_closest_point_on_spline", "target", "samples"),
        &Spline3DResource::get_closest_point_on_spline,
        DEFVAL(10)
    );
    ClassDB::bind_method(
        D_METHOD("get_closest_percent_on_spline", "target", "samples", "weighted"),
        &Spline3DResource::get_closest_percent_on_spline,
        DEFVAL(10),
        DEFVAL(false)
    );

    // --- 3D Spatial Transformations & Presets ---
    ClassDB::bind_method(D_METHOD("rotate_spline", "euler_radians"), &Spline3DResource::rotate_spline);
    ClassDB::bind_method(D_METHOD("scale_spline", "scale"), &Spline3DResource::scale_spline);
    ClassDB::bind_method(D_METHOD("commit_rotate_spline", "euler_radians"), &Spline3DResource::commit_rotate_spline);
    ClassDB::bind_method(D_METHOD("commit_scale_spline", "scale"), &Spline3DResource::commit_scale_spline);

    ClassDB::bind_method(
        D_METHOD("setup_line", "start", "end", "points", "type"),
        &Spline3DResource::setup_line,
        DEFVAL(Vector3(0.0f, 0.0f, 0.0f)),
        DEFVAL(Vector3(0.0f, 0.0f, -10.0f)),
        DEFVAL(4),
        DEFVAL(CURVE_CATMULL_ROM)
    );
    ClassDB::bind_method(
        D_METHOD("setup_circle_xz", "radius", "points"),
        &Spline3DResource::setup_circle_xz,
        DEFVAL(5.0),
        DEFVAL(8)
    );
    ClassDB::bind_method(
        D_METHOD("setup_helix", "radius", "height", "turns", "points_per_turn"),
        &Spline3DResource::setup_helix,
        DEFVAL(5.0),
        DEFVAL(10.0),
        DEFVAL(2.0),
        DEFVAL(8)
    );

    ClassDB::bind_static_method(
        "Spline3DResource",
        D_METHOD("create_line", "start", "end", "points", "type"),
        &Spline3DResource::create_line,
        DEFVAL(Vector3(0.0f, 0.0f, 0.0f)),
        DEFVAL(Vector3(0.0f, 0.0f, -10.0f)),
        DEFVAL(4),
        DEFVAL(CURVE_CATMULL_ROM)
    );
    ClassDB::bind_static_method(
        "Spline3DResource",
        D_METHOD("create_circle_xz", "radius", "points"),
        &Spline3DResource::create_circle_xz,
        DEFVAL(5.0),
        DEFVAL(8)
    );
    ClassDB::bind_static_method(
        "Spline3DResource",
        D_METHOD("create_helix", "radius", "height", "turns", "points_per_turn"),
        &Spline3DResource::create_helix,
        DEFVAL(5.0),
        DEFVAL(10.0),
        DEFVAL(2.0),
        DEFVAL(8)
    );
}

Spline3DResource::Spline3DResource() {
    // Initialize with a default 4-point 3D path along -Z so new resources evaluate immediately
    setup_line(
        godot::Vector3(0.0f, 0.0f, 0.0f),
        godot::Vector3(0.0f, 0.0f, -10.0f),
        4,
        CURVE_CATMULL_ROM
    );
}

Spline3DResource::~Spline3DResource() = default;

// ============================================================================
// SplineResource Virtual Overrides
// ============================================================================

int Spline3DResource::get_point_count() const {
    return static_cast<int>(core_spline.get_point_count());
}

int Spline3DResource::get_length() const {
    return static_cast<int>(core_spline.get_length());
}

int Spline3DResource::get_section_count() const {
    return static_cast<int>(core_spline.get_section_count());
}

void Spline3DResource::set_wrap(bool p_wrap) {
    if (core_spline.get_wrap() == p_wrap) return;
    core_spline.set_wrap(p_wrap);
    emit_signal("structure_changed");
    emit_changed();
}

bool Spline3DResource::get_wrap() const {
    return core_spline.get_wrap();
}

void Spline3DResource::set_default_curve_type(SplineCurveType p_type) {
    core_spline.set_default_curve_type(static_cast<core::CurveType>(p_type));
}

SplineCurveType Spline3DResource::get_default_curve_type() const {
    return static_cast<SplineCurveType>(core_spline.get_default_curve_type());
}

void Spline3DResource::set_default_weight(real_t p_weight) {
    core_spline.set_default_weight(p_weight);
}

real_t Spline3DResource::get_default_weight() const {
    return core_spline.get_default_weight();
}

real_t Spline3DResource::get_total_weight() const {
    return core_spline.get_total_weight();
}

real_t Spline3DResource::get_total_spline_length(real_t p_sample_percent, bool p_force_recalculate) const {
    return core_spline.get_total_spline_length(p_sample_percent, p_force_recalculate);
}

void Spline3DResource::remove_point(int p_index) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.remove_point(static_cast<size_t>(p_index));
    emit_signal("structure_changed");
    emit_changed();
}

void Spline3DResource::clear() {
    if (core_spline.get_point_count() == 0) return;
    core_spline.clear();
    emit_signal("structure_changed");
    emit_changed();
}

void Spline3DResource::set_point_use_back_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_back_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline3DResource::get_point_use_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_back_handle;
}

void Spline3DResource::set_point_use_fore_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_fore_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline3DResource::get_point_use_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_fore_handle;
}

void Spline3DResource::set_section_curve_type(int p_section_index, SplineCurveType p_type) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_curve_type(
        static_cast<size_t>(p_section_index),
        static_cast<core::CurveType>(p_type)
    );
    emit_changed();
}

SplineCurveType Spline3DResource::get_section_curve_type(int p_section_index) const {
    return static_cast<SplineCurveType>(
        core_spline.get_section_curve_type(static_cast<size_t>(p_section_index))
    );
}

void Spline3DResource::set_section_weight(int p_section_index, real_t p_weight) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_weight(static_cast<size_t>(p_section_index), p_weight);
    emit_signal("section_weights_changed");
    emit_changed();
}

real_t Spline3DResource::get_section_weight(int p_section_index) const {
    return core_spline.get_section_weight(static_cast<size_t>(p_section_index));
}

real_t Spline3DResource::get_section_length(int p_section_index) const {
    return core_spline.get_section_length(static_cast<size_t>(p_section_index));
}

real_t Spline3DResource::section_local_to_global_percent(
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

godot::Vector2 Spline3DResource::get_section_at_percent(real_t p_percent, bool p_weighted) const {
    real_t local_t = static_cast<real_t>(0.0);
    const size_t sec_idx = core_spline.get_section_at_percent(p_percent, p_weighted, local_t);
    return godot::Vector2(static_cast<float>(sec_idx), static_cast<float>(local_t));
}

godot::Variant Spline3DResource::get_point_variant(real_t p_percent, bool p_weighted) const {
    return sample(p_percent, p_weighted);
}

godot::Variant Spline3DResource::get_derivative_variant(real_t p_percent, bool p_weighted) const {
    return sample_derivative(p_percent, p_weighted);
}

godot::Variant Spline3DResource::get_tangent_variant(real_t p_percent, bool p_weighted) const {
    return get_tangent(p_percent, p_weighted);
}

// ============================================================================
// 3D Point & Handle API
// ============================================================================

void Spline3DResource::add_point(
    const godot::Vector3 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    core_spline.add_point(_to_glm(p_position), static_cast<core::CurveType>(p_type), p_weight);
    emit_signal("structure_changed");
    emit_changed();
}

void Spline3DResource::insert_point(
    int p_index,
    const godot::Vector3 &p_position,
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

void Spline3DResource::set_point_position(int p_index, const godot::Vector3 &p_position) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_position(static_cast<size_t>(p_index), _to_glm(p_position));
    emit_changed();
}

godot::Vector3 Spline3DResource::get_point_position(int p_index) const {
    if (p_index < 0 || p_index >= get_point_count()) return godot::Vector3();
    return _to_godot(core_spline.get_point_position(static_cast<size_t>(p_index)));
}

void Spline3DResource::set_point_back_handle(
    int p_index,
    const godot::Vector3 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_back_handle(static_cast<size_t>(p_index), _to_glm(p_handle), p_enable);
    emit_changed();
}

godot::Vector3 Spline3DResource::get_point_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return godot::Vector3();
    return _to_godot(pts[static_cast<size_t>(p_index)].back_handle);
}

void Spline3DResource::set_point_fore_handle(
    int p_index,
    const godot::Vector3 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_fore_handle(static_cast<size_t>(p_index), _to_glm(p_handle), p_enable);
    emit_changed();
}

godot::Vector3 Spline3DResource::get_point_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return godot::Vector3();
    return _to_godot(pts[static_cast<size_t>(p_index)].fore_handle);
}

void Spline3DResource::set_point_handles(
    int p_index,
    const godot::Vector3 &p_back_handle,
    bool p_use_back,
    const godot::Vector3 &p_fore_handle,
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

void Spline3DResource::commit_add_point(
    const godot::Vector3 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    const godot::Dictionary before = get_snapshot_state();
    add_point(p_position, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Add Spline3D Point", before, after);
}

void Spline3DResource::commit_insert_point(
    int p_index,
    const godot::Vector3 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    const godot::Dictionary before = get_snapshot_state();
    insert_point(p_index, p_position, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Insert Spline3D Point", before, after);
}

void Spline3DResource::commit_set_point_position(int p_index, const godot::Vector3 &p_position) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector3 prev_pos = get_point_position(p_index);
    if (prev_pos == p_position) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Move Spline3D Point", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_position").bind(p_index, p_position));
    ur->add_undo_method(godot::Callable(this, "set_point_position").bind(p_index, prev_pos));
    ur->commit_action(true);
}

void Spline3DResource::commit_set_point_back_handle(
    int p_index,
    const godot::Vector3 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector3 prev_h = get_point_back_handle(p_index);
    const bool prev_en          = get_point_use_back_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline3D Back Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_back_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_back_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

void Spline3DResource::commit_set_point_fore_handle(
    int p_index,
    const godot::Vector3 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector3 prev_h = get_point_fore_handle(p_index);
    const bool prev_en          = get_point_use_fore_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline3D Fore Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

// ============================================================================
// Order-Independent Packed Array Serialization
// ============================================================================

void Spline3DResource::_apply_pending_arrays() {
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

void Spline3DResource::set_points_array(const godot::PackedVector3Array &p_points) {
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

godot::PackedVector3Array Spline3DResource::get_points_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].point));
    }
    return out;
}

void Spline3DResource::set_back_handles_array(const godot::PackedVector3Array &p_handles) {
    pending_back_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_back_handle(i);
        core_spline.set_point_back_handle(static_cast<size_t>(i), _to_glm(p_handles[i]), en);
    }
    emit_changed();
}

godot::PackedVector3Array Spline3DResource::get_back_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].back_handle));
    }
    return out;
}

void Spline3DResource::set_fore_handles_array(const godot::PackedVector3Array &p_handles) {
    pending_fore_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_fore_handle(i);
        core_spline.set_point_fore_handle(static_cast<size_t>(i), _to_glm(p_handles[i]), en);
    }
    emit_changed();
}

godot::PackedVector3Array Spline3DResource::get_fore_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].fore_handle));
    }
    return out;
}

godot::PackedByteArray Spline3DResource::get_handle_flags_array() const {
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

void Spline3DResource::set_handle_flags_array(const godot::PackedByteArray &p_flags) {
    pending_handle_flags = p_flags;
    const int count = std::min(static_cast<int>(p_flags.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const uint8_t fl = p_flags[i];
        core_spline.set_use_back_handle(static_cast<size_t>(i), (fl & HANDLE_USE_BACK) != 0);
        core_spline.set_use_fore_handle(static_cast<size_t>(i), (fl & HANDLE_USE_FORE) != 0);
    }
    emit_changed();
}

godot::PackedByteArray Spline3DResource::get_section_types_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedByteArray out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<uint8_t>(secs[i].curve_type));
    }
    return out;
}

void Spline3DResource::set_section_types_array(const godot::PackedByteArray &p_types) {
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

godot::PackedFloat32Array Spline3DResource::get_section_weights_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(secs[i].weight));
    }
    return out;
}

void Spline3DResource::set_section_weights_array(const godot::PackedFloat32Array &p_weights) {
    pending_section_weights = p_weights;
    const int count = std::min(static_cast<int>(p_weights.size()), get_section_count());
    for (int i = 0; i < count; ++i) {
        core_spline.set_section_weight(static_cast<size_t>(i), static_cast<real_t>(p_weights[i]));
    }
    emit_signal("section_weights_changed");
    emit_changed();
}

godot::Dictionary Spline3DResource::get_snapshot_state() const {
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

void Spline3DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("default_curve_type")) {
        set_default_curve_type(static_cast<SplineCurveType>(static_cast<int>(p_state["default_curve_type"])));
    }
    if (p_state.has("default_weight")) {
        set_default_weight(static_cast<real_t>(p_state["default_weight"]));
    }

    pending_back_handles    = p_state.get("back_handles", godot::PackedVector3Array());
    pending_fore_handles    = p_state.get("fore_handles", godot::PackedVector3Array());
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
// 3D Parametric Evaluation, Bishop RMF Frames & Proximity Solvers
// ============================================================================

godot::Vector3 Spline3DResource::sample(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_point(p_percent, p_weighted));
}

godot::Vector3 Spline3DResource::sample_section(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector3();
    return _to_godot(core_spline.evaluate_section(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector3 Spline3DResource::sample_derivative(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_derivative(p_percent, p_weighted));
}

godot::Vector3 Spline3DResource::sample_section_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector3();
    return _to_godot(core_spline.evaluate_section_derivative(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector3 Spline3DResource::sample_section_second_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector3();
    return _to_godot(core_spline.evaluate_section_second_derivative(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector3 Spline3DResource::get_tangent(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_tangent(p_percent, p_weighted));
}

godot::Transform3D Spline3DResource::get_transform_at_percent(
    real_t p_percent,
    bool p_weighted,
    const godot::Vector3 &p_up_hint
) const {
    const Vec3 pos   = core_spline.get_point(p_percent, p_weighted);
    const Vec3 tan   = core_spline.get_tangent(p_percent, p_weighted);
    const auto frame = core::ParallelTransport3D<real_t>::make_initial_frame(pos, tan, _to_glm(p_up_hint));

    // In Godot 3D, -Z is forward, Y is up (normal), and X is right (binormal)
    const godot::Vector3 x_axis = _to_godot(frame.binormal);
    const godot::Vector3 y_axis = _to_godot(frame.normal);
    const godot::Vector3 z_axis = -_to_godot(frame.tangent);
    return godot::Transform3D(godot::Basis(x_axis, y_axis, z_axis), _to_godot(frame.position));
}

godot::TypedArray<godot::Transform3D> Spline3DResource::compute_bishop_transforms(
    int p_subdivisions_per_section,
    const godot::Vector3 &p_initial_up
) const {
    godot::TypedArray<godot::Transform3D> out;
    const auto frames = core_spline.compute_bishop_frames(
        std::max(1, p_subdivisions_per_section),
        _to_glm(p_initial_up)
    );
    out.resize(static_cast<int>(frames.size()));
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto &f = frames[i];
        const godot::Vector3 x_axis = _to_godot(f.binormal);
        const godot::Vector3 y_axis = _to_godot(f.normal);
        const godot::Vector3 z_axis = -_to_godot(f.tangent);
        out[static_cast<int>(i)] = godot::Transform3D(
            godot::Basis(x_axis, y_axis, z_axis),
            _to_godot(f.position)
        );
    }
    return out;
}

godot::PackedVector3Array Spline3DResource::get_smoothed_points(int p_max_subdivisions) const {
    const auto pts = core_spline.get_smoothed_points(p_max_subdivisions);
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i]));
    }
    return out;
}

godot::AABB Spline3DResource::get_aabb(bool p_use_smoothed) const {
    if (core_spline.get_point_count() == 0) {
        return godot::AABB();
    }
    Vec3 min_v(static_cast<real_t>(0.0));
    Vec3 max_v(static_cast<real_t>(0.0));
    core_spline.get_min_max(min_v, max_v, p_use_smoothed);
    const godot::Vector3 g_min = _to_godot(min_v);
    const godot::Vector3 g_max = _to_godot(max_v);
    return godot::AABB(g_min, g_max - g_min);
}

godot::Vector3 Spline3DResource::get_closest_point_on_spline(
    const godot::Vector3 &p_target,
    int p_samples
) const {
    return _to_godot(core_spline.get_closest_point_on_spline(_to_glm(p_target), p_samples));
}

real_t Spline3DResource::get_closest_percent_on_spline(
    const godot::Vector3 &p_target,
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
// 3D Spatial Transformations & Shape Presets
// ============================================================================

void Spline3DResource::rotate_spline(const godot::Vector3 &p_euler_radians) {
    if (core_spline.get_point_count() == 0) return;
    core_spline.rotate_spline(_to_glm(p_euler_radians));
    emit_changed();
}

void Spline3DResource::scale_spline(const godot::Vector3 &p_scale) {
    if (core_spline.get_point_count() == 0) return;
    core_spline.scale_spline(_to_glm(p_scale));
    emit_changed();
}

void Spline3DResource::commit_rotate_spline(const godot::Vector3 &p_euler_radians) {
    if (core_spline.get_point_count() == 0) return;
    const godot::Dictionary before = get_snapshot_state();
    rotate_spline(p_euler_radians);
    commit_snapshot_action("Rotate 3D Spline", before, get_snapshot_state());
}

void Spline3DResource::commit_scale_spline(const godot::Vector3 &p_scale) {
    if (core_spline.get_point_count() == 0) return;
    const godot::Dictionary before = get_snapshot_state();
    scale_spline(p_scale);
    commit_snapshot_action("Scale 3D Spline", before, get_snapshot_state());
}

void Spline3DResource::setup_line(
    const godot::Vector3 &p_start,
    const godot::Vector3 &p_end,
    int p_points,
    SplineCurveType p_type
) {
    const int n = std::max(2, p_points);
    core_spline.set_wrap(false);
    core_spline.clear();
    core_spline.set_default_curve_type(static_cast<core::CurveType>(p_type));

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n - 1);
        core_spline.add_point(_to_glm(p_start.lerp(p_end, t)), static_cast<core::CurveType>(p_type));
    }
    emit_signal("structure_changed");
    emit_changed();
}

void Spline3DResource::setup_circle_xz(real_t p_radius, int p_points) {
    const int n = std::max(4, p_points);
    core_spline.set_wrap(false);
    core_spline.clear();
    core_spline.set_default_curve_type(core::CurveType::CATMULL_ROM);

    const real_t two_pi = static_cast<real_t>(2.0 * std::numbers::pi_v<double>);
    for (int i = 0; i < n; ++i) {
        const real_t angle = (static_cast<real_t>(i) / static_cast<real_t>(n)) * two_pi;
        const Vec3 pt(
            std::cos(angle) * p_radius,
            static_cast<real_t>(0.0),
            std::sin(angle) * p_radius
        );
        core_spline.add_point(pt, core::CurveType::CATMULL_ROM);
    }
    core_spline.set_wrap(true);
    core_spline.set_section_curve_type(static_cast<size_t>(n - 1), core::CurveType::CATMULL_ROM);
    emit_signal("structure_changed");
    emit_changed();
}

void Spline3DResource::setup_helix(
    real_t p_radius,
    real_t p_height,
    real_t p_turns,
    int p_points_per_turn
) {
    const int ppt   = std::max(4, p_points_per_turn);
    const real_t tr = std::max(static_cast<real_t>(0.25), p_turns);
    const int total_pts = std::max(4, static_cast<int>( std::round(tr * static_cast<real_t>(ppt)) ) + 1);

    core_spline.set_wrap(false);
    core_spline.clear();
    core_spline.set_default_curve_type(core::CurveType::CATMULL_ROM);

    const real_t total_angle = tr * static_cast<real_t>(2.0 * std::numbers::pi_v<double>);
    for (int i = 0; i < total_pts; ++i) {
        const real_t t     = static_cast<real_t>(i) / static_cast<real_t>(total_pts - 1);
        const real_t angle = t * total_angle;
        const Vec3 pt(
            std::cos(angle) * p_radius,
            t * p_height,
            std::sin(angle) * p_radius
        );
        core_spline.add_point(pt, core::CurveType::CATMULL_ROM);
    }
    emit_signal("structure_changed");
    emit_changed();
}

godot::Ref<Spline3DResource> Spline3DResource::create_line(
    const godot::Vector3 &p_start,
    const godot::Vector3 &p_end,
    int p_points,
    SplineCurveType p_type
) {
    godot::Ref<Spline3DResource> res;
    res.instantiate();
    res->setup_line(p_start, p_end, p_points, p_type);
    return res;
}

godot::Ref<Spline3DResource> Spline3DResource::create_circle_xz(real_t p_radius, int p_points) {
    godot::Ref<Spline3DResource> res;
    res.instantiate();
    res->setup_circle_xz(p_radius, p_points);
    return res;
}

godot::Ref<Spline3DResource> Spline3DResource::create_helix(
    real_t p_radius,
    real_t p_height,
    real_t p_turns,
    int p_points_per_turn
) {
    godot::Ref<Spline3DResource> res;
    res.instantiate();
    res->setup_helix(p_radius, p_height, p_turns, p_points_per_turn);
    return res;
}

} // namespace ideam::godot_ext
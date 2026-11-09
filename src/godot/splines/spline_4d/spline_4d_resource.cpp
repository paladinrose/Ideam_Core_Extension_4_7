#include "spline_4d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

void Spline4DResource::_bind_methods() {
    using namespace godot;

    // --- 4D Point & Handle API ---
    ClassDB::bind_method(
        D_METHOD("add_point", "position", "type", "weight"),
        &Spline4DResource::add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("insert_point", "index", "position", "type", "weight"),
        &Spline4DResource::insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );

    ClassDB::bind_method(D_METHOD("set_point_position", "index", "position"), &Spline4DResource::set_point_position);
    ClassDB::bind_method(D_METHOD("get_point_position", "index"), &Spline4DResource::get_point_position);

    ClassDB::bind_method(D_METHOD("set_point_xyz", "index", "xyz"), &Spline4DResource::set_point_xyz);
    ClassDB::bind_method(D_METHOD("get_point_xyz", "index"), &Spline4DResource::get_point_xyz);

    ClassDB::bind_method(D_METHOD("set_point_w", "index", "w"), &Spline4DResource::set_point_w);
    ClassDB::bind_method(D_METHOD("get_point_w", "index"), &Spline4DResource::get_point_w);

    ClassDB::bind_method(
        D_METHOD("set_point_back_handle", "index", "handle", "enable"),
        &Spline4DResource::set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_back_handle", "index"), &Spline4DResource::get_point_back_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_fore_handle", "index", "handle", "enable"),
        &Spline4DResource::set_point_fore_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_fore_handle", "index"), &Spline4DResource::get_point_fore_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_handles", "index", "back_handle", "use_back", "fore_handle", "use_fore"),
        &Spline4DResource::set_point_handles
    );

    // --- 4D Undo/Redo Commit Counterparts ---
    ClassDB::bind_method(
        D_METHOD("commit_add_point", "position", "type", "weight"),
        &Spline4DResource::commit_add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_insert_point", "index", "position", "type", "weight"),
        &Spline4DResource::commit_insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_position", "index", "position"),
        &Spline4DResource::commit_set_point_position
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_w", "index", "w"),
        &Spline4DResource::commit_set_point_w
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_back_handle", "index", "handle", "enable"),
        &Spline4DResource::commit_set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_fore_handle", "index", "handle", "enable"),
        &Spline4DResource::commit_set_point_fore_handle,
        DEFVAL(true)
    );

    // --- 4D Serialized Packed Arrays ---
    ADD_GROUP("Serialized Arrays", "");

    ClassDB::bind_method(D_METHOD("set_points_array", "points"), &Spline4DResource::set_points_array);
    ClassDB::bind_method(D_METHOD("get_points_array"), &Spline4DResource::get_points_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR4_ARRAY, "points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT),
        "set_points_array",
        "get_points_array"
    );

    ClassDB::bind_method(D_METHOD("set_back_handles_array", "handles"), &Spline4DResource::set_back_handles_array);
    ClassDB::bind_method(D_METHOD("get_back_handles_array"), &Spline4DResource::get_back_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR4_ARRAY, "back_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_back_handles_array",
        "get_back_handles_array"
    );

    ClassDB::bind_method(D_METHOD("set_fore_handles_array", "handles"), &Spline4DResource::set_fore_handles_array);
    ClassDB::bind_method(D_METHOD("get_fore_handles_array"), &Spline4DResource::get_fore_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_VECTOR4_ARRAY, "fore_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_fore_handles_array",
        "get_fore_handles_array"
    );

    // --- 4D Parametric Evaluation, Bishop RMF & Color Sampling ---
    ClassDB::bind_method(D_METHOD("sample", "percent", "weighted"), &Spline4DResource::sample, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_point", "percent", "weighted"), &Spline4DResource::get_point, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section", "section_index", "local_t"), &Spline4DResource::sample_section);
    ClassDB::bind_method(D_METHOD("sample_derivative", "percent", "weighted"), &Spline4DResource::sample_derivative, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section_derivative", "section_index", "local_t"), &Spline4DResource::sample_section_derivative);
    ClassDB::bind_method(D_METHOD("sample_section_second_derivative", "section_index", "local_t"), &Spline4DResource::sample_section_second_derivative);

    ClassDB::bind_method(D_METHOD("get_tangent", "percent", "weighted"), &Spline4DResource::get_tangent, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_tangent_3d", "percent", "weighted"), &Spline4DResource::get_tangent_3d, DEFVAL(true));

    ClassDB::bind_method(
        D_METHOD("sample_color", "percent", "weighted", "hsva_to_rgba"),
        &Spline4DResource::sample_color,
        DEFVAL(true),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("get_transform_3d_at_percent", "percent", "weighted", "w_is_roll", "up_hint"),
        &Spline4DResource::get_transform_3d_at_percent,
        DEFVAL(true),
        DEFVAL(false),
        DEFVAL(Vector3(0.0f, 1.0f, 0.0f))
    );
    ClassDB::bind_method(
        D_METHOD("compute_bishop_transforms", "subdivisions_per_section", "initial_up", "w_is_roll"),
        &Spline4DResource::compute_bishop_transforms,
        DEFVAL(10),
        DEFVAL(Vector3(0.0f, 1.0f, 0.0f)),
        DEFVAL(false)
    );

    ClassDB::bind_method(
        D_METHOD("get_smoothed_points", "max_subdivisions"),
        &Spline4DResource::get_smoothed_points,
        DEFVAL(10)
    );
    ClassDB::bind_method(D_METHOD("get_aabb_3d", "use_smoothed"), &Spline4DResource::get_aabb_3d, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_w_range", "use_smoothed"), &Spline4DResource::get_w_range, DEFVAL(true));
    ClassDB::bind_method(
        D_METHOD("get_closest_point_on_spline", "target", "samples"),
        &Spline4DResource::get_closest_point_on_spline,
        DEFVAL(10)
    );
    ClassDB::bind_method(
        D_METHOD("get_closest_percent_on_spline", "target", "samples", "weighted"),
        &Spline4DResource::get_closest_percent_on_spline,
        DEFVAL(10),
        DEFVAL(false)
    );

    // --- Semi-Spatial 3D Transformations & 4D Presets ---
    ClassDB::bind_method(D_METHOD("rotate_spline", "euler_radians"), &Spline4DResource::rotate_spline);
    ClassDB::bind_method(D_METHOD("scale_spline", "scale"), &Spline4DResource::scale_spline);
    ClassDB::bind_method(D_METHOD("commit_rotate_spline", "euler_radians"), &Spline4DResource::commit_rotate_spline);
    ClassDB::bind_method(D_METHOD("commit_scale_spline", "scale"), &Spline4DResource::commit_scale_spline);

    ClassDB::bind_method(
        D_METHOD("setup_tapered_line", "start", "start_w", "end", "end_w", "points", "type"),
        &Spline4DResource::setup_tapered_line,
        DEFVAL(Vector3(0.0f, 0.0f, 0.0f)),
        DEFVAL(1.0),
        DEFVAL(Vector3(0.0f, 0.0f, -10.0f)),
        DEFVAL(0.25),
        DEFVAL(4),
        DEFVAL(CURVE_CATMULL_ROM)
    );
    ClassDB::bind_method(
        D_METHOD("setup_banked_circle_xz", "radius", "bank_roll_rad", "points"),
        &Spline4DResource::setup_banked_circle_xz,
        DEFVAL(5.0),
        DEFVAL(0.35),
        DEFVAL(8)
    );
    ClassDB::bind_method(
        D_METHOD("setup_color_gradient", "colors", "convert_to_hsva"),
        &Spline4DResource::setup_color_gradient,
        DEFVAL(false)
    );

    ClassDB::bind_static_method(
        "Spline4DResource",
        D_METHOD("create_tapered_line", "start", "start_w", "end", "end_w", "points", "type"),
        &Spline4DResource::create_tapered_line,
        DEFVAL(Vector3(0.0f, 0.0f, 0.0f)),
        DEFVAL(1.0),
        DEFVAL(Vector3(0.0f, 0.0f, -10.0f)),
        DEFVAL(0.25),
        DEFVAL(4),
        DEFVAL(CURVE_CATMULL_ROM)
    );
    ClassDB::bind_static_method(
        "Spline4DResource",
        D_METHOD("create_banked_circle_xz", "radius", "bank_roll_rad", "points"),
        &Spline4DResource::create_banked_circle_xz,
        DEFVAL(5.0),
        DEFVAL(0.35),
        DEFVAL(8)
    );
    ClassDB::bind_static_method(
        "Spline4DResource",
        D_METHOD("create_color_gradient", "colors", "convert_to_hsva"),
        &Spline4DResource::create_color_gradient,
        DEFVAL(false)
    );
}

Spline4DResource::Spline4DResource() {
    // Initialize with a default 4-point tapered 3D path (W = 1.0 -> 0.25)
    setup_tapered_line(
        godot::Vector3(0.0f, 0.0f, 0.0f),
        static_cast<real_t>(1.0),
        godot::Vector3(0.0f, 0.0f, -10.0f),
        static_cast<real_t>(0.25),
        4,
        CURVE_CATMULL_ROM
    );
}

Spline4DResource::~Spline4DResource() = default;

// ============================================================================
// SplineResource Virtual Overrides
// ============================================================================

int Spline4DResource::get_point_count() const {
    return static_cast<int>(core_spline.get_point_count());
}

int Spline4DResource::get_length() const {
    return static_cast<int>(core_spline.get_length());
}

int Spline4DResource::get_section_count() const {
    return static_cast<int>(core_spline.get_section_count());
}

void Spline4DResource::set_wrap(bool p_wrap) {
    if (core_spline.get_wrap() == p_wrap) return;
    core_spline.set_wrap(p_wrap);
    emit_signal("structure_changed");
    emit_changed();
}

bool Spline4DResource::get_wrap() const {
    return core_spline.get_wrap();
}

void Spline4DResource::set_default_curve_type(SplineCurveType p_type) {
    core_spline.set_default_curve_type(static_cast<core::CurveType>(p_type));
}

SplineCurveType Spline4DResource::get_default_curve_type() const {
    return static_cast<SplineCurveType>(core_spline.get_default_curve_type());
}

void Spline4DResource::set_default_weight(real_t p_weight) {
    core_spline.set_default_weight(p_weight);
}

real_t Spline4DResource::get_default_weight() const {
    return core_spline.get_default_weight();
}

real_t Spline4DResource::get_total_weight() const {
    return core_spline.get_total_weight();
}

real_t Spline4DResource::get_total_spline_length(real_t p_sample_percent, bool p_force_recalculate) const {
    return core_spline.get_total_spline_length(p_sample_percent, p_force_recalculate);
}

void Spline4DResource::remove_point(int p_index) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.remove_point(static_cast<size_t>(p_index));
    emit_signal("structure_changed");
    emit_changed();
}

void Spline4DResource::clear() {
    if (core_spline.get_point_count() == 0) return;
    core_spline.clear();
    emit_signal("structure_changed");
    emit_changed();
}

void Spline4DResource::set_point_use_back_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_back_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline4DResource::get_point_use_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_back_handle;
}

void Spline4DResource::set_point_use_fore_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_fore_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline4DResource::get_point_use_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_fore_handle;
}

void Spline4DResource::set_section_curve_type(int p_section_index, SplineCurveType p_type) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_curve_type(
        static_cast<size_t>(p_section_index),
        static_cast<core::CurveType>(p_type)
    );
    emit_changed();
}

SplineCurveType Spline4DResource::get_section_curve_type(int p_section_index) const {
    return static_cast<SplineCurveType>(
        core_spline.get_section_curve_type(static_cast<size_t>(p_section_index))
    );
}

void Spline4DResource::set_section_weight(int p_section_index, real_t p_weight) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_weight(static_cast<size_t>(p_section_index), p_weight);
    emit_signal("section_weights_changed");
    emit_changed();
}

real_t Spline4DResource::get_section_weight(int p_section_index) const {
    return core_spline.get_section_weight(static_cast<size_t>(p_section_index));
}

real_t Spline4DResource::get_section_length(int p_section_index) const {
    return core_spline.get_section_length(static_cast<size_t>(p_section_index));
}

real_t Spline4DResource::section_local_to_global_percent(
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

godot::Vector2 Spline4DResource::get_section_at_percent(real_t p_percent, bool p_weighted) const {
    real_t local_t = static_cast<real_t>(0.0);
    const size_t sec_idx = core_spline.get_section_at_percent(p_percent, p_weighted, local_t);
    return godot::Vector2(static_cast<float>(sec_idx), static_cast<float>(local_t));
}

godot::Variant Spline4DResource::get_point_variant(real_t p_percent, bool p_weighted) const {
    return sample(p_percent, p_weighted);
}

godot::Variant Spline4DResource::get_derivative_variant(real_t p_percent, bool p_weighted) const {
    return sample_derivative(p_percent, p_weighted);
}

godot::Variant Spline4DResource::get_tangent_variant(real_t p_percent, bool p_weighted) const {
    return get_tangent(p_percent, p_weighted);
}

// ============================================================================
// 4D Point & Handle API
// ============================================================================

void Spline4DResource::add_point(
    const godot::Vector4 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    core_spline.add_point(_to_glm(p_position), static_cast<core::CurveType>(p_type), p_weight);
    emit_signal("structure_changed");
    emit_changed();
}

void Spline4DResource::insert_point(
    int p_index,
    const godot::Vector4 &p_position,
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

void Spline4DResource::set_point_position(int p_index, const godot::Vector4 &p_position) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_position(static_cast<size_t>(p_index), _to_glm(p_position));
    emit_changed();
}

godot::Vector4 Spline4DResource::get_point_position(int p_index) const {
    if (p_index < 0 || p_index >= get_point_count()) return godot::Vector4();
    return _to_godot(core_spline.get_point_position(static_cast<size_t>(p_index)));
}

void Spline4DResource::set_point_xyz(int p_index, const godot::Vector3 &p_xyz) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    godot::Vector4 pos = get_point_position(p_index);
    pos.x = p_xyz.x;
    pos.y = p_xyz.y;
    pos.z = p_xyz.z;
    set_point_position(p_index, pos);
}

godot::Vector3 Spline4DResource::get_point_xyz(int p_index) const {
    const godot::Vector4 pos = get_point_position(p_index);
    return godot::Vector3(pos.x, pos.y, pos.z);
}

void Spline4DResource::set_point_w(int p_index, real_t p_w) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    godot::Vector4 pos = get_point_position(p_index);
    pos.w = static_cast<float>(p_w);
    set_point_position(p_index, pos);
}

real_t Spline4DResource::get_point_w(int p_index) const {
    return static_cast<real_t>(get_point_position(p_index).w);
}

void Spline4DResource::set_point_back_handle(
    int p_index,
    const godot::Vector4 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_back_handle(static_cast<size_t>(p_index), _to_glm(p_handle), p_enable);
    emit_changed();
}

godot::Vector4 Spline4DResource::get_point_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return godot::Vector4();
    return _to_godot(pts[static_cast<size_t>(p_index)].back_handle);
}

void Spline4DResource::set_point_fore_handle(
    int p_index,
    const godot::Vector4 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_fore_handle(static_cast<size_t>(p_index), _to_glm(p_handle), p_enable);
    emit_changed();
}

godot::Vector4 Spline4DResource::get_point_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return godot::Vector4();
    return _to_godot(pts[static_cast<size_t>(p_index)].fore_handle);
}

void Spline4DResource::set_point_handles(
    int p_index,
    const godot::Vector4 &p_back_handle,
    bool p_use_back,
    const godot::Vector4 &p_fore_handle,
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

void Spline4DResource::commit_add_point(
    const godot::Vector4 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    const godot::Dictionary before = get_snapshot_state();
    add_point(p_position, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Add Spline4D Point", before, after);
}

void Spline4DResource::commit_insert_point(
    int p_index,
    const godot::Vector4 &p_position,
    SplineCurveType p_type,
    real_t p_weight
) {
    const godot::Dictionary before = get_snapshot_state();
    insert_point(p_index, p_position, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Insert Spline4D Point", before, after);
}

void Spline4DResource::commit_set_point_position(int p_index, const godot::Vector4 &p_position) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector4 prev_pos = get_point_position(p_index);
    if (prev_pos == p_position) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Move Spline4D Point", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_position").bind(p_index, p_position));
    ur->add_undo_method(godot::Callable(this, "set_point_position").bind(p_index, prev_pos));
    ur->commit_action(true);
}

void Spline4DResource::commit_set_point_w(int p_index, real_t p_w) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    godot::Vector4 next_pos = get_point_position(p_index);
    next_pos.w = static_cast<float>(p_w);
    commit_set_point_position(p_index, next_pos);
}

void Spline4DResource::commit_set_point_back_handle(
    int p_index,
    const godot::Vector4 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector4 prev_h = get_point_back_handle(p_index);
    const bool prev_en          = get_point_use_back_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline4D Back Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_back_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_back_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

void Spline4DResource::commit_set_point_fore_handle(
    int p_index,
    const godot::Vector4 &p_handle,
    bool p_enable
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Vector4 prev_h = get_point_fore_handle(p_index);
    const bool prev_en          = get_point_use_fore_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline4D Fore Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

// ============================================================================
// Order-Independent Packed Array Serialization
// ============================================================================

void Spline4DResource::_apply_pending_arrays() {
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

void Spline4DResource::set_points_array(const godot::PackedVector4Array &p_points) {
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

godot::PackedVector4Array Spline4DResource::get_points_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector4Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].point));
    }
    return out;
}

void Spline4DResource::set_back_handles_array(const godot::PackedVector4Array &p_handles) {
    pending_back_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_back_handle(i);
        core_spline.set_point_back_handle(static_cast<size_t>(i), _to_glm(p_handles[i]), en);
    }
    emit_changed();
}

godot::PackedVector4Array Spline4DResource::get_back_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector4Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].back_handle));
    }
    return out;
}

void Spline4DResource::set_fore_handles_array(const godot::PackedVector4Array &p_handles) {
    pending_fore_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_fore_handle(i);
        core_spline.set_point_fore_handle(static_cast<size_t>(i), _to_glm(p_handles[i]), en);
    }
    emit_changed();
}

godot::PackedVector4Array Spline4DResource::get_fore_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedVector4Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i].fore_handle));
    }
    return out;
}

godot::PackedByteArray Spline4DResource::get_handle_flags_array() const {
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

void Spline4DResource::set_handle_flags_array(const godot::PackedByteArray &p_flags) {
    pending_handle_flags = p_flags;
    const int count = std::min(static_cast<int>(p_flags.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const uint8_t fl = p_flags[i];
        core_spline.set_use_back_handle(static_cast<size_t>(i), (fl & HANDLE_USE_BACK) != 0);
        core_spline.set_use_fore_handle(static_cast<size_t>(i), (fl & HANDLE_USE_FORE) != 0);
    }
    emit_changed();
}

godot::PackedByteArray Spline4DResource::get_section_types_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedByteArray out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<uint8_t>(secs[i].curve_type));
    }
    return out;
}

void Spline4DResource::set_section_types_array(const godot::PackedByteArray &p_types) {
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

godot::PackedFloat32Array Spline4DResource::get_section_weights_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(secs[i].weight));
    }
    return out;
}

void Spline4DResource::set_section_weights_array(const godot::PackedFloat32Array &p_weights) {
    pending_section_weights = p_weights;
    const int count = std::min(static_cast<int>(p_weights.size()), get_section_count());
    for (int i = 0; i < count; ++i) {
        core_spline.set_section_weight(static_cast<size_t>(i), static_cast<real_t>(p_weights[i]));
    }
    emit_signal("section_weights_changed");
    emit_changed();
}

godot::Dictionary Spline4DResource::get_snapshot_state() const {
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

void Spline4DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("default_curve_type")) {
        set_default_curve_type(static_cast<SplineCurveType>(static_cast<int>(p_state["default_curve_type"])));
    }
    if (p_state.has("default_weight")) {
        set_default_weight(static_cast<real_t>(p_state["default_weight"]));
    }

    pending_back_handles    = p_state.get("back_handles", godot::PackedVector4Array());
    pending_fore_handles    = p_state.get("fore_handles", godot::PackedVector4Array());
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
// 4D Parametric Evaluation, Bishop RMF (XYZ + W) & Color Sampling
// ============================================================================

godot::Vector4 Spline4DResource::sample(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_point(p_percent, p_weighted));
}

godot::Vector4 Spline4DResource::sample_section(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector4();
    return _to_godot(core_spline.evaluate_section(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector4 Spline4DResource::sample_derivative(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_derivative(p_percent, p_weighted));
}

godot::Vector4 Spline4DResource::sample_section_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector4();
    return _to_godot(core_spline.evaluate_section_derivative(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector4 Spline4DResource::sample_section_second_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return godot::Vector4();
    return _to_godot(core_spline.evaluate_section_second_derivative(static_cast<size_t>(p_section_index), p_local_t));
}

godot::Vector4 Spline4DResource::get_tangent(real_t p_percent, bool p_weighted) const {
    return _to_godot(core_spline.get_tangent(p_percent, p_weighted));
}

godot::Vector3 Spline4DResource::get_tangent_3d(real_t p_percent, bool p_weighted) const {
    const godot::Vector4 deriv = sample_derivative(p_percent, p_weighted);
    const godot::Vector3 d3(deriv.x, deriv.y, deriv.z);
    if (d3.length_squared() > 1e-8f) {
        return d3.normalized();
    }
    return godot::Vector3(0.0f, 0.0f, -1.0f);
}

godot::Color Spline4DResource::sample_color(
    real_t p_percent,
    bool p_weighted,
    bool p_hsva_to_rgba
) const {
    const godot::Vector4 v = sample(p_percent, p_weighted);
    if (p_hsva_to_rgba) {
        // Hue in core::Spline4D::create_gradient is in degrees [0, 360] or normalized [0, 1]
        float h = (v.x > 1.0f) ? std::fmod(v.x / 360.0f, 1.0f) : std::fmod(v.x, 1.0f);
        if (h < 0.0f) h += 1.0f;
        const float s = std::clamp(v.y, 0.0f, 1.0f);
        const float val = std::clamp(v.z, 0.0f, 1.0f);
        const float a = std::clamp(v.w, 0.0f, 1.0f);
        return godot::Color::from_hsv(h, s, val, a);
    }
    return godot::Color(v.x, v.y, v.z, v.w);
}

godot::Transform3D Spline4DResource::get_transform_3d_at_percent(
    real_t p_percent,
    bool p_weighted,
    bool p_w_is_roll,
    const godot::Vector3 &p_up_hint
) const {
    const godot::Vector4 pt4 = sample(p_percent, p_weighted);
    const Vec3 pos(static_cast<real_t>(pt4.x), static_cast<real_t>(pt4.y), static_cast<real_t>(pt4.z));
    const Vec3 tan = _to_glm3(get_tangent_3d(p_percent, p_weighted));

    const real_t scale_val = p_w_is_roll ? static_cast<real_t>(1.0) : static_cast<real_t>(pt4.w);
    const real_t roll_val  = p_w_is_roll ? static_cast<real_t>(pt4.w) : static_cast<real_t>(0.0);

    auto frame = core::ParallelTransport3D<real_t>::make_initial_frame(
        pos,
        tan,
        _to_glm3(p_up_hint),
        scale_val,
        roll_val
    );
    if (p_w_is_roll && std::abs(roll_val) > static_cast<real_t>(1e-6)) {
        frame = core::ParallelTransport3D<real_t>::apply_axial_roll(frame, roll_val);
    }

    const float s = static_cast<float>(frame.scale);
    const godot::Vector3 x_axis = _to_godot3(frame.binormal) * s;
    const godot::Vector3 y_axis = _to_godot3(frame.normal) * s;
    const godot::Vector3 z_axis = -_to_godot3(frame.tangent) * s;
    return godot::Transform3D(godot::Basis(x_axis, y_axis, z_axis), _to_godot3(frame.position));
}

godot::TypedArray<godot::Transform3D> Spline4DResource::compute_bishop_transforms(
    int p_subdivisions_per_section,
    const godot::Vector3 &p_initial_up,
    bool p_w_is_roll
) const {
    godot::TypedArray<godot::Transform3D> out;
    const auto frames = core_spline.compute_bishop_frames(
        std::max(1, p_subdivisions_per_section),
        _to_glm3(p_initial_up),
        p_w_is_roll
    );
    out.resize(static_cast<int>(frames.size()));
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto &f = frames[i];
        const float s = p_w_is_roll ? 1.0f : static_cast<float>(f.scale);
        const godot::Vector3 x_axis = _to_godot3(f.binormal) * s;
        const godot::Vector3 y_axis = _to_godot3(f.normal) * s;
        const godot::Vector3 z_axis = -_to_godot3(f.tangent) * s;
        out[static_cast<int>(i)] = godot::Transform3D(
            godot::Basis(x_axis, y_axis, z_axis),
            _to_godot3(f.position)
        );
    }
    return out;
}

godot::PackedVector4Array Spline4DResource::get_smoothed_points(int p_max_subdivisions) const {
    const auto pts = core_spline.get_smoothed_points(p_max_subdivisions);
    godot::PackedVector4Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i]));
    }
    return out;
}

godot::AABB Spline4DResource::get_aabb_3d(bool p_use_smoothed) const {
    if (core_spline.get_point_count() == 0) {
        return godot::AABB();
    }
    Vec4 min_v(static_cast<real_t>(0.0));
    Vec4 max_v(static_cast<real_t>(0.0));
    core_spline.get_min_max(min_v, max_v, p_use_smoothed);
    const godot::Vector3 g_min(static_cast<float>(min_v.x), static_cast<float>(min_v.y), static_cast<float>(min_v.z));
    const godot::Vector3 g_max(static_cast<float>(max_v.x), static_cast<float>(max_v.y), static_cast<float>(max_v.z));
    return godot::AABB(g_min, g_max - g_min);
}

godot::Vector2 Spline4DResource::get_w_range(bool p_use_smoothed) const {
    if (core_spline.get_point_count() == 0) {
        return godot::Vector2(0.0f, 1.0f);
    }
    Vec4 min_v(static_cast<real_t>(0.0));
    Vec4 max_v(static_cast<real_t>(0.0));
    core_spline.get_min_max(min_v, max_v, p_use_smoothed);
    return godot::Vector2(static_cast<float>(min_v.w), static_cast<float>(max_v.w));
}

godot::Vector4 Spline4DResource::get_closest_point_on_spline(
    const godot::Vector4 &p_target,
    int p_samples
) const {
    return _to_godot(core_spline.get_closest_point_on_spline(_to_glm(p_target), p_samples));
}

real_t Spline4DResource::get_closest_percent_on_spline(
    const godot::Vector4 &p_target,
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
// Semi-Spatial 3D Transformations & 4D Presets
// ============================================================================

void Spline4DResource::rotate_spline(const godot::Vector3 &p_euler_radians) {
    if (core_spline.get_point_count() == 0) return;
    core_spline.rotate_spline(_to_glm3(p_euler_radians));
    emit_changed();
}

void Spline4DResource::scale_spline(const godot::Vector3 &p_scale) {
    if (core_spline.get_point_count() == 0) return;
    core_spline.scale_spline(_to_glm3(p_scale));
    emit_changed();
}

void Spline4DResource::commit_rotate_spline(const godot::Vector3 &p_euler_radians) {
    if (core_spline.get_point_count() == 0) return;
    const godot::Dictionary before = get_snapshot_state();
    rotate_spline(p_euler_radians);
    commit_snapshot_action("Rotate 4D Spline XYZ", before, get_snapshot_state());
}

void Spline4DResource::commit_scale_spline(const godot::Vector3 &p_scale) {
    if (core_spline.get_point_count() == 0) return;
    const godot::Dictionary before = get_snapshot_state();
    scale_spline(p_scale);
    commit_snapshot_action("Scale 4D Spline XYZ", before, get_snapshot_state());
}

void Spline4DResource::setup_tapered_line(
    const godot::Vector3 &p_start,
    real_t p_start_w,
    const godot::Vector3 &p_end,
    real_t p_end_w,
    int p_points,
    SplineCurveType p_type
) {
    const int n = std::max(2, p_points);
    core_spline.set_wrap(false);
    core_spline.clear();
    core_spline.set_default_curve_type(static_cast<core::CurveType>(p_type));

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n - 1);
        const godot::Vector3 xyz = p_start.lerp(p_end, t);
        const real_t w = p_start_w + (p_end_w - p_start_w) * static_cast<real_t>(t);
        core_spline.add_point(
            Vec4(static_cast<real_t>(xyz.x), static_cast<real_t>(xyz.y), static_cast<real_t>(xyz.z), w),
            static_cast<core::CurveType>(p_type)
        );
    }
    emit_signal("structure_changed");
    emit_changed();
}

void Spline4DResource::setup_banked_circle_xz(
    real_t p_radius,
    real_t p_bank_roll_rad,
    int p_points
) {
    const int n = std::max(4, p_points);
    core_spline.set_wrap(false);
    core_spline.clear();
    core_spline.set_default_curve_type(core::CurveType::CATMULL_ROM);

    const real_t two_pi = static_cast<real_t>(2.0 * std::numbers::pi_v<double>);
    for (int i = 0; i < n; ++i) {
        const real_t angle = (static_cast<real_t>(i) / static_cast<real_t>(n)) * two_pi;
        const real_t w_roll = std::sin(angle * static_cast<real_t>(2.0)) * p_bank_roll_rad;
        const Vec4 pt(
            std::cos(angle) * p_radius,
            static_cast<real_t>(0.0),
            std::sin(angle) * p_radius,
            w_roll
        );
        core_spline.add_point(pt, core::CurveType::CATMULL_ROM);
    }
    core_spline.set_wrap(true);
    core_spline.set_section_curve_type(static_cast<size_t>(n - 1), core::CurveType::CATMULL_ROM);
    emit_signal("structure_changed");
    emit_changed();
}

void Spline4DResource::setup_color_gradient(
    const godot::PackedColorArray &p_colors,
    bool p_convert_to_hsva
) {
    std::vector<Vec4> cols;
    cols.reserve(static_cast<size_t>(p_colors.size()));
    for (int i = 0; i < p_colors.size(); ++i) {
        const godot::Color c = p_colors[i];
        cols.emplace_back(
            static_cast<real_t>(c.r),
            static_cast<real_t>(c.g),
            static_cast<real_t>(c.b),
            static_cast<real_t>(c.a)
        );
    }

    if (p_convert_to_hsva) {
        core_spline = CoreSpline::create_gradient(cols);
    } else {
        core_spline.set_wrap(false);
        core_spline.clear();
        core_spline.set_default_curve_type(core::CurveType::CATMULL_ROM);
        if (cols.size() < 2) {
            core_spline.add_point(Vec4(0.15f, 0.55f, 0.98f, 1.0f), core::CurveType::CATMULL_ROM);
            core_spline.add_point(Vec4(0.25f, 0.92f, 0.65f, 1.0f), core::CurveType::CATMULL_ROM);
            core_spline.add_point(Vec4(0.98f, 0.78f, 0.24f, 1.0f), core::CurveType::CATMULL_ROM);
            core_spline.add_point(Vec4(0.96f, 0.32f, 0.52f, 1.0f), core::CurveType::CATMULL_ROM);
        } else {
            for (const auto &v : cols) {
                core_spline.add_point(v, core::CurveType::CATMULL_ROM);
            }
        }
    }

    emit_signal("structure_changed");
    emit_changed();
}

godot::Ref<Spline4DResource> Spline4DResource::create_tapered_line(
    const godot::Vector3 &p_start,
    real_t p_start_w,
    const godot::Vector3 &p_end,
    real_t p_end_w,
    int p_points,
    SplineCurveType p_type
) {
    godot::Ref<Spline4DResource> res;
    res.instantiate();
    res->setup_tapered_line(p_start, p_start_w, p_end, p_end_w, p_points, p_type);
    return res;
}

godot::Ref<Spline4DResource> Spline4DResource::create_banked_circle_xz(
    real_t p_radius,
    real_t p_bank_roll_rad,
    int p_points
) {
    godot::Ref<Spline4DResource> res;
    res.instantiate();
    res->setup_banked_circle_xz(p_radius, p_bank_roll_rad, p_points);
    return res;
}

godot::Ref<Spline4DResource> Spline4DResource::create_color_gradient(
    const godot::PackedColorArray &p_colors,
    bool p_convert_to_hsva
) {
    godot::Ref<Spline4DResource> res;
    res.instantiate();
    res->setup_color_gradient(p_colors, p_convert_to_hsva);
    return res;
}

} // namespace ideam::godot_ext
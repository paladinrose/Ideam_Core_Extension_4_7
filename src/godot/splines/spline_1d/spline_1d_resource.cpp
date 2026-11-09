#include "spline_1d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void Spline1DResource::_bind_methods() {
    using namespace godot;

    // --- 1D Point & Handle API ---
    ClassDB::bind_method(
        D_METHOD("add_point", "value", "type", "weight"),
        &Spline1DResource::add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("insert_point", "index", "value", "type", "weight"),
        &Spline1DResource::insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );

    ClassDB::bind_method(D_METHOD("set_point_position", "index", "value"), &Spline1DResource::set_point_position);
    ClassDB::bind_method(D_METHOD("get_point_position", "index"), &Spline1DResource::get_point_position);

    ClassDB::bind_method(
        D_METHOD("set_point_back_handle", "index", "handle", "enable"),
        &Spline1DResource::set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_back_handle", "index"), &Spline1DResource::get_point_back_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_fore_handle", "index", "handle", "enable"),
        &Spline1DResource::set_point_fore_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_point_fore_handle", "index"), &Spline1DResource::get_point_fore_handle);

    ClassDB::bind_method(
        D_METHOD("set_point_handles", "index", "back_handle", "use_back", "fore_handle", "use_fore"),
        &Spline1DResource::set_point_handles
    );

    // --- 1D Undo/Redo Commit Counterparts ---
    ClassDB::bind_method(
        D_METHOD("commit_add_point", "value", "type", "weight"),
        &Spline1DResource::commit_add_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(
        D_METHOD("commit_insert_point", "index", "value", "type", "weight"),
        &Spline1DResource::commit_insert_point,
        DEFVAL(CURVE_NONE),
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(D_METHOD("commit_set_point_position", "index", "value"), &Spline1DResource::commit_set_point_position);
    ClassDB::bind_method(
        D_METHOD("commit_set_point_back_handle", "index", "handle", "enable"),
        &Spline1DResource::commit_set_point_back_handle,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_point_fore_handle", "index", "handle", "enable"),
        &Spline1DResource::commit_set_point_fore_handle,
        DEFVAL(true)
    );

    // --- 1D Serialized Packed Arrays ---
    ADD_GROUP("Serialized Arrays", "");

    ClassDB::bind_method(D_METHOD("set_points_array", "points"), &Spline1DResource::set_points_array);
    ClassDB::bind_method(D_METHOD("get_points_array"), &Spline1DResource::get_points_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT),
        "set_points_array",
        "get_points_array"
    );

    ClassDB::bind_method(D_METHOD("set_back_handles_array", "handles"), &Spline1DResource::set_back_handles_array);
    ClassDB::bind_method(D_METHOD("get_back_handles_array"), &Spline1DResource::get_back_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "back_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_back_handles_array",
        "get_back_handles_array"
    );

    ClassDB::bind_method(D_METHOD("set_fore_handles_array", "handles"), &Spline1DResource::set_fore_handles_array);
    ClassDB::bind_method(D_METHOD("get_fore_handles_array"), &Spline1DResource::get_fore_handles_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "fore_handles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_fore_handles_array",
        "get_fore_handles_array"
    );

    // --- 1D Evaluation & Solvers ---
    ClassDB::bind_method(D_METHOD("sample", "percent", "weighted"), &Spline1DResource::sample, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_point", "percent", "weighted"), &Spline1DResource::get_point, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section", "section_index", "local_t"), &Spline1DResource::sample_section);
    ClassDB::bind_method(D_METHOD("sample_derivative", "percent", "weighted"), &Spline1DResource::sample_derivative, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("sample_section_derivative", "section_index", "local_t"), &Spline1DResource::sample_section_derivative);
    ClassDB::bind_method(D_METHOD("sample_section_second_derivative", "section_index", "local_t"), &Spline1DResource::sample_section_second_derivative);

    ClassDB::bind_method(
        D_METHOD("get_smoothed_points", "max_subdivisions"),
        &Spline1DResource::get_smoothed_points,
        DEFVAL(10)
    );
    ClassDB::bind_method(
        D_METHOD("get_percents_at_value", "value", "samples", "weighted"),
        &Spline1DResource::get_percents_at_value,
        DEFVAL(10),
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_min_value", "use_smoothed"), &Spline1DResource::get_min_value, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_max_value", "use_smoothed"), &Spline1DResource::get_max_value, DEFVAL(false));
    ClassDB::bind_method(
        D_METHOD("get_closest_value_on_spline", "target_value", "samples"),
        &Spline1DResource::get_closest_value_on_spline,
        DEFVAL(10)
    );

    // --- 1D Presets & Static Builders ---
    ClassDB::bind_method(D_METHOD("setup_zero_to_one"), &Spline1DResource::setup_zero_to_one);
    ClassDB::bind_method(D_METHOD("setup_one_to_zero"), &Spline1DResource::setup_one_to_zero);
    ClassDB::bind_method(
        D_METHOD("setup_equidistant", "count", "type"),
        &Spline1DResource::setup_equidistant,
        DEFVAL(4),
        DEFVAL(CURVE_LINEAR)
    );

    ClassDB::bind_static_method(
        "Spline1DResource",
        D_METHOD("create_zero_to_one"),
        &Spline1DResource::create_zero_to_one
    );
    ClassDB::bind_static_method(
        "Spline1DResource",
        D_METHOD("create_one_to_zero"),
        &Spline1DResource::create_one_to_zero
    );
    ClassDB::bind_static_method(
        "Spline1DResource",
        D_METHOD("create_equidistant", "count", "type"),
        &Spline1DResource::create_equidistant,
        DEFVAL(4),
        DEFVAL(CURVE_LINEAR)
    );
}

Spline1DResource::Spline1DResource() {
    // Initialize with a sensible 0 -> 1 linear ramp by default so new resources in the
    // Inspector immediately evaluate meaningfully.
    core_spline = CoreSpline::create_zero_to_one();
}

Spline1DResource::~Spline1DResource() {
}

// ============================================================================
// SplineResource Virtual Overrides
// ============================================================================

int Spline1DResource::get_point_count() const {
    return static_cast<int>(core_spline.get_point_count());
}

int Spline1DResource::get_length() const {
    return static_cast<int>(core_spline.get_length());
}

int Spline1DResource::get_section_count() const {
    return static_cast<int>(core_spline.get_section_count());
}

void Spline1DResource::set_wrap(bool p_wrap) {
    if (core_spline.get_wrap() == p_wrap) return;
    core_spline.set_wrap(p_wrap);
    emit_signal("structure_changed");
    emit_changed();
}

bool Spline1DResource::get_wrap() const {
    return core_spline.get_wrap();
}

void Spline1DResource::set_default_curve_type(SplineCurveType p_type) {
    core_spline.set_default_curve_type(static_cast<core::CurveType>(p_type));
}

SplineCurveType Spline1DResource::get_default_curve_type() const {
    return static_cast<SplineCurveType>(core_spline.get_default_curve_type());
}

void Spline1DResource::set_default_weight(real_t p_weight) {
    core_spline.set_default_weight(p_weight);
}

real_t Spline1DResource::get_default_weight() const {
    return core_spline.get_default_weight();
}

real_t Spline1DResource::get_total_weight() const {
    return core_spline.get_total_weight();
}

real_t Spline1DResource::get_total_spline_length(real_t p_sample_percent, bool p_force_recalculate) const {
    return core_spline.get_total_spline_length(p_sample_percent, p_force_recalculate);
}

void Spline1DResource::remove_point(int p_index) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.remove_point(static_cast<size_t>(p_index));
    emit_signal("structure_changed");
    emit_changed();
}

void Spline1DResource::clear() {
    if (core_spline.get_point_count() == 0) return;
    core_spline.clear();
    emit_signal("structure_changed");
    emit_changed();
}

void Spline1DResource::set_point_use_back_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_back_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline1DResource::get_point_use_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_back_handle;
}

void Spline1DResource::set_point_use_fore_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_use_fore_handle(static_cast<size_t>(p_index), p_enable);
    emit_changed();
}

bool Spline1DResource::get_point_use_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return false;
    return pts[static_cast<size_t>(p_index)].use_fore_handle;
}

void Spline1DResource::set_section_curve_type(int p_section_index, SplineCurveType p_type) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_curve_type(
        static_cast<size_t>(p_section_index),
        static_cast<core::CurveType>(p_type)
    );
    emit_changed();
}

SplineCurveType Spline1DResource::get_section_curve_type(int p_section_index) const {
    return static_cast<SplineCurveType>(
        core_spline.get_section_curve_type(static_cast<size_t>(p_section_index))
    );
}

void Spline1DResource::set_section_weight(int p_section_index, real_t p_weight) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    core_spline.set_section_weight(static_cast<size_t>(p_section_index), p_weight);
    emit_signal("section_weights_changed");
    emit_changed();
}

real_t Spline1DResource::get_section_weight(int p_section_index) const {
    return core_spline.get_section_weight(static_cast<size_t>(p_section_index));
}

real_t Spline1DResource::get_section_length(int p_section_index) const {
    return core_spline.get_section_length(static_cast<size_t>(p_section_index));
}

real_t Spline1DResource::section_local_to_global_percent(
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

godot::Vector2 Spline1DResource::get_section_at_percent(real_t p_percent, bool p_weighted) const {
    real_t local_t = static_cast<real_t>(0.0);
    const size_t sec_idx = core_spline.get_section_at_percent(p_percent, p_weighted, local_t);
    return godot::Vector2(static_cast<float>(sec_idx), static_cast<float>(local_t));
}

godot::Variant Spline1DResource::get_point_variant(real_t p_percent, bool p_weighted) const {
    return sample(p_percent, p_weighted);
}

godot::Variant Spline1DResource::get_derivative_variant(real_t p_percent, bool p_weighted) const {
    return sample_derivative(p_percent, p_weighted);
}

godot::Variant Spline1DResource::get_tangent_variant(real_t p_percent, bool p_weighted) const {
    return core_spline.get_tangent(p_percent, p_weighted).x;
}

// ============================================================================
// 1D Point & Handle API
// ============================================================================

void Spline1DResource::add_point(real_t p_value, SplineCurveType p_type, real_t p_weight) {
    core_spline.add_point(Vec1(p_value), static_cast<core::CurveType>(p_type), p_weight);
    emit_signal("structure_changed");
    emit_changed();
}

void Spline1DResource::insert_point(int p_index, real_t p_value, SplineCurveType p_type, real_t p_weight) {
    if (p_index < 0) p_index = 0;
    core_spline.insert_point(
        static_cast<size_t>(p_index),
        Vec1(p_value),
        static_cast<core::CurveType>(p_type),
        p_weight
    );
    emit_signal("structure_changed");
    emit_changed();
}

void Spline1DResource::set_point_position(int p_index, real_t p_value) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_position(static_cast<size_t>(p_index), Vec1(p_value));
    emit_changed();
}

real_t Spline1DResource::get_point_position(int p_index) const {
    if (p_index < 0 || p_index >= get_point_count()) return static_cast<real_t>(0.0);
    return core_spline.get_point_position(static_cast<size_t>(p_index)).x;
}

void Spline1DResource::set_point_back_handle(int p_index, real_t p_handle, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_back_handle(static_cast<size_t>(p_index), Vec1(p_handle), p_enable);
    emit_changed();
}

real_t Spline1DResource::get_point_back_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return static_cast<real_t>(0.0);
    return pts[static_cast<size_t>(p_index)].back_handle.x;
}

void Spline1DResource::set_point_fore_handle(int p_index, real_t p_handle, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_fore_handle(static_cast<size_t>(p_index), Vec1(p_handle), p_enable);
    emit_changed();
}

real_t Spline1DResource::get_point_fore_handle(int p_index) const {
    const auto pts = core_spline.get_points();
    if (p_index < 0 || static_cast<size_t>(p_index) >= pts.size()) return static_cast<real_t>(0.0);
    return pts[static_cast<size_t>(p_index)].fore_handle.x;
}

void Spline1DResource::set_point_handles(
    int p_index,
    real_t p_back_handle,
    bool p_use_back,
    real_t p_fore_handle,
    bool p_use_fore
) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    core_spline.set_point_handles(
        static_cast<size_t>(p_index),
        Vec1(p_back_handle),
        p_use_back,
        Vec1(p_fore_handle),
        p_use_fore
    );
    emit_changed();
}

void Spline1DResource::commit_add_point(real_t p_value, SplineCurveType p_type, real_t p_weight) {
    const godot::Dictionary before = get_snapshot_state();
    add_point(p_value, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Add Spline1D Point", before, after);
}

void Spline1DResource::commit_insert_point(int p_index, real_t p_value, SplineCurveType p_type, real_t p_weight) {
    const godot::Dictionary before = get_snapshot_state();
    insert_point(p_index, p_value, p_type, p_weight);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Insert Spline1D Point", before, after);
}

void Spline1DResource::commit_set_point_position(int p_index, real_t p_value) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const real_t prev_val = get_point_position(p_index);
    if (prev_val == p_value) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Move Spline1D Point", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_position").bind(p_index, p_value));
    ur->add_undo_method(godot::Callable(this, "set_point_position").bind(p_index, prev_val));
    ur->commit_action(true);
}

void Spline1DResource::commit_set_point_back_handle(int p_index, real_t p_handle, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const real_t prev_h = get_point_back_handle(p_index);
    const bool prev_en  = get_point_use_back_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline1D Back Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_back_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_back_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

void Spline1DResource::commit_set_point_fore_handle(int p_index, real_t p_handle, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const real_t prev_h = get_point_fore_handle(p_index);
    const bool prev_en  = get_point_use_fore_handle(p_index);
    if (prev_h == p_handle && prev_en == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Spline1D Fore Handle", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, p_handle, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_fore_handle").bind(p_index, prev_h, prev_en));
    ur->commit_action(true);
}

// ============================================================================
// Order-Independent Packed Array Serialization
// ============================================================================

void Spline1DResource::_apply_pending_arrays() {
    const int pt_count  = get_point_count();
    const int sec_count = get_section_count();

    if (pending_back_handles.size() == pt_count) {
        for (int i = 0; i < pt_count; ++i) {
            const bool en = get_point_use_back_handle(i);
            core_spline.set_point_back_handle(static_cast<size_t>(i), Vec1(pending_back_handles[i]), en);
        }
    }
    if (pending_fore_handles.size() == pt_count) {
        for (int i = 0; i < pt_count; ++i) {
            const bool en = get_point_use_fore_handle(i);
            core_spline.set_point_fore_handle(static_cast<size_t>(i), Vec1(pending_fore_handles[i]), en);
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

void Spline1DResource::set_points_array(const godot::PackedFloat32Array &p_points) {
    // Snapshot current auxiliary arrays if counts match
    if (pending_back_handles.is_empty())    pending_back_handles    = get_back_handles_array();
    if (pending_fore_handles.is_empty())    pending_fore_handles    = get_fore_handles_array();
    if (pending_handle_flags.is_empty())    pending_handle_flags    = get_handle_flags_array();
    if (pending_section_types.is_empty())   pending_section_types   = get_section_types_array();
    if (pending_section_weights.is_empty()) pending_section_weights = get_section_weights_array();

    const bool was_wrap = core_spline.get_wrap();
    core_spline.set_wrap(false);
    core_spline.clear();

    for (int i = 0; i < p_points.size(); ++i) {
        core_spline.add_point(Vec1(static_cast<real_t>(p_points[i])), core_spline.get_default_curve_type());
    }
    if (was_wrap) {
        core_spline.set_wrap(true);
    }

    _apply_pending_arrays();
    emit_signal("structure_changed");
    emit_changed();
}

godot::PackedFloat32Array Spline1DResource::get_points_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(pts[i].point.x));
    }
    return out;
}

void Spline1DResource::set_back_handles_array(const godot::PackedFloat32Array &p_handles) {
    pending_back_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_back_handle(i);
        core_spline.set_point_back_handle(static_cast<size_t>(i), Vec1(static_cast<real_t>(p_handles[i])), en);
    }
    emit_changed();
}

godot::PackedFloat32Array Spline1DResource::get_back_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(pts[i].back_handle.x));
    }
    return out;
}

void Spline1DResource::set_fore_handles_array(const godot::PackedFloat32Array &p_handles) {
    pending_fore_handles = p_handles;
    const int count = std::min(static_cast<int>(p_handles.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const bool en = get_point_use_fore_handle(i);
        core_spline.set_point_fore_handle(static_cast<size_t>(i), Vec1(static_cast<real_t>(p_handles[i])), en);
    }
    emit_changed();
}

godot::PackedFloat32Array Spline1DResource::get_fore_handles_array() const {
    const auto pts = core_spline.get_points();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(pts[i].fore_handle.x));
    }
    return out;
}

godot::PackedByteArray Spline1DResource::get_handle_flags_array() const {
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

void Spline1DResource::set_handle_flags_array(const godot::PackedByteArray &p_flags) {
    pending_handle_flags = p_flags;
    const int count = std::min(static_cast<int>(p_flags.size()), get_point_count());
    for (int i = 0; i < count; ++i) {
        const uint8_t fl = p_flags[i];
        core_spline.set_use_back_handle(static_cast<size_t>(i), (fl & HANDLE_USE_BACK) != 0);
        core_spline.set_use_fore_handle(static_cast<size_t>(i), (fl & HANDLE_USE_FORE) != 0);
    }
    emit_changed();
}

godot::PackedByteArray Spline1DResource::get_section_types_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedByteArray out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<uint8_t>(secs[i].curve_type));
    }
    return out;
}

void Spline1DResource::set_section_types_array(const godot::PackedByteArray &p_types) {
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

godot::PackedFloat32Array Spline1DResource::get_section_weights_array() const {
    const auto secs = core_spline.get_sections();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(secs[i].weight));
    }
    return out;
}

void Spline1DResource::set_section_weights_array(const godot::PackedFloat32Array &p_weights) {
    pending_section_weights = p_weights;
    const int count = std::min(static_cast<int>(p_weights.size()), get_section_count());
    for (int i = 0; i < count; ++i) {
        core_spline.set_section_weight(static_cast<size_t>(i), static_cast<real_t>(p_weights[i]));
    }
    emit_signal("section_weights_changed");
    emit_changed();
}

godot::Dictionary Spline1DResource::get_snapshot_state() const {
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

void Spline1DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("default_curve_type")) {
        set_default_curve_type(static_cast<SplineCurveType>(static_cast<int>(p_state["default_curve_type"])));
    }
    if (p_state.has("default_weight")) {
        set_default_weight(static_cast<real_t>(p_state["default_weight"]));
    }

    pending_back_handles    = p_state.get("back_handles", godot::PackedFloat32Array());
    pending_fore_handles    = p_state.get("fore_handles", godot::PackedFloat32Array());
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
// 1D Evaluation, Root-Finding & Solvers
// ============================================================================

real_t Spline1DResource::sample(real_t p_percent, bool p_weighted) const {
    return core_spline.get_point(p_percent, p_weighted).x;
}

real_t Spline1DResource::sample_section(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return static_cast<real_t>(0.0);
    return core_spline.evaluate_section(static_cast<size_t>(p_section_index), p_local_t).x;
}

real_t Spline1DResource::sample_derivative(real_t p_percent, bool p_weighted) const {
    return core_spline.get_derivative(p_percent, p_weighted).x;
}

real_t Spline1DResource::sample_section_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return static_cast<real_t>(0.0);
    return core_spline.evaluate_section_derivative(static_cast<size_t>(p_section_index), p_local_t).x;
}

real_t Spline1DResource::sample_section_second_derivative(int p_section_index, real_t p_local_t) const {
    if (p_section_index < 0) return static_cast<real_t>(0.0);
    return core_spline.evaluate_section_second_derivative(static_cast<size_t>(p_section_index), p_local_t).x;
}

godot::PackedFloat32Array Spline1DResource::get_smoothed_points(int p_max_subdivisions) const {
    const auto pts = core_spline.get_smoothed_points(p_max_subdivisions);
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(pts[i].x));
    }
    return out;
}

godot::PackedFloat32Array Spline1DResource::get_percents_at_value(
    real_t p_value,
    int p_samples,
    bool p_weighted
) const {
    const auto hits = core_spline.get_percents_at_value(p_value, p_samples, p_weighted);
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(hits.size()));
    for (size_t i = 0; i < hits.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(hits[i]));
    }
    return out;
}

real_t Spline1DResource::get_min_value(bool p_use_smoothed) const {
    return core_spline.get_min_value(p_use_smoothed);
}

real_t Spline1DResource::get_max_value(bool p_use_smoothed) const {
    return core_spline.get_max_value(p_use_smoothed);
}

real_t Spline1DResource::get_closest_value_on_spline(real_t p_target_value, int p_samples) const {
    return core_spline.get_closest_point_on_spline(Vec1(p_target_value), p_samples).x;
}

// ============================================================================
// 1D Presets & Static Builders
// ============================================================================

void Spline1DResource::setup_zero_to_one() {
    core_spline = CoreSpline::create_zero_to_one();
    emit_signal("structure_changed");
    emit_changed();
}

void Spline1DResource::setup_one_to_zero() {
    core_spline = CoreSpline::create_one_to_zero();
    emit_signal("structure_changed");
    emit_changed();
}

void Spline1DResource::setup_equidistant(int p_count, SplineCurveType p_type) {
    if (p_count < 2) p_count = 2;
    core_spline = CoreSpline::create_equidistant(
        static_cast<size_t>(p_count),
        static_cast<core::CurveType>(p_type)
    );
    emit_signal("structure_changed");
    emit_changed();
}

godot::Ref<Spline1DResource> Spline1DResource::create_zero_to_one() {
    godot::Ref<Spline1DResource> res;
    res.instantiate();
    res->setup_zero_to_one();
    return res;
}

godot::Ref<Spline1DResource> Spline1DResource::create_one_to_zero() {
    godot::Ref<Spline1DResource> res;
    res.instantiate();
    res->setup_one_to_zero();
    return res;
}

godot::Ref<Spline1DResource> Spline1DResource::create_equidistant(int p_count, SplineCurveType p_type) {
    godot::Ref<Spline1DResource> res;
    res.instantiate();
    res->setup_equidistant(p_count, p_type);
    return res;
}

} // namespace ideam::godot_ext
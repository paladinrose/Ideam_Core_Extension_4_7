#include "spline_resource.h"
#include "ideam_splines_plugin.h"
#include "../geometry/ideam_geometry_plugin.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void SplineResource::_bind_methods() {
    using namespace godot;

    // --- Enums ---
    BIND_ENUM_CONSTANT(CURVE_LINEAR);
    BIND_ENUM_CONSTANT(CURVE_CATMULL_ROM);
    BIND_ENUM_CONSTANT(CURVE_CUBIC_BEZIER);
    BIND_ENUM_CONSTANT(CURVE_STEP);
    BIND_ENUM_CONSTANT(CURVE_NONE);

    BIND_ENUM_CONSTANT(HANDLE_NONE);
    BIND_ENUM_CONSTANT(HANDLE_USE_BACK);
    BIND_ENUM_CONSTANT(HANDLE_USE_FORE);
    BIND_ENUM_CONSTANT(HANDLE_USE_BOTH);

    // --- Signals ---
    ADD_SIGNAL(MethodInfo("structure_changed"));
    ADD_SIGNAL(MethodInfo("section_weights_changed"));

    // --- Dimension & Counts ---
    ClassDB::bind_method(D_METHOD("get_dimension"), &SplineResource::get_dimension);
    ClassDB::bind_method(D_METHOD("get_point_count"), &SplineResource::get_point_count);
    ClassDB::bind_method(D_METHOD("get_length"), &SplineResource::get_length);
    ClassDB::bind_method(D_METHOD("get_section_count"), &SplineResource::get_section_count);

    // --- Spline-Wide Configuration ---
    ADD_GROUP("Spline Settings", "");

    ClassDB::bind_method(D_METHOD("set_wrap", "wrap"), &SplineResource::set_wrap);
    ClassDB::bind_method(D_METHOD("get_wrap"), &SplineResource::get_wrap);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wrap"), "set_wrap", "get_wrap");

    ClassDB::bind_method(D_METHOD("set_default_curve_type", "type"), &SplineResource::set_default_curve_type);
    ClassDB::bind_method(D_METHOD("get_default_curve_type"), &SplineResource::get_default_curve_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "default_curve_type", PROPERTY_HINT_ENUM, "Linear:0,Catmull-Rom:1,Cubic Bezier:2,Step:3"),
        "set_default_curve_type",
        "get_default_curve_type"
    );

    ClassDB::bind_method(D_METHOD("set_default_weight", "weight"), &SplineResource::set_default_weight);
    ClassDB::bind_method(D_METHOD("get_default_weight"), &SplineResource::get_default_weight);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "default_weight", PROPERTY_HINT_RANGE, "0.001,100.0,0.01,or_greater"),
        "set_default_weight",
        "get_default_weight"
    );

    ClassDB::bind_method(D_METHOD("get_total_weight"), &SplineResource::get_total_weight);
    ClassDB::bind_method(
        D_METHOD("get_total_spline_length", "sample_percent", "force_recalculate"),
        &SplineResource::get_total_spline_length,
        DEFVAL(0.05),
        DEFVAL(false)
    );

    // --- Per-Point & Per-Section Methods ---
    ClassDB::bind_method(D_METHOD("remove_point", "index"), &SplineResource::remove_point);
    ClassDB::bind_method(D_METHOD("clear"), &SplineResource::clear);

    ClassDB::bind_method(D_METHOD("set_point_use_back_handle", "index", "enable"), &SplineResource::set_point_use_back_handle);
    ClassDB::bind_method(D_METHOD("get_point_use_back_handle", "index"), &SplineResource::get_point_use_back_handle);

    ClassDB::bind_method(D_METHOD("set_point_use_fore_handle", "index", "enable"), &SplineResource::set_point_use_fore_handle);
    ClassDB::bind_method(D_METHOD("get_point_use_fore_handle", "index"), &SplineResource::get_point_use_fore_handle);

    ClassDB::bind_method(D_METHOD("set_point_handle_flags", "index", "flags"), &SplineResource::set_point_handle_flags);
    ClassDB::bind_method(D_METHOD("get_point_handle_flags", "index"), &SplineResource::get_point_handle_flags);

    ClassDB::bind_method(D_METHOD("set_section_curve_type", "section_index", "type"), &SplineResource::set_section_curve_type);
    ClassDB::bind_method(D_METHOD("get_section_curve_type", "section_index"), &SplineResource::get_section_curve_type);

    ClassDB::bind_method(D_METHOD("set_section_weight", "section_index", "weight"), &SplineResource::set_section_weight);
    ClassDB::bind_method(D_METHOD("get_section_weight", "section_index"), &SplineResource::get_section_weight);

    ClassDB::bind_method(D_METHOD("get_section_length", "section_index"), &SplineResource::get_section_length);
    ClassDB::bind_method(
        D_METHOD("section_local_to_global_percent", "section_index", "local_t", "weighted"),
        &SplineResource::section_local_to_global_percent,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("get_section_at_percent", "percent", "weighted"),
        &SplineResource::get_section_at_percent,
        DEFVAL(true)
    );

    // --- Polymorphic Evaluation & Snapshot Serialization ---
    ClassDB::bind_method(D_METHOD("get_point_variant", "percent", "weighted"), &SplineResource::get_point_variant, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_derivative_variant", "percent", "weighted"), &SplineResource::get_derivative_variant, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_tangent_variant", "percent", "weighted"), &SplineResource::get_tangent_variant, DEFVAL(true));

    ADD_GROUP("Serialized Arrays", "");

    ClassDB::bind_method(D_METHOD("set_handle_flags_array", "flags"), &SplineResource::set_handle_flags_array);
    ClassDB::bind_method(D_METHOD("get_handle_flags_array"), &SplineResource::get_handle_flags_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_BYTE_ARRAY, "handle_flags", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_handle_flags_array",
        "get_handle_flags_array"
    );

    ClassDB::bind_method(D_METHOD("set_section_types_array", "types"), &SplineResource::set_section_types_array);
    ClassDB::bind_method(D_METHOD("get_section_types_array"), &SplineResource::get_section_types_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_BYTE_ARRAY, "section_types", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_section_types_array",
        "get_section_types_array"
    );

    ClassDB::bind_method(D_METHOD("set_section_weights_array", "weights"), &SplineResource::set_section_weights_array);
    ClassDB::bind_method(D_METHOD("get_section_weights_array"), &SplineResource::get_section_weights_array);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "section_weights", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
        "set_section_weights_array",
        "get_section_weights_array"
    );

    // --- Shared Undo/Redo Commit Actions ---
    ClassDB::bind_method(D_METHOD("commit_set_wrap", "wrap"), &SplineResource::commit_set_wrap);
    ClassDB::bind_method(D_METHOD("commit_set_section_curve_type", "section_index", "type"), &SplineResource::commit_set_section_curve_type);
    ClassDB::bind_method(D_METHOD("commit_set_section_weight", "section_index", "weight"), &SplineResource::commit_set_section_weight);
    ClassDB::bind_method(D_METHOD("commit_set_point_use_back_handle", "index", "enable"), &SplineResource::commit_set_point_use_back_handle);
    ClassDB::bind_method(D_METHOD("commit_set_point_use_fore_handle", "index", "enable"), &SplineResource::commit_set_point_use_fore_handle);
    ClassDB::bind_method(D_METHOD("commit_remove_point", "index"), &SplineResource::commit_remove_point);
    ClassDB::bind_method(D_METHOD("commit_clear"), &SplineResource::commit_clear);
}

SplineResource::SplineResource() = default;

SplineResource::~SplineResource() = default;

godot::Ref<IdeamUndoRedo> SplineResource::_resolve_undo_redo() const {
    if (undo_redo.is_null()) {
        undo_redo.instantiate();
    }
#ifdef TOOLS_ENABLED
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        if (godot::EditorUndoRedoManager *sp_ur = IdeamSplinesPlugin::get_static_undo_redo()) {
            undo_redo->set_editor_undo_redo(sp_ur);
        } else if (godot::EditorUndoRedoManager *geom_ur = IdeamGeometryPlugin::get_static_undo_redo()) {
            undo_redo->set_editor_undo_redo(geom_ur);
        }
    }
#endif
    return undo_redo;
}

void SplineResource::commit_set_wrap(bool p_wrap) {
    const bool prev_wrap = get_wrap();
    if (prev_wrap == p_wrap) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Toggle Spline Wrap", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_wrap").bind(p_wrap));
    ur->add_undo_method(godot::Callable(this, "set_wrap").bind(prev_wrap));
    ur->commit_action(true);
}

void SplineResource::commit_set_section_curve_type(int p_section_index, SplineCurveType p_type) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    const SplineCurveType prev_type = get_section_curve_type(p_section_index);
    if (prev_type == p_type) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Set Spline Section Curve Type", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_section_curve_type").bind(p_section_index, static_cast<int>(p_type)));
    ur->add_undo_method(godot::Callable(this, "set_section_curve_type").bind(p_section_index, static_cast<int>(prev_type)));
    ur->commit_action(true);
}

void SplineResource::commit_set_section_weight(int p_section_index, real_t p_weight) {
    if (p_section_index < 0 || p_section_index >= get_section_count()) return;
    const real_t prev_weight = get_section_weight(p_section_index);
    if (prev_weight == p_weight) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Set Spline Section Weight", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_section_weight").bind(p_section_index, p_weight));
    ur->add_undo_method(godot::Callable(this, "set_section_weight").bind(p_section_index, prev_weight));
    ur->commit_action(true);
}

void SplineResource::commit_set_point_use_back_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const bool prev = get_point_use_back_handle(p_index);
    if (prev == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Toggle Spline Back Handle", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_point_use_back_handle").bind(p_index, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_use_back_handle").bind(p_index, prev));
    ur->commit_action(true);
}

void SplineResource::commit_set_point_use_fore_handle(int p_index, bool p_enable) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const bool prev = get_point_use_fore_handle(p_index);
    if (prev == p_enable) return;

    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Toggle Spline Fore Handle", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_point_use_fore_handle").bind(p_index, p_enable));
    ur->add_undo_method(godot::Callable(this, "set_point_use_fore_handle").bind(p_index, prev));
    ur->commit_action(true);
}

void SplineResource::commit_remove_point(int p_index) {
    if (p_index < 0 || p_index >= get_point_count()) return;
    const godot::Dictionary before = get_snapshot_state();
    remove_point(p_index);
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Remove Spline Point", before, after);
}

void SplineResource::commit_clear() {
    if (get_point_count() == 0) return;
    const godot::Dictionary before = get_snapshot_state();
    clear();
    const godot::Dictionary after = get_snapshot_state();
    commit_snapshot_action("Clear Spline", before, after);
}

} // namespace ideam::godot_ext
#include "geometry_resource.h"
#include "ideam_geometry_plugin.h"
#include "../splines/ideam_splines_plugin.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void GeometryResource::_bind_methods() {
    using namespace godot;

    // --- Winding Order Enum Constants ---
    BIND_ENUM_CONSTANT(WINDING_CLOCKWISE);
    BIND_ENUM_CONSTANT(WINDING_COLINEAR);
    BIND_ENUM_CONSTANT(WINDING_COUNTER_CLOCKWISE);

    // --- Point Containment Enum Constants ---
    BIND_ENUM_CONSTANT(CONTAINMENT_OUTSIDE);
    BIND_ENUM_CONSTANT(CONTAINMENT_INSIDE);
    BIND_ENUM_CONSTANT(CONTAINMENT_ON_BOUNDARY);

    // --- Polygon Offset Join Style Enum Constants ---
    BIND_ENUM_CONSTANT(JOIN_STYLE_MITER);
    BIND_ENUM_CONSTANT(JOIN_STYLE_BEVEL);
    BIND_ENUM_CONSTANT(JOIN_STYLE_ROUND);

    // --- CSG Operation Enum Constants ---
    BIND_ENUM_CONSTANT(CSG_OP_UNION);
    BIND_ENUM_CONSTANT(CSG_OP_INTERSECTION);
    BIND_ENUM_CONSTANT(CSG_OP_DIFFERENCE);
    BIND_ENUM_CONSTANT(CSG_OP_XOR);

    // --- Clip Classification Enum Constants ---
    BIND_ENUM_CONSTANT(CLIP_CLASS_FULLY_OUTSIDE);
    BIND_ENUM_CONSTANT(CLIP_CLASS_FULLY_INSIDE);
    BIND_ENUM_CONSTANT(CLIP_CLASS_STRADDLING_BOUNDARY);

    // --- SDF Combine Mode Enum Constants ---
    BIND_ENUM_CONSTANT(SDF_COMBINE_REPLACE);
    BIND_ENUM_CONSTANT(SDF_COMBINE_UNION);
    BIND_ENUM_CONSTANT(SDF_COMBINE_SUBTRACT);
    BIND_ENUM_CONSTANT(SDF_COMBINE_INTERSECT);
    BIND_ENUM_CONSTANT(SDF_COMBINE_SMOOTH_UNION);
    BIND_ENUM_CONSTANT(SDF_COMBINE_SMOOTH_SUBTRACT);
    BIND_ENUM_CONSTANT(SDF_COMBINE_SMOOTH_INTERSECT);

    // --- Signals ---
    ADD_SIGNAL(MethodInfo("geometry_changed"));

    // --- Undo/Redo Injection Property (Hidden from Inspector UI & .tres serialization) ---
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &GeometryResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &GeometryResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    // --- Polymorphic Queries & Snapshot Serialization ---
    ClassDB::bind_method(D_METHOD("get_geometry_dimension"), &GeometryResource::get_geometry_dimension);
    ClassDB::bind_method(D_METHOD("get_bounds_2d"), &GeometryResource::get_bounds_2d);
    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &GeometryResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &GeometryResource::apply_snapshot_state);
    ClassDB::bind_method(
        D_METHOD("commit_snapshot_action", "action_name", "before_state", "after_state"),
        &GeometryResource::commit_snapshot_action
    );
}

GeometryResource::GeometryResource() {
    undo_redo.instantiate();
}

GeometryResource::~GeometryResource() = default;

void GeometryResource::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    if (p_undo_redo.is_valid()) {
        undo_redo = p_undo_redo;
    }
}

godot::Ref<IdeamUndoRedo> GeometryResource::get_undo_redo() const {
    return _resolve_undo_redo();
}

godot::Ref<IdeamUndoRedo> GeometryResource::_resolve_undo_redo() const {
    if (undo_redo.is_null()) {
        undo_redo.instantiate();
    }
#ifdef TOOLS_ENABLED
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        if (godot::EditorUndoRedoManager *ed_ur = IdeamGeometryPlugin::get_static_undo_redo()) {
            undo_redo->set_editor_undo_redo(ed_ur);
        } else if (godot::EditorUndoRedoManager *sp_ur = IdeamSplinesPlugin::get_static_undo_redo()) {
            undo_redo->set_editor_undo_redo(sp_ur);
        }
    }
#endif
    return undo_redo;
}

void GeometryResource::commit_snapshot_action(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state,
    const godot::Dictionary &p_after_state
) {
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action(p_action_name, godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "apply_snapshot_state").bind(p_after_state));
    ur->add_undo_method(godot::Callable(this, "apply_snapshot_state").bind(p_before_state));
    ur->commit_action(true);
}

} // namespace ideam::godot_ext

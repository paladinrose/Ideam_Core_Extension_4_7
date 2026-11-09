#include "tiling_resource.h"

#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void TilingResource::_bind_methods() {
    using namespace godot;

    // --- Wallpaper Group Enum Constants (17 Groups) ---
    BIND_ENUM_CONSTANT(WALLPAPER_P1);
    BIND_ENUM_CONSTANT(WALLPAPER_P2);
    BIND_ENUM_CONSTANT(WALLPAPER_PM);
    BIND_ENUM_CONSTANT(WALLPAPER_PG);
    BIND_ENUM_CONSTANT(WALLPAPER_CM);
    BIND_ENUM_CONSTANT(WALLPAPER_PMM);
    BIND_ENUM_CONSTANT(WALLPAPER_PMG);
    BIND_ENUM_CONSTANT(WALLPAPER_PGG);
    BIND_ENUM_CONSTANT(WALLPAPER_CMM);
    BIND_ENUM_CONSTANT(WALLPAPER_P4);
    BIND_ENUM_CONSTANT(WALLPAPER_P4M);
    BIND_ENUM_CONSTANT(WALLPAPER_P4G);
    BIND_ENUM_CONSTANT(WALLPAPER_P3);
    BIND_ENUM_CONSTANT(WALLPAPER_P3M1);
    BIND_ENUM_CONSTANT(WALLPAPER_P31M);
    BIND_ENUM_CONSTANT(WALLPAPER_P6);
    BIND_ENUM_CONSTANT(WALLPAPER_P6M);

    // --- Bravais Lattice Family Constants ---
    BIND_ENUM_CONSTANT(LATTICE_OBLIQUE);
    BIND_ENUM_CONSTANT(LATTICE_RECTANGULAR);
    BIND_ENUM_CONSTANT(LATTICE_RHOMBIC);
    BIND_ENUM_CONSTANT(LATTICE_SQUARE);
    BIND_ENUM_CONSTANT(LATTICE_HEXAGONAL);

    // --- Authoring Mode & Preset Constants ---
    BIND_ENUM_CONSTANT(TILING_MODE_WALLPAPER_17);
    BIND_ENUM_CONSTANT(TILING_MODE_ISOHEDRAL_81);

    BIND_ENUM_CONSTANT(SHAPE_PRESET_TACTILE_DEFAULT);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_QUAD);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_BRICK_HEX);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_RIGHT_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_EQUILATERAL_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_FLAT_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_TALL_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_RHOMBUS_60);

    // --- Tactile Edge Shape Constants (J, U, S, I) ---
    BIND_ENUM_CONSTANT(EDGE_SHAPE_J);
    BIND_ENUM_CONSTANT(EDGE_SHAPE_U);
    BIND_ENUM_CONSTANT(EDGE_SHAPE_S);
    BIND_ENUM_CONSTANT(EDGE_SHAPE_I);

    // --- Coverage, Stitch & Port Direction Constants ---
    BIND_ENUM_CONSTANT(COVERAGE_STRICT_INTERIOR);
    BIND_ENUM_CONSTANT(COVERAGE_CENTROID_INSIDE);
    BIND_ENUM_CONSTANT(COVERAGE_INCLUDE_STRADDLING);
    BIND_ENUM_CONSTANT(COVERAGE_CLIP_EXACT_TO_SPLINE);

    BIND_ENUM_CONSTANT(STITCH_QUAD_BRIDGE);
    BIND_ENUM_CONSTANT(STITCH_TOPOLOGICAL_SEAM_WELD);
    BIND_ENUM_CONSTANT(STITCH_CURVILINEAR_METRIC_MORPH);

    BIND_ENUM_CONSTANT(PORT_DIR_FREE);
    BIND_ENUM_CONSTANT(PORT_DIR_IN_ONLY);
    BIND_ENUM_CONSTANT(PORT_DIR_OUT_ONLY);
    BIND_ENUM_CONSTANT(PORT_DIR_CLOSED);

    // --- Signals ---
    ADD_SIGNAL(MethodInfo("topology_changed"));
    ADD_SIGNAL(MethodInfo("prototile_changed"));

    // --- Undo/Redo Injection Property (Hidden from Inspector & .tres serialization) ---
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &TilingResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &TilingResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    // --- Shared Polymorphic Methods ---
    ClassDB::bind_method(D_METHOD("get_vertex_count"), &TilingResource::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_aspect_count"), &TilingResource::get_aspect_count);
    ClassDB::bind_method(D_METHOD("get_edge_shape_count"), &TilingResource::get_edge_shape_count);
    ClassDB::bind_method(
        D_METHOD("get_prototile_outline", "subdivisions"),
        &TilingResource::get_prototile_outline,
        DEFVAL(12)
    );
    ClassDB::bind_method(
        D_METHOD("get_prototile_triangles", "subdivisions"),
        &TilingResource::get_prototile_triangles,
        DEFVAL(12)
    );

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &TilingResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &TilingResource::apply_snapshot_state);
    ClassDB::bind_method(
        D_METHOD("commit_snapshot_action", "action_name", "before_state", "after_state"),
        &TilingResource::commit_snapshot_action
    );
}

TilingResource::TilingResource() {
    undo_redo.instantiate();
}

TilingResource::~TilingResource() = default;

void TilingResource::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    if (p_undo_redo.is_valid()) {
        undo_redo = p_undo_redo;
    }
}

godot::Ref<IdeamUndoRedo> TilingResource::get_undo_redo() const {
    return _resolve_undo_redo();
}

godot::Ref<IdeamUndoRedo> TilingResource::_resolve_undo_redo() const {
    if (undo_redo.is_null()) {
        undo_redo.instantiate();
    }
    return undo_redo;
}

void TilingResource::commit_snapshot_action(
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
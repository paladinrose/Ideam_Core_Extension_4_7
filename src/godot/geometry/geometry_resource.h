#pragma once

/**
 * geometry_resource.h
 * Abstract Godot Resource base class for all geometric data objects in the Ideam ecosystem
 * (`src/godot/geometry/`).
 *
 * Serves as the shared foundation for:
 *   1. Pure geometric resources (`Polygon2DResource`, `VoronoiDiagram2DResource`, `SDFGrid2DResource`).
 *   2. Parametric curve resources (`SplineResource` -> `Spline1DResource`..`Spline4DResource`).
 *   3. Crystallographic & isohedral tiling resources (`TilingResource` -> `WallpaperTilingResource`,
 *      `CompositeTilingResource`).
 *
 * Centralizes:
 *   - Godot-exposed enums mapped 1:1 to `ideam::core` geometry primitives (`GeometryWindingOrder`,
 *     `GeometryPointContainment`, `GeometryJoinStyle2D`, `GeometryCSGOperation2D`,
 *     `GeometryClipClassification2D`, `GeometrySDFCombineMode`).
 *   - Pre-instantiated `mutable godot::Ref<IdeamUndoRedo> undo_redo` property (hidden from the
 *     Inspector UI and `.tres` serialization via `PROPERTY_USAGE_NONE`) so `IdeamEditorInspectorPlugin`
 *     automatically injects `EditorUndoRedoManager` when inspected in the Godot Editor, while falling
 *     back cleanly to runtime `UndoRedo` in-game.
 *   - Unified snapshot dictionary serialization (`get_snapshot_state`, `apply_snapshot_state`) and
 *     `commit_snapshot_action` for single-step Undo/Redo of continuous canvas/gizmo drags.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../../core/geometry/csg_2d.h"
#include "../../core/geometry/geometry_common.h"
#include "../../core/geometry/polygon_math_2d.h"
#include "../utilities/ideam_undo_redo.h"

namespace ideam::godot_ext {

/**
 * Godot-facing enum for 2D polygon winding order (1:1 with `core::WindingOrder`).
 */
enum GeometryWindingOrder : int32_t {
    WINDING_CLOCKWISE         = static_cast<int32_t>(core::WindingOrder::CLOCKWISE),
    WINDING_COLINEAR          = static_cast<int32_t>(core::WindingOrder::COLINEAR),
    WINDING_COUNTER_CLOCKWISE = static_cast<int32_t>(core::WindingOrder::COUNTER_CLOCKWISE)
};

/**
 * Godot-facing enum for 2D point containment classification (1:1 with `core::PointContainment`).
 */
enum GeometryPointContainment : int32_t {
    CONTAINMENT_OUTSIDE     = static_cast<int32_t>(core::PointContainment::OUTSIDE),
    CONTAINMENT_INSIDE      = static_cast<int32_t>(core::PointContainment::INSIDE),
    CONTAINMENT_ON_BOUNDARY = static_cast<int32_t>(core::PointContainment::ON_BOUNDARY)
};

/**
 * Godot-facing enum for 2D polygon offset corner join styles (1:1 with `core::JoinStyle2D`).
 */
enum GeometryJoinStyle2D : int32_t {
    JOIN_STYLE_MITER = static_cast<int32_t>(core::JoinStyle2D::MITER),
    JOIN_STYLE_BEVEL = static_cast<int32_t>(core::JoinStyle2D::BEVEL),
    JOIN_STYLE_ROUND = static_cast<int32_t>(core::JoinStyle2D::ROUND)
};

/**
 * Godot-facing enum for 2D boolean CSG operations (1:1 with `core::CSGOperation2D`).
 */
enum GeometryCSGOperation2D : int32_t {
    CSG_OP_UNION        = static_cast<int32_t>(core::CSGOperation2D::UNION),
    CSG_OP_INTERSECTION = static_cast<int32_t>(core::CSGOperation2D::INTERSECTION),
    CSG_OP_DIFFERENCE   = static_cast<int32_t>(core::CSGOperation2D::DIFFERENCE),
    CSG_OP_XOR          = static_cast<int32_t>(core::CSGOperation2D::XOR)
};

/**
 * Godot-facing enum for Wave 2 cell/polygon boundary classification (1:1 with `core::ClipClassification2D`).
 */
enum GeometryClipClassification2D : int32_t {
    CLIP_CLASS_FULLY_OUTSIDE       = static_cast<int32_t>(core::ClipClassification2D::FULLY_OUTSIDE),
    CLIP_CLASS_FULLY_INSIDE        = static_cast<int32_t>(core::ClipClassification2D::FULLY_INSIDE),
    CLIP_CLASS_STRADDLING_BOUNDARY = static_cast<int32_t>(core::ClipClassification2D::STRADDLING_BOUNDARY)
};

/**
 * Godot-facing enum for sharp and smooth SDF boolean combinators in `SDFGrid2DResource`.
 */
enum GeometrySDFCombineMode : int32_t {
    SDF_COMBINE_REPLACE          = 0,
    SDF_COMBINE_UNION            = 1,
    SDF_COMBINE_SUBTRACT         = 2,
    SDF_COMBINE_INTERSECT        = 3,
    SDF_COMBINE_SMOOTH_UNION     = 4,
    SDF_COMBINE_SMOOTH_SUBTRACT  = 5,
    SDF_COMBINE_SMOOTH_INTERSECT = 6
};

class GeometryResource : public godot::Resource {
    GDCLASS(GeometryResource, godot::Resource)

protected:
    static void _bind_methods();

    // Pre-instantiated so IdeamEditorInspectorPlugin::_parse_begin always sees a non-NIL
    // `undo_redo` property and injects EditorUndoRedoManager when inspected in the Editor.
    mutable godot::Ref<IdeamUndoRedo> undo_redo;

    /**
     * Ensures `undo_redo` is valid and (when running in the Godot Editor) bound to the active
     * EditorUndoRedoManager from `IdeamGeometryPlugin` or `IdeamSplinesPlugin`.
     */
    virtual godot::Ref<IdeamUndoRedo> _resolve_undo_redo() const;

public:
    GeometryResource();
    ~GeometryResource() override;

    // ========================================================================
    // Undo/Redo Injection & Management
    // ========================================================================

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const;

    // ========================================================================
    // Polymorphic Geometry Queries & Snapshot Serialization
    // ========================================================================

    /**
     * Returns the primary spatial dimension of this geometry resource (1, 2, 3, or 4).
     */
    [[nodiscard]] virtual int get_geometry_dimension() const { return 2; }

    /**
     * Returns the 2D axis-aligned bounding box of this geometry resource in local coordinates.
     */
    [[nodiscard]] virtual godot::Rect2 get_bounds_2d() const { return godot::Rect2(); }

    /**
     * Captures or restores the complete serialized state of the resource as a Dictionary.
     * Used by `GeometryCanvas2DBase`, `SplineCanvasBase`, and 3D gizmos to commit continuous
     * mouse drags as a single atomic Undo/Redo step.
     */
    [[nodiscard]] virtual godot::Dictionary get_snapshot_state() const { return {}; }
    virtual void apply_snapshot_state(const godot::Dictionary &p_state) {}

    /**
     * Records a single atomic Undo/Redo transaction restoring `p_before_state` on undo and
     * `p_after_state` on redo.
     */
    void commit_snapshot_action(
        const godot::String &p_action_name,
        const godot::Dictionary &p_before_state,
        const godot::Dictionary &p_after_state
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::GeometryWindingOrder);
VARIANT_ENUM_CAST(ideam::godot_ext::GeometryPointContainment);
VARIANT_ENUM_CAST(ideam::godot_ext::GeometryJoinStyle2D);
VARIANT_ENUM_CAST(ideam::godot_ext::GeometryCSGOperation2D);
VARIANT_ENUM_CAST(ideam::godot_ext::GeometryClipClassification2D);
VARIANT_ENUM_CAST(ideam::godot_ext::GeometrySDFCombineMode);

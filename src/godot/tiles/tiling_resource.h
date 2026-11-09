#pragma once

/**
 * tiling_resource.h
 * Abstract Godot Resource base class shared across `WallpaperTilingResource` and
 * `CompositeTilingResource` (`src/godot/tiles/`).
 *
 * Mirrors the exact architectural conventions of `SplineResource` (`src/godot/splines/spline_resource.h`):
 *   1. Godot-exposed enums (`WallpaperGroupType`, `TilingAuthoringModeType`, `TilingShapePresetType`,
 *      `TilingEdgeShapeType`, `TilingCoverageModeType`, `TilingStitchModeType`, `TilingPortDirectionType`)
 *      mapped 1:1 to `ideam::core` enums in `src/core/geometry/symmetry_2d.h` and `src/core/tiles/tile_common.h`.
 *   2. Pre-instantiated `mutable godot::Ref<IdeamUndoRedo> undo_redo` property so
 *      `IdeamEditorInspectorPlugin::_parse_begin` automatically injects `EditorUndoRedoManager`
 *      when inspected in the Godot Editor, while falling back cleanly to runtime `UndoRedo` in-game.
 *   3. Snapshot dictionary serialization (`get_snapshot_state`, `apply_snapshot_state`) and
 *      `commit_snapshot_action` for single-step Undo/Redo of continuous canvas/gizmo drags.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../../core/geometry/symmetry_2d.h"
#include "../../core/tiles/tile_common.h"
#include "../utilities/ideam_undo_redo.h"

namespace ideam::godot_ext {

/**
 * Godot-facing enum for the 17 Euclidean Wallpaper Groups (1:1 with `core::WallpaperGroupId`).
 */
enum WallpaperGroupType : int32_t {
    WALLPAPER_P1   = static_cast<int32_t>(core::WallpaperGroupId::P1),
    WALLPAPER_P2   = static_cast<int32_t>(core::WallpaperGroupId::P2),
    WALLPAPER_PM   = static_cast<int32_t>(core::WallpaperGroupId::PM),
    WALLPAPER_PG   = static_cast<int32_t>(core::WallpaperGroupId::PG),
    WALLPAPER_CM   = static_cast<int32_t>(core::WallpaperGroupId::CM),
    WALLPAPER_PMM  = static_cast<int32_t>(core::WallpaperGroupId::PMM),
    WALLPAPER_PMG  = static_cast<int32_t>(core::WallpaperGroupId::PMG),
    WALLPAPER_PGG  = static_cast<int32_t>(core::WallpaperGroupId::PGG),
    WALLPAPER_CMM  = static_cast<int32_t>(core::WallpaperGroupId::CMM),
    WALLPAPER_P4   = static_cast<int32_t>(core::WallpaperGroupId::P4),
    WALLPAPER_P4M  = static_cast<int32_t>(core::WallpaperGroupId::P4M),
    WALLPAPER_P4G  = static_cast<int32_t>(core::WallpaperGroupId::P4G),
    WALLPAPER_P3   = static_cast<int32_t>(core::WallpaperGroupId::P3),
    WALLPAPER_P3M1 = static_cast<int32_t>(core::WallpaperGroupId::P3M1),
    WALLPAPER_P31M = static_cast<int32_t>(core::WallpaperGroupId::P31M),
    WALLPAPER_P6   = static_cast<int32_t>(core::WallpaperGroupId::P6),
    WALLPAPER_P6M  = static_cast<int32_t>(core::WallpaperGroupId::P6M)
};

/**
 * Godot-facing enum for the 5 Bravais Lattice families (1:1 with `core::LatticeType2D`).
 */
enum TilingLatticeFamily : int32_t {
    LATTICE_OBLIQUE     = static_cast<int32_t>(core::LatticeType2D::OBLIQUE),
    LATTICE_RECTANGULAR = static_cast<int32_t>(core::LatticeType2D::RECTANGULAR),
    LATTICE_RHOMBIC     = static_cast<int32_t>(core::LatticeType2D::RHOMBIC),
    LATTICE_SQUARE      = static_cast<int32_t>(core::LatticeType2D::SQUARE),
    LATTICE_HEXAGONAL   = static_cast<int32_t>(core::LatticeType2D::HEXAGONAL)
};

/**
 * Godot-facing enum for `core::TilingAuthoringMode`.
 */
enum TilingAuthoringModeType : int32_t {
    TILING_MODE_WALLPAPER_17 = static_cast<int32_t>(core::TilingAuthoringMode::WALLPAPER_GROUP_17),
    TILING_MODE_ISOHEDRAL_81 = static_cast<int32_t>(core::TilingAuthoringMode::ISOHEDRAL_TACTILE_81)
};

/**
 * Godot-facing enum for `core::FundamentalShapePreset` (`old_unity_code/Topology/Topology.cs`).
 */
enum TilingShapePresetType : int32_t {
    SHAPE_PRESET_TACTILE_DEFAULT      = static_cast<int32_t>(core::FundamentalShapePreset::TACTILE_DEFAULT),
    SHAPE_PRESET_QUAD                 = static_cast<int32_t>(core::FundamentalShapePreset::QUAD),
    SHAPE_PRESET_BRICK_HEX            = static_cast<int32_t>(core::FundamentalShapePreset::BRICK_HEX),
    SHAPE_PRESET_RIGHT_TRIANGLE       = static_cast<int32_t>(core::FundamentalShapePreset::RIGHT_TRIANGLE),
    SHAPE_PRESET_EQUILATERAL_TRIANGLE = static_cast<int32_t>(core::FundamentalShapePreset::EQUILATERAL_TRIANGLE),
    SHAPE_PRESET_FLAT_TRIANGLE        = static_cast<int32_t>(core::FundamentalShapePreset::FLAT_TRIANGLE),
    SHAPE_PRESET_TALL_TRIANGLE        = static_cast<int32_t>(core::FundamentalShapePreset::TALL_TRIANGLE),
    SHAPE_PRESET_RHOMBUS_60           = static_cast<int32_t>(core::FundamentalShapePreset::RHOMBUS_60)
};

/**
 * Godot-facing enum for `core::TactileEdgeShape` (`J`, `U`, `S`, `I`).
 */
enum TilingEdgeShapeType : int32_t {
    EDGE_SHAPE_J = static_cast<int32_t>(core::TactileEdgeShape::EDGE_J),
    EDGE_SHAPE_U = static_cast<int32_t>(core::TactileEdgeShape::EDGE_U),
    EDGE_SHAPE_S = static_cast<int32_t>(core::TactileEdgeShape::EDGE_S),
    EDGE_SHAPE_I = static_cast<int32_t>(core::TactileEdgeShape::EDGE_I)
};

/**
 * Godot-facing enum for `core::TilingCoverageMode`.
 */
enum TilingCoverageModeType : int32_t {
    COVERAGE_STRICT_INTERIOR      = static_cast<int32_t>(core::TilingCoverageMode::STRICT_INTERIOR),
    COVERAGE_CENTROID_INSIDE      = static_cast<int32_t>(core::TilingCoverageMode::CENTROID_INSIDE),
    COVERAGE_INCLUDE_STRADDLING   = static_cast<int32_t>(core::TilingCoverageMode::INCLUDE_STRADDLING),
    COVERAGE_CLIP_EXACT_TO_SPLINE = static_cast<int32_t>(core::TilingCoverageMode::CLIP_EXACT_TO_SPLINE)
};

/**
 * Godot-facing enum for `core::TilingStitchMode`.
 */
enum TilingStitchModeType : int32_t {
    STITCH_QUAD_BRIDGE              = static_cast<int32_t>(core::TilingStitchMode::QUAD_BRIDGE),
    STITCH_TOPOLOGICAL_SEAM_WELD    = static_cast<int32_t>(core::TilingStitchMode::TOPOLOGICAL_SEAM_WELD),
    STITCH_CURVILINEAR_METRIC_MORPH = static_cast<int32_t>(core::TilingStitchMode::CURVILINEAR_METRIC_MORPH)
};

/**
 * Godot-facing enum for `core::TilePortDirection`.
 */
enum TilingPortDirectionType : int32_t {
    PORT_DIR_FREE     = static_cast<int32_t>(core::TilePortDirection::PORT_FREE),
    PORT_DIR_IN_ONLY  = static_cast<int32_t>(core::TilePortDirection::PORT_IN_ONLY),
    PORT_DIR_OUT_ONLY = static_cast<int32_t>(core::TilePortDirection::PORT_OUT_ONLY),
    PORT_DIR_CLOSED   = static_cast<int32_t>(core::TilePortDirection::PORT_CLOSED)
};

class TilingResource : public godot::Resource {
    GDCLASS(TilingResource, godot::Resource)

protected:
    static void _bind_methods();

    // Pre-instantiated so IdeamEditorInspectorPlugin::_parse_begin detects a non-NIL `undo_redo`
    // property and injects EditorUndoRedoManager when inspected in the Editor.
    mutable godot::Ref<IdeamUndoRedo> undo_redo;

    [[nodiscard]] godot::Ref<IdeamUndoRedo> _resolve_undo_redo() const;

public:
    TilingResource();
    ~TilingResource() override;

    // --- Undo/Redo Injection & Management ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const;

    // --- Shared Polymorphic Tiling Interface ---
    [[nodiscard]] virtual int get_vertex_count() const { return 0; }
    [[nodiscard]] virtual int get_aspect_count() const { return 0; }
    [[nodiscard]] virtual int get_edge_shape_count() const { return 0; }

    [[nodiscard]] virtual godot::PackedVector2Array get_prototile_outline(int p_subdivisions = 12) const {
        (void)p_subdivisions;
        return {};
    }
    [[nodiscard]] virtual godot::PackedInt32Array get_prototile_triangles(int p_subdivisions = 12) const {
        (void)p_subdivisions;
        return {};
    }

    // --- Snapshot State & Undo/Redo Transaction Helpers ---
    [[nodiscard]] virtual godot::Dictionary get_snapshot_state() const { return {}; }
    virtual void apply_snapshot_state(const godot::Dictionary &p_state) { (void)p_state; }

    void commit_snapshot_action(
        const godot::String &p_action_name,
        const godot::Dictionary &p_before_state,
        const godot::Dictionary &p_after_state
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::WallpaperGroupType);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingLatticeFamily);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingAuthoringModeType);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingShapePresetType);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingEdgeShapeType);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingCoverageModeType);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingStitchModeType);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingPortDirectionType);
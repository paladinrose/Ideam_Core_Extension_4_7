#pragma once

/**
 * spline_resource.h
 * Abstract Godot Resource base class shared across Spline1DResource, Spline2DResource,
 * Spline3DResource, and Spline4DResource.
 *
 * Centralizes all dimension-agnostic spline state and tooling protocols:
 *   1. Godot-exposed CurveType (`SplineCurveType`) and handle bitmask (`SplineHandleFlags`) enums.
 *   2. Shared topological queries & mutators (`wrap`, `default_curve_type`, `default_weight`,
 *      per-section `curve_type`, `weight`, and `length`, per-point `use_back_handle` / `use_fore_handle`,
 *      `remove_point`, `clear`, and `get_section_at_percent`).
 *   3. Compact packed-array serialization helpers (`handle_flags`, `section_types`, `section_weights`).
 *   4. Unified `IdeamUndoRedo` integration (pre-instantiated so `IdeamEditorInspectorPlugin::_parse_begin`
 *      always detects a non-NIL `undo_redo` property, with automatic fallback to `IdeamSplinesPlugin`
 *      in the Editor and runtime `UndoRedo` in-game) plus `commit_*` action wrappers.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../../core/splines/spline_base.h"
#include "../utilities/ideam_undo_redo.h"

namespace ideam::godot_ext {

/**
 * Godot-facing enum mirroring `ideam::core::CurveType` (1:1 integer values).
 */
enum SplineCurveType : int32_t {
    CURVE_LINEAR       = static_cast<int32_t>(core::CurveType::LINEAR),
    CURVE_CATMULL_ROM  = static_cast<int32_t>(core::CurveType::CATMULL_ROM),
    CURVE_CUBIC_BEZIER = static_cast<int32_t>(core::CurveType::CUBIC_BEZIER),
    CURVE_STEP         = static_cast<int32_t>(core::CurveType::STEP),
    CURVE_NONE         = static_cast<int32_t>(core::CurveType::NONE)
};

/**
 * Bitmask flags packing `use_back_handle` and `use_fore_handle` into 1 byte per point.
 */
enum SplineHandleFlags : uint8_t {
    HANDLE_NONE     = 0u,
    HANDLE_USE_BACK = 1u << 0,
    HANDLE_USE_FORE = 1u << 1,
    HANDLE_USE_BOTH = HANDLE_USE_BACK | HANDLE_USE_FORE
};

class SplineResource : public godot::Resource {
    GDCLASS(SplineResource, godot::Resource)

protected:
    static void _bind_methods();

    // Pre-instantiated so IdeamEditorInspectorPlugin::_parse_begin sees a non-NIL property
    // and injects EditorUndoRedoManager when inspected in the Editor.
    mutable godot::Ref<IdeamUndoRedo> undo_redo;

    /**
     * Ensures `undo_redo` is valid and (when running in the Godot Editor) bound to
     * `IdeamSplinesPlugin::get_static_undo_redo()` if not already injected.
     */
    godot::Ref<IdeamUndoRedo> _resolve_undo_redo() const;

public:
    SplineResource();
    ~SplineResource() override;

    // ========================================================================
    // Undo/Redo Injection & Management
    // ========================================================================

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    godot::Ref<IdeamUndoRedo> get_undo_redo() const;

    // ========================================================================
    // Dimension-Agnostic Virtual Interface (Implemented by 1D..4D Subclasses)
    // ========================================================================

    [[nodiscard]] virtual int get_dimension() const { return 0; }
    [[nodiscard]] virtual int get_point_count() const { return 0; }
    [[nodiscard]] virtual int get_length() const { return 0; }
    [[nodiscard]] virtual int get_section_count() const { return 0; }

    virtual void set_wrap(bool p_wrap) {}
    [[nodiscard]] virtual bool get_wrap() const { return false; }

    virtual void set_default_curve_type(SplineCurveType p_type) {}
    [[nodiscard]] virtual SplineCurveType get_default_curve_type() const { return CURVE_LINEAR; }

    virtual void set_default_weight(real_t p_weight) {}
    [[nodiscard]] virtual real_t get_default_weight() const { return static_cast<real_t>(1.0); }

    [[nodiscard]] virtual real_t get_total_weight() const { return static_cast<real_t>(0.0); }
    [[nodiscard]] virtual real_t get_total_spline_length(
        real_t p_sample_percent = static_cast<real_t>(0.05),
        bool p_force_recalculate = false
    ) const {
        return static_cast<real_t>(0.0);
    }

    // --- Per-Point Structural & Handle Flag Operations ---
    virtual void remove_point(int p_index) {}
    virtual void clear() {}

    virtual void set_point_use_back_handle(int p_index, bool p_enable) {}
    [[nodiscard]] virtual bool get_point_use_back_handle(int p_index) const { return false; }

    virtual void set_point_use_fore_handle(int p_index, bool p_enable) {}
    [[nodiscard]] virtual bool get_point_use_fore_handle(int p_index) const { return false; }

    virtual void set_point_handle_flags(int p_index, int p_flags) {
        set_point_use_back_handle(p_index, (p_flags & HANDLE_USE_BACK) != 0);
        set_point_use_fore_handle(p_index, (p_flags & HANDLE_USE_FORE) != 0);
    }
    [[nodiscard]] virtual int get_point_handle_flags(int p_index) const {
        int flags = HANDLE_NONE;
        if (get_point_use_back_handle(p_index)) flags |= HANDLE_USE_BACK;
        if (get_point_use_fore_handle(p_index)) flags |= HANDLE_USE_FORE;
        return flags;
    }

    // --- Per-Section Operations ---
    virtual void set_section_curve_type(int p_section_index, SplineCurveType p_type) {}
    [[nodiscard]] virtual SplineCurveType get_section_curve_type(int p_section_index) const { return CURVE_LINEAR; }

    virtual void set_section_weight(int p_section_index, real_t p_weight) {}
    [[nodiscard]] virtual real_t get_section_weight(int p_section_index) const { return static_cast<real_t>(1.0); }

    [[nodiscard]] virtual real_t get_section_length(int p_section_index) const { return static_cast<real_t>(0.0); }

    [[nodiscard]] virtual real_t section_local_to_global_percent(
        int p_section_index,
        real_t p_local_t,
        bool p_weighted = true
    ) const {
        return static_cast<real_t>(0.0);
    }

    /**
     * Resolves global `p_percent` into `Vector2(section_index, local_t)`.
     */
    [[nodiscard]] virtual godot::Vector2 get_section_at_percent(
        real_t p_percent,
        bool p_weighted = true
    ) const {
        return godot::Vector2(0.0f, 0.0f);
    }

    // --- Polymorphic Variant Evaluation & Snapshot Serialization ---
    [[nodiscard]] virtual godot::Variant get_point_variant(real_t p_percent, bool p_weighted = true) const {
        return godot::Variant();
    }
    [[nodiscard]] virtual godot::Variant get_derivative_variant(real_t p_percent, bool p_weighted = true) const {
        return godot::Variant();
    }
    [[nodiscard]] virtual godot::Variant get_tangent_variant(real_t p_percent, bool p_weighted = true) const {
        return godot::Variant();
    }

    [[nodiscard]] virtual godot::PackedByteArray get_handle_flags_array() const { return {}; }
    virtual void set_handle_flags_array(const godot::PackedByteArray &p_flags) {}

    [[nodiscard]] virtual godot::PackedByteArray get_section_types_array() const { return {}; }
    virtual void set_section_types_array(const godot::PackedByteArray &p_types) {}

    [[nodiscard]] virtual godot::PackedFloat32Array get_section_weights_array() const { return {}; }
    virtual void set_section_weights_array(const godot::PackedFloat32Array &p_weights) {}

    /**
     * Captures or restores the complete serialized state of the spline as a Dictionary.
     * Used by undo/redo snapshot transactions (e.g., committing a continuous gizmo/control drag).
     */
    [[nodiscard]] virtual godot::Dictionary get_snapshot_state() const { return {}; }
    virtual void apply_snapshot_state(const godot::Dictionary &p_state) {}

    // ========================================================================
    // Shared Undo/Redo Commit Wrappers
    // ========================================================================

    void commit_set_wrap(bool p_wrap);
    void commit_set_section_curve_type(int p_section_index, SplineCurveType p_type);
    void commit_set_section_weight(int p_section_index, real_t p_weight);
    void commit_set_point_use_back_handle(int p_index, bool p_enable);
    void commit_set_point_use_fore_handle(int p_index, bool p_enable);
    void commit_remove_point(int p_index);
    void commit_clear();
    void commit_snapshot_action(
        const godot::String &p_action_name,
        const godot::Dictionary &p_before_state,
        const godot::Dictionary &p_after_state
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplineCurveType);
VARIANT_ENUM_CAST(ideam::godot_ext::SplineHandleFlags);
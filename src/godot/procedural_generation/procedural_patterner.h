#pragma once

/**
 * procedural_patterner.h
 * `ProceduralPatterner` (`src/godot/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. ROLE:
 *    - Evaluates a cascading `AnalysisStyle` (collection of `AnalysisRule`s) against any
 *      Godot source artifact (`PackedScene` via `SceneState`, live `Node` subtree,
 *      `Resource`, or `RefCounted`) and synthesizes a `ProceduralPattern` containing
 *      cascading `GenerationRule`s.
 *
 * 2. GODOT REFLECTION CAPABILITIES LEVERAGED:
 *    - Inspects `SceneState` (for `PackedScene` without instantiating nodes) or `Object`
 *      reflection (`ClassDB::is_parent_class`, `get_property_list`, `get_method_list`,
 *      `get_signal_list`, node names, groups, and parent/child/sibling topology).
 *    - Emits parent acceptance rules (`CastleGrounds | CastleRuins -> CastleTower`),
 *      child cardinality budgets (`TowerGate [1..2]`, `TowerStairs [0..1]`), and
 *      sibling co-occurrence requirements (`TowerLanding [0..3]` requires `TowerStairs`).
 */

#include "analysis_rule.h"
#include "procedural_pattern.h"
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class ProceduralPatterner : public godot::RefCounted {
    GDCLASS(ProceduralPatterner, godot::RefCounted)

private:
    godot::Ref<AnalysisStyle> analysis_style_;

protected:
    static void _bind_methods();

public:
    ProceduralPatterner() = default;
    ~ProceduralPatterner() override = default;

    void set_analysis_style(const godot::Ref<AnalysisStyle> &p_style) { analysis_style_ = p_style; }
    [[nodiscard]] godot::Ref<AnalysisStyle> get_analysis_style() const { return analysis_style_; }

    [[nodiscard]] godot::Ref<ProceduralPattern> create_pattern_from_packed_scene(
        const godot::Ref<godot::PackedScene> &p_scene
    ) const;

    [[nodiscard]] godot::Ref<ProceduralPattern> create_pattern_from_object(
        godot::Object *p_root_object
    ) const;
};

} // namespace ideam::godot_ext

#pragma once

/**
 * procedural_patterner.h
 * `ProceduralPatterner` (`src/godot/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (Master Plan Section 1.3)
 * ============================================================================
 * 1. ROLE:
 *    - Evaluates a cascading `AnalysisStyle` (collection of `AnalysisRule`s) against any
 *      Godot source artifact (`PackedScene` via `SceneState`, live `Node` subtree,
 *      `Resource`, or `RefCounted`) and synthesizes a `ProceduralPattern` containing
 *      cascading `GenerationRule`s.
 * 2. GODOT REFLECTION HARVESTING:
 *    - `create_pattern_from_packed_scene`: Zero-instantiation `SceneState` inspection
 *      (`get_node_count()`, `get_node_type()`, `get_node_name()`, `get_node_path()`,
 *      `get_node_groups()`, `get_node_property_count()`).
 *    - `create_pattern_from_object`: Live duck-typed reflection (`get_class()`,
 *      `ClassDB::is_parent_class()`, `get_property_list()`, `has_method()`, `has_signal()`,
 *      and attached `SememeResource` / `MetadataLinguisticsManager` tags).
 */

#include "analysis_rule.h"
#include "procedural_pattern.h"
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>

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

    [[nodiscard]] godot::Ref<ProceduralPattern> create_pattern_from_hierarchy_dicts(
        const godot::Array &p_hierarchy_nodes
    ) const;
};

} // namespace ideam::godot_ext

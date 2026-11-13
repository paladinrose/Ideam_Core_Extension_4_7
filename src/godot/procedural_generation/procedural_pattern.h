#pragma once

/**
 * procedural_pattern.h
 * `ProceduralPattern` (`src/godot/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. ROLE:
 *    - Output of `ProceduralPatterner` (and editable directly as a Godot `Resource`).
 *    - Acts as the cascading stylesheet/rubric manager for `GenerationRule`s (paralleling
 *      how `GameplayStyle` manages `GameplayRule`s in `IdeamGames`).
 *
 * 2. MANAGERIAL RESPONSIBILITIES:
 *    - Maintains `GenerationRule`s in CSS cascade precedence order (`!important`,
 *      `PatternCascadeLayer`, `priority`, and selector specificity).
 *    - Compiles the object-level `GenerationRule` tree into flat, cache-aligned
 *      `core::GenerationRulePOD` and `core::TokenDescriptorPOD` buffers inside
 *      `core::ProceduralEnvironment` so downstream `TaskGraphDOD` nodes (WFC,
 *      Quantum Darwinism, etc.) can execute with zero pointer chasing.
 */

#include "procedural_generator_resource.h"
#include "generation_rule.h"
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

class ProceduralPattern : public ProceduralGeneratorResource {
    GDCLASS(ProceduralPattern, ProceduralGeneratorResource)

private:
    godot::TypedArray<GenerationRule> generation_rules_;

protected:
    static void _bind_methods();

public:
    ProceduralPattern() = default;
    ~ProceduralPattern() override = default;

    void set_generation_rules(const godot::TypedArray<GenerationRule> &p_rules);
    [[nodiscard]] godot::TypedArray<GenerationRule> get_generation_rules() const { return generation_rules_; }

    void add_generation_rule(const godot::Ref<GenerationRule> &p_rule);
    void clear_generation_rules();
    [[nodiscard]] int get_generation_rule_count() const { return generation_rules_.size(); }
};

} // namespace ideam::godot_ext

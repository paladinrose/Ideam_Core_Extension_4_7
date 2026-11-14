#pragma once

/**
 * wfc_rule_tensor_resource.h
 * `WFCRuleTensorResource`, `WFCSceneAnalyzer`, `WFCSolverNode`, and `WFCRuleTensorInspector`
 * (`src/godot/wave_function_collapse/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 1.4 — GODOT ENVIRONMENT WFC)
 * ============================================================================
 * Leans on `ProceduralGeneratorResource`, `ProceduralPattern`, `ProceduralPatterner`,
 * `ProceduralGenerationHost`, and `ProceduralGeneratorInspectorBase` from
 * `src/godot/procedural_generation/`, enabling Wave Function Collapse over arbitrary
 * Godot environment shapes (scene hierarchies, gameplay configuration trees, resource
 * compositions, and 2D/3D spatial socket graphs).
 */

#include "../procedural_generation/procedural_generator_resource.h"
#include "../procedural_generation/procedural_pattern.h"
#include "../procedural_generation/procedural_patterner.h"
#include "../procedural_generation/editor/procedural_generator_inspector_base.h"
#include "../../core/wave_function_collapse/wfc_solver_core.h"

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace ideam::godot_ext {

/**
 * Concrete WFC rule tensor & environment-shape resource inheriting from
 * `ProceduralGeneratorResource` (`src/godot/procedural_generation/`).
 */
class WFCRuleTensorResource : public ProceduralGeneratorResource {
    GDCLASS(WFCRuleTensorResource, ProceduralGeneratorResource)

private:
    int state_count_                                  = 4;
    int entropy_heuristic_                            = static_cast<int>(core::WFCEntropyHeuristic::HEURISTIC_SHANNON_WEIGHTED);
    bool enable_hierarchy_and_cardinality_rules_      = true;
    int contradiction_horizon_radius_                 = 4;
    godot::PackedStringArray token_names_;
    godot::PackedFloat32Array token_weights_;
    godot::Ref<ProceduralPattern> source_pattern_;
    std::vector<core::WFCObservedCoOccurrencePod> co_occurrence_rules_;
    std::vector<core::WFCRuleEntryPOD> explicit_rule_entries_;

protected:
    static void _bind_methods();

public:
    WFCRuleTensorResource() = default;
    ~WFCRuleTensorResource() override = default;

    // --- Properties ---
    void set_state_count(int p_count);
    [[nodiscard]] int get_state_count() const noexcept { return state_count_; }

    void set_entropy_heuristic(int p_heuristic);
    [[nodiscard]] int get_entropy_heuristic() const noexcept { return entropy_heuristic_; }

    void set_enable_hierarchy_and_cardinality_rules(bool p_enable);
    [[nodiscard]] bool get_enable_hierarchy_and_cardinality_rules() const noexcept {
        return enable_hierarchy_and_cardinality_rules_;
    }

    void set_contradiction_horizon_radius(int p_radius);
    [[nodiscard]] int get_contradiction_horizon_radius() const noexcept { return contradiction_horizon_radius_; }

    void set_token_names(const godot::PackedStringArray &p_names);
    [[nodiscard]] godot::PackedStringArray get_token_names() const { return token_names_; }

    void set_token_weights(const godot::PackedFloat32Array &p_weights);
    [[nodiscard]] godot::PackedFloat32Array get_token_weights() const { return token_weights_; }

    void set_source_pattern(const godot::Ref<ProceduralPattern> &p_pattern);
    [[nodiscard]] godot::Ref<ProceduralPattern> get_source_pattern() const { return source_pattern_; }

    // --- Co-Occurrence & Rule Tensor Authoring ---
    void clear_observed_rules();
    [[nodiscard]] int get_observed_rule_count() const noexcept {
        return static_cast<int>(co_occurrence_rules_.size());
    }
    void add_observed_co_occurrence(int p_anchor_state, int p_neighbor_state, int p_port_tag);
    void add_observed_hierarchy_rule(
        int p_parent_state,
        int p_child_state,
        int p_min_occurrences,
        int p_max_occurrences,
        int p_required_sibling_state = -1,
        int p_forbidden_sibling_state = -1
    );
    [[nodiscard]] godot::Array get_observed_rules_as_dicts() const;

    void compile_from_procedural_pattern(const godot::Ref<ProceduralPattern> &p_pattern);
    void apply_to_solver_and_environment(
        core::WFCSolverCore &r_solver,
        core::ProceduralEnvironment &r_env,
        int p_slot_count
    ) const;

    // --- Tier 1 `IdeamUndoRedo` Commit Wrappers ---
    void commit_set_state_count(int p_new_count, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_add_observed_co_occurrence(
        int p_anchor_state,
        int p_neighbor_state,
        int p_port_tag,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_clear_observed_rules(const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());

    [[nodiscard]] const std::vector<core::WFCObservedCoOccurrencePod> &co_occurrence_rules() const noexcept {
        return co_occurrence_rules_;
    }
    [[nodiscard]] const std::vector<core::WFCRuleEntryPOD> &explicit_rule_entries() const noexcept {
        return explicit_rule_entries_;
    }
};

/**
 * `WFCSceneAnalyzer` (Section 1.4 & Section 8.3):
 * Inspects `PackedScene::get_state()` (`SceneState`), live Godot `Object` / `Node` / `Resource`
 * trees, or `ProceduralPattern` resources to compile a `WFCRuleTensorResource`.
 */
class WFCSceneAnalyzer : public godot::RefCounted {
    GDCLASS(WFCSceneAnalyzer, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    WFCSceneAnalyzer() = default;
    ~WFCSceneAnalyzer() override = default;

    [[nodiscard]] godot::Ref<WFCRuleTensorResource> extract_rules_from_packed_scene(
        const godot::Ref<godot::PackedScene> &p_scene,
        const godot::Ref<AnalysisStyle> &p_style = godot::Ref<AnalysisStyle>()
    ) const;

    [[nodiscard]] godot::Ref<WFCRuleTensorResource> extract_rules_from_object_hierarchy(
        godot::Object *p_root,
        const godot::Ref<AnalysisStyle> &p_style = godot::Ref<AnalysisStyle>()
    ) const;

    [[nodiscard]] godot::Ref<WFCRuleTensorResource> extract_rules_from_procedural_pattern(
        const godot::Ref<ProceduralPattern> &p_pattern
    ) const;
};

/**
 * `WFCSolverNode`:
 * SceneTree WFC solver node extending `ProceduralGenerationHost` (`src/godot/procedural_generation/`),
 * solving whole-game configurations, scene hierarchies (`CastleGrounds -> CastleTower -> children`),
 * and 2D/3D/hypergraph slot topologies.
 */
class WFCSolverNode : public ProceduralGenerationHost {
    GDCLASS(WFCSolverNode, ProceduralGenerationHost)

private:
    core::ProceduralEnvironment environment_;
    core::WFCSolverCore solver_core_;
    godot::Ref<ProceduralPattern> procedural_pattern_;
    int slot_count_ = 16;

    void ensure_environment_prepared();

protected:
    static void _bind_methods();

public:
    WFCSolverNode();
    ~WFCSolverNode() override = default;

    void set_slot_count(int p_count);
    [[nodiscard]] int get_slot_count() const noexcept { return slot_count_; }

    void set_procedural_pattern(const godot::Ref<ProceduralPattern> &p_pattern);
    [[nodiscard]] godot::Ref<ProceduralPattern> get_procedural_pattern() const { return procedural_pattern_; }

    // --- Topology & Landmark Configuration ---
    void set_slot_children(int p_parent_slot_id, const godot::PackedInt32Array &p_child_slot_ids);
    void add_directed_edge(int p_from_slot, int p_to_slot, int p_port_id, int p_axis = 0, bool p_bidirectional = false);
    void configure_environment_shape(int p_slot_count, const godot::Array &p_slot_hierarchy_dicts);

    void pin_landmark_state(int p_slot_id, int p_state_id);
    void pin_landmark_by_name(int p_slot_id, const godot::String &p_token_name);
    void clear_pinned_landmarks();

    // --- Solving & Introspection ---
    [[nodiscard]] godot::PackedInt32Array run_wfc_collapse();
    [[nodiscard]] int step_wfc_collapse();
    [[nodiscard]] int get_solver_status() const noexcept { return static_cast<int>(solver_core_.status()); }
    [[nodiscard]] godot::String get_collapsed_token_name(int p_slot_id) const;
    [[nodiscard]] float get_slot_entropy(int p_slot_id) const;
    [[nodiscard]] godot::PackedInt32Array get_contradiction_slots() const;
    [[nodiscard]] godot::Array get_contradiction_reports_as_dicts() const;
    [[nodiscard]] godot::Array export_solved_configuration_summary() const;

    [[nodiscard]] core::ProceduralEnvironment &environment() noexcept { return environment_; }
    [[nodiscard]] const core::ProceduralEnvironment &environment() const noexcept { return environment_; }
    [[nodiscard]] core::WFCSolverCore &solver_core() noexcept { return solver_core_; }
    [[nodiscard]] const core::WFCSolverCore &solver_core() const noexcept { return solver_core_; }
};

/**
 * EditorInspectorPlugin for `WFCRuleTensorResource` and `WFCSolverNode`, inheriting from
 * `ProceduralGeneratorInspectorBase` (`src/godot/procedural_generation/editor/`).
 */
class WFCRuleTensorInspector : public ProceduralGeneratorInspectorBase {
    GDCLASS(WFCRuleTensorInspector, ProceduralGeneratorInspectorBase)

protected:
    static void _bind_methods();

public:
    WFCRuleTensorInspector() = default;
    ~WFCRuleTensorInspector() override = default;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext

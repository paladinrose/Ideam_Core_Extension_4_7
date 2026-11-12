#pragma once

/**
 * wfc_rule_tensor_resource.h
 * `WFCRuleTensorResource`, `WFCSceneAnalyzer`, `WFCSolverNode`, and `WFCRuleTensorInspector`
 * (`src/godot/wave_function_collapse/`).
 *
 * Leans on `ProceduralGeneratorResource`, `ProceduralGenerationHost`, and
 * `ProceduralGeneratorInspectorBase` from `src/godot/procedural_generation/`,
 * paralleling how `IdeamSplines` and `IdeamTiles` lean on `IdeamGeometry`.
 */

#include "../procedural_generation/procedural_generator_resource.h"
#include "../procedural_generation/editor/procedural_generator_inspector_base.h"
#include "../../core/wave_function_collapse/wfc_solver_core.h"

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

namespace ideam::godot_ext {

/**
 * Concrete WFC rule tensor resource inheriting from `ProceduralGeneratorResource`
 * (`src/godot/procedural_generation/`).
 */
class WFCRuleTensorResource : public ProceduralGeneratorResource {
    GDCLASS(WFCRuleTensorResource, ProceduralGeneratorResource)

private:
    int state_count_ = 4;
    std::vector<core::WFCObservedCoOccurrencePod> co_occurrence_rules_;

protected:
    static void _bind_methods();

public:
    WFCRuleTensorResource() = default;
    ~WFCRuleTensorResource() override = default;

    void set_state_count(int p_count);
    [[nodiscard]] int get_state_count() const noexcept { return state_count_; }

    void clear_observed_rules();
    [[nodiscard]] int get_observed_rule_count() const noexcept {
        return static_cast<int>(co_occurrence_rules_.size());
    }
    void add_observed_co_occurrence(int p_anchor_state, int p_neighbor_state, int p_port_tag);
};

/**
 * `WFCSceneAnalyzer` (Section 8.3):
 * Inspects `PackedScene::get_state()` (`SceneState`) node types, parent/child hierarchies,
 * and relative `Transform3D` offsets without instantiating nodes to compile a `WFCRuleTensorResource`.
 */
class WFCSceneAnalyzer : public godot::RefCounted {
    GDCLASS(WFCSceneAnalyzer, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    WFCSceneAnalyzer() = default;
    ~WFCSceneAnalyzer() override = default;

    [[nodiscard]] godot::Ref<WFCRuleTensorResource> extract_rules_from_packed_scene(
        const godot::Ref<godot::PackedScene> &p_scene
    ) const;
};

/**
 * `WFCSolverNode`:
 * SceneTree WFC solver node extending `ProceduralGenerationHost` (`src/godot/procedural_generation/`),
 * supporting pinned landmarks (`A, B, C`) and 2D/3D/hypergraph collapse.
 */
class WFCSolverNode : public ProceduralGenerationHost {
    GDCLASS(WFCSolverNode, ProceduralGenerationHost)

private:
    core::WFCSolverCore solver_core_;
    int slot_count_ = 16;

protected:
    static void _bind_methods();

public:
    WFCSolverNode() = default;
    ~WFCSolverNode() override = default;

    void set_slot_count(int p_count);
    [[nodiscard]] int get_slot_count() const noexcept { return slot_count_; }

    void pin_landmark_state(int p_slot_id, int p_state_id);
    void clear_pinned_landmarks();
    [[nodiscard]] godot::PackedInt32Array run_wfc_collapse();
};

/**
 * EditorInspectorPlugin for `WFCRuleTensorResource`, inheriting from
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

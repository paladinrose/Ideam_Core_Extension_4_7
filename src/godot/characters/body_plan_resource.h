#pragma once

/**
 * body_plan_resource.h
 * Authoring-Side Species / Body-Plan Schema Resource (`BodyPlanResource`, `src/godot/characters/`).
 *
 * Splits character authoring from character generation:
 *   - References a base Layer 1 `FormScaffoldResource` (from `IdeamDesign`).
 *   - Defines the valid `[min, default, max]` parameter space, symmetry policies, and
 *     cross-parameter coupling rules (QuantumDarwinism-ready).
 *   - Stores Layer 2 relative-anchored Topology Landmarks & Topological Bridge Rules
 *     (armpit / hip / wing-root regime thresholds) and Layer 3 Parametric Feature templates.
 *   - Exposes `IdeamUndoRedo` and `IdeamTutorials` help-topic hooks.
 */

#include "../../core/characters/character_layered_stack.h"
#include "../design/form_scaffold_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

enum BodyPlanArchetypePreset : int32_t {
    BODY_PLAN_HUMANOID  = 0,
    BODY_PLAN_QUADRUPED = 1,
    BODY_PLAN_AVIAN     = 2
};

class BodyPlanResource : public godot::Resource {
    GDCLASS(BodyPlanResource, godot::Resource)

private:
    core::BodyPlanCore core_plan_;
    godot::Ref<FormScaffoldResource> base_scaffold_;
    BodyPlanArchetypePreset preset_ = BODY_PLAN_HUMANOID;

    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray semantic_tags_;

    void _on_scaffold_changed();

protected:
    static void _bind_methods();

public:
    BodyPlanResource();
    ~BodyPlanResource() override;

    [[nodiscard]] const core::BodyPlanCore &get_core_plan() const noexcept { return core_plan_; }
    [[nodiscard]] core::BodyPlanCore &get_core_plan_mut() noexcept { return core_plan_; }

    void set_species_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_species_name() const;

    void set_archetype_preset(BodyPlanArchetypePreset p_preset);
    [[nodiscard]] BodyPlanArchetypePreset get_archetype_preset() const noexcept { return preset_; }

    void set_base_scaffold(const godot::Ref<FormScaffoldResource> &p_scaffold);
    [[nodiscard]] godot::Ref<FormScaffoldResource> get_base_scaffold() const { return base_scaffold_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    void set_semantic_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    // --- Authoring API ---
    int add_parameter_bound(
        const godot::String &p_name,
        int p_category,
        const godot::String &p_target_name,
        real_t p_min_val,
        real_t p_default_val,
        real_t p_max_val,
        int p_symmetry = 1
    );

    void add_coupling_rule(
        const godot::String &p_source_param,
        const godot::String &p_target_param,
        real_t p_weight
    );

    int add_topology_landmark(
        const godot::String &p_name,
        int p_scaffold_node_id,
        real_t p_u,
        real_t p_v,
        real_t p_normal_offset = 0.0f,
        int p_symmetry = 1,
        real_t p_planar_sharpness = 0.25f
    );

    int add_bridge_rule(
        const godot::String &p_name,
        int p_node_a,
        int p_node_b,
        const godot::String &p_driving_param,
        real_t p_crease_threshold = 0.50f,
        real_t p_deep_fold_threshold = 0.80f
    );

    int add_feature(
        const godot::String &p_name,
        int p_kind,
        int p_parent_landmark_id,
        const godot::Vector3 &p_half_extents,
        int p_symmetry = 1,
        real_t p_secondary_param = 0.0f
    );

    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_parameter_bounds_list() const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_topology_landmarks_list() const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_bridge_rules_list() const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_features_list() const;

    // --- Built-In Loomis Starter Plans ---
    void setup_loomis_humanoid_plan();
    void setup_loomis_quadruped_plan();
    void setup_loomis_avian_plan();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::BodyPlanArchetypePreset);

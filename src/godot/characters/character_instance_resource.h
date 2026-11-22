#pragma once

/**
 * character_instance_resource.h
 * Generator-Side Lightweight Character Instance Resource (`CharacterInstanceResource`).
 *
 * References a `BodyPlanResource` and stores ONLY:
 *   - Chosen parameter values (`parameter_values` Dictionary)
 *   - Layer 4 unconstrained asymmetric detail deltas (`detail_deltas` Array)
 *
 * Keeps saved characters tiny, non-destructive, trivially randomizable
 * (`randomize_within_bounds(seed)`), and blendable (`blend_with(other, alpha)`).
 */

#include "body_plan_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class CharacterInstanceResource : public godot::Resource {
    GDCLASS(CharacterInstanceResource, godot::Resource)

private:
    godot::Ref<BodyPlanResource> body_plan_;
    core::CharacterInstanceCore core_instance_;
    godot::Color base_color_ = godot::Color(0.86f, 0.78f, 0.70f, 1.0f);

    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray semantic_tags_;

    void _on_body_plan_changed();

protected:
    static void _bind_methods();

public:
    CharacterInstanceResource() = default;
    ~CharacterInstanceResource() override;

    [[nodiscard]] const core::CharacterInstanceCore &get_core_instance() const noexcept { return core_instance_; }
    [[nodiscard]] core::CharacterInstanceCore &get_core_instance_mut() noexcept { return core_instance_; }

    void set_body_plan(const godot::Ref<BodyPlanResource> &p_plan);
    [[nodiscard]] godot::Ref<BodyPlanResource> get_body_plan() const { return body_plan_; }

    void set_character_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_character_name() const;

    void set_parameter_values(const godot::Dictionary &p_values);
    [[nodiscard]] godot::Dictionary get_parameter_values() const;

    void set_detail_deltas(const godot::TypedArray<godot::Dictionary> &p_deltas);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_detail_deltas() const;

    void set_base_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_base_color() const noexcept { return base_color_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) { undo_redo_ = p_ur; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    void set_semantic_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    // --- Direct & Undoable Generator API ---
    void set_parameter_value(const godot::String &p_name, real_t p_value);
    void commit_set_parameter_value(const godot::String &p_name, real_t p_value);
    [[nodiscard]] real_t get_parameter_value(const godot::String &p_name) const;

    int add_detail_delta(
        const godot::String &p_label,
        int p_scaffold_node_id,
        real_t p_u,
        real_t p_v,
        const godot::Vector3 &p_local_delta_ntuv,
        real_t p_radius = 0.05f,
        bool p_is_crease = false
    );
    void clear_detail_deltas();

    void randomize_within_bounds(int64_t p_seed = 0);
    [[nodiscard]] godot::Ref<CharacterInstanceResource> blend_with(
        const godot::Ref<CharacterInstanceResource> &p_other,
        real_t p_alpha = 0.5f
    ) const;

    // --- 4-Layer Evaluation Outputs ---
    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_array_mesh(int p_lod_level = 0) const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_evaluated_bridge_regimes() const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> build_skeleton_export_plan() const;
    [[nodiscard]] godot::Dictionary get_balance_report() const;
};

} // namespace ideam::godot_ext

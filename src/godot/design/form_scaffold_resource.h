#pragma once

/**
 * form_scaffold_resource.h
 * Godot `Resource` wrapping `ideam::core::FormScaffoldCore` (`src/godot/design/`).
 *
 * Bridges volumetric Andrew Loomis scaffolds, `ProportionRulerGraphResource`,
 * optional `Spline3DResource` Line of Action, Contrapposto, Center-of-Gravity balance,
 * curvilinear `sample_scaffold_frame(node_id, u, v)` anchoring, and `Skeleton3D` /
 * ragdoll bone plans into Godot.
 */

#include "proportion_ruler_resource.h"
#include "../../core/design/form_scaffold.h"
#include "../splines/spline_3d/spline_3d_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

enum FormScaffoldArchetype : int32_t {
    SCAFFOLD_ARCHETYPE_BIPED_NORMAL     = 0,
    SCAFFOLD_ARCHETYPE_BIPED_IDEALISTIC = 1,
    SCAFFOLD_ARCHETYPE_BIPED_HEROIC     = 2,
    SCAFFOLD_ARCHETYPE_BIPED_CHILD      = 3,
    SCAFFOLD_ARCHETYPE_BIPED_STYLIZED   = 4,
    SCAFFOLD_ARCHETYPE_QUADRUPED        = 5,
    SCAFFOLD_ARCHETYPE_AVIAN            = 6,
    SCAFFOLD_ARCHETYPE_SERPENTINE       = 7
};

class FormScaffoldResource : public godot::Resource {
    GDCLASS(FormScaffoldResource, godot::Resource)

private:
    core::FormScaffoldCore core_scaffold_;
    godot::Ref<ProportionRulerGraphResource> ruler_graph_;
    godot::Ref<Spline3DResource> line_of_action_;
    FormScaffoldArchetype archetype_ = SCAFFOLD_ARCHETYPE_BIPED_NORMAL;
    godot::Color scaffold_color_ = godot::Color(0.86f, 0.80f, 0.72f, 1.0f);

    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray semantic_tags_;

    void _on_dependency_changed();
    void _sync_from_dependencies();

protected:
    static void _bind_methods();

public:
    FormScaffoldResource();
    ~FormScaffoldResource() override;

    [[nodiscard]] const core::FormScaffoldCore &get_core_scaffold() const noexcept { return core_scaffold_; }
    [[nodiscard]] core::FormScaffoldCore &get_core_scaffold_mut() noexcept { return core_scaffold_; }

    void set_scaffold_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_scaffold_name() const;

    void set_archetype(FormScaffoldArchetype p_arch);
    [[nodiscard]] FormScaffoldArchetype get_archetype() const noexcept { return archetype_; }

    void set_ruler_graph(const godot::Ref<ProportionRulerGraphResource> &p_ruler);
    [[nodiscard]] godot::Ref<ProportionRulerGraphResource> get_ruler_graph() const { return ruler_graph_; }

    void set_line_of_action(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_line_of_action() const { return line_of_action_; }

    void set_contrapposto_amount(real_t p_amount);
    [[nodiscard]] real_t get_contrapposto_amount() const noexcept { return static_cast<real_t>(core_scaffold_.get_contrapposto_amount()); }

    void set_total_mass_kg(real_t p_mass);
    [[nodiscard]] real_t get_total_mass_kg() const noexcept { return static_cast<real_t>(core_scaffold_.get_total_mass_kg()); }

    void set_scaffold_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_scaffold_color() const noexcept { return scaffold_color_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    void set_semantic_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    // --- Node Inspection & Undoable Manipulation ---
    [[nodiscard]] int get_node_count() const noexcept { return static_cast<int>(core_scaffold_.nodes().size()); }
    [[nodiscard]] godot::Vector3 get_node_local_offset(int p_node_id) const;
    void set_node_local_offset(int p_node_id, const godot::Vector3 &p_offset);
    void commit_set_node_local_offset(int p_node_id, const godot::Vector3 &p_offset);
    [[nodiscard]] godot::Vector3 get_node_world_position(int p_node_id) const;
    [[nodiscard]] godot::String get_node_name(int p_node_id) const;

    // --- Curvilinear Anchoring, Balance, Mesh & Skeleton Plan ---
    [[nodiscard]] godot::Transform3D sample_scaffold_frame(int p_node_id, real_t p_u, real_t p_v) const;
    [[nodiscard]] godot::Dictionary get_balance_report() const;
    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_preview_array_mesh(int p_subdivs = 10) const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> build_skeleton_export_plan() const;

    // --- Loomis Body-Plan Builders ---
    void setup_loomis_biped(LoomisBipedCanonPreset p_preset = LOOMIS_BIPED_NORMAL_7_5H, real_t p_head_height_m = 0.23f);
    void setup_loomis_quadruped(real_t p_shoulder_height_m = 0.80f);
    void setup_loomis_avian(real_t p_keel_length_m = 0.22f);
    void setup_loomis_serpentine(real_t p_skull_length_m = 0.14f, int p_segments = 10);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::FormScaffoldArchetype);

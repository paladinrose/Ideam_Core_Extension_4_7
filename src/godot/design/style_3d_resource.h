#pragma once

/**
 * style_3d_resource.h
 * `Style3DResource` and `Theme3DResource` mapping `SemanticRoleID` bitpaths and
 * `IdeamLinguistics` `Sememe` tags to 3D meshes, materials, edge-snap policies,
 * and CSS-cascade scale modes (`src/godot/design/`).
 */

#include "../../core/design/design_common.h"
#include "../linguistics/sememe_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <vector>

namespace ideam::godot_ext {

class Style3DResource : public godot::Resource {
    GDCLASS(Style3DResource, godot::Resource)

private:
    uint32_t semantic_role_id_              = core::SEMANTIC_ROLE_SURFACE;
    uint32_t parent_role_selector_          = core::SEMANTIC_ROLE_NONE;
    core::DesignScaleMode scale_mode_       = core::DesignScaleMode::UNIFORM_FIT;
    core::EdgeSnapPolicy edge_snap_policy_  = core::EdgeSnapPolicy::SNAP_WATERTIGHT;
    core::DesignCascadeLayer cascade_layer_ = core::DesignCascadeLayer::LAYER_ARCHETYPE_DEFAULT;
    int priority_                           = 0;
    bool is_important_                      = false;
    godot::Vector3 target_extents_scale_    = godot::Vector3(1.0f, 1.0f, 1.0f);
    godot::Vector3 margin_override_         = godot::Vector3(0.0f, 0.0f, 0.0f);
    float tile_repeat_density_              = 1.0f;
    godot::Ref<godot::Mesh> mesh_asset_;
    godot::Ref<godot::Material> material_override_;
    godot::Ref<SemanticTagSetResource> semantic_tags_;

protected:
    static void _bind_methods();

public:
    Style3DResource() = default;
    ~Style3DResource() override = default;

    void set_semantic_role_id(int p_role_id);
    [[nodiscard]] int get_semantic_role_id() const noexcept { return static_cast<int>(semantic_role_id_); }

    void set_parent_role_selector(int p_role_id);
    [[nodiscard]] int get_parent_role_selector() const noexcept { return static_cast<int>(parent_role_selector_); }

    void set_scale_mode(int p_mode);
    [[nodiscard]] int get_scale_mode() const noexcept { return static_cast<int>(scale_mode_); }

    void set_edge_snap_policy(int p_policy);
    [[nodiscard]] int get_edge_snap_policy() const noexcept { return static_cast<int>(edge_snap_policy_); }

    void set_cascade_layer(int p_layer);
    [[nodiscard]] int get_cascade_layer() const noexcept { return static_cast<int>(cascade_layer_); }

    void set_priority(int p_priority);
    [[nodiscard]] int get_priority() const noexcept { return priority_; }

    void set_is_important(bool p_important);
    [[nodiscard]] bool get_is_important() const noexcept { return is_important_; }

    void set_target_extents_scale(const godot::Vector3 &p_scale);
    [[nodiscard]] godot::Vector3 get_target_extents_scale() const noexcept { return target_extents_scale_; }

    void set_margin_override(const godot::Vector3 &p_margin);
    [[nodiscard]] godot::Vector3 get_margin_override() const noexcept { return margin_override_; }

    void set_tile_repeat_density(float p_density);
    [[nodiscard]] float get_tile_repeat_density() const noexcept { return tile_repeat_density_; }

    void set_mesh_asset(const godot::Ref<godot::Mesh> &p_mesh);
    [[nodiscard]] godot::Ref<godot::Mesh> get_mesh_asset() const { return mesh_asset_; }

    void set_material_override(const godot::Ref<godot::Material> &p_mat);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags);
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    [[nodiscard]] core::Style3DRulePod to_style_rule_pod(uint32_t p_rule_id = 0u) const;

    // --- Tier 1 `IdeamUndoRedo` Commit Wrappers ---
    void commit_set_semantic_role_id(int p_role_id, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_set_scale_mode(int p_mode, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_set_mesh_asset(
        const godot::Ref<godot::Mesh> &p_mesh,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
};

class Theme3DResource : public godot::Resource {
    GDCLASS(Theme3DResource, godot::Resource)

private:
    godot::Dictionary role_to_style_map_;
    godot::TypedArray<Style3DResource> cascade_rules_;
    int default_layer_ = static_cast<int>(core::DesignCascadeLayer::LAYER_THEME_ERA);

protected:
    static void _bind_methods();

public:
    Theme3DResource() = default;
    ~Theme3DResource() override = default;

    void set_default_layer(int p_layer);
    [[nodiscard]] int get_default_layer() const noexcept { return default_layer_; }

    void set_cascade_rules(const godot::TypedArray<Style3DResource> &p_rules);
    [[nodiscard]] godot::TypedArray<Style3DResource> get_cascade_rules() const { return cascade_rules_; }

    void add_style_rule(const godot::Ref<Style3DResource> &p_style);
    void clear_style_rules();

    void set_style_for_role(int p_semantic_role_id, const godot::Ref<Style3DResource> &p_style);
    [[nodiscard]] godot::Ref<Style3DResource> get_style_for_role(int p_semantic_role_id) const;

    /**
     * Evaluates the full CSS specificity cascade (`!important > layer > priority > specificity`)
     * for a candidate `SemanticRoleID` and optional `SemanticTagSetResource`.
     */
    [[nodiscard]] godot::Ref<Style3DResource> resolve_cascaded_style(
        int p_candidate_role_id,
        int p_parent_role_id = 0,
        const godot::Ref<SemanticTagSetResource> &p_candidate_tags = godot::Ref<SemanticTagSetResource>()
    ) const;

    void compile_to_pod_rules(std::vector<core::Style3DRulePod> &r_out) const;
};

} // namespace ideam::godot_ext

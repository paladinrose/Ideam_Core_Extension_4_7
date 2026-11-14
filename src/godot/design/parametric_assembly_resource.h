#pragma once

/**
 * parametric_assembly_resource.h
 * `ParametricAssemblyRecipeResource` and `ParametricAssemblyNode3D`
 * (`src/godot/design/`).
 */

#include "../../core/design/parametric_assembly_core.h"
#include "container_3d.h"
#include "style_3d_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class ParametricAssemblyRecipeResource : public godot::Resource {
    GDCLASS(ParametricAssemblyRecipeResource, godot::Resource)

private:
    godot::String preset_archetype_ = "Chair";
    int root_semantic_role_id_      = static_cast<int>(core::SEMANTIC_ROLE_SURFACE_CHAIR);
    int nuclear_seme_mask_          = 0x0F;
    int peripheral_seme_mask_       = 0xF0;
    godot::Dictionary parameter_overrides_;
    core::ParametricAssemblyRecipe core_recipe_;

    void _rebuild_core_recipe();

protected:
    static void _bind_methods();

public:
    ParametricAssemblyRecipeResource();
    ~ParametricAssemblyRecipeResource() override = default;

    void set_preset_archetype(const godot::String &p_preset);
    [[nodiscard]] godot::String get_preset_archetype() const { return preset_archetype_; }

    void set_root_semantic_role_id(int p_role);
    [[nodiscard]] int get_root_semantic_role_id() const noexcept { return root_semantic_role_id_; }

    void set_nuclear_seme_mask(int p_mask);
    [[nodiscard]] int get_nuclear_seme_mask() const noexcept { return nuclear_seme_mask_; }

    void set_peripheral_seme_mask(int p_mask);
    [[nodiscard]] int get_peripheral_seme_mask() const noexcept { return peripheral_seme_mask_; }

    void set_parameter_overrides(const godot::Dictionary &p_overrides);
    [[nodiscard]] godot::Dictionary get_parameter_overrides() const { return parameter_overrides_; }

    void set_parameter_value(const godot::String &p_param_name, float p_value);
    [[nodiscard]] float get_parameter_value(const godot::String &p_param_name, float p_default = 0.0f) const;

    // Tier 1 IdeamUndoRedo commit_* wrappers
    void commit_set_preset_archetype(
        const godot::String &p_new_preset,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_parameter_value(
        const godot::String &p_param_name,
        float p_new_value,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );

    [[nodiscard]] const core::ParametricAssemblyRecipe &get_core_recipe() const noexcept { return core_recipe_; }
};

class ParametricAssemblyNode3D : public SemanticStructure3D {
    GDCLASS(ParametricAssemblyNode3D, SemanticStructure3D)

private:
    godot::Ref<ParametricAssemblyRecipeResource> recipe_;
    float dream_morph_factor_   = 0.0f;
    int random_seed_            = 1337;
    bool auto_rebuild_children_ = true;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    ParametricAssemblyNode3D() = default;
    ~ParametricAssemblyNode3D() override = default;

    void set_recipe(const godot::Ref<ParametricAssemblyRecipeResource> &p_recipe);
    [[nodiscard]] godot::Ref<ParametricAssemblyRecipeResource> get_recipe() const { return recipe_; }

    void set_dream_morph_factor(float p_factor);
    [[nodiscard]] float get_dream_morph_factor() const noexcept { return dream_morph_factor_; }

    void set_random_seed(int p_seed);
    [[nodiscard]] int get_random_seed() const noexcept { return random_seed_; }

    void set_auto_rebuild_children(bool p_auto);
    [[nodiscard]] bool get_auto_rebuild_children() const noexcept { return auto_rebuild_children_; }

    void set_assembly_parameter(const godot::String &p_name, float p_value);
    [[nodiscard]] float get_assembly_parameter(const godot::String &p_name) const;

    void rebuild_assembly();
    [[nodiscard]] godot::Dictionary get_solved_assembly_snapshot() const;
};

} // namespace ideam::godot_ext

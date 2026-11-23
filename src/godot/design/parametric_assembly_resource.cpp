#include "parametric_assembly_resource.h"
#include "ideam_design_plugin.h"
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>

namespace ideam::godot_ext {

ParametricAssemblyRecipeResource::ParametricAssemblyRecipeResource() {
    _rebuild_core_recipe();
}

void ParametricAssemblyRecipeResource::_rebuild_core_recipe() {
    if (preset_archetype_ == "Table") {
        core_recipe_ = core::ParametricAssemblyRecipe::create_table_recipe();
    } else if (preset_archetype_ == "OrganicTree") {
        core_recipe_ = core::ParametricAssemblyRecipe::create_organic_tree_recipe();
    } else if (preset_archetype_ == "GeologicOutcrop") {
        core_recipe_ = core::ParametricAssemblyRecipe::create_geologic_outcrop_recipe();
    } else {
        core_recipe_ = core::ParametricAssemblyRecipe::create_chair_recipe();
    }
    root_semantic_role_id_ = static_cast<int>(core_recipe_.root_semantic_role_id());
    core_recipe_.set_root_seme_masks(
        static_cast<uint32_t>(nuclear_seme_mask_),
        static_cast<uint32_t>(peripheral_seme_mask_)
    );

    godot::Array keys = parameter_overrides_.keys();
    for (int i = 0; i < keys.size(); ++i) {
        godot::String k = keys[i];
        float val = static_cast<float>(parameter_overrides_[k]);
        godot::CharString cs = k.utf8();
        if (cs.get_data()) {
            core_recipe_.set_parameter_value(cs.get_data(), val);
        }
    }
}

void ParametricAssemblyRecipeResource::set_preset_archetype(const godot::String &p_preset) {
    preset_archetype_ = p_preset;
    _rebuild_core_recipe();
    emit_changed();
}

void ParametricAssemblyRecipeResource::set_root_semantic_role_id(int p_role) {
    root_semantic_role_id_ = p_role;
    core_recipe_.set_root_semantic_role_id(static_cast<uint32_t>(p_role));
    emit_changed();
}

void ParametricAssemblyRecipeResource::set_nuclear_seme_mask(int p_mask) {
    nuclear_seme_mask_ = p_mask;
    core_recipe_.set_root_seme_masks(
        static_cast<uint32_t>(nuclear_seme_mask_),
        static_cast<uint32_t>(peripheral_seme_mask_)
    );
    emit_changed();
}

void ParametricAssemblyRecipeResource::set_peripheral_seme_mask(int p_mask) {
    peripheral_seme_mask_ = p_mask;
    core_recipe_.set_root_seme_masks(
        static_cast<uint32_t>(nuclear_seme_mask_),
        static_cast<uint32_t>(peripheral_seme_mask_)
    );
    emit_changed();
}

void ParametricAssemblyRecipeResource::set_parameter_overrides(const godot::Dictionary &p_overrides) {
    parameter_overrides_ = p_overrides;
    _rebuild_core_recipe();
    emit_changed();
}

void ParametricAssemblyRecipeResource::set_parameter_value(const godot::String &p_param_name, float p_value) {
    parameter_overrides_[p_param_name] = p_value;
    godot::CharString cs = p_param_name.utf8();
    if (cs.get_data()) {
        core_recipe_.set_parameter_value(cs.get_data(), p_value);
    }
    emit_changed();
}

float ParametricAssemblyRecipeResource::get_parameter_value(const godot::String &p_param_name, float p_default) const {
    godot::CharString cs = p_param_name.utf8();
    if (cs.get_data()) {
        return core_recipe_.get_parameter_value(cs.get_data(), p_default);
    }
    return p_default;
}

void ParametricAssemblyRecipeResource::commit_set_preset_archetype(
    const godot::String &p_new_preset,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    if (preset_archetype_ == p_new_preset) {
        return;
    }
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamDesignPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const godot::String old_preset = preset_archetype_;
        ur->create_action("Set Parametric Assembly Preset");
        ur->add_do_property(this, "preset_archetype", p_new_preset);
        ur->add_undo_property(this, "preset_archetype", old_preset);
        ur->commit_action();
    } else {
        set_preset_archetype(p_new_preset);
    }
}

void ParametricAssemblyRecipeResource::commit_set_parameter_value(
    const godot::String &p_param_name,
    float p_new_value,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Dictionary next_dict = parameter_overrides_.duplicate();
    next_dict[p_param_name] = p_new_value;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamDesignPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const godot::Dictionary old_dict = parameter_overrides_.duplicate();
        ur->create_action(godot::String("Adjust Assembly Parameter: ") + p_param_name);
        ur->add_do_property(this, "parameter_overrides", next_dict);
        ur->add_undo_property(this, "parameter_overrides", old_dict);
        ur->commit_action();
    } else {
        set_parameter_value(p_param_name, p_new_value);
    }
}

void ParametricAssemblyRecipeResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_preset_archetype", "preset"), &ParametricAssemblyRecipeResource::set_preset_archetype);
    ClassDB::bind_method(D_METHOD("get_preset_archetype"), &ParametricAssemblyRecipeResource::get_preset_archetype);

    ClassDB::bind_method(D_METHOD("set_root_semantic_role_id", "role"), &ParametricAssemblyRecipeResource::set_root_semantic_role_id);
    ClassDB::bind_method(D_METHOD("get_root_semantic_role_id"), &ParametricAssemblyRecipeResource::get_root_semantic_role_id);

    ClassDB::bind_method(D_METHOD("set_nuclear_seme_mask", "mask"), &ParametricAssemblyRecipeResource::set_nuclear_seme_mask);
    ClassDB::bind_method(D_METHOD("get_nuclear_seme_mask"), &ParametricAssemblyRecipeResource::get_nuclear_seme_mask);

    ClassDB::bind_method(D_METHOD("set_peripheral_seme_mask", "mask"), &ParametricAssemblyRecipeResource::set_peripheral_seme_mask);
    ClassDB::bind_method(D_METHOD("get_peripheral_seme_mask"), &ParametricAssemblyRecipeResource::get_peripheral_seme_mask);

    ClassDB::bind_method(D_METHOD("set_parameter_overrides", "overrides"), &ParametricAssemblyRecipeResource::set_parameter_overrides);
    ClassDB::bind_method(D_METHOD("get_parameter_overrides"), &ParametricAssemblyRecipeResource::get_parameter_overrides);

    ClassDB::bind_method(D_METHOD("set_parameter_value", "param_name", "value"), &ParametricAssemblyRecipeResource::set_parameter_value);
    ClassDB::bind_method(D_METHOD("get_parameter_value", "param_name", "default_value"), &ParametricAssemblyRecipeResource::get_parameter_value, DEFVAL(0.0f));

    ClassDB::bind_method(
        D_METHOD("commit_set_preset_archetype", "new_preset", "undo_redo"),
        &ParametricAssemblyRecipeResource::commit_set_preset_archetype,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_parameter_value", "param_name", "new_value", "undo_redo"),
        &ParametricAssemblyRecipeResource::commit_set_parameter_value,
        DEFVAL(Ref<IdeamUndoRedo>())
    );

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "preset_archetype", PROPERTY_HINT_ENUM, "Chair,Table,OrganicTree,GeologicOutcrop"), "set_preset_archetype", "get_preset_archetype");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "root_semantic_role_id"), "set_root_semantic_role_id", "get_root_semantic_role_id");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "nuclear_seme_mask"), "set_nuclear_seme_mask", "get_nuclear_seme_mask");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "peripheral_seme_mask"), "set_peripheral_seme_mask", "get_peripheral_seme_mask");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "parameter_overrides"), "set_parameter_overrides", "get_parameter_overrides");
}

// ============================================================================
// ParametricAssemblyNode3D
// ============================================================================

void ParametricAssemblyNode3D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        if (auto_rebuild_children_ && recipe_.is_valid()) {
            rebuild_assembly();
        }
    }
}

void ParametricAssemblyNode3D::set_recipe(const godot::Ref<ParametricAssemblyRecipeResource> &p_recipe) {
    recipe_ = p_recipe;
    if (recipe_.is_valid()) {
        set_semantic_role_id(recipe_->get_root_semantic_role_id());
        if (auto_rebuild_children_ && is_inside_tree()) {
            rebuild_assembly();
        }
    }
}

void ParametricAssemblyNode3D::set_dream_morph_factor(float p_factor) {
    dream_morph_factor_ = p_factor;
    if (auto_rebuild_children_ && is_inside_tree()) {
        rebuild_assembly();
    }
}

void ParametricAssemblyNode3D::set_random_seed(int p_seed) {
    random_seed_ = p_seed;
    if (auto_rebuild_children_ && is_inside_tree()) {
        rebuild_assembly();
    }
}

void ParametricAssemblyNode3D::set_auto_rebuild_children(bool p_auto) {
    auto_rebuild_children_ = p_auto;
}

void ParametricAssemblyNode3D::set_assembly_parameter(const godot::String &p_name, float p_value) {
    if (recipe_.is_valid()) {
        recipe_->set_parameter_value(p_name, p_value);
        if (auto_rebuild_children_ && is_inside_tree()) {
            rebuild_assembly();
        }
    }
}

float ParametricAssemblyNode3D::get_assembly_parameter(const godot::String &p_name) const {
    if (recipe_.is_valid()) {
        return recipe_->get_parameter_value(p_name, 0.0f);
    }
    return 0.0f;
}

void ParametricAssemblyNode3D::rebuild_assembly() {
    using namespace godot;
    if (recipe_.is_null()) {
        return;
    }

    core::StructureLayoutSolver solver;
    Ref<Theme3DResource> active_theme = get_active_theme_3d();
    if (active_theme.is_valid()) {
        std::vector<core::Style3DRulePod> rules;
        active_theme->compile_to_pod_rules(rules);
        for (const auto &r : rules) {
            solver.add_style_rule(r);
        }
    }

    recipe_->get_core_recipe().synthesize_to_layout_solver(
        solver,
        dream_morph_factor_,
        static_cast<uint64_t>(random_seed_)
    );

    for (int i = get_child_count() - 1; i >= 0; --i) {
        Node *child = get_child(i);
        if (child && child->has_meta("_ideam_generated_part")) {
            remove_child(child);
            memdelete(child);
        }
    }

    const auto &nodes = solver.nodes();
    for (size_t i = 1; i < nodes.size(); ++i) {
        const auto &pod = nodes[i];
        SemanticStructure3D *part = memnew(SemanticStructure3D);
        part->set_meta("_ideam_generated_part", true);
        part->set_name(godot::String("Part_") + String::num_int64(pod.node_id));
        part->set_semantic_role_id(static_cast<int>(pod.semantic_tag_id));
        part->set_rest_obb_extents(Vector3(pod.rest_obb_extents.x, pod.rest_obb_extents.y, pod.rest_obb_extents.z));
        part->set_position(Vector3(pod.fitted_transform[3][0], pod.fitted_transform[3][1], pod.fitted_transform[3][2]));

        MeshInstance3D *mesh_inst = memnew(MeshInstance3D);
        uint32_t role_hi = pod.semantic_tag_id & 0xFFFF0000u;
        if (role_hi == core::SEMANTIC_ROLE_COMPONENT_LEG ||
            role_hi == core::SEMANTIC_ROLE_ORGANIC_TRUNK ||
            role_hi == core::SEMANTIC_ROLE_ORGANIC_BRANCH) {
            Ref<CylinderMesh> cyl;
            cyl.instantiate();
            cyl->set_top_radius(pod.rest_obb_extents.x * 0.85f);
            cyl->set_bottom_radius(pod.rest_obb_extents.x);
            cyl->set_height(pod.rest_obb_extents.y * 2.0f);
            mesh_inst->set_mesh(cyl);
        } else if (role_hi == core::SEMANTIC_ROLE_ORGANIC_FOLIAGE_CANOPY ||
                   (pod.semantic_tag_id & 0xFFFFFF00u) == core::SEMANTIC_ROLE_COMPONENT_CASTER_WHEEL) {
            Ref<SphereMesh> sph;
            sph.instantiate();
            sph->set_radius(pod.rest_obb_extents.x);
            sph->set_height(pod.rest_obb_extents.y * 2.0f);
            mesh_inst->set_mesh(sph);
        } else {
            Ref<BoxMesh> box;
            box.instantiate();
            box->set_size(Vector3(pod.rest_obb_extents.x * 2.0f, pod.rest_obb_extents.y * 2.0f, pod.rest_obb_extents.z * 2.0f));
            mesh_inst->set_mesh(box);
        }
        part->add_child(mesh_inst);
        if (active_theme.is_valid()) {
            part->set_active_theme_3d(active_theme);
            part->apply_theme_and_bounds_to_children();
        }
        add_child(part);
    }
}

godot::Dictionary ParametricAssemblyNode3D::get_solved_assembly_snapshot() const {
    godot::Dictionary out;
    if (recipe_.is_null()) {
        return out;
    }
    core::StructureLayoutSolver solver;
    recipe_->get_core_recipe().synthesize_to_layout_solver(
        solver,
        dream_morph_factor_,
        static_cast<uint64_t>(random_seed_)
    );
    out["node_count"]   = static_cast<int>(solver.nodes().size());
    out["socket_count"] = static_cast<int>(solver.sockets().size());
    return out;
}

void ParametricAssemblyNode3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_recipe", "recipe"), &ParametricAssemblyNode3D::set_recipe);
    ClassDB::bind_method(D_METHOD("get_recipe"), &ParametricAssemblyNode3D::get_recipe);

    ClassDB::bind_method(D_METHOD("set_dream_morph_factor", "factor"), &ParametricAssemblyNode3D::set_dream_morph_factor);
    ClassDB::bind_method(D_METHOD("get_dream_morph_factor"), &ParametricAssemblyNode3D::get_dream_morph_factor);

    ClassDB::bind_method(D_METHOD("set_random_seed", "seed"), &ParametricAssemblyNode3D::set_random_seed);
    ClassDB::bind_method(D_METHOD("get_random_seed"), &ParametricAssemblyNode3D::get_random_seed);

    ClassDB::bind_method(D_METHOD("set_auto_rebuild_children", "auto_rebuild"), &ParametricAssemblyNode3D::set_auto_rebuild_children);
    ClassDB::bind_method(D_METHOD("get_auto_rebuild_children"), &ParametricAssemblyNode3D::get_auto_rebuild_children);

    ClassDB::bind_method(D_METHOD("set_assembly_parameter", "name", "value"), &ParametricAssemblyNode3D::set_assembly_parameter);
    ClassDB::bind_method(D_METHOD("get_assembly_parameter", "name"), &ParametricAssemblyNode3D::get_assembly_parameter);

    ClassDB::bind_method(D_METHOD("rebuild_assembly"), &ParametricAssemblyNode3D::rebuild_assembly);
    ClassDB::bind_method(D_METHOD("get_solved_assembly_snapshot"), &ParametricAssemblyNode3D::get_solved_assembly_snapshot);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "recipe", PROPERTY_HINT_RESOURCE_TYPE, "ParametricAssemblyRecipeResource"), "set_recipe", "get_recipe");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dream_morph_factor", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_dream_morph_factor", "get_dream_morph_factor");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed"), "set_random_seed", "get_random_seed");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_rebuild_children"), "set_auto_rebuild_children", "get_auto_rebuild_children");
}

} // namespace ideam::godot_ext

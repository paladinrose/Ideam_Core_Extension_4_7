#include "style_3d_resource.h"
#include "ideam_design_plugin.h"
#include "../../core/design/structure_layout_solver.h"
#include <algorithm>

namespace ideam::godot_ext {

void Style3DResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_semantic_role_id", "role_id"), &Style3DResource::set_semantic_role_id);
    ClassDB::bind_method(D_METHOD("get_semantic_role_id"), &Style3DResource::get_semantic_role_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "semantic_role_id"), "set_semantic_role_id", "get_semantic_role_id");

    ClassDB::bind_method(D_METHOD("set_parent_role_selector", "role_id"), &Style3DResource::set_parent_role_selector);
    ClassDB::bind_method(D_METHOD("get_parent_role_selector"), &Style3DResource::get_parent_role_selector);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "parent_role_selector"), "set_parent_role_selector", "get_parent_role_selector");

    ClassDB::bind_method(D_METHOD("set_scale_mode", "scale_mode"), &Style3DResource::set_scale_mode);
    ClassDB::bind_method(D_METHOD("get_scale_mode"), &Style3DResource::get_scale_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "scale_mode",
            PROPERTY_HINT_ENUM,
            "Uniform Fit,Stretch Middle,Repeat Tiles,Spline Loft Deform,Proportional Axis"
        ),
        "set_scale_mode",
        "get_scale_mode"
    );

    ClassDB::bind_method(D_METHOD("set_edge_snap_policy", "policy"), &Style3DResource::set_edge_snap_policy);
    ClassDB::bind_method(D_METHOD("get_edge_snap_policy"), &Style3DResource::get_edge_snap_policy);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "edge_snap_policy",
            PROPERTY_HINT_ENUM,
            "None,Watertight,Miter Corner,Socket Anchor,Surface Normal Drape"
        ),
        "set_edge_snap_policy",
        "get_edge_snap_policy"
    );

    ClassDB::bind_method(D_METHOD("set_cascade_layer", "layer"), &Style3DResource::set_cascade_layer);
    ClassDB::bind_method(D_METHOD("get_cascade_layer"), &Style3DResource::get_cascade_layer);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "cascade_layer",
            PROPERTY_HINT_ENUM,
            "ArchetypeDefault:0,BlueprintRoom:1,ThemeEra:2,DamageOrWear:3,DreamSurrealism:4,UserCustomization:5"
        ),
        "set_cascade_layer",
        "get_cascade_layer"
    );

    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &Style3DResource::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &Style3DResource::get_priority);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");

    ClassDB::bind_method(D_METHOD("set_is_important", "important"), &Style3DResource::set_is_important);
    ClassDB::bind_method(D_METHOD("get_is_important"), &Style3DResource::get_is_important);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_important"), "set_is_important", "get_is_important");

    ClassDB::bind_method(D_METHOD("set_target_extents_scale", "scale"), &Style3DResource::set_target_extents_scale);
    ClassDB::bind_method(D_METHOD("get_target_extents_scale"), &Style3DResource::get_target_extents_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "target_extents_scale"), "set_target_extents_scale", "get_target_extents_scale");

    ClassDB::bind_method(D_METHOD("set_margin_override", "margin"), &Style3DResource::set_margin_override);
    ClassDB::bind_method(D_METHOD("get_margin_override"), &Style3DResource::get_margin_override);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "margin_override"), "set_margin_override", "get_margin_override");

    ClassDB::bind_method(D_METHOD("set_tile_repeat_density", "density"), &Style3DResource::set_tile_repeat_density);
    ClassDB::bind_method(D_METHOD("get_tile_repeat_density"), &Style3DResource::get_tile_repeat_density);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "tile_repeat_density", PROPERTY_HINT_RANGE, "0.05,64.0,0.05"),
        "set_tile_repeat_density",
        "get_tile_repeat_density"
    );

    ClassDB::bind_method(D_METHOD("set_mesh_asset", "mesh"), &Style3DResource::set_mesh_asset);
    ClassDB::bind_method(D_METHOD("get_mesh_asset"), &Style3DResource::get_mesh_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "mesh_asset", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"),
        "set_mesh_asset",
        "get_mesh_asset"
    );

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &Style3DResource::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &Style3DResource::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &Style3DResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &Style3DResource::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(
        D_METHOD("commit_set_semantic_role_id", "role_id", "undo_redo"),
        &Style3DResource::commit_set_semantic_role_id,
        DEFVAL(Variant())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_scale_mode", "scale_mode", "undo_redo"),
        &Style3DResource::commit_set_scale_mode,
        DEFVAL(Variant())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_mesh_asset", "mesh", "undo_redo"),
        &Style3DResource::commit_set_mesh_asset,
        DEFVAL(Variant())
    );
}

void Style3DResource::set_semantic_role_id(int p_role_id) {
    semantic_role_id_ = static_cast<uint32_t>(p_role_id);
    emit_changed();
}

void Style3DResource::set_parent_role_selector(int p_role_id) {
    parent_role_selector_ = static_cast<uint32_t>(p_role_id);
    emit_changed();
}

void Style3DResource::set_scale_mode(int p_mode) {
    scale_mode_ = static_cast<core::DesignScaleMode>(std::clamp(p_mode, 0, 4));
    emit_changed();
}

void Style3DResource::set_edge_snap_policy(int p_policy) {
    edge_snap_policy_ = static_cast<core::EdgeSnapPolicy>(std::clamp(p_policy, 0, 4));
    emit_changed();
}

void Style3DResource::set_cascade_layer(int p_layer) {
    cascade_layer_ = static_cast<core::DesignCascadeLayer>(std::clamp(p_layer, 0, 5));
    emit_changed();
}

void Style3DResource::set_priority(int p_priority) {
    priority_ = std::clamp(p_priority, -32768, 32767);
    emit_changed();
}

void Style3DResource::set_is_important(bool p_important) {
    is_important_ = p_important;
    emit_changed();
}

void Style3DResource::set_target_extents_scale(const godot::Vector3 &p_scale) {
    target_extents_scale_ = p_scale;
    emit_changed();
}

void Style3DResource::set_margin_override(const godot::Vector3 &p_margin) {
    margin_override_ = p_margin;
    emit_changed();
}

void Style3DResource::set_tile_repeat_density(float p_density) {
    tile_repeat_density_ = std::max(0.05f, p_density);
    emit_changed();
}

void Style3DResource::set_mesh_asset(const godot::Ref<godot::Mesh> &p_mesh) {
    mesh_asset_ = p_mesh;
    emit_changed();
}

void Style3DResource::set_material_override(const godot::Ref<godot::Material> &p_mat) {
    material_override_ = p_mat;
    emit_changed();
}

void Style3DResource::set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

core::Style3DRulePod Style3DResource::to_style_rule_pod(uint32_t p_rule_id) const {
    core::Style3DRulePod pod{};
    pod.rule_id                = p_rule_id;
    pod.semantic_role_selector = semantic_role_id_;
    pod.parent_role_selector   = parent_role_selector_;
    if (semantic_tags_.is_valid()) {
        const auto desc                   = semantic_tags_->compile_combined_descriptor();
        pod.required_nuclear_seme_mask    = static_cast<uint32_t>(desc.nuclear_seme_words[0] & 0xFFFFFFFFULL);
        pod.required_peripheral_seme_mask = static_cast<uint32_t>(desc.peripheral_seme_words[0] & 0xFFFFFFFFULL);
    }
    pod.mesh_asset_id        = mesh_asset_.is_valid() ? static_cast<uint32_t>(mesh_asset_->get_instance_id()) : 0u;
    pod.material_asset_id    = material_override_.is_valid() ? static_cast<uint32_t>(material_override_->get_instance_id()) : 0u;
    pod.priority             = static_cast<int16_t>(std::clamp(priority_, -32768, 32767));
    pod.target_extents_scale = glm::vec3(target_extents_scale_.x, target_extents_scale_.y, target_extents_scale_.z);
    pod.margin_override      = glm::vec3(margin_override_.x, margin_override_.y, margin_override_.z);
    pod.tile_repeat_density  = tile_repeat_density_;
    pod.layer                = cascade_layer_;
    pod.scale_mode           = scale_mode_;
    pod.edge_snap_policy     = edge_snap_policy_;
    pod.is_important         = is_important_ ? 1u : 0u;
    pod.specificity          = core::StructureLayoutSolver::compute_style_specificity(pod);
    return pod;
}

void Style3DResource::commit_set_semantic_role_id(int p_role_id, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamDesignPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const int old_val = static_cast<int>(semantic_role_id_);
        ur->create_action("Set Style3D Semantic Role");
        ur->add_do_method(this, "set_semantic_role_id", p_role_id);
        ur->add_undo_method(this, "set_semantic_role_id", old_val);
        ur->commit_action();
    } else {
        set_semantic_role_id(p_role_id);
    }
}

void Style3DResource::commit_set_scale_mode(int p_mode, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamDesignPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const int old_val = static_cast<int>(scale_mode_);
        ur->create_action("Set Style3D Scale Mode");
        ur->add_do_method(this, "set_scale_mode", p_mode);
        ur->add_undo_method(this, "set_scale_mode", old_val);
        ur->commit_action();
    } else {
        set_scale_mode(p_mode);
    }
}

void Style3DResource::commit_set_mesh_asset(
    const godot::Ref<godot::Mesh> &p_mesh,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamDesignPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const godot::Ref<godot::Mesh> old_mesh = mesh_asset_;
        ur->create_action("Set Style3D Mesh Asset");
        ur->add_do_method(this, "set_mesh_asset", p_mesh);
        ur->add_undo_method(this, "set_mesh_asset", old_mesh);
        ur->commit_action();
    } else {
        set_mesh_asset(p_mesh);
    }
}

// ============================================================================
// Theme3DResource
// ============================================================================

void Theme3DResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_default_layer", "layer"), &Theme3DResource::set_default_layer);
    ClassDB::bind_method(D_METHOD("get_default_layer"), &Theme3DResource::get_default_layer);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "default_layer",
            PROPERTY_HINT_ENUM,
            "ArchetypeDefault:0,BlueprintRoom:1,ThemeEra:2,DamageOrWear:3,DreamSurrealism:4,UserCustomization:5"
        ),
        "set_default_layer",
        "get_default_layer"
    );

    ClassDB::bind_method(D_METHOD("set_cascade_rules", "rules"), &Theme3DResource::set_cascade_rules);
    ClassDB::bind_method(D_METHOD("get_cascade_rules"), &Theme3DResource::get_cascade_rules);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "cascade_rules", PROPERTY_HINT_ARRAY_TYPE, "Style3DResource"),
        "set_cascade_rules",
        "get_cascade_rules"
    );

    ClassDB::bind_method(D_METHOD("add_style_rule", "style"), &Theme3DResource::add_style_rule);
    ClassDB::bind_method(D_METHOD("clear_style_rules"), &Theme3DResource::clear_style_rules);
    ClassDB::bind_method(D_METHOD("set_style_for_role", "semantic_role_id", "style"), &Theme3DResource::set_style_for_role);
    ClassDB::bind_method(D_METHOD("get_style_for_role", "semantic_role_id"), &Theme3DResource::get_style_for_role);
    ClassDB::bind_method(
        D_METHOD("resolve_cascaded_style", "candidate_role_id", "parent_role_id", "candidate_tags"),
        &Theme3DResource::resolve_cascaded_style,
        DEFVAL(0),
        DEFVAL(Variant())
    );
}

void Theme3DResource::set_default_layer(int p_layer) {
    default_layer_ = std::clamp(p_layer, 0, 5);
    emit_changed();
}

void Theme3DResource::set_cascade_rules(const godot::TypedArray<Style3DResource> &p_rules) {
    cascade_rules_ = p_rules;
    emit_changed();
}

void Theme3DResource::add_style_rule(const godot::Ref<Style3DResource> &p_style) {
    if (p_style.is_null()) return;
    cascade_rules_.push_back(p_style);
    role_to_style_map_[p_style->get_semantic_role_id()] = p_style;
    emit_changed();
}

void Theme3DResource::clear_style_rules() {
    cascade_rules_.clear();
    role_to_style_map_.clear();
    emit_changed();
}

void Theme3DResource::set_style_for_role(int p_semantic_role_id, const godot::Ref<Style3DResource> &p_style) {
    role_to_style_map_[p_semantic_role_id] = p_style;
    if (p_style.is_valid()) {
        p_style->set_semantic_role_id(p_semantic_role_id);
        if (!cascade_rules_.has(p_style)) {
            cascade_rules_.push_back(p_style);
        }
    }
    emit_changed();
}

godot::Ref<Style3DResource> Theme3DResource::get_style_for_role(int p_semantic_role_id) const {
    if (role_to_style_map_.has(p_semantic_role_id)) {
        return role_to_style_map_[p_semantic_role_id];
    }
    return resolve_cascaded_style(p_semantic_role_id);
}

godot::Ref<Style3DResource> Theme3DResource::resolve_cascaded_style(
    int p_candidate_role_id,
    int p_parent_role_id,
    const godot::Ref<SemanticTagSetResource> &p_candidate_tags
) const {
    uint32_t cand_nuc = 0u;
    uint32_t cand_per = 0u;
    if (p_candidate_tags.is_valid()) {
        const auto desc = p_candidate_tags->compile_combined_descriptor();
        cand_nuc = static_cast<uint32_t>(desc.nuclear_seme_words[0] & 0xFFFFFFFFULL);
        cand_per = static_cast<uint32_t>(desc.peripheral_seme_words[0] & 0xFFFFFFFFULL);
    }

    godot::Ref<Style3DResource> winning_style;
    core::Style3DRulePod winning_pod{};
    bool has_winner = false;

    for (int i = 0; i < cascade_rules_.size(); ++i) {
        const godot::Ref<Style3DResource> style = cascade_rules_[i];
        if (style.is_null()) continue;
        const core::Style3DRulePod pod = style->to_style_rule_pod(static_cast<uint32_t>(i));

        if (core::match_semantic_role_hierarchy(pod.semantic_role_selector, static_cast<uint32_t>(p_candidate_role_id)) == 0u) {
            continue;
        }
        if (pod.parent_role_selector != 0u &&
            core::match_semantic_role_hierarchy(pod.parent_role_selector, static_cast<uint32_t>(p_parent_role_id)) == 0u) {
            continue;
        }
        if ((cand_nuc & pod.required_nuclear_seme_mask) != pod.required_nuclear_seme_mask) continue;
        if ((cand_per & pod.required_peripheral_seme_mask) != pod.required_peripheral_seme_mask) continue;

        if (!has_winner) {
            winning_style = style;
            winning_pod   = pod;
            has_winner    = true;
            continue;
        }

        // Compare CSS cascade precedence
        const bool beats_current =
            (pod.is_important > winning_pod.is_important) ||
            (pod.is_important == winning_pod.is_important && static_cast<uint8_t>(pod.layer) > static_cast<uint8_t>(winning_pod.layer)) ||
            (pod.is_important == winning_pod.is_important && pod.layer == winning_pod.layer && pod.priority > winning_pod.priority) ||
            (pod.is_important == winning_pod.is_important && pod.layer == winning_pod.layer && pod.priority == winning_pod.priority && pod.specificity >= winning_pod.specificity);

        if (beats_current) {
            winning_style = style;
            winning_pod   = pod;
        }
    }

    return winning_style;
}

void Theme3DResource::compile_to_pod_rules(std::vector<core::Style3DRulePod> &r_out) const {
    r_out.clear();
    r_out.reserve(static_cast<size_t>(cascade_rules_.size()));
    for (int i = 0; i < cascade_rules_.size(); ++i) {
        const godot::Ref<Style3DResource> style = cascade_rules_[i];
        if (style.is_valid()) {
            r_out.push_back(style->to_style_rule_pod(static_cast<uint32_t>(i)));
        }
    }
    core::StructureLayoutSolver::sort_style_rules_by_cascade(r_out);
}

} // namespace ideam::godot_ext

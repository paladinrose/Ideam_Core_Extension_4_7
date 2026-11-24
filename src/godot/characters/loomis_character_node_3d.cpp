#include "loomis_character_node_3d.h"

#include "character_hierarchy_synchronizer.h"
#include "ideam_characters_plugin.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void LoomisCharacterNode3D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(HIERARCHY_TRANSPARENT_LIVE);
    BIND_ENUM_CONSTANT(HIERARCHY_BAKED_COMBINED);

    ClassDB::bind_method(D_METHOD("set_character_resource", "character_resource"), &LoomisCharacterNode3D::set_character_resource);
    ClassDB::bind_method(D_METHOD("get_character_resource"), &LoomisCharacterNode3D::get_character_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "character_resource", PROPERTY_HINT_RESOURCE_TYPE, "LoomisCharacter3DResource"),
        "set_character_resource",
        "get_character_resource"
    );

    ClassDB::bind_method(D_METHOD("set_primary_apparel", "apparel"), &LoomisCharacterNode3D::set_primary_apparel);
    ClassDB::bind_method(D_METHOD("get_primary_apparel"), &LoomisCharacterNode3D::get_primary_apparel);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "primary_apparel", PROPERTY_HINT_RESOURCE_TYPE, "CharacterApparelResource"),
        "set_primary_apparel",
        "get_primary_apparel"
    );

    ClassDB::bind_method(D_METHOD("set_hierarchy_mode", "mode"), &LoomisCharacterNode3D::set_hierarchy_mode);
    ClassDB::bind_method(D_METHOD("get_hierarchy_mode"), &LoomisCharacterNode3D::get_hierarchy_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "hierarchy_mode", PROPERTY_HINT_ENUM, "Transparent Live (Scene Tree):0,Baked Combined (RID):1"),
        "set_hierarchy_mode",
        "get_hierarchy_mode"
    );

    ClassDB::bind_method(D_METHOD("set_lod_level", "lod_level"), &LoomisCharacterNode3D::set_lod_level);
    ClassDB::bind_method(D_METHOD("get_lod_level"), &LoomisCharacterNode3D::get_lod_level);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lod_level", PROPERTY_HINT_RANGE, "0,3,1"), "set_lod_level", "get_lod_level");

    ClassDB::bind_method(D_METHOD("set_auto_rebuild_meshes", "auto_rebuild"), &LoomisCharacterNode3D::set_auto_rebuild_meshes);
    ClassDB::bind_method(D_METHOD("get_auto_rebuild_meshes"), &LoomisCharacterNode3D::get_auto_rebuild_meshes);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_rebuild_meshes"), "set_auto_rebuild_meshes", "get_auto_rebuild_meshes");

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &LoomisCharacterNode3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &LoomisCharacterNode3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("get_built_mesh"), &LoomisCharacterNode3D::get_built_mesh);
    ClassDB::bind_method(D_METHOD("get_built_apparel_mesh"), &LoomisCharacterNode3D::get_built_apparel_mesh);
    ClassDB::bind_method(D_METHOD("build_skeleton_3d"), &LoomisCharacterNode3D::build_skeleton_3d);
    ClassDB::bind_method(
        D_METHOD("apply_xr_trigger_grip_morphs", "trigger", "grip", "thumb"),
        &LoomisCharacterNode3D::apply_xr_trigger_grip_morphs,
        DEFVAL(-1.0f)
    );
    ClassDB::bind_method(D_METHOD("rebuild_character"), &LoomisCharacterNode3D::rebuild_character);
    ClassDB::bind_method(D_METHOD("get_character_telemetry_summary"), &LoomisCharacterNode3D::get_character_telemetry_summary);
    ClassDB::bind_method(
        D_METHOD("commit_set_character_resource", "character_resource", "undo_redo"),
        &LoomisCharacterNode3D::commit_set_character_resource,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_primary_apparel", "apparel", "undo_redo"),
        &LoomisCharacterNode3D::commit_set_primary_apparel,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(D_METHOD("_on_character_changed"), &LoomisCharacterNode3D::_on_character_changed);
}

LoomisCharacterNode3D::LoomisCharacterNode3D() {
    set_notify_transform(true);
    mesh_instance_rid_ = godot::RenderingServer::get_singleton()->instance_create();
}

LoomisCharacterNode3D::~LoomisCharacterNode3D() {
    const godot::Callable cb(this, "_on_character_changed");
    if (character_resource_.is_valid() && character_resource_->is_connected("changed", cb)) {
        character_resource_->disconnect("changed", cb);
    }
    if (primary_apparel_.is_valid() && primary_apparel_->is_connected("changed", cb)) {
        primary_apparel_->disconnect("changed", cb);
    }
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->free_rid(mesh_instance_rid_);
        mesh_instance_rid_ = godot::RID();
    }
}

void LoomisCharacterNode3D::_update_rid_visibility() {
    if (!mesh_instance_rid_.is_valid()) return;
    const bool show_rid = (hierarchy_mode_ == HIERARCHY_BAKED_COMBINED) && is_visible_in_tree();
    godot::RenderingServer::get_singleton()->instance_set_visible(mesh_instance_rid_, show_rid);
}

void LoomisCharacterNode3D::_notification(int p_what) {
    auto *rs = godot::RenderingServer::get_singleton();
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            if (mesh_instance_rid_.is_valid() && get_world_3d().is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, get_world_3d()->get_scenario());
                rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
                _update_rid_visibility();
            }
            if (auto_rebuild_meshes_) {
                rebuild_character();
            }
            break;
        }
        case NOTIFICATION_EXIT_WORLD: {
            if (mesh_instance_rid_.is_valid()) rs->instance_set_scenario(mesh_instance_rid_, godot::RID());
            break;
        }
        case NOTIFICATION_TRANSFORM_CHANGED: {
            if (mesh_instance_rid_.is_valid()) rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            _update_rid_visibility();
            break;
        }
        default:
            break;
    }
}

void LoomisCharacterNode3D::_on_character_changed() {
    if (auto_rebuild_meshes_) {
        rebuild_character();
    }
}

void LoomisCharacterNode3D::set_character_resource(const godot::Ref<LoomisCharacter3DResource> &p_resource) {
    const godot::Callable cb(this, "_on_character_changed");
    if (character_resource_.is_valid() && character_resource_->is_connected("changed", cb)) {
        character_resource_->disconnect("changed", cb);
    }
    character_resource_ = p_resource;
    if (character_resource_.is_valid()) {
        character_resource_->ensure_default_body_plan();
        if (!character_resource_->is_connected("changed", cb)) {
            character_resource_->connect("changed", cb);
        }
    }
    if (auto_rebuild_meshes_) {
        rebuild_character();
    }
}

void LoomisCharacterNode3D::set_primary_apparel(const godot::Ref<CharacterApparelResource> &p_apparel) {
    const godot::Callable cb(this, "_on_character_changed");
    if (primary_apparel_.is_valid() && primary_apparel_->is_connected("changed", cb)) {
        primary_apparel_->disconnect("changed", cb);
    }
    primary_apparel_ = p_apparel;
    if (primary_apparel_.is_valid() && !primary_apparel_->is_connected("changed", cb)) {
        primary_apparel_->connect("changed", cb);
    }
    if (auto_rebuild_meshes_) {
        rebuild_character();
    }
}

void LoomisCharacterNode3D::set_lod_level(int p_lod) {
    lod_level_ = std::clamp(p_lod, 0, 3);
    if (character_resource_.is_valid()) {
        character_resource_->set_subdivision_lod_level(lod_level_);
    }
    if (auto_rebuild_meshes_) {
        rebuild_character();
    }
}

void LoomisCharacterNode3D::set_hierarchy_mode(CharacterHierarchyMode p_mode) {
    if (hierarchy_mode_ == p_mode) return;
    hierarchy_mode_ = p_mode;
    rebuild_character();
}

void LoomisCharacterNode3D::set_auto_rebuild_meshes(bool p_auto) {
    auto_rebuild_meshes_ = p_auto;
}

void LoomisCharacterNode3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
    material_override_ = p_material;
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
    }
    if (hierarchy_mode_ == HIERARCHY_TRANSPARENT_LIVE && is_inside_tree()) {
        CharacterHierarchySynchronizer::synchronize(this, built_mesh_, material_override_);
    }
}

void LoomisCharacterNode3D::apply_xr_trigger_grip_morphs(float p_trigger, float p_grip, float p_thumb) {
    if (character_resource_.is_null()) return;
    const float trig  = std::clamp(p_trigger, 0.0f, 1.0f);
    const float grip  = std::clamp(p_grip, 0.0f, 1.0f);
    const float thumb = (p_thumb >= 0.0f) ? std::clamp(p_thumb, 0.0f, 1.0f) : grip;

    character_resource_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_0), thumb);
    character_resource_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_1), trig);
    character_resource_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_2), grip);
    character_resource_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_3), grip);
    character_resource_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_4), grip);

    if (auto_rebuild_meshes_ && is_inside_tree()) {
        rebuild_character();
    }
}

void LoomisCharacterNode3D::rebuild_character() {
    if (character_resource_.is_valid()) {
        character_resource_->ensure_default_body_plan();
        built_mesh_ = character_resource_->build_array_mesh(lod_level_);
        if (primary_apparel_.is_valid()) {
            built_apparel_mesh_ = primary_apparel_->build_refitted_apparel_mesh(character_resource_);
        } else {
            built_apparel_mesh_.unref();
        }
    } else {
        built_mesh_.unref();
        built_apparel_mesh_.unref();
    }

    if (mesh_instance_rid_.is_valid()) {
        auto *rs = godot::RenderingServer::get_singleton();
        rs->instance_set_base(mesh_instance_rid_, built_mesh_.is_valid() ? built_mesh_->get_rid() : godot::RID());
        rs->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
        _update_rid_visibility();
    }

    if (hierarchy_mode_ == HIERARCHY_TRANSPARENT_LIVE) {
        if (is_inside_tree()) {
            CharacterHierarchySynchronizer::synchronize(this, built_mesh_, material_override_);
        }
    } else {
        CharacterHierarchySynchronizer::clear_managed_nodes(this);
    }

    update_gizmos();
}

godot::Skeleton3D *LoomisCharacterNode3D::build_skeleton_3d() const {
    auto *skel = memnew(godot::Skeleton3D);
    if (!character_resource_.is_valid()) return skel;

    skel->set_name(character_resource_->get_character_name().replace(" ", "_") + godot::String("_Skeleton3D"));
    const godot::TypedArray<godot::Dictionary> plan = character_resource_->build_skeleton_export_plan();
    for (int64_t i = 0; i < plan.size(); ++i) {
        const godot::Dictionary b = plan[i];
        const int32_t bone_idx   = skel->add_bone(godot::String(b.get("bone_name", "Bone")));
        const int32_t parent_idx = int32_t(b.get("parent_bone_index", -1));
        if (parent_idx >= 0 && parent_idx < bone_idx) {
            skel->set_bone_parent(bone_idx, parent_idx);
        }
        const godot::Transform3D rest = b.get("rest_local_transform", godot::Transform3D());
        skel->set_bone_rest(bone_idx, rest);
        skel->set_bone_pose_position(bone_idx, rest.origin);
        skel->set_bone_pose_rotation(bone_idx, rest.basis.get_rotation_quaternion());
    }
    return skel;
}

godot::Dictionary LoomisCharacterNode3D::get_character_telemetry_summary() const {
    godot::Dictionary d;
    if (character_resource_.is_valid()) {
        d["character_name"]        = character_resource_->get_character_name();
        d["details_enabled"]       = character_resource_->get_details_enabled();
        d["detail_override_count"] = character_resource_->get_detail_override_count();
        d["body_archetype"]        = character_resource_->get_body_archetype();
        d["subdivision_lod_level"] = character_resource_->get_subdivision_lod_level();
        d["base_vertex_count"]     = character_resource_->get_base_vertex_count();
        d["base_quad_count"]       = character_resource_->get_base_quad_count();
        d["morph_delta_count"]     = character_resource_->get_sparse_morph_delta_count();
    }
    if (primary_apparel_.is_valid()) {
        d["apparel_normal_offset"] = primary_apparel_->get_normal_offset();
        d["apparel_xpbd_iters"]    = primary_apparel_->get_xpbd_relax_iterations();
    }
    return d;
}

void LoomisCharacterNode3D::commit_set_character_resource(
    const godot::Ref<LoomisCharacter3DResource> &p_resource,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const godot::Ref<LoomisCharacter3DResource> old_val = character_resource_;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set LoomisCharacterNode3D Resource");
        ur->add_do_property(this, "character_resource", p_resource);
        ur->add_undo_property(this, "character_resource", old_val);
        ur->commit_action();
    } else {
        set_character_resource(p_resource);
    }
}

void LoomisCharacterNode3D::commit_set_primary_apparel(
    const godot::Ref<CharacterApparelResource> &p_apparel,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const godot::Ref<CharacterApparelResource> old_val = primary_apparel_;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set LoomisCharacterNode3D Primary Apparel");
        ur->add_do_property(this, "primary_apparel", p_apparel);
        ur->add_undo_property(this, "primary_apparel", old_val);
        ur->commit_action();
    } else {
        set_primary_apparel(p_apparel);
    }
}

} // namespace ideam::godot_ext

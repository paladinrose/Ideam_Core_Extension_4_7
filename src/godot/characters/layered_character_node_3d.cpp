#include "layered_character_node_3d.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void LayeredCharacterNode3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_character_instance", "instance"), &LayeredCharacterNode3D::set_character_instance);
    ClassDB::bind_method(D_METHOD("get_character_instance"), &LayeredCharacterNode3D::get_character_instance);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "character_instance", PROPERTY_HINT_RESOURCE_TYPE, "CharacterInstanceResource"),
        "set_character_instance",
        "get_character_instance"
    );

    ClassDB::bind_method(D_METHOD("set_lod_level", "lod_level"), &LayeredCharacterNode3D::set_lod_level);
    ClassDB::bind_method(D_METHOD("get_lod_level"), &LayeredCharacterNode3D::get_lod_level);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lod_level", PROPERTY_HINT_RANGE, "0,3,1"), "set_lod_level", "get_lod_level");

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &LayeredCharacterNode3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &LayeredCharacterNode3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("get_built_mesh"), &LayeredCharacterNode3D::get_built_mesh);
    ClassDB::bind_method(D_METHOD("build_skeleton_3d"), &LayeredCharacterNode3D::build_skeleton_3d);
    ClassDB::bind_method(D_METHOD("rebuild_character"), &LayeredCharacterNode3D::rebuild_character);
    ClassDB::bind_method(D_METHOD("_on_instance_changed"), &LayeredCharacterNode3D::_on_instance_changed);
}

LayeredCharacterNode3D::LayeredCharacterNode3D() {
    set_notify_transform(true);
    mesh_instance_rid_ = godot::RenderingServer::get_singleton()->instance_create();
}

LayeredCharacterNode3D::~LayeredCharacterNode3D() {
    if (character_instance_.is_valid()) {
        const godot::Callable cb(this, "_on_instance_changed");
        if (character_instance_->is_connected("changed", cb)) character_instance_->disconnect("changed", cb);
    }
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->free_rid(mesh_instance_rid_);
        mesh_instance_rid_ = godot::RID();
    }
}

void LayeredCharacterNode3D::_notification(int p_what) {
    auto *rs = godot::RenderingServer::get_singleton();
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            if (mesh_instance_rid_.is_valid() && get_world_3d().is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, get_world_3d()->get_scenario());
                rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
                rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            }
            rebuild_character();
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
            if (mesh_instance_rid_.is_valid()) rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            break;
        }
        default:
            break;
    }
}

void LayeredCharacterNode3D::_on_instance_changed() {
    rebuild_character();
}

void LayeredCharacterNode3D::set_character_instance(const godot::Ref<CharacterInstanceResource> &p_instance) {
    const godot::Callable cb(this, "_on_instance_changed");
    if (character_instance_.is_valid() && character_instance_->is_connected("changed", cb)) {
        character_instance_->disconnect("changed", cb);
    }
    character_instance_ = p_instance;
    if (character_instance_.is_valid() && !character_instance_->is_connected("changed", cb)) {
        character_instance_->connect("changed", cb);
    }
    rebuild_character();
}

void LayeredCharacterNode3D::set_lod_level(int p_lod) {
    lod_level_ = std::clamp(p_lod, 0, 3);
    rebuild_character();
}

void LayeredCharacterNode3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
    material_override_ = p_material;
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
    }
}

void LayeredCharacterNode3D::rebuild_character() {
    if (character_instance_.is_valid()) {
        built_mesh_ = character_instance_->build_array_mesh(lod_level_);
    } else {
        built_mesh_.unref();
    }

    if (mesh_instance_rid_.is_valid()) {
        auto *rs = godot::RenderingServer::get_singleton();
        rs->instance_set_base(mesh_instance_rid_, built_mesh_.is_valid() ? built_mesh_->get_rid() : godot::RID());
        rs->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
    }
}

godot::Skeleton3D *LayeredCharacterNode3D::build_skeleton_3d() const {
    auto *skel = memnew(godot::Skeleton3D);
    if (!character_instance_.is_valid()) return skel;

    skel->set_name(character_instance_->get_character_name().replace(" ", "_") + "_Skeleton3D");
    const godot::TypedArray<godot::Dictionary> plan = character_instance_->build_skeleton_export_plan();
    for (int64_t i = 0; i < plan.size(); ++i) {
        const godot::Dictionary b = plan[i];
        const int32_t bone_idx = skel->add_bone(godot::String(b.get("bone_name", "Bone")));
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

} // namespace ideam::godot_ext

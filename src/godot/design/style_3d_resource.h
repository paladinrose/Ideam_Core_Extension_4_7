#pragma once

/**
 * style_3d_resource.h
 * `Style3DResource` and `Theme3DResource` mapping `SemanticRoleID` tags to 3D meshes,
 * edge-snap policies, and reskinning scale modes (`src/godot/design/`).
 */

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "../../core/design/design_common.h"

namespace ideam::godot_ext {

class Style3DResource : public godot::Resource {
    GDCLASS(Style3DResource, godot::Resource)

private:
    uint32_t semantic_role_id_              = core::SEMANTIC_ROLE_SURFACE;
    core::DesignScaleMode scale_mode_       = core::DesignScaleMode::UNIFORM_FIT;
    core::EdgeSnapPolicy edge_snap_policy_  = core::EdgeSnapPolicy::SNAP_WATERTIGHT;
    godot::Ref<godot::Mesh> mesh_asset_;

protected:
    static void _bind_methods();

public:
    Style3DResource() = default;
    ~Style3DResource() override = default;

    void set_semantic_role_id(int p_role_id);
    [[nodiscard]] int get_semantic_role_id() const { return static_cast<int>(semantic_role_id_); }

    void set_scale_mode(int p_mode);
    [[nodiscard]] int get_scale_mode() const { return static_cast<int>(scale_mode_); }

    void set_edge_snap_policy(int p_policy);
    [[nodiscard]] int get_edge_snap_policy() const { return static_cast<int>(edge_snap_policy_); }

    void set_mesh_asset(const godot::Ref<godot::Mesh> &p_mesh);
    [[nodiscard]] godot::Ref<godot::Mesh> get_mesh_asset() const { return mesh_asset_; }
};

class Theme3DResource : public godot::Resource {
    GDCLASS(Theme3DResource, godot::Resource)

private:
    godot::Dictionary role_to_style_map_;

protected:
    static void _bind_methods();

public:
    Theme3DResource() = default;
    ~Theme3DResource() override = default;

    void set_style_for_role(int p_semantic_role_id, const godot::Ref<Style3DResource> &p_style);
    [[nodiscard]] godot::Ref<Style3DResource> get_style_for_role(int p_semantic_role_id) const;
};

} // namespace ideam::godot_ext

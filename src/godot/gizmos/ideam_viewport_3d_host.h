#pragma once

/**
 * ideam_viewport_3d_host.h
 * Reusable 3D Studio SubViewport Host (`IdeamViewport3DHost`, `godot::SubViewportContainer`)
 * in `src/godot/gizmos/`.
 *
 * Powers Context 2 (Custom Editor 3D SubViewport inside `ToolWorkspace::center_host_`)
 * and Context 4 (Runtime Dedicated Avatar Creator / Character Modeler):
 *   - Owns an isolated `SubViewport` with dedicated `World3D`, studio `DirectionalLight3D`,
 *     and `WorldEnvironment`.
 *   - Provides smooth orbit / pan / zoom `Camera3D` rig (RMB / MMB drag + scroll wheel).
 *   - Hosts an embedded `IdeamGizmoSession` so any `IdeamGizmo` works identically inside
 *     the SubViewport with `ToolContext` undo/redo and `Theme3DResource` reskinning.
 */

#include "ideam_gizmo_backend.h"
#include "../tools/tool_context.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class IdeamViewport3DHost : public godot::SubViewportContainer {
    GDCLASS(IdeamViewport3DHost, godot::SubViewportContainer)

private:
    godot::SubViewport *sub_viewport_      = nullptr;
    godot::Node3D *stage_root_             = nullptr;
    godot::Camera3D *camera_               = nullptr;
    godot::DirectionalLight3D *key_light_  = nullptr;
    godot::WorldEnvironment *world_env_    = nullptr;
    IdeamGizmoSession *gizmo_session_      = nullptr;

    godot::Vector3 orbit_target_ = godot::Vector3(0.0f, 0.9f, 0.0f);
    real_t orbit_distance_       = static_cast<real_t>(3.2);
    real_t orbit_yaw_rad_        = static_cast<real_t>(0.35);
    real_t orbit_pitch_rad_      = static_cast<real_t>(-0.20);
    bool orbiting_               = false;
    bool panning_                = false;

    void _update_camera_transform();

protected:
    static void _bind_methods();

public:
    IdeamViewport3DHost();
    ~IdeamViewport3DHost() override = default;

    void _gui_input(const godot::Ref<godot::InputEvent> &p_event) override;

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const;

    [[nodiscard]] godot::SubViewport *get_sub_viewport() const noexcept { return sub_viewport_; }
    [[nodiscard]] godot::Node3D *get_stage_root() const noexcept { return stage_root_; }
    [[nodiscard]] godot::Camera3D *get_camera() const noexcept { return camera_; }
    [[nodiscard]] IdeamGizmoSession *get_gizmo_session() const noexcept { return gizmo_session_; }

    void set_orbit_target(const godot::Vector3 &p_target);
    [[nodiscard]] godot::Vector3 get_orbit_target() const noexcept { return orbit_target_; }

    void set_orbit_distance(real_t p_distance);
    [[nodiscard]] real_t get_orbit_distance() const noexcept { return orbit_distance_; }

    void reset_camera();
    void attach_preview_node(godot::Node3D *p_node, bool p_set_as_gizmo_target = true);
};

} // namespace ideam::godot_ext

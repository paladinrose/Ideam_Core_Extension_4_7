#include "ideam_viewport_3d_host.h"

#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/procedural_sky_material.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void IdeamViewport3DHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &IdeamViewport3DHost::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &IdeamViewport3DHost::get_tool_context);
    ClassDB::bind_method(D_METHOD("get_sub_viewport"), &IdeamViewport3DHost::get_sub_viewport);
    ClassDB::bind_method(D_METHOD("get_stage_root"), &IdeamViewport3DHost::get_stage_root);
    ClassDB::bind_method(D_METHOD("get_camera"), &IdeamViewport3DHost::get_camera);
    ClassDB::bind_method(D_METHOD("get_gizmo_session"), &IdeamViewport3DHost::get_gizmo_session);
    ClassDB::bind_method(D_METHOD("set_orbit_target", "target"), &IdeamViewport3DHost::set_orbit_target);
    ClassDB::bind_method(D_METHOD("get_orbit_target"), &IdeamViewport3DHost::get_orbit_target);
    ClassDB::bind_method(D_METHOD("set_orbit_distance", "distance"), &IdeamViewport3DHost::set_orbit_distance);
    ClassDB::bind_method(D_METHOD("get_orbit_distance"), &IdeamViewport3DHost::get_orbit_distance);
    ClassDB::bind_method(D_METHOD("reset_camera"), &IdeamViewport3DHost::reset_camera);
    ClassDB::bind_method(
        D_METHOD("attach_preview_node", "node", "set_as_gizmo_target"),
        &IdeamViewport3DHost::attach_preview_node,
        DEFVAL(true)
    );
}

IdeamViewport3DHost::IdeamViewport3DHost() {
    set_name("IdeamViewport3DHost");
    set_stretch(true);
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);
    set_focus_mode(Control::FOCUS_CLICK);

    sub_viewport_ = memnew(godot::SubViewport);
    sub_viewport_->set_name("StudioSubViewport");
    sub_viewport_->set_use_own_world_3d(true);
    sub_viewport_->set_handle_input_locally(true);
    sub_viewport_->set_msaa_3d(godot::Viewport::MSAA_4X);
    add_child(sub_viewport_);

    stage_root_ = memnew(godot::Node3D);
    stage_root_->set_name("StageRoot");
    sub_viewport_->add_child(stage_root_);

    // Studio Environment & Key Light
    world_env_ = memnew(godot::WorldEnvironment);
    world_env_->set_name("StudioEnvironment");
    godot::Ref<godot::Environment> env;
    env.instantiate();
    env->set_background(godot::Environment::BG_SKY);
    godot::Ref<godot::Sky> sky;
    sky.instantiate();
    godot::Ref<godot::ProceduralSkyMaterial> sky_mat;
    sky_mat.instantiate();
    sky->set_material(sky_mat);
    env->set_sky(sky);
    env->set_ambient_source(godot::Environment::AMBIENT_SOURCE_SKY);
    world_env_->set_environment(env);
    sub_viewport_->add_child(world_env_);

    key_light_ = memnew(godot::DirectionalLight3D);
    key_light_->set_name("KeyLight");
    key_light_->set_rotation_degrees(godot::Vector3(-42.0f, 35.0f, 0.0f));
    key_light_->set_shadow(true);
    sub_viewport_->add_child(key_light_);

    camera_ = memnew(godot::Camera3D);
    camera_->set_name("OrbitCamera3D");
    camera_->set_fov(45.0f);
    camera_->set_near(0.05f);
    camera_->set_far(200.0f);
    sub_viewport_->add_child(camera_);
    camera_->make_current();

    gizmo_session_ = memnew(IdeamGizmoSession);
    gizmo_session_->set_name("IdeamGizmoSession");
    gizmo_session_->set_camera(camera_);
    sub_viewport_->add_child(gizmo_session_);

    _update_camera_transform();
}

void IdeamViewport3DHost::_update_camera_transform() {
    if (!camera_) return;
    const real_t pitch = std::clamp(orbit_pitch_rad_, static_cast<real_t>(-1.45), static_cast<real_t>(1.45));
    const real_t cp    = std::cos(pitch);
    const godot::Vector3 offset(
        std::sin(orbit_yaw_rad_) * cp * orbit_distance_,
        -std::sin(pitch) * orbit_distance_,
        std::cos(orbit_yaw_rad_) * cp * orbit_distance_
    );
    camera_->set_position(orbit_target_ + offset);
    camera_->look_at(orbit_target_, godot::Vector3(0.0f, 1.0f, 0.0f));
}

void IdeamViewport3DHost::set_tool_context(const godot::Ref<ToolContext> &p_context) {
    if (gizmo_session_) {
        gizmo_session_->set_tool_context(p_context);
    }
}

godot::Ref<ToolContext> IdeamViewport3DHost::get_tool_context() const {
    return gizmo_session_ ? gizmo_session_->get_tool_context() : godot::Ref<ToolContext>();
}

void IdeamViewport3DHost::set_orbit_target(const godot::Vector3 &p_target) {
    orbit_target_ = p_target;
    _update_camera_transform();
}

void IdeamViewport3DHost::set_orbit_distance(real_t p_distance) {
    orbit_distance_ = std::clamp(p_distance, static_cast<real_t>(0.35), static_cast<real_t>(40.0));
    _update_camera_transform();
}

void IdeamViewport3DHost::reset_camera() {
    orbit_target_   = godot::Vector3(0.0f, 0.9f, 0.0f);
    orbit_distance_ = static_cast<real_t>(3.2);
    orbit_yaw_rad_  = static_cast<real_t>(0.35);
    orbit_pitch_rad_ = static_cast<real_t>(-0.20);
    _update_camera_transform();
}

void IdeamViewport3DHost::attach_preview_node(godot::Node3D *p_node, bool p_set_as_gizmo_target) {
    if (!p_node || !stage_root_) return;
    if (p_node->get_parent() != stage_root_) {
        if (p_node->get_parent()) {
            p_node->get_parent()->remove_child(p_node);
        }
        stage_root_->add_child(p_node);
    }
    if (p_set_as_gizmo_target && gizmo_session_) {
        gizmo_session_->set_target_node(p_node);
    }
}

void IdeamViewport3DHost::_gui_input(const godot::Ref<godot::InputEvent> &p_event) {
    if (p_event.is_null()) return;

    // 1. Give embedded IdeamGizmoSession first chance at handle picking/dragging & shortcuts
    if (gizmo_session_ && gizmo_session_->handle_input_event(p_event)) {
        accept_event();
        return;
    }

    // 2. Studio Orbit / Pan / Zoom controls
    if (auto *mb = godot::Object::cast_to<godot::InputEventMouseButton>(p_event.ptr())) {
        if (mb->get_button_index() == godot::MOUSE_BUTTON_RIGHT) {
            orbiting_ = mb->is_pressed();
            accept_event();
            return;
        }
        if (mb->get_button_index() == godot::MOUSE_BUTTON_MIDDLE) {
            if (mb->is_shift_pressed()) {
                panning_  = mb->is_pressed();
                orbiting_ = false;
            } else {
                orbiting_ = mb->is_pressed();
                panning_  = false;
            }
            accept_event();
            return;
        }
        if (mb->is_pressed() && mb->get_button_index() == godot::MOUSE_BUTTON_WHEEL_UP) {
            set_orbit_distance(orbit_distance_ * static_cast<real_t>(0.88));
            accept_event();
            return;
        }
        if (mb->is_pressed() && mb->get_button_index() == godot::MOUSE_BUTTON_WHEEL_DOWN) {
            set_orbit_distance(orbit_distance_ * static_cast<real_t>(1.14));
            accept_event();
            return;
        }
    }

    if (auto *mm = godot::Object::cast_to<godot::InputEventMouseMotion>(p_event.ptr())) {
        if (orbiting_) {
            orbit_yaw_rad_   -= mm->get_relative().x * static_cast<real_t>(0.008);
            orbit_pitch_rad_  = std::clamp(
                orbit_pitch_rad_ - mm->get_relative().y * static_cast<real_t>(0.008),
                static_cast<real_t>(-1.45),
                static_cast<real_t>(1.45)
            );
            _update_camera_transform();
            accept_event();
            return;
        }
        if (panning_ && camera_) {
            const godot::Basis b = camera_->get_global_transform().basis;
            const real_t scale   = orbit_distance_ * static_cast<real_t>(0.002);
            orbit_target_ -= b.get_column(0) * (mm->get_relative().x * scale);
            orbit_target_ += b.get_column(1) * (mm->get_relative().y * scale);
            _update_camera_transform();
            accept_event();
            return;
        }
    }
}

} // namespace ideam::godot_ext

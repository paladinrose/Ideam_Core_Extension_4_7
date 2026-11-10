#include "geometry_task_host.h"

#include <godot_cpp/classes/engine.hpp>

namespace ideam::godot_ext {

void GeometryTaskHost::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Geometry Task Host", "");

    ClassDB::bind_method(D_METHOD("set_geometry_resource", "geometry"), &GeometryTaskHost::set_geometry_resource);
    ClassDB::bind_method(D_METHOD("get_geometry_resource"), &GeometryTaskHost::get_geometry_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "geometry_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedGeometry"),
        "set_geometry_resource",
        "get_geometry_resource"
    );

    ClassDB::bind_method(D_METHOD("set_shared_host_node_path", "path"), &GeometryTaskHost::set_shared_host_node_path);
    ClassDB::bind_method(D_METHOD("get_shared_host_node_path"), &GeometryTaskHost::get_shared_host_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "shared_host_node_path"),
        "set_shared_host_node_path",
        "get_shared_host_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_auto_setup_on_ready", "auto_setup"), &GeometryTaskHost::set_auto_setup_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_setup_on_ready"), &GeometryTaskHost::get_auto_setup_on_ready);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_setup_on_ready"),
        "set_auto_setup_on_ready",
        "get_auto_setup_on_ready"
    );

    ClassDB::bind_method(D_METHOD("set_auto_execute_in_process", "auto_execute"), &GeometryTaskHost::set_auto_execute_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_execute_in_process"), &GeometryTaskHost::get_auto_execute_in_process);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_execute_in_process"),
        "set_auto_execute_in_process",
        "get_auto_execute_in_process"
    );

    ClassDB::bind_method(D_METHOD("set_share_host_memory", "share"), &GeometryTaskHost::set_share_host_memory);
    ClassDB::bind_method(D_METHOD("get_share_host_memory"), &GeometryTaskHost::get_share_host_memory);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "share_host_memory"),
        "set_share_host_memory",
        "get_share_host_memory"
    );

    ClassDB::bind_method(D_METHOD("setup_geometry_host"), &GeometryTaskHost::setup_geometry_host);
    ClassDB::bind_method(D_METHOD("execute_geometry_step", "delta"), &GeometryTaskHost::execute_geometry_step);
    ClassDB::bind_method(D_METHOD("bind_shared_task_host", "other_host"), &GeometryTaskHost::bind_shared_task_host);
}

void GeometryTaskHost::_ready() {
    TaskGraphHost::_ready();
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_setup_on_ready_) {
        setup_geometry_host();
    }
}

void GeometryTaskHost::_process(double p_delta) {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_execute_in_process_) {
        execute_geometry_step(p_delta);
    }
}

void GeometryTaskHost::set_geometry_resource(const godot::Ref<DataOrientedGeometry> &p_geometry) {
    geometry_resource_ = p_geometry;
}

void GeometryTaskHost::set_shared_host_node_path(const godot::NodePath &p_path) {
    shared_host_node_path_ = p_path;
}

void GeometryTaskHost::set_auto_setup_on_ready(bool p_auto_setup) {
    auto_setup_on_ready_ = p_auto_setup;
}

void GeometryTaskHost::set_auto_execute_in_process(bool p_auto_execute) {
    auto_execute_in_process_ = p_auto_execute;
}

void GeometryTaskHost::set_share_host_memory(bool p_share) {
    share_host_memory_ = p_share;
}

void GeometryTaskHost::_wire_shared_host_if_present() {
    if (shared_host_node_path_.is_empty() || !is_inside_tree()) return;
    godot::Node *n = get_node_or_null(shared_host_node_path_);
    if (TaskGraphHost *other = godot::Object::cast_to<TaskGraphHost>(n)) {
        bind_shared_task_host(other);
    }
}

void GeometryTaskHost::setup_geometry_host() {
    if (geometry_resource_.is_null()) return;

    if (!shared_host_node_path_.is_empty() && share_host_memory_) {
        _wire_shared_host_if_present();
        return;
    }

    geometry_resource_->setup_dod_pipeline();
    memory_manager = geometry_resource_->get_memory_manager_shared();
    task_graph     = geometry_resource_->get_active_task_graph_shared();
}

void GeometryTaskHost::execute_geometry_step(double p_delta) {
    if (geometry_resource_.is_valid()) {
        geometry_resource_->execute_dod_step(p_delta);
    }
}

bool GeometryTaskHost::bind_shared_task_host(TaskGraphHost *p_other_host) {
    if (!p_other_host || geometry_resource_.is_null()) return false;
    std::shared_ptr<core::MemoryManagerDOD> shared_mgr = p_other_host->get_memory_manager();
    if (!shared_mgr) return false;

    geometry_resource_->setup_dod_pipeline_with_manager(shared_mgr, nullptr);
    memory_manager = geometry_resource_->get_memory_manager_shared();
    task_graph     = geometry_resource_->get_active_task_graph_shared();
    return true;
}

} // namespace ideam::godot_ext

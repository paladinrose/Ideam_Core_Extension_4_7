#include "mind_task_host.h"

#include <godot_cpp/classes/engine.hpp>

namespace ideam::godot_ext {

void MindTaskHost::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Mind Task Host", "");

    ClassDB::bind_method(D_METHOD("set_mind_resource", "mind"), &MindTaskHost::set_mind_resource);
    ClassDB::bind_method(D_METHOD("get_mind_resource"), &MindTaskHost::get_mind_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "mind_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedMind"),
        "set_mind_resource",
        "get_mind_resource"
    );

    ClassDB::bind_method(D_METHOD("set_shared_host_node_path", "path"), &MindTaskHost::set_shared_host_node_path);
    ClassDB::bind_method(D_METHOD("get_shared_host_node_path"), &MindTaskHost::get_shared_host_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "shared_host_node_path"),
        "set_shared_host_node_path",
        "get_shared_host_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_auto_setup_on_ready", "auto_setup"), &MindTaskHost::set_auto_setup_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_setup_on_ready"), &MindTaskHost::get_auto_setup_on_ready);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_setup_on_ready"),
        "set_auto_setup_on_ready",
        "get_auto_setup_on_ready"
    );

    ClassDB::bind_method(D_METHOD("set_auto_execute_in_process", "auto_execute"), &MindTaskHost::set_auto_execute_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_execute_in_process"), &MindTaskHost::get_auto_execute_in_process);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_execute_in_process"),
        "set_auto_execute_in_process",
        "get_auto_execute_in_process"
    );

    ClassDB::bind_method(D_METHOD("set_share_host_memory", "share"), &MindTaskHost::set_share_host_memory);
    ClassDB::bind_method(D_METHOD("get_share_host_memory"), &MindTaskHost::get_share_host_memory);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "share_host_memory"),
        "set_share_host_memory",
        "get_share_host_memory"
    );

    ClassDB::bind_method(D_METHOD("setup_mind_host"), &MindTaskHost::setup_mind_host);
    ClassDB::bind_method(D_METHOD("execute_mind_step", "delta"), &MindTaskHost::execute_mind_step);
    ClassDB::bind_method(D_METHOD("bind_shared_task_host", "other_host"), &MindTaskHost::bind_shared_task_host);
}

void MindTaskHost::_ready() {
    TaskGraphHost::_ready();
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_setup_on_ready_) {
        setup_mind_host();
    }
}

void MindTaskHost::_process(double p_delta) {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_execute_in_process_) {
        execute_mind_step(p_delta);
    }
}

void MindTaskHost::set_mind_resource(const godot::Ref<DataOrientedMind> &p_mind) {
    mind_resource_ = p_mind;
}

void MindTaskHost::set_shared_host_node_path(const godot::NodePath &p_path) {
    shared_host_node_path_ = p_path;
}

void MindTaskHost::set_auto_setup_on_ready(bool p_auto_setup) {
    auto_setup_on_ready_ = p_auto_setup;
}

void MindTaskHost::set_auto_execute_in_process(bool p_auto_execute) {
    auto_execute_in_process_ = p_auto_execute;
}

void MindTaskHost::set_share_host_memory(bool p_share) {
    share_host_memory_ = p_share;
}

void MindTaskHost::_wire_shared_host_if_present() {
    if (shared_host_node_path_.is_empty() || !is_inside_tree()) return;
    godot::Node *n = get_node_or_null(shared_host_node_path_);
    if (TaskGraphHost *other = godot::Object::cast_to<TaskGraphHost>(n)) {
        bind_shared_task_host(other);
    }
}

void MindTaskHost::setup_mind_host() {
    if (mind_resource_.is_null()) return;

    if (!shared_host_node_path_.is_empty() && share_host_memory_) {
        _wire_shared_host_if_present();
        return;
    }

    mind_resource_->setup_dod_pipeline();
    memory_manager = mind_resource_->get_memory_manager_shared();
    task_graph     = mind_resource_->get_active_task_graph_shared();
}

void MindTaskHost::execute_mind_step(double p_delta) {
    if (mind_resource_.is_valid()) {
        mind_resource_->execute_dod_step(p_delta);
    }
}

bool MindTaskHost::bind_shared_task_host(TaskGraphHost *p_other_host) {
    if (!p_other_host || mind_resource_.is_null()) return false;
    std::shared_ptr<core::MemoryManagerDOD> shared_mgr = p_other_host->get_memory_manager();
    if (!shared_mgr) return false;

    mind_resource_->setup_dod_pipeline_with_manager(shared_mgr, nullptr);
    memory_manager = mind_resource_->get_memory_manager_shared();
    task_graph     = mind_resource_->get_active_task_graph_shared();
    return true;
}

} // namespace ideam::godot_ext

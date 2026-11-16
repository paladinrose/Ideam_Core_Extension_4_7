#include "linguistics_task_host.h"
#include <godot_cpp/classes/engine.hpp>

namespace ideam::godot_ext {

void LinguisticsTaskHost::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Linguistics Task Host", "");

    ClassDB::bind_method(D_METHOD("set_domain_resource", "resource"), &LinguisticsTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &LinguisticsTaskHost::get_domain_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedLinguistics"),
        "set_domain_resource",
        "get_domain_resource"
    );

    ClassDB::bind_method(D_METHOD("set_shared_host_node_path", "path"), &LinguisticsTaskHost::set_shared_host_node_path);
    ClassDB::bind_method(D_METHOD("get_shared_host_node_path"), &LinguisticsTaskHost::get_shared_host_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "shared_host_node_path"),
        "set_shared_host_node_path",
        "get_shared_host_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_auto_setup_on_ready", "auto_setup"), &LinguisticsTaskHost::set_auto_setup_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_setup_on_ready"), &LinguisticsTaskHost::get_auto_setup_on_ready);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_setup_on_ready"),
        "set_auto_setup_on_ready",
        "get_auto_setup_on_ready"
    );

    ClassDB::bind_method(D_METHOD("set_auto_execute_in_process", "auto_execute"), &LinguisticsTaskHost::set_auto_execute_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_execute_in_process"), &LinguisticsTaskHost::get_auto_execute_in_process);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_execute_in_process"),
        "set_auto_execute_in_process",
        "get_auto_execute_in_process"
    );

    ClassDB::bind_method(D_METHOD("set_share_host_memory", "share"), &LinguisticsTaskHost::set_share_host_memory);
    ClassDB::bind_method(D_METHOD("get_share_host_memory"), &LinguisticsTaskHost::get_share_host_memory);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "share_host_memory"),
        "set_share_host_memory",
        "get_share_host_memory"
    );

    ClassDB::bind_method(D_METHOD("setup_linguistics_host"), &LinguisticsTaskHost::setup_linguistics_host);
    ClassDB::bind_method(D_METHOD("execute_linguistics_step", "delta"), &LinguisticsTaskHost::execute_linguistics_step);
    ClassDB::bind_method(D_METHOD("bind_shared_task_host", "other_host"), &LinguisticsTaskHost::bind_shared_task_host);
}

void LinguisticsTaskHost::_ready() {
    TaskGraphHost::_ready();
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_setup_on_ready_) {
        setup_linguistics_host();
    }
}

void LinguisticsTaskHost::_process(double p_delta) {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_execute_in_process_) {
        execute_linguistics_step(p_delta);
    }
}

void LinguisticsTaskHost::_wire_shared_host_if_present() {
    if (shared_host_node_path_.is_empty() || !is_inside_tree()) return;
    godot::Node *n = get_node_or_null(shared_host_node_path_);
    if (TaskGraphHost *other = godot::Object::cast_to<TaskGraphHost>(n)) {
        bind_shared_task_host(other);
    }
}

void LinguisticsTaskHost::setup_linguistics_host() {
    if (domain_resource_.is_null()) return;

    if (!shared_host_node_path_.is_empty() && share_host_memory_) {
        _wire_shared_host_if_present();
        return;
    }

    domain_resource_->setup_dod_pipeline();
    active_manager = domain_resource_->get_memory_manager_shared();
    active_graph   = domain_resource_->get_active_task_graph_shared();
}

void LinguisticsTaskHost::execute_linguistics_step(double p_delta) {
    if (domain_resource_.is_valid()) {
        domain_resource_->execute_dod_step(p_delta);
    }
}

bool LinguisticsTaskHost::bind_shared_task_host(TaskGraphHost *p_other_host) {
    if (!p_other_host || domain_resource_.is_null()) return false;
    std::shared_ptr<core::MemoryManagerDOD> shared_mgr = p_other_host->get_memory_manager();
    if (!shared_mgr) return false;

    domain_resource_->setup_dod_pipeline_with_manager(shared_mgr, nullptr);
    active_manager = domain_resource_->get_memory_manager_shared();
    active_graph   = domain_resource_->get_active_task_graph_shared();
    return true;
}

} // namespace ideam::godot_ext

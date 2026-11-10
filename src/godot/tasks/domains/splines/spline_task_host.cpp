#include "spline_task_host.h"

#include <godot_cpp/classes/engine.hpp>

namespace ideam::godot_ext {

void SplineTaskHost::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Spline Task Host", "");

    ClassDB::bind_method(D_METHOD("set_splines_resource", "splines"), &SplineTaskHost::set_splines_resource);
    ClassDB::bind_method(D_METHOD("get_splines_resource"), &SplineTaskHost::get_splines_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "splines_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedSplines"),
        "set_splines_resource",
        "get_splines_resource"
    );

    ClassDB::bind_method(D_METHOD("set_geometry_host_node_path", "path"), &SplineTaskHost::set_geometry_host_node_path);
    ClassDB::bind_method(D_METHOD("get_geometry_host_node_path"), &SplineTaskHost::get_geometry_host_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "geometry_host_node_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "GeometryTaskHost"),
        "set_geometry_host_node_path",
        "get_geometry_host_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_auto_setup_on_ready", "auto_setup"), &SplineTaskHost::set_auto_setup_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_setup_on_ready"), &SplineTaskHost::get_auto_setup_on_ready);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_setup_on_ready"),
        "set_auto_setup_on_ready",
        "get_auto_setup_on_ready"
    );

    ClassDB::bind_method(D_METHOD("set_auto_execute_in_process", "auto_execute"), &SplineTaskHost::set_auto_execute_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_execute_in_process"), &SplineTaskHost::get_auto_execute_in_process);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_execute_in_process"),
        "set_auto_execute_in_process",
        "get_auto_execute_in_process"
    );

    ClassDB::bind_method(D_METHOD("set_share_geometry_memory", "share"), &SplineTaskHost::set_share_geometry_memory);
    ClassDB::bind_method(D_METHOD("get_share_geometry_memory"), &SplineTaskHost::get_share_geometry_memory);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "share_geometry_memory"),
        "set_share_geometry_memory",
        "get_share_geometry_memory"
    );

    ClassDB::bind_method(D_METHOD("setup_spline_host"), &SplineTaskHost::setup_spline_host);
    ClassDB::bind_method(D_METHOD("execute_spline_step", "delta"), &SplineTaskHost::execute_spline_step);
    ClassDB::bind_method(D_METHOD("bind_geometry_task_host", "geometry_host"), &SplineTaskHost::bind_geometry_task_host);
}

void SplineTaskHost::_ready() {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_setup_on_ready_) {
        setup_spline_host();
    }
}

void SplineTaskHost::_process(double p_delta) {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_execute_in_process_) {
        execute_spline_step(p_delta);
    }
}

void SplineTaskHost::set_splines_resource(const godot::Ref<DataOrientedSplines> &p_splines) {
    splines_resource_ = p_splines;
}

void SplineTaskHost::set_geometry_host_node_path(const godot::NodePath &p_path) {
    geometry_host_node_path_ = p_path;
}

void SplineTaskHost::set_auto_setup_on_ready(bool p_auto_setup) {
    auto_setup_on_ready_ = p_auto_setup;
}

void SplineTaskHost::set_auto_execute_in_process(bool p_auto_execute) {
    auto_execute_in_process_ = p_auto_execute;
}

void SplineTaskHost::set_share_geometry_memory(bool p_share) {
    share_geometry_memory_ = p_share;
}

void SplineTaskHost::_wire_geometry_host_if_present() {
    if (geometry_host_node_path_.is_empty() || !is_inside_tree()) return;
    godot::Node *n = get_node_or_null(geometry_host_node_path_);
    if (GeometryTaskHost *geom_host = godot::Object::cast_to<GeometryTaskHost>(n)) {
        bind_geometry_task_host(geom_host);
    }
}

void SplineTaskHost::setup_spline_host() {
    if (graph_resource.is_valid()) {
        TaskGraphHost::setup();
    }
    if (splines_resource_.is_null()) return;

    if (!geometry_host_node_path_.is_empty() && share_geometry_memory_) {
        _wire_geometry_host_if_present();
        if (active_manager) return;
    }

    splines_resource_->setup_dod_pipeline_with_manager(active_manager, active_graph);
    active_manager = splines_resource_->get_memory_manager_shared();
    active_graph   = splines_resource_->get_active_task_graph_shared();
}

void SplineTaskHost::execute_spline_step(double p_delta) {
    if (splines_resource_.is_valid()) {
        splines_resource_->execute_dod_step(p_delta);
        if (graph_resource.is_valid() && active_graph) {
            graph_resource->sync_execution_snapshots(active_graph, ui_to_dod_map);
        }
    } else if (is_ready()) {
        TaskGraphHost::execute_graph(p_delta);
    }
}

bool SplineTaskHost::bind_geometry_task_host(GeometryTaskHost *p_geometry_host) {
    if (!p_geometry_host || splines_resource_.is_null()) return false;
    if (!p_geometry_host->is_ready() && p_geometry_host->get_geometry_resource().is_valid()) {
        p_geometry_host->setup_geometry_host();
    }

    if (p_geometry_host->get_geometry_resource().is_valid()) {
        splines_resource_->bind_data_oriented_geometry(p_geometry_host->get_geometry_resource());
    } else if (p_geometry_host->get_active_manager()) {
        splines_resource_->setup_dod_pipeline_with_manager(
            p_geometry_host->get_active_manager(),
            p_geometry_host->get_active_graph()
        );
    }

    active_manager = splines_resource_->get_memory_manager_shared();
    active_graph   = splines_resource_->get_active_task_graph_shared();
    return active_manager != nullptr;
}

} // namespace ideam::godot_ext

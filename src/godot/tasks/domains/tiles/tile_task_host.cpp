#include "tile_task_host.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void TileTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tiling_resource", "tiling"), &TileTaskHost::set_tiling_resource);
    ClassDB::bind_method(D_METHOD("get_tiling_resource"), &TileTaskHost::get_tiling_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tiling_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedTiling"),
        "set_tiling_resource",
        "get_tiling_resource"
    );

    ClassDB::bind_method(D_METHOD("set_geometry_host_node_path", "path"), &TileTaskHost::set_geometry_host_node_path);
    ClassDB::bind_method(D_METHOD("get_geometry_host_node_path"), &TileTaskHost::get_geometry_host_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "geometry_host_node_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "GeometryTaskHost"),
        "set_geometry_host_node_path",
        "get_geometry_host_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_spline_host_node_path", "path"), &TileTaskHost::set_spline_host_node_path);
    ClassDB::bind_method(D_METHOD("get_spline_host_node_path"), &TileTaskHost::get_spline_host_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "spline_host_node_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "SplineTaskHost"),
        "set_spline_host_node_path",
        "get_spline_host_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_auto_setup_on_ready", "auto_setup"), &TileTaskHost::set_auto_setup_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_setup_on_ready"), &TileTaskHost::get_auto_setup_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_setup_on_ready"), "set_auto_setup_on_ready", "get_auto_setup_on_ready");

    ClassDB::bind_method(D_METHOD("set_auto_execute_in_process", "auto_execute"), &TileTaskHost::set_auto_execute_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_execute_in_process"), &TileTaskHost::get_auto_execute_in_process);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_execute_in_process"), "set_auto_execute_in_process", "get_auto_execute_in_process");

    ClassDB::bind_method(D_METHOD("set_share_geometry_memory", "share"), &TileTaskHost::set_share_geometry_memory);
    ClassDB::bind_method(D_METHOD("get_share_geometry_memory"), &TileTaskHost::get_share_geometry_memory);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "share_geometry_memory"), "set_share_geometry_memory", "get_share_geometry_memory");

    ClassDB::bind_method(D_METHOD("setup_tile_host"), &TileTaskHost::setup_tile_host);
    ClassDB::bind_method(D_METHOD("execute_tile_step", "delta"), &TileTaskHost::execute_tile_step);
    ClassDB::bind_method(D_METHOD("bind_geometry_task_host", "geometry_host"), &TileTaskHost::bind_geometry_task_host);
    ClassDB::bind_method(D_METHOD("bind_spline_task_host", "spline_host"), &TileTaskHost::bind_spline_task_host);
}

void TileTaskHost::set_tiling_resource(const godot::Ref<DataOrientedTiling> &p_tiling) {
    tiling_resource_ = p_tiling;
}

void TileTaskHost::set_geometry_host_node_path(const godot::NodePath &p_path) {
    geometry_host_node_path_ = p_path;
}

void TileTaskHost::set_spline_host_node_path(const godot::NodePath &p_path) {
    spline_host_node_path_ = p_path;
}

void TileTaskHost::set_auto_setup_on_ready(bool p_auto_setup) {
    auto_setup_on_ready_ = p_auto_setup;
}

void TileTaskHost::set_auto_execute_in_process(bool p_auto_execute) {
    auto_execute_in_process_ = p_auto_execute;
    set_process(auto_execute_in_process_);
}

void TileTaskHost::set_share_geometry_memory(bool p_share) {
    share_geometry_memory_ = p_share;
}

void TileTaskHost::_ready() {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_setup_on_ready_) {
        setup_tile_host();
    }
    set_process(auto_execute_in_process_);
}

void TileTaskHost::_process(double p_delta) {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_execute_in_process_) {
        execute_tile_step(p_delta);
    }
}

void TileTaskHost::_wire_external_hosts_if_present() {
    if (!geometry_host_node_path_.is_empty() && is_inside_tree()) {
        if (godot::Node *n = get_node_or_null(geometry_host_node_path_)) {
            if (auto *geom_host = godot::Object::cast_to<GeometryTaskHost>(n)) {
                bind_geometry_task_host(geom_host);
            }
        }
    }
    if (!spline_host_node_path_.is_empty() && is_inside_tree()) {
        if (godot::Node *n = get_node_or_null(spline_host_node_path_)) {
            if (auto *sp_host = godot::Object::cast_to<SplineTaskHost>(n)) {
                bind_spline_task_host(sp_host);
            }
        }
    }
}

void TileTaskHost::setup_tile_host() {
    if (tiling_resource_.is_null()) return;

    _wire_external_hosts_if_present();

    if (active_manager) {
        tiling_resource_->setup_dod_pipeline_with_manager(active_manager, active_graph);
    } else {
        tiling_resource_->setup_dod_pipeline();
        active_manager = tiling_resource_->get_memory_manager_shared();
        active_graph   = tiling_resource_->get_active_task_graph_shared();
    }
}

void TileTaskHost::execute_tile_step(double p_delta) {
    if (tiling_resource_.is_valid()) {
        tiling_resource_->execute_dod_step(p_delta);
    }
}

bool TileTaskHost::bind_geometry_task_host(GeometryTaskHost *p_geometry_host) {
    if (!p_geometry_host || tiling_resource_.is_null()) return false;
    godot::Ref<DataOrientedGeometry> geom_res = p_geometry_host->get_geometry_resource();
    if (geom_res.is_null()) return false;

    if (!geom_res->get_geometry_dod()) {
        p_geometry_host->setup_geometry_host();
    }
    if (share_geometry_memory_) {
        active_manager = geom_res->get_memory_manager_shared();
        active_graph   = geom_res->get_active_task_graph_shared();
    }
    return tiling_resource_->bind_data_oriented_geometry(geom_res);
}

bool TileTaskHost::bind_spline_task_host(SplineTaskHost *p_spline_host) {
    if (!p_spline_host || tiling_resource_.is_null()) return false;
    godot::Ref<DataOrientedSplines> sp_res = p_spline_host->get_splines_resource();
    if (sp_res.is_null()) return false;

    if (!sp_res->get_spline_dod()) {
        p_spline_host->setup_spline_host();
    }
    if (share_geometry_memory_ && !active_manager) {
        active_manager = sp_res->get_memory_manager_shared();
        active_graph   = sp_res->get_active_task_graph_shared();
    }
    return tiling_resource_->bind_data_oriented_splines(sp_res);
}

} // namespace ideam::godot_ext

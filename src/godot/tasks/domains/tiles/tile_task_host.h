#pragma once

/**
 * tile_task_host.h
 * SceneTree Node host for `DataOrientedTiling` in `src/godot/tasks/domains/tiles/`.
 *
 * Extends `ideam::godot_ext::TaskGraphHost` so a `DataOrientedTiling` Resource can
 * execute inside the Godot SceneTree, automatically bind to a `GeometryTaskHost`
 * and/or `SplineTaskHost` (sharing `MemoryManagerDOD`, `TaskGraphDOD`, and
 * `GeometryDOD`'s polyline and polygon mesh buffers via `bind_external_geometry_buffers`),
 * and drive frame-by-frame or on-demand tiling evaluation and A* / flow-field navigation.
 */

#include "data_oriented_tiling.h"
#include "../geometry/geometry_task_host.h"
#include "../splines/spline_task_host.h"
#include "../../task_graph_host.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace ideam::godot_ext {

class TileTaskHost : public TaskGraphHost {
    GDCLASS(TileTaskHost, TaskGraphHost)

protected:
    static void _bind_methods();

private:
    godot::Ref<DataOrientedTiling> tiling_resource_;
    godot::NodePath geometry_host_node_path_;
    godot::NodePath spline_host_node_path_;
    bool auto_setup_on_ready_     = true;
    bool auto_execute_in_process_ = false;
    bool share_geometry_memory_   = true;

    void _wire_external_hosts_if_present();

public:
    TileTaskHost() = default;
    ~TileTaskHost() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    // --- Inspector Properties ---
    void set_tiling_resource(const godot::Ref<DataOrientedTiling> &p_tiling);
    [[nodiscard]] godot::Ref<DataOrientedTiling> get_tiling_resource() const { return tiling_resource_; }

    void set_geometry_host_node_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_geometry_host_node_path() const { return geometry_host_node_path_; }

    void set_spline_host_node_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_spline_host_node_path() const { return spline_host_node_path_; }

    void set_auto_setup_on_ready(bool p_auto_setup);
    [[nodiscard]] bool get_auto_setup_on_ready() const noexcept { return auto_setup_on_ready_; }

    void set_auto_execute_in_process(bool p_auto_execute);
    [[nodiscard]] bool get_auto_execute_in_process() const noexcept { return auto_execute_in_process_; }

    void set_share_geometry_memory(bool p_share);
    [[nodiscard]] bool get_share_geometry_memory() const noexcept { return share_geometry_memory_; }

    // --- Host Lifecycle & Cross-Domain Bridging ---
    void setup_tile_host();
    void execute_tile_step(double p_delta);
    bool bind_geometry_task_host(GeometryTaskHost *p_geometry_host);
    bool bind_spline_task_host(SplineTaskHost *p_spline_host);

    [[nodiscard]] core::TileDOD *get_tile_dod() const noexcept {
        return tiling_resource_.is_valid() ? tiling_resource_->get_tile_dod() : nullptr;
    }
};

} // namespace ideam::godot_ext

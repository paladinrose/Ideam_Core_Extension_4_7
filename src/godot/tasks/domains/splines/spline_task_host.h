#pragma once

/**
 * spline_task_host.h
 * SceneTree Node host for `DataOrientedSplines` in `src/godot/tasks/domains/splines/`.
 *
 * Extends `ideam::godot_ext::TaskGraphHost` so a `DataOrientedSplines` Resource can
 * execute inside the Godot SceneTree, automatically bind to a `GeometryTaskHost`
 * (sharing `MemoryManagerDOD`, `TaskGraphDOD`, and `GeometryDOD`'s polyline and 3D
 * Bishop frame buffers via `bind_external_geometry_buffers`), and drive frame-by-frame
 * or on-demand spline evaluation.
 */

#include "data_oriented_splines.h"
#include "../geometry/geometry_task_host.h"
#include "../../task_graph_host.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace ideam::godot_ext {

class SplineTaskHost : public TaskGraphHost {
    GDCLASS(SplineTaskHost, TaskGraphHost)

protected:
    static void _bind_methods();

private:
    godot::Ref<DataOrientedSplines> splines_resource_;
    godot::NodePath geometry_host_node_path_;
    bool auto_setup_on_ready_         = true;
    bool auto_execute_in_process_     = false;
    bool share_geometry_memory_       = true;

    void _wire_geometry_host_if_present();

public:
    SplineTaskHost() = default;
    ~SplineTaskHost() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    // --- Inspector Properties ---
    void set_splines_resource(const godot::Ref<DataOrientedSplines> &p_splines);
    [[nodiscard]] godot::Ref<DataOrientedSplines> get_splines_resource() const { return splines_resource_; }

    void set_geometry_host_node_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_geometry_host_node_path() const { return geometry_host_node_path_; }

    void set_auto_setup_on_ready(bool p_auto_setup);
    [[nodiscard]] bool get_auto_setup_on_ready() const noexcept { return auto_setup_on_ready_; }

    void set_auto_execute_in_process(bool p_auto_execute);
    [[nodiscard]] bool get_auto_execute_in_process() const noexcept { return auto_execute_in_process_; }

    void set_share_geometry_memory(bool p_share);
    [[nodiscard]] bool get_share_geometry_memory() const noexcept { return share_geometry_memory_; }

    // --- Host Lifecycle & Cross-Domain Bridging ---
    void setup_spline_host();
    void execute_spline_step(double p_delta);
    bool bind_geometry_task_host(GeometryTaskHost *p_geometry_host);

    [[nodiscard]] core::SplineDOD *get_spline_dod() const noexcept {
        return splines_resource_.is_valid() ? splines_resource_->get_spline_dod() : nullptr;
    }
};

} // namespace ideam::godot_ext

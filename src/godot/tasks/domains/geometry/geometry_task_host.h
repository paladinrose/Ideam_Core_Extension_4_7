#pragma once

/**
 * geometry_task_host.h
 * SceneTree Node host for `DataOrientedGeometry` in `src/godot/tasks/domains/geometry/`.
 *
 * Extends `ideam::godot_ext::TaskGraphHost` so a `DataOrientedGeometry` Resource can
 * execute inside the Godot SceneTree, share its `MemoryManagerDOD` and `TaskGraphDOD`
 * with another `TaskGraphHost` (such as `NarrativeTaskHost`, `TileTaskHost`, or
 * `DataOrientedGame`), and drive frame-by-frame or on-demand geometric evaluation.
 */

#include "data_oriented_geometry.h"
#include "../../task_graph_host.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace ideam::godot_ext {

class GeometryTaskHost : public TaskGraphHost {
    GDCLASS(GeometryTaskHost, TaskGraphHost)

protected:
    static void _bind_methods();

private:
    godot::Ref<DataOrientedGeometry> geometry_resource_;
    godot::NodePath shared_host_node_path_;
    bool auto_setup_on_ready_     = true;
    bool auto_execute_in_process_ = false;
    bool share_host_memory_       = true;

    void _wire_shared_host_if_present();

public:
    GeometryTaskHost() = default;
    ~GeometryTaskHost() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    // --- Inspector Properties ---
    void set_geometry_resource(const godot::Ref<DataOrientedGeometry> &p_geometry);
    [[nodiscard]] godot::Ref<DataOrientedGeometry> get_geometry_resource() const { return geometry_resource_; }

    void set_shared_host_node_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_shared_host_node_path() const { return shared_host_node_path_; }

    void set_auto_setup_on_ready(bool p_auto_setup);
    [[nodiscard]] bool get_auto_setup_on_ready() const noexcept { return auto_setup_on_ready_; }

    void set_auto_execute_in_process(bool p_auto_execute);
    [[nodiscard]] bool get_auto_execute_in_process() const noexcept { return auto_execute_in_process_; }

    void set_share_host_memory(bool p_share);
    [[nodiscard]] bool get_share_host_memory() const noexcept { return share_host_memory_; }

    // --- Host Lifecycle & Cross-Domain Bridging ---
    void setup_geometry_host();
    void execute_geometry_step(double p_delta);
    bool bind_shared_task_host(TaskGraphHost *p_other_host);

    [[nodiscard]] core::GeometryDOD *get_geometry_dod() const noexcept {
        return geometry_resource_.is_valid() ? geometry_resource_->get_geometry_dod() : nullptr;
    }
};

} // namespace ideam::godot_ext

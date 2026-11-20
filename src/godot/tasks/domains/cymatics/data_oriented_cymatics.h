#pragma once

/**
 * data_oriented_cymatics.h
 * Godot `Resource` bridge for `CymaticsDOD` (`src/godot/tasks/domains/cymatics/`).
 */

#include "../../../../core/tasks/domains/cymatics/cymatics_dod.h"
#include "../../../memory/memory_manager_resource.h"
#include "../../../tasks/task_graph_resource.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <memory>

namespace ideam::godot_ext {

class DataOrientedCymatics : public godot::Resource {
    GDCLASS(DataOrientedCymatics, godot::Resource)

private:
    std::unique_ptr<core::CymaticsDOD> dod_engine_;
    godot::Ref<MemoryManagerResource> memory_manager_resource_;
    godot::Ref<TaskGraphResource> task_graph_resource_;
    int grid_width_  = 32;
    int grid_height_ = 32;
    int grid_depth_  = 1;

protected:
    static void _bind_methods();

public:
    DataOrientedCymatics();
    ~DataOrientedCymatics() override = default;

    [[nodiscard]] core::CymaticsDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }

    void set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem) { memory_manager_resource_ = p_mem; }
    [[nodiscard]] godot::Ref<MemoryManagerResource> get_memory_manager_resource() const { return memory_manager_resource_; }

    void set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph) { task_graph_resource_ = p_graph; }
    [[nodiscard]] godot::Ref<TaskGraphResource> get_task_graph_resource() const { return task_graph_resource_; }

    void set_grid_width(int p_w);
    [[nodiscard]] int get_grid_width() const noexcept { return grid_width_; }

    void set_grid_height(int p_h);
    [[nodiscard]] int get_grid_height() const noexcept { return grid_height_; }

    void set_grid_depth(int p_d);
    [[nodiscard]] int get_grid_depth() const noexcept { return grid_depth_; }

    bool execute_pipeline_direct(
        int p_stage_mask = static_cast<int>(core::CYM_STAGE_DEFAULT_MASK),
        const godot::PackedFloat32Array &p_fft_peaks = godot::PackedFloat32Array()
    );
    void inject_impulse(int p_x, int p_y, float p_amplitude);
    void set_listener(const godot::Vector2 &p_grid_pos, float p_yaw_rad);
    void set_primary_source(const godot::Vector2 &p_grid_pos);

    [[nodiscard]] godot::PackedFloat32Array read_modal_field() const;
    [[nodiscard]] godot::PackedInt32Array read_shadow_classes() const;
    [[nodiscard]] godot::Array read_sectors_as_dicts() const;
};

} // namespace ideam::godot_ext

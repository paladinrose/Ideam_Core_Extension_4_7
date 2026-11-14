#pragma once

/**
 * data_oriented_wfc.h
 * Godot `Resource` bridge for `WaveFunctionCollapseDOD`
 * (`src/godot/tasks/domains/wave_function_collapse/`).
 */

#include "../../../../core/tasks/domains/wave_function_collapse/wfc_dod.h"
#include "../../../memory/memory_manager_resource.h"
#include "../../../tasks/task_graph_resource.h"
#include "../../../wave_function_collapse/wfc_rule_tensor_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <memory>

namespace ideam::godot_ext {

class DataOrientedWaveFunctionCollapse : public godot::Resource {
    GDCLASS(DataOrientedWaveFunctionCollapse, godot::Resource)

private:
    std::unique_ptr<core::WaveFunctionCollapseDOD> dod_engine_;

    godot::Ref<MemoryManagerResource> memory_manager_resource_;
    godot::Ref<TaskGraphResource> task_graph_resource_;
    godot::Ref<WFCRuleTensorResource> rule_tensor_resource_;
    godot::Ref<ProceduralPattern> active_pattern_;

    int slot_count_                     = 16;
    int max_collapse_steps_             = 64;
    float pointer_redundancy_threshold_ = 0.75f;
    int horizon_radius_                 = 4;

protected:
    static void _bind_methods();

public:
    DataOrientedWaveFunctionCollapse();
    ~DataOrientedWaveFunctionCollapse() override = default;

    [[nodiscard]] core::WaveFunctionCollapseDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }

    void set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem);
    [[nodiscard]] godot::Ref<MemoryManagerResource> get_memory_manager_resource() const { return memory_manager_resource_; }

    void set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_task_graph_resource() const { return task_graph_resource_; }

    void set_rule_tensor_resource(const godot::Ref<WFCRuleTensorResource> &p_res) {
        rule_tensor_resource_ = p_res;
        emit_changed();
    }
    [[nodiscard]] godot::Ref<WFCRuleTensorResource> get_rule_tensor_resource() const { return rule_tensor_resource_; }

    void set_active_pattern(const godot::Ref<ProceduralPattern> &p_pattern) {
        active_pattern_ = p_pattern;
        emit_changed();
    }
    [[nodiscard]] godot::Ref<ProceduralPattern> get_active_pattern() const { return active_pattern_; }

    void set_slot_count(int p_count);
    [[nodiscard]] int get_slot_count() const noexcept { return slot_count_; }

    void set_max_collapse_steps(int p_steps);
    [[nodiscard]] int get_max_collapse_steps() const noexcept { return max_collapse_steps_; }

    void set_pointer_redundancy_threshold(float p_thresh);
    [[nodiscard]] float get_pointer_redundancy_threshold() const noexcept { return pointer_redundancy_threshold_; }

    void set_horizon_radius(int p_radius);
    [[nodiscard]] int get_horizon_radius() const noexcept { return horizon_radius_; }

    // --- DOD Arena Setup, Hierarchy Configuration, Staging & Execution ---
    bool initialize_dod_buffers();
    void set_slot_children(int p_parent_slot, const godot::PackedInt32Array &p_child_slots);
    void pin_slot_token(int p_slot_id, const godot::String &p_token_name);
    bool stage_environment_to_dod();
    bool execute_pipeline_direct(int p_stage_mask = static_cast<int>(core::WFC_STAGE_DEFAULT_PIPELINE_MASK));

    [[nodiscard]] godot::Array read_environment_slots_as_dicts() const;
    [[nodiscard]] godot::Array read_contradiction_reports_as_dicts() const;
    [[nodiscard]] godot::PackedInt32Array read_pointer_or_conflict_masks() const;
};

} // namespace ideam::godot_ext

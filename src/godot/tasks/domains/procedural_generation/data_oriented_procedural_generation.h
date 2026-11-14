#pragma once

/**
 * data_oriented_procedural_generation.h
 * Godot `Resource` bridge for `ProceduralGenerationDOD`
 * (`src/godot/tasks/domains/procedural_generation/`).
 */

#include "../../../../core/tasks/domains/procedural_generation/procedural_generation_dod.h"
#include "../../../memory/memory_manager_resource.h"
#include "../../../procedural_generation/procedural_patterner.h"
#include "../../../tasks/task_graph_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <memory>

namespace ideam::godot_ext {

class DataOrientedProceduralGeneration : public godot::Resource {
    GDCLASS(DataOrientedProceduralGeneration, godot::Resource)

private:
    std::unique_ptr<core::ProceduralGenerationDOD> dod_engine_;

    godot::Ref<MemoryManagerResource> memory_manager_resource_;
    godot::Ref<TaskGraphResource> task_graph_resource_;
    godot::Ref<ProceduralPattern> active_pattern_;

    int slot_count_                     = 16;
    float pointer_redundancy_threshold_ = 0.75f;
    int max_propagation_sweeps_         = 16;

protected:
    static void _bind_methods();

public:
    DataOrientedProceduralGeneration();
    ~DataOrientedProceduralGeneration() override = default;

    [[nodiscard]] core::ProceduralGenerationDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }

    void set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem);
    [[nodiscard]] godot::Ref<MemoryManagerResource> get_memory_manager_resource() const { return memory_manager_resource_; }

    void set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_task_graph_resource() const { return task_graph_resource_; }

    void set_active_pattern(const godot::Ref<ProceduralPattern> &p_pattern) { active_pattern_ = p_pattern; emit_changed(); }
    [[nodiscard]] godot::Ref<ProceduralPattern> get_active_pattern() const { return active_pattern_; }

    void set_slot_count(int p_count);
    [[nodiscard]] int get_slot_count() const noexcept { return slot_count_; }

    void set_pointer_redundancy_threshold(float p_thresh);
    [[nodiscard]] float get_pointer_redundancy_threshold() const noexcept { return pointer_redundancy_threshold_; }

    void set_max_propagation_sweeps(int p_sweeps);
    [[nodiscard]] int get_max_propagation_sweeps() const noexcept { return max_propagation_sweeps_; }

    // --- DOD Arena Setup, Hierarchy Configuration, Staging & Execution ---
    bool initialize_dod_buffers();
    void set_slot_children(int p_parent_slot, const godot::PackedInt32Array &p_child_slots);
    void pin_slot_token(int p_slot_id, const godot::String &p_token_name);
    bool stage_pattern_to_dod();
    bool execute_pipeline_direct(int p_stage_mask = static_cast<int>(core::PROCGEN_STAGE_DEFAULT_PIPELINE_MASK));

    [[nodiscard]] godot::Array read_environment_slots_as_dicts() const;
    [[nodiscard]] godot::PackedInt32Array read_pointer_or_conflict_masks() const;
};

} // namespace ideam::godot_ext

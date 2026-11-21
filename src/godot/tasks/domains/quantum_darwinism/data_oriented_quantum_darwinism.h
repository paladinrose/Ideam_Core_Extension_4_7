#pragma once

/**
 * data_oriented_quantum_darwinism.h
 * Godot `Resource` bridge for `QuantumDarwinismDOD`
 * (`src/godot/tasks/domains/quantum_darwinism/`).
 */

#include "../../../../core/tasks/domains/quantum_darwinism/quantum_darwinism_dod.h"
#include "../../../memory/memory_manager_resource.h"
#include "../../../quantum_darwinism/quantum_darwinism_generator_resource.h"
#include "../../../quantum_darwinism/quantum_darwinism_stabilizer_resource.h"
#include "../../../tasks/task_graph_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <memory>

namespace ideam::godot_ext {

class DataOrientedQuantumDarwinism : public godot::Resource {
    GDCLASS(DataOrientedQuantumDarwinism, godot::Resource)

private:
    std::unique_ptr<core::QuantumDarwinismDOD> dod_engine_;

    godot::Ref<MemoryManagerResource> memory_manager_resource_;
    godot::Ref<TaskGraphResource> task_graph_resource_;
    godot::Ref<QuantumDarwinismStabilizerResource> stabilizer_resource_;
    godot::Ref<QuantumDarwinismGeneratorResource> generator_resource_;
    godot::Ref<ProceduralPattern> active_pattern_;

    int slot_count_ = 16;

protected:
    static void _bind_methods();

public:
    DataOrientedQuantumDarwinism();
    ~DataOrientedQuantumDarwinism() override = default;

    [[nodiscard]] core::QuantumDarwinismDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }

    void set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem);
    [[nodiscard]] godot::Ref<MemoryManagerResource> get_memory_manager_resource() const { return memory_manager_resource_; }

    void set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_task_graph_resource() const { return task_graph_resource_; }

    void set_stabilizer_resource(const godot::Ref<QuantumDarwinismStabilizerResource> &p_res);
    [[nodiscard]] godot::Ref<QuantumDarwinismStabilizerResource> get_stabilizer_resource() const { return stabilizer_resource_; }

    void set_generator_resource(const godot::Ref<QuantumDarwinismGeneratorResource> &p_res);
    [[nodiscard]] godot::Ref<QuantumDarwinismGeneratorResource> get_generator_resource() const { return generator_resource_; }

    void set_active_pattern(const godot::Ref<ProceduralPattern> &p_pattern) {
        active_pattern_ = p_pattern;
        emit_changed();
    }
    [[nodiscard]] godot::Ref<ProceduralPattern> get_active_pattern() const { return active_pattern_; }

    void set_slot_count(int p_count);
    [[nodiscard]] int get_slot_count() const noexcept { return slot_count_; }

    // --- DOD Arena Setup, Staging & Execution ---
    bool initialize_dod_buffers();
    void set_slot_children(int p_parent_slot, const godot::PackedInt32Array &p_child_slots);
    void flag_slot_conflict(int p_slot_id);
    bool stage_environment_to_dod();
    bool stage_channels_and_couplings_to_dod();
    bool stage_perturbation_field_to_dod(const godot::PackedFloat32Array &p_field);
    bool execute_pipeline_direct(int p_stage_mask = static_cast<int>(core::QD_STAGE_DEFAULT_PIPELINE_MASK));
    bool execute_generative_pipeline_direct();

    [[nodiscard]] godot::Array read_environment_slots_as_dicts() const;
    [[nodiscard]] godot::Array read_horizon_partitions_as_dicts() const;
    [[nodiscard]] godot::Array read_pointer_states_as_dicts() const;
    [[nodiscard]] godot::Array read_coherence_bridge_slots_as_dicts() const;
    [[nodiscard]] godot::Array read_slot_channel_states_as_dicts() const;
    [[nodiscard]] godot::PackedInt32Array read_pointer_or_conflict_masks() const;
};

} // namespace ideam::godot_ext

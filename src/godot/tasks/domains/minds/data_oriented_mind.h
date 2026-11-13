#pragma once

/**
 * data_oriented_mind.h
 * Godot-facing Resource bridge class in `src/godot/tasks/domains/minds/`.
 *
 * Extends `ideam::godot_ext::InformationMatrixResource` to unite the Object-Oriented
 * authoring Resources in `src/godot/minds/` (`InformationMatrixResource`, `AgentMindResource`)
 * with the pure C++26 Data-Oriented Design (DOD) memory tables (Multi-Source Akashic Tomes +
 * Per-Agent Epistemic Reference Catalogues) and Kahn-sorted `TaskGraphDOD` pipeline in
 * `src/core/tasks/domains/minds/`.
 */

#include "../../../../core/tasks/domains/minds/mind_dod.h"
#include "../../../minds/agent_mind_resource.h"
#include "../../../minds/information_matrix_resource.h"
#include "../../task_graph_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ideam::godot_ext {

enum MindDODExecutionMode : int32_t {
    MIND_DOD_EXEC_DISABLED              = 0, // Pure OOP Resource evaluation fallback
    MIND_DOD_EXEC_COMPOSABLE_STAGES     = 1, // Bitmask-configurable cognitive wave pipeline + custom co-scheduling
    MIND_DOD_EXEC_CUSTOM_GRAPH_RESOURCE = 2, // User-authored TaskGraphResource bound to MindDOD buffers
    MIND_DOD_EXEC_MANUAL_GRAPH          = 3  // Buffers allocated; graph left open for manual node construction
};

class DataOrientedMind : public InformationMatrixResource {
    GDCLASS(DataOrientedMind, InformationMatrixResource)

protected:
    static void _bind_methods();

private:
    std::unique_ptr<core::MindDOD> mind_dod_;
    std::shared_ptr<core::TaskGraphDOD> active_task_graph_;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map_;

    // --- Managed AgentMindResource Profiles ---
    godot::TypedArray<AgentMindResource> agent_minds_;

    // --- Inspector Configuration ---
    MindDODExecutionMode dod_execution_mode_ = MIND_DOD_EXEC_COMPOSABLE_STAGES;
    uint32_t pipeline_stage_mask_            = core::MINDS_STAGE_DEFAULT_PIPELINE_MASK;
    bool auto_connect_pipeline_stages_       = true;
    bool auto_sync_resources_from_dod_       = true;

    godot::Ref<TaskGraphResource> custom_task_graph_resource_;

    // Configurable Buffer Capacities
    int max_dod_akashic_tomes_ = 64;
    int max_dod_facts_         = 8192;
    int max_dod_agents_        = 1024;
    int max_dod_beliefs_       = 32768;
    int max_dod_hypotheses_    = 2048;
    int max_dod_syllogisms_    = 2048;

    // --- Bidirectional ObjectID / AgentID <-> DOD Slot Registries ---
    std::unordered_map<uint64_t, uint32_t> object_id_to_agent_slot_;
    std::unordered_map<uint32_t, godot::Ref<AgentMindResource>> agent_slot_to_resource_;
    std::vector<uint32_t> baked_agent_ids_;

    core::MindsBufferCapacities _build_capacities() const noexcept;
    void _ensure_mind_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager = nullptr);
    void _clear_resource_maps() noexcept;

public:
    DataOrientedMind() = default;
    ~DataOrientedMind() override = default;

    // --- Direct C++ Accessors ---
    [[nodiscard]] core::MindDOD *get_mind_dod() const noexcept { return mind_dod_.get(); }
    [[nodiscard]] std::shared_ptr<core::TaskGraphDOD> get_active_task_graph_shared() const noexcept { return active_task_graph_; }
    [[nodiscard]] core::TaskGraphDOD *get_active_task_graph_dod() const noexcept { return active_task_graph_.get(); }
    [[nodiscard]] std::shared_ptr<core::MemoryManagerDOD> get_memory_manager_shared() const noexcept {
        return mind_dod_ ? mind_dod_->get_memory_manager() : nullptr;
    }
    [[nodiscard]] core::MemoryManagerDOD *get_memory_manager_dod() const noexcept {
        return mind_dod_ ? mind_dod_->get_memory_manager_raw() : nullptr;
    }

    // --- Resource Collection Setters / Getters ---
    void set_agent_minds(const godot::TypedArray<AgentMindResource> &p_minds);
    [[nodiscard]] godot::TypedArray<AgentMindResource> get_agent_minds() const { return agent_minds_; }

    // --- Inspector Execution Setters / Getters ---
    void set_dod_execution_mode(MindDODExecutionMode p_mode);
    [[nodiscard]] MindDODExecutionMode get_dod_execution_mode() const noexcept { return dod_execution_mode_; }

    void set_pipeline_stage_mask(int p_mask);
    [[nodiscard]] int get_pipeline_stage_mask() const noexcept { return static_cast<int>(pipeline_stage_mask_); }

    void set_auto_connect_pipeline_stages(bool p_auto_connect);
    [[nodiscard]] bool get_auto_connect_pipeline_stages() const noexcept { return auto_connect_pipeline_stages_; }

    void set_auto_sync_resources_from_dod(bool p_auto_sync);
    [[nodiscard]] bool get_auto_sync_resources_from_dod() const noexcept { return auto_sync_resources_from_dod_; }

    void set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_custom_task_graph_resource() const { return custom_task_graph_resource_; }

    void set_max_dod_akashic_tomes(int p_max);
    [[nodiscard]] int get_max_dod_akashic_tomes() const noexcept { return max_dod_akashic_tomes_; }

    void set_max_dod_facts(int p_max);
    [[nodiscard]] int get_max_dod_facts() const noexcept { return max_dod_facts_; }

    void set_max_dod_agents(int p_max);
    [[nodiscard]] int get_max_dod_agents() const noexcept { return max_dod_agents_; }

    void set_max_dod_beliefs(int p_max);
    [[nodiscard]] int get_max_dod_beliefs() const noexcept { return max_dod_beliefs_; }

    // --- Pipeline Setup, Baking & Execution API ---
    void setup_dod_pipeline_with_manager(
        std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
        std::shared_ptr<core::TaskGraphDOD> p_shared_graph = nullptr
    );
    void setup_dod_pipeline();
    void bake_to_dod();
    void rebuild_dod_task_graph();
    void execute_dod_step(double p_delta);
    void sync_oop_resources_from_dod();

    // --- Buffer, Slot & Event Ring API ---
    [[nodiscard]] int get_buffer_id(const godot::StringName &p_semantic_name) const;
    [[nodiscard]] godot::Dictionary get_buffer_manifest_dictionary() const;
    [[nodiscard]] int get_agent_dod_slot(const godot::Ref<AgentMindResource> &p_mind) const;

    bool queue_sensory_observation_event(
        int p_sensor_agent_id,
        int p_target_fact_id,
        int64_t p_sensed_value,
        float p_raw_clarity = 1.0f,
        float p_environmental_noise = 0.0f,
        int p_perceived_fact_id = 0,
        int p_tome_id = 0,
        int p_modality = 0
    );

    // --- Dynamic Task Node & Wave Co-Scheduling API ---
    int add_minds_transform_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_minds_query_node(
        int p_logic_id,
        int p_query_op = 0,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_minds_metadata_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );

    bool schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream = false);
    bool schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::MindDODExecutionMode);

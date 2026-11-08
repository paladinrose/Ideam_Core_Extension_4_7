#pragma once

/**
 * data_oriented_narrative.h
 * Godot-facing Resource bridge class in src/godot/tasks/domains/narratives/.
 *
 * Extends ideam::godot_ext::Narrative to unite the Object-Oriented authoring
 * Resources in src/godot/narratives with the pure C++26 Data-Oriented Design (DOD)
 * memory tables and Kahn-sorted TaskGraphDOD pipeline in src/core/tasks/domains/narratives.
 *
 * Key Responsibilities:
 *   1. Bakes the 5 Literal/Symbolic TypedArray pairs (Character/Role, Prop/Symbol,
 *      Location/Place, Incident/Moment, Plot/Story), their parallel *_assignments arrays,
 *      Relationships, Incident_Conditions (Narrative, Causal, Gameplay), and Plot_Events
 *      into contiguous NarrativeDOD buffers.
 *   2. Maintains O(1) bidirectional mappings between Resource::get_instance_id() handles
 *      and DOD row indices (Narreme slot, Symbolic slot, Condition slot, Incident slot, Plot slot).
 *   3. Drains NarrativeSignalCommandPOD and CharacterGoalEventPOD rings on the main thread
 *      to synchronize OOP Resource properties and emit standard Godot Resource signals.
 */

#include "../../../../core/tasks/domains/narratives/narrative_dod.h"
#include "../../../narratives/narremes/narrative.h"
#include "../../task_graph_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ideam::godot_ext {

class Incident_Condition;
class Narrative_Condition;
class Causal_Condition;
class Gameplay_Condition;
class Plot_Event;

enum NarrativeDODExecutionMode : int32_t {
    NARRATIVE_DOD_EXEC_DISABLED              = 0, // Pure OOP Resource evaluation fallback
    NARRATIVE_DOD_EXEC_COMPOSABLE_STAGES     = 1, // Bitmask-configurable stage pipeline + custom wave co-scheduling
    NARRATIVE_DOD_EXEC_CUSTOM_GRAPH_RESOURCE = 2, // User-authored TaskGraphResource bound to NarrativeDOD buffers
    NARRATIVE_DOD_EXEC_MANUAL_GRAPH          = 3  // Buffers allocated; graph left open for manual node construction
};

class DataOrientedNarrative : public Narrative {
    GDCLASS(DataOrientedNarrative, Narrative)

protected:
    static void _bind_methods();

private:
    std::unique_ptr<core::NarrativeDOD> narrative_dod_;
    std::shared_ptr<core::TaskGraphDOD> active_task_graph_;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map_;

    // --- Inspector Configuration ---
    NarrativeDODExecutionMode dod_execution_mode_ = NARRATIVE_DOD_EXEC_COMPOSABLE_STAGES;
    uint32_t pipeline_stage_mask_                 = core::NARRATIVES_STAGE_DEFAULT_PIPELINE_MASK;
    bool auto_connect_pipeline_stages_            = true;
    bool auto_dispatch_signals_to_oop_            = true;
    bool auto_sync_resources_from_dod_            = true;

    godot::Ref<TaskGraphResource> custom_task_graph_resource_;

    // Configurable Section 3.2 Buffer Capacities
    int max_dod_narremes_       = 2048;
    int max_dod_symbolic_slots_ = 2048;
    int max_dod_relationships_  = 8192;
    int max_dod_conditions_     = 4096;
    int max_dod_incidents_      = 2048;
    int max_dod_plots_          = 1024;

    // --- Bidirectional ObjectID <-> DOD Row Index Registries ---
    std::unordered_map<uint64_t, uint32_t> object_id_to_narreme_slot_;
    std::unordered_map<uint32_t, godot::Ref<Narreme>> narreme_slot_to_resource_;

    std::unordered_map<uint64_t, uint32_t> object_id_to_symbolic_slot_;
    std::unordered_map<uint32_t, godot::Ref<Narreme>> symbolic_slot_to_resource_;

    std::unordered_map<uint64_t, uint32_t> object_id_to_condition_slot_;
    std::unordered_map<uint32_t, godot::Ref<Incident_Condition>> condition_slot_to_resource_;

    std::unordered_map<uint64_t, uint32_t> object_id_to_incident_slot_;
    std::unordered_map<uint32_t, godot::Ref<Incident>> incident_slot_to_resource_;

    std::unordered_map<uint64_t, uint32_t> object_id_to_plot_slot_;
    std::unordered_map<uint32_t, godot::Ref<Plot>> plot_slot_to_resource_;

    // --- Internal Bake & Sync Helpers ---
    core::NarrativesBufferCapacities _build_capacities() const noexcept;
    void _ensure_narrative_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager = nullptr);
    void _clear_resource_maps() noexcept;

    uint32_t _register_narreme_resource(const godot::Ref<Narreme>& p_res, uint8_t p_narreme_type, bool p_is_symbolic);
    uint32_t _bake_condition_recursive(const godot::Ref<Incident_Condition>& p_cond);

    template <typename T_Sym, typename T_Lit>
    void _bake_symbolic_assignment_pair(
        const godot::TypedArray<T_Sym>& p_symbolic_arr,
        const godot::TypedArray<T_Lit>& p_literal_arr,
        const godot::TypedArray<int>& p_assignments
    );

public:
    DataOrientedNarrative() = default;
    ~DataOrientedNarrative() override = default;

    virtual void initialize() override;

    // --- Direct C++ Accessors ---
    [[nodiscard]] core::NarrativeDOD* get_narrative_dod() const noexcept { return narrative_dod_.get(); }
    [[nodiscard]] std::shared_ptr<core::TaskGraphDOD> get_active_task_graph_shared() const noexcept { return active_task_graph_; }
    [[nodiscard]] core::TaskGraphDOD* get_active_task_graph_dod() const noexcept { return active_task_graph_.get(); }
    [[nodiscard]] std::shared_ptr<core::MemoryManagerDOD> get_memory_manager_shared() const noexcept {
        return narrative_dod_ ? narrative_dod_->get_memory_manager() : nullptr;
    }
    [[nodiscard]] core::MemoryManagerDOD* get_memory_manager_dod() const noexcept {
        return narrative_dod_ ? narrative_dod_->get_memory_manager_raw() : nullptr;
    }

    // --- Inspector Setters / Getters ---
    void set_dod_execution_mode(NarrativeDODExecutionMode p_mode);
    NarrativeDODExecutionMode get_dod_execution_mode() const;

    void set_pipeline_stage_mask(int p_mask);
    int get_pipeline_stage_mask() const;

    void set_auto_connect_pipeline_stages(bool p_auto_connect);
    bool get_auto_connect_pipeline_stages() const;

    void set_auto_dispatch_signals_to_oop(bool p_auto_dispatch);
    bool get_auto_dispatch_signals_to_oop() const;

    void set_auto_sync_resources_from_dod(bool p_auto_sync);
    bool get_auto_sync_resources_from_dod() const;

    void set_custom_task_graph_resource(const godot::Ref<TaskGraphResource>& p_resource);
    godot::Ref<TaskGraphResource> get_custom_task_graph_resource() const;

    void set_max_dod_narremes(int p_max);
    int get_max_dod_narremes() const;

    void set_max_dod_symbolic_slots(int p_max);
    int get_max_dod_symbolic_slots() const;

    void set_max_dod_relationships(int p_max);
    int get_max_dod_relationships() const;

    void set_max_dod_conditions(int p_max);
    int get_max_dod_conditions() const;

    void set_max_dod_incidents(int p_max);
    int get_max_dod_incidents() const;

    void set_max_dod_plots(int p_max);
    int get_max_dod_plots() const;

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
    int dispatch_dod_signals_to_oop();

    // --- Buffer & Slot Lookup API ---
    int get_buffer_id(const godot::StringName& p_semantic_name) const;
    godot::Dictionary get_buffer_manifest_dictionary() const;

    int get_narreme_dod_slot(const godot::Ref<Narreme>& p_narreme) const;
    int get_symbolic_dod_slot(const godot::Ref<Narreme>& p_symbolic) const;
    int get_condition_dod_slot(const godot::Ref<Incident_Condition>& p_condition) const;
    int get_incident_dod_slot(const godot::Ref<Incident>& p_incident) const;
    int get_plot_dod_slot(const godot::Ref<Plot>& p_plot) const;

    godot::Ref<Narreme> get_narreme_for_dod_slot(int p_slot) const;
    godot::Ref<Narreme> get_symbolic_for_dod_slot(int p_slot) const;
    godot::Ref<Incident_Condition> get_condition_for_dod_slot(int p_slot) const;
    godot::Ref<Incident> get_incident_for_dod_slot(int p_slot) const;
    godot::Ref<Plot> get_plot_for_dod_slot(int p_slot) const;

    // --- Event Ring Staging Helpers ---
    bool queue_spatial_transfer(
        const godot::Ref<Narreme>& p_narreme,
        const godot::Ref<Location>& p_new_location,
        const godot::Ref<Character>& p_new_possessor = godot::Ref<Character>()
    );
    bool queue_gameplay_condition_event(
        const godot::Ref<Incident_Condition>& p_condition,
        int p_new_status,
        int p_property_value = 0
    );

    // --- Dynamic Task Node & Wave Co-Scheduling API ---
    int add_narratives_transform_node(
        int p_logic_id,
        const godot::Dictionary& p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false,
        int p_view = 0,
        int p_strategy = 0,
        int p_element_type = 2
    );

    int add_narratives_query_node(
        int p_logic_id,
        int p_query_op = 0,
        const godot::Dictionary& p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false,
        int p_view = 0,
        int p_strategy = 0,
        int p_element_type = 2
    );

    int add_narratives_metadata_node(
        int p_logic_id,
        const godot::Dictionary& p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false,
        int p_view = 0,
        int p_strategy = 0,
        int p_element_type = 2
    );

    bool schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream = false);
    bool schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::NarrativeDODExecutionMode);
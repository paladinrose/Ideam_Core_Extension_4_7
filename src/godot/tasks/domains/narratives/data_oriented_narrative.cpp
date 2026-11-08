#include "data_oriented_narrative.h"

#include "../../../narratives/helpers/causal_condition.h"
#include "../../../narratives/helpers/gameplay_condition.h"
#include "../../../narratives/helpers/incident_condition.h"
#include "../../../narratives/helpers/narrative_condition.h"
#include "../../../narratives/narremes/character.h"
#include "../../../narratives/narremes/incident.h"
#include "../../../narratives/narremes/location.h"
#include "../../../narratives/narremes/prop.h"
#include "../../../narratives/narremes/plot.h"
#include "../../../narratives/helpers/plot_event.h"
#include "../../../narratives/helpers/relationship.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <span>
#include <string>
#include <string_view>

namespace ideam::godot_ext {

void DataOrientedNarrative::_bind_methods() {
    using namespace godot;

    // --- Execution Mode Enum ---
    BIND_ENUM_CONSTANT(NARRATIVE_DOD_EXEC_DISABLED);
    BIND_ENUM_CONSTANT(NARRATIVE_DOD_EXEC_COMPOSABLE_STAGES);
    BIND_ENUM_CONSTANT(NARRATIVE_DOD_EXEC_CUSTOM_GRAPH_RESOURCE);
    BIND_ENUM_CONSTANT(NARRATIVE_DOD_EXEC_MANUAL_GRAPH);

    // --- Inspector Properties via compact registration macro ---
#define IDEAM_BIND_NARRATIVE_PROP(m_type, m_name, m_setter, m_getter, ...) \
    ClassDB::bind_method(D_METHOD(#m_setter, "value"), &DataOrientedNarrative::m_setter); \
    ClassDB::bind_method(D_METHOD(#m_getter), &DataOrientedNarrative::m_getter); \
    ADD_PROPERTY(PropertyInfo(m_type, m_name, ##__VA_ARGS__), #m_setter, #m_getter)

    ADD_GROUP("DOD Pipeline Execution", "dod_");
    IDEAM_BIND_NARRATIVE_PROP(
        Variant::INT, "dod_execution_mode", set_dod_execution_mode, get_dod_execution_mode,
        PROPERTY_HINT_ENUM, "Disabled:0,Composable Stages:1,Custom TaskGraphResource:2,Manual Graph:3"
    );
    IDEAM_BIND_NARRATIVE_PROP(
        Variant::INT, "dod_pipeline_stage_mask", set_pipeline_stage_mask, get_pipeline_stage_mask,
        PROPERTY_HINT_FLAGS,
        "Symbolic Assignment:1,Condition Gather:2,Causal Wave:4,Incident & Plot:8,Spatial & Resource Commit:16,Goal & Relationship:32,Observability:64"
    );
    IDEAM_BIND_NARRATIVE_PROP(Variant::BOOL, "dod_auto_connect_pipeline_stages", set_auto_connect_pipeline_stages, get_auto_connect_pipeline_stages);
    IDEAM_BIND_NARRATIVE_PROP(Variant::BOOL, "dod_auto_dispatch_signals_to_oop", set_auto_dispatch_signals_to_oop, get_auto_dispatch_signals_to_oop);
    IDEAM_BIND_NARRATIVE_PROP(Variant::BOOL, "dod_auto_sync_resources_from_dod", set_auto_sync_resources_from_dod, get_auto_sync_resources_from_dod);
    IDEAM_BIND_NARRATIVE_PROP(
        Variant::OBJECT, "dod_custom_task_graph_resource", set_custom_task_graph_resource, get_custom_task_graph_resource,
        PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"
    );

    ADD_GROUP("DOD Buffer Capacities", "max_dod_");
    IDEAM_BIND_NARRATIVE_PROP(Variant::INT, "max_dod_narremes", set_max_dod_narremes, get_max_dod_narremes, PROPERTY_HINT_RANGE, "16,65536,1");
    IDEAM_BIND_NARRATIVE_PROP(Variant::INT, "max_dod_symbolic_slots", set_max_dod_symbolic_slots, get_max_dod_symbolic_slots, PROPERTY_HINT_RANGE, "16,65536,1");
    IDEAM_BIND_NARRATIVE_PROP(Variant::INT, "max_dod_relationships", set_max_dod_relationships, get_max_dod_relationships, PROPERTY_HINT_RANGE, "16,262144,1");
    IDEAM_BIND_NARRATIVE_PROP(Variant::INT, "max_dod_conditions", set_max_dod_conditions, get_max_dod_conditions, PROPERTY_HINT_RANGE, "16,65536,1");
    IDEAM_BIND_NARRATIVE_PROP(Variant::INT, "max_dod_incidents", set_max_dod_incidents, get_max_dod_incidents, PROPERTY_HINT_RANGE, "16,65536,1");
    IDEAM_BIND_NARRATIVE_PROP(Variant::INT, "max_dod_plots", set_max_dod_plots, get_max_dod_plots, PROPERTY_HINT_RANGE, "16,65536,1");

#undef IDEAM_BIND_NARRATIVE_PROP

    // --- Pipeline Setup, Baking & Execution Methods ---
    ClassDB::bind_method(D_METHOD("setup_dod_pipeline"), &DataOrientedNarrative::setup_dod_pipeline);
    ClassDB::bind_method(D_METHOD("bake_to_dod"), &DataOrientedNarrative::bake_to_dod);
    ClassDB::bind_method(D_METHOD("rebuild_dod_task_graph"), &DataOrientedNarrative::rebuild_dod_task_graph);
    ClassDB::bind_method(D_METHOD("execute_dod_step", "delta"), &DataOrientedNarrative::execute_dod_step);
    ClassDB::bind_method(D_METHOD("sync_oop_resources_from_dod"), &DataOrientedNarrative::sync_oop_resources_from_dod);
    ClassDB::bind_method(D_METHOD("dispatch_dod_signals_to_oop"), &DataOrientedNarrative::dispatch_dod_signals_to_oop);

    // --- Buffer & Slot Lookup Methods ---
    ClassDB::bind_method(D_METHOD("get_buffer_id", "semantic_name"), &DataOrientedNarrative::get_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_manifest_dictionary"), &DataOrientedNarrative::get_buffer_manifest_dictionary);

    ClassDB::bind_method(D_METHOD("get_narreme_dod_slot", "narreme"), &DataOrientedNarrative::get_narreme_dod_slot);
    ClassDB::bind_method(D_METHOD("get_symbolic_dod_slot", "symbolic"), &DataOrientedNarrative::get_symbolic_dod_slot);
    ClassDB::bind_method(D_METHOD("get_condition_dod_slot", "condition"), &DataOrientedNarrative::get_condition_dod_slot);
    ClassDB::bind_method(D_METHOD("get_incident_dod_slot", "incident"), &DataOrientedNarrative::get_incident_dod_slot);
    ClassDB::bind_method(D_METHOD("get_plot_dod_slot", "plot"), &DataOrientedNarrative::get_plot_dod_slot);

    ClassDB::bind_method(D_METHOD("get_narreme_for_dod_slot", "slot"), &DataOrientedNarrative::get_narreme_for_dod_slot);
    ClassDB::bind_method(D_METHOD("get_symbolic_for_dod_slot", "slot"), &DataOrientedNarrative::get_symbolic_for_dod_slot);
    ClassDB::bind_method(D_METHOD("get_condition_for_dod_slot", "slot"), &DataOrientedNarrative::get_condition_for_dod_slot);
    ClassDB::bind_method(D_METHOD("get_incident_for_dod_slot", "slot"), &DataOrientedNarrative::get_incident_for_dod_slot);
    ClassDB::bind_method(D_METHOD("get_plot_for_dod_slot", "slot"), &DataOrientedNarrative::get_plot_for_dod_slot);

    // --- Event Ring Staging Methods ---
    ClassDB::bind_method(
        D_METHOD("queue_spatial_transfer", "narreme", "new_location", "new_possessor"),
        &DataOrientedNarrative::queue_spatial_transfer,
        DEFVAL(Ref<Character>())
    );
    ClassDB::bind_method(
        D_METHOD("queue_gameplay_condition_event", "condition", "new_status", "property_value"),
        &DataOrientedNarrative::queue_gameplay_condition_event,
        DEFVAL(0)
    );

    // --- Dynamic Task Node & Wave Co-Scheduling Methods ---
    ClassDB::bind_method(
        D_METHOD("add_narratives_transform_node", "logic_id", "properties", "priority", "use_registry_factory", "view", "strategy", "element_type"),
        &DataOrientedNarrative::add_narratives_transform_node,
        DEFVAL(Dictionary()), DEFVAL(0), DEFVAL(false), DEFVAL(0), DEFVAL(0), DEFVAL(2)
    );
    ClassDB::bind_method(
        D_METHOD("add_narratives_query_node", "logic_id", "query_op", "properties", "priority", "use_registry_factory", "view", "strategy", "element_type"),
        &DataOrientedNarrative::add_narratives_query_node,
        DEFVAL(0), DEFVAL(Dictionary()), DEFVAL(0), DEFVAL(false), DEFVAL(0), DEFVAL(0), DEFVAL(2)
    );
    ClassDB::bind_method(
        D_METHOD("add_narratives_metadata_node", "logic_id", "properties", "priority", "use_registry_factory", "view", "strategy", "element_type"),
        &DataOrientedNarrative::add_narratives_metadata_node,
        DEFVAL(Dictionary()), DEFVAL(0), DEFVAL(false), DEFVAL(0), DEFVAL(0), DEFVAL(2)
    );
    ClassDB::bind_method(
        D_METHOD("schedule_node_with_stage", "stage_flag", "custom_node_id", "join_downstream"),
        &DataOrientedNarrative::schedule_node_with_stage,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("schedule_node_between_stages", "upstream_stage_flag", "downstream_stage_flag", "custom_node_id"),
        &DataOrientedNarrative::schedule_node_between_stages
    );

    // --- Signals ---
    ADD_SIGNAL(MethodInfo("dod_pipeline_ready"));
    ADD_SIGNAL(MethodInfo("dod_step_completed", PropertyInfo(Variant::FLOAT, "delta")));
    ADD_SIGNAL(MethodInfo(
        "dod_story_signal_emitted",
        PropertyInfo(Variant::INT, "signal_type"),
        PropertyInfo(Variant::INT, "source_slot"),
        PropertyInfo(Variant::INT, "related_slot"),
        PropertyInfo(Variant::INT, "status_payload")
    ));
}

void DataOrientedNarrative::initialize() {
    Narrative::initialize();
    if (dod_execution_mode_ != NARRATIVE_DOD_EXEC_DISABLED) {
        setup_dod_pipeline();
    }
}

// --- Property Setters / Getters ---
void DataOrientedNarrative::set_dod_execution_mode(NarrativeDODExecutionMode p_mode) { dod_execution_mode_ = p_mode; }
NarrativeDODExecutionMode DataOrientedNarrative::get_dod_execution_mode() const { return dod_execution_mode_; }

void DataOrientedNarrative::set_pipeline_stage_mask(int p_mask) { pipeline_stage_mask_ = static_cast<uint32_t>(p_mask); }
int DataOrientedNarrative::get_pipeline_stage_mask() const { return static_cast<int>(pipeline_stage_mask_); }

void DataOrientedNarrative::set_auto_connect_pipeline_stages(bool p_auto_connect) { auto_connect_pipeline_stages_ = p_auto_connect; }
bool DataOrientedNarrative::get_auto_connect_pipeline_stages() const { return auto_connect_pipeline_stages_; }

void DataOrientedNarrative::set_auto_dispatch_signals_to_oop(bool p_auto_dispatch) { auto_dispatch_signals_to_oop_ = p_auto_dispatch; }
bool DataOrientedNarrative::get_auto_dispatch_signals_to_oop() const { return auto_dispatch_signals_to_oop_; }

void DataOrientedNarrative::set_auto_sync_resources_from_dod(bool p_auto_sync) { auto_sync_resources_from_dod_ = p_auto_sync; }
bool DataOrientedNarrative::get_auto_sync_resources_from_dod() const { return auto_sync_resources_from_dod_; }

void DataOrientedNarrative::set_custom_task_graph_resource(const godot::Ref<TaskGraphResource>& p_resource) { custom_task_graph_resource_ = p_resource; }
godot::Ref<TaskGraphResource> DataOrientedNarrative::get_custom_task_graph_resource() const { return custom_task_graph_resource_; }

void DataOrientedNarrative::set_max_dod_narremes(int p_max) { max_dod_narremes_ = std::max(16, p_max); }
int DataOrientedNarrative::get_max_dod_narremes() const { return max_dod_narremes_; }

void DataOrientedNarrative::set_max_dod_symbolic_slots(int p_max) { max_dod_symbolic_slots_ = std::max(16, p_max); }
int DataOrientedNarrative::get_max_dod_symbolic_slots() const { return max_dod_symbolic_slots_; }

void DataOrientedNarrative::set_max_dod_relationships(int p_max) { max_dod_relationships_ = std::max(16, p_max); }
int DataOrientedNarrative::get_max_dod_relationships() const { return max_dod_relationships_; }

void DataOrientedNarrative::set_max_dod_conditions(int p_max) { max_dod_conditions_ = std::max(16, p_max); }
int DataOrientedNarrative::get_max_dod_conditions() const { return max_dod_conditions_; }

void DataOrientedNarrative::set_max_dod_incidents(int p_max) { max_dod_incidents_ = std::max(16, p_max); }
int DataOrientedNarrative::get_max_dod_incidents() const { return max_dod_incidents_; }

void DataOrientedNarrative::set_max_dod_plots(int p_max) { max_dod_plots_ = std::max(16, p_max); }
int DataOrientedNarrative::get_max_dod_plots() const { return max_dod_plots_; }

// ============================================================================
// Pipeline Setup, Authoring Resource Baking & Graph Compilation
// ============================================================================

core::NarrativesBufferCapacities DataOrientedNarrative::_build_capacities() const noexcept {
    core::NarrativesBufferCapacities caps{};
    caps.max_narremes        = static_cast<uint32_t>(std::max(16, max_dod_narremes_));
    caps.max_symbolic_slots  = static_cast<uint32_t>(std::max(16, max_dod_symbolic_slots_));
    caps.max_relationships   = static_cast<uint32_t>(std::max(16, max_dod_relationships_));
    caps.max_conditions      = static_cast<uint32_t>(std::max(16, max_dod_conditions_));
    caps.max_incidents       = static_cast<uint32_t>(std::max(16, max_dod_incidents_));
    caps.max_plots           = static_cast<uint32_t>(std::max(16, max_dod_plots_));
    caps.max_plot_events     = caps.max_plots * 4u;
    caps.max_causal_links    = caps.max_conditions * 2u;
    caps.max_incident_condition_refs = caps.max_incidents * 4u;
    return caps;
}

void DataOrientedNarrative::_ensure_narrative_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager) {
    if (!narrative_dod_ || (p_shared_manager && narrative_dod_->get_memory_manager() != p_shared_manager)) {
        narrative_dod_ = std::make_unique<core::NarrativeDOD>(std::move(p_shared_manager), _build_capacities());
        active_task_graph_ = narrative_dod_->get_default_task_graph();
    }
}

void DataOrientedNarrative::_clear_resource_maps() noexcept {
    object_id_to_narreme_slot_.clear();
    narreme_slot_to_resource_.clear();
    object_id_to_symbolic_slot_.clear();
    symbolic_slot_to_resource_.clear();
    object_id_to_condition_slot_.clear();
    condition_slot_to_resource_.clear();
    object_id_to_incident_slot_.clear();
    incident_slot_to_resource_.clear();
    object_id_to_plot_slot_.clear();
    plot_slot_to_resource_.clear();
}

void DataOrientedNarrative::setup_dod_pipeline_with_manager(
    std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
    std::shared_ptr<core::TaskGraphDOD> p_shared_graph
) {
    _ensure_narrative_dod_initialized(std::move(p_shared_manager));
    if (p_shared_graph && narrative_dod_) {
        narrative_dod_->set_default_task_graph(p_shared_graph);
        active_task_graph_ = std::move(p_shared_graph);
    }
    bake_to_dod();
    rebuild_dod_task_graph();
    emit_signal("dod_pipeline_ready");
}

void DataOrientedNarrative::setup_dod_pipeline() {
    setup_dod_pipeline_with_manager(nullptr, nullptr);
}

uint32_t DataOrientedNarrative::_register_narreme_resource(
    const godot::Ref<Narreme>& p_res,
    uint8_t p_narreme_type,
    bool p_is_symbolic
) {
    if (p_res.is_null() || !narrative_dod_) return core::INVALID_ID;
    const uint64_t obj_id = p_res->get_instance_id();

    if (auto it = object_id_to_narreme_slot_.find(obj_id); it != object_id_to_narreme_slot_.end()) {
        return it->second;
    }

    core::NarremeIdentityRowPOD row{};
    row.object_id    = obj_id;
    row.narreme_type = p_narreme_type;
    row.is_symbolic  = p_is_symbolic ? 1u : 0u;

    const std::string name_utf8 = p_res->get_official_name().utf8().get_data();
    const uint32_t name_token   = narrative_dod_->intern_name_token(name_utf8);
    if (!name_utf8.empty()) {
        const std::string_view trait_sv(name_utf8);
        narrative_dod_->compute_trait_bitmask_128(std::span<const std::string_view>(&trait_sv, 1), row.trait_bitmask_lo, row.trait_bitmask_hi);
    }

    const uint32_t slot = narrative_dod_->allocate_narreme(row);
    if (slot == core::INVALID_ID) return core::INVALID_ID;

    object_id_to_narreme_slot_[obj_id] = slot;
    narreme_slot_to_resource_[slot]    = p_res;

    if (p_is_symbolic) {
        core::SymbolicAssignmentRecordPOD sym_rec{};
        sym_rec.narreme_type = p_narreme_type;
        const uint32_t sym_slot = narrative_dod_->allocate_symbolic_slot(sym_rec, name_token);
        if (sym_slot != core::INVALID_ID) {
            object_id_to_symbolic_slot_[obj_id]  = sym_slot;
            symbolic_slot_to_resource_[sym_slot] = p_res;
        }
    }

    if (p_narreme_type == core::NARREME_TYPE_INCIDENT) {
        if (godot::Ref<Incident> inc = p_res; inc.is_valid()) {
            core::IncidentStateRowPOD inc_row{};
            inc_row.status = static_cast<int8_t>(inc->get_status());
            const uint32_t inc_slot = narrative_dod_->allocate_incident(inc_row);
            if (inc_slot != core::INVALID_ID) {
                object_id_to_incident_slot_[obj_id]  = inc_slot;
                incident_slot_to_resource_[inc_slot] = inc;
            }
        }
    } else if (p_narreme_type == core::NARREME_TYPE_PLOT) {
        if (godot::Ref<Plot> plot = p_res; plot.is_valid()) {
            core::PlotProgressionRowPOD plot_row{};
            plot_row.status = static_cast<int8_t>(plot->get_status());
            const uint32_t plot_slot = narrative_dod_->allocate_plot(plot_row);
            if (plot_slot != core::INVALID_ID) {
                object_id_to_plot_slot_[obj_id]  = plot_slot;
                plot_slot_to_resource_[plot_slot] = plot;
            }
        }
    }

    return slot;
}

template <typename T_Sym, typename T_Lit>
void DataOrientedNarrative::_bake_symbolic_assignment_pair(
    const godot::TypedArray<T_Sym>& p_symbolic_arr,
    const godot::TypedArray<T_Lit>& p_literal_arr,
    const godot::TypedArray<int>& p_assignments
) {
    if (!narrative_dod_) return;
    const size_t sym_cnt = p_symbolic_arr.size();
    const size_t lit_cnt = p_literal_arr.size();
    const size_t asg_cnt = p_assignments.size();

    for (size_t i = 0; i < sym_cnt; ++i) {
        godot::Ref<Narreme> sym_res = p_symbolic_arr[i];
        if (sym_res.is_null()) continue;
        const int sym_slot = get_symbolic_dod_slot(sym_res);
        if (sym_slot < 0) continue;

        int32_t lit_slot = -1;
        if (i < asg_cnt) {
            const int lit_idx = static_cast<int>(p_assignments[i]);
            if (lit_idx >= 0 && static_cast<size_t>(lit_idx) < lit_cnt) {
                godot::Ref<Narreme> lit_res = p_literal_arr[lit_idx];
                if (lit_res.is_valid()) {
                    lit_slot = get_narreme_dod_slot(lit_res);
                }
            }
        }
        narrative_dod_->assign_symbolic_to_literal(static_cast<uint32_t>(sym_slot), lit_slot, 0);
    }
}

uint32_t DataOrientedNarrative::_bake_condition_recursive(const godot::Ref<Incident_Condition>& p_cond) {
    if (p_cond.is_null() || !narrative_dod_) return core::INVALID_ID;
    const uint64_t obj_id = p_cond->get_instance_id();

    if (auto it = object_id_to_condition_slot_.find(obj_id); it != object_id_to_condition_slot_.end()) {
        return it->second;
    }

    core::ConditionEvaluationRowPOD row{};
    row.current_status = static_cast<int8_t>(p_cond->get_last_status());
    std::vector<core::CausalLinkPOD> causal_links;

    if (godot::Ref<Narrative_Condition> nar_cond = p_cond; nar_cond.is_valid()) {
        row.condition_type      = core::CONDITION_TYPE_NARRATIVE;
        row.predicate_op        = static_cast<uint16_t>(std::max(0, nar_cond->get_condition()));
        row.subject_is_symbolic = nar_cond->get_subject_is_symbolic() ? 1u : 0u;
        row.object_is_symbolic  = nar_cond->get_object_is_symbolic() ? 1u : 0u;

        if (godot::Ref<Narreme> subj = nar_cond->get_subject(); subj.is_valid()) {
            const int s_slot = row.subject_is_symbolic ? get_symbolic_dod_slot(subj) : get_narreme_dod_slot(subj);
            if (s_slot >= 0) row.subject_id = static_cast<uint32_t>(s_slot);
        }
        if (godot::Ref<Narreme> obj = nar_cond->get_object(); obj.is_valid()) {
            const int o_slot = row.object_is_symbolic ? get_symbolic_dod_slot(obj) : get_narreme_dod_slot(obj);
            if (o_slot >= 0) row.object_id = static_cast<uint32_t>(o_slot);
        }
    } else if (godot::Ref<Causal_Condition> caus_cond = p_cond; caus_cond.is_valid()) {
        row.condition_type = core::CONDITION_TYPE_CAUSAL;
        const godot::TypedArray<Incident_Condition> children = caus_cond->get_conditions();
        const godot::TypedArray<int> causes = caus_cond->get_causes();
        const size_t cnt = children.size();
        causal_links.reserve(cnt);

        for (size_t i = 0; i < cnt; ++i) {
            godot::Ref<Incident_Condition> child = children[i];
            const uint32_t child_slot = _bake_condition_recursive(child);
            if (child_slot != core::INVALID_ID) {
                core::CausalLinkPOD link{};
                link.child_condition_id    = child_slot;
                link.required_cause_status = (i < causes.size())
                    ? static_cast<int8_t>(static_cast<int>(causes[i]))
                    : static_cast<int8_t>(core::COND_STATUS_MET);
                causal_links.push_back(link);
            }
        }
    } else if (godot::Ref<Gameplay_Condition> gp_cond = p_cond; gp_cond.is_valid()) {
        row.condition_type = core::CONDITION_TYPE_GAMEPLAY;
        row.current_status = static_cast<int8_t>(gp_cond->get_gameplay_status());
    }

    const uint32_t cond_slot = narrative_dod_->allocate_condition(row, causal_links);
    if (cond_slot != core::INVALID_ID) {
        object_id_to_condition_slot_[obj_id]   = cond_slot;
        condition_slot_to_resource_[cond_slot] = p_cond;
    }
    return cond_slot;
}

void DataOrientedNarrative::bake_to_dod() {
    _ensure_narrative_dod_initialized();
    if (!narrative_dod_) return;

    narrative_dod_->reset_all_slots();
    _clear_resource_maps();

    auto register_array = [&](const auto& p_arr, uint8_t p_type, bool p_sym) {
        for (size_t i = 0, n = p_arr.size(); i < n; ++i) {
            _register_narreme_resource(p_arr[i], p_type, p_sym);
        }
    };

    // 1. Register all 5 Literal and 5 Symbolic Narreme TypedArrays
    register_array(get_locations(), core::NARREME_TYPE_LOCATION, false);
    register_array(get_places(),    core::NARREME_TYPE_LOCATION, true);
    register_array(get_characters(),core::NARREME_TYPE_CHARACTER, false);
    register_array(get_roles(),     core::NARREME_TYPE_CHARACTER, true);
    register_array(get_props(),     core::NARREME_TYPE_PROP, false);
    register_array(get_symbols(),   core::NARREME_TYPE_PROP, true);
    register_array(get_incidents(), core::NARREME_TYPE_INCIDENT, false);
    register_array(get_moments(),   core::NARREME_TYPE_INCIDENT, true);
    register_array(get_plots(),     core::NARREME_TYPE_PLOT, false);
    register_array(get_stories(),   core::NARREME_TYPE_PLOT, true);

    // 2. Bake initial Symbolic <-> Literal assignments across all 5 pairs
    _bake_symbolic_assignment_pair(get_roles(),   get_characters(), get_role_assignments());
    _bake_symbolic_assignment_pair(get_symbols(), get_props(),      get_symbol_assignments());
    _bake_symbolic_assignment_pair(get_places(),  get_locations(),  get_place_assignments());
    _bake_symbolic_assignment_pair(get_moments(), get_incidents(),  get_moment_assignments());
    _bake_symbolic_assignment_pair(get_stories(), get_plots(),      get_story_assignments());

    // 3. Bake spatial occupancy, possessions, character goals, and relationship CSR slices
    auto bake_character_group = [&](const auto& p_chars) {
        for (size_t i = 0, n = p_chars.size(); i < n; ++i) {
            godot::Ref<Character> chr = p_chars[i];
            if (chr.is_null()) continue;
            const int slot = get_narreme_dod_slot(chr);
            if (slot < 0) continue;

            core::NarremeIdentityRowPOD row = narrative_dod_->get_narreme_row(static_cast<uint32_t>(slot));
            if (godot::Ref<Location> loc = chr->get_current_location(); loc.is_valid()) {
                const int loc_slot = get_narreme_dod_slot(loc);
                if (loc_slot >= 0) row.location_id = static_cast<uint32_t>(loc_slot);
            }

            const godot::TypedArray<Plot> goals = chr->get_goals();
            for (size_t g = 0; g < goals.size(); ++g) {
                const int plot_slot = get_plot_dod_slot(goals[g]);
                if (plot_slot >= 0) {
                    if (row.primary_goal_plot_id == core::INVALID_ID) {
                        row.primary_goal_plot_id = static_cast<uint32_t>(plot_slot);
                    }
                    if (plot_slot < 64) row.character_goal_mask |= (1ULL << plot_slot);
                }
            }
            narrative_dod_->set_narreme_row(static_cast<uint32_t>(slot), row);

            const godot::TypedArray<Relationship> rels = chr->get_relationships();
            std::vector<core::RelationshipEdgePOD> edges;
            edges.reserve(rels.size());
            for (size_t r = 0; r < rels.size(); ++r) {
                godot::Ref<Relationship> rel = rels[r];
                if (rel.is_null() || rel->get_relation().is_null()) continue;
                const int tgt_slot = get_narreme_dod_slot(rel->get_relation());
                if (tgt_slot >= 0) {
                    core::RelationshipEdgePOD edge{};
                    edge.target_narreme_id        = static_cast<uint32_t>(tgt_slot);
                    edge.relationship_title_token = narrative_dod_->intern_name_token(rel->get_title().utf8().get_data());
                    edges.push_back(edge);
                }
            }
            if (!edges.empty()) {
                narrative_dod_->append_narreme_relationships(static_cast<uint32_t>(slot), edges);
            }
        }
    };
    bake_character_group(get_characters());
    bake_character_group(get_roles());

    auto bake_prop_group = [&](const auto& p_props) {
        for (size_t i = 0, n = p_props.size(); i < n; ++i) {
            godot::Ref<Prop> prop = p_props[i];
            if (prop.is_null()) continue;
            const int slot = get_narreme_dod_slot(prop);
            if (slot < 0) continue;

            core::NarremeIdentityRowPOD row = narrative_dod_->get_narreme_row(static_cast<uint32_t>(slot));
            if (godot::Ref<Location> loc = prop->get_current_location(); loc.is_valid()) {
                const int loc_slot = get_narreme_dod_slot(loc);
                if (loc_slot >= 0) row.location_id = static_cast<uint32_t>(loc_slot);
            }
            if (godot::Ref<Character> owner = prop->get_possessor(); owner.is_valid()) {
                const int owner_slot = get_narreme_dod_slot(owner);
                if (owner_slot >= 0) row.possessor_id = static_cast<uint32_t>(owner_slot);
            }
            narrative_dod_->set_narreme_row(static_cast<uint32_t>(slot), row);
        }
    };
    bake_prop_group(get_props());
    bake_prop_group(get_symbols());

    // 4. Bake Incident condition trees and Plot event progressions
    auto bake_incident_group = [&](const auto& p_incidents) {
        for (size_t i = 0, n = p_incidents.size(); i < n; ++i) {
            godot::Ref<Incident> inc = p_incidents[i];
            if (inc.is_null()) continue;
            const int inc_slot = get_incident_dod_slot(inc);
            if (inc_slot < 0) continue;

            const godot::TypedArray<Incident_Condition> conds = inc->get_conditions();
            std::vector<uint32_t> prereq_ids;
            std::vector<uint32_t> causal_ids;
            prereq_ids.reserve(conds.size());

            for (size_t c = 0; c < conds.size(); ++c) {
                godot::Ref<Incident_Condition> cond = conds[c];
                const uint32_t c_slot = _bake_condition_recursive(cond);
                if (c_slot != core::INVALID_ID) {
                    prereq_ids.push_back(c_slot);
                    if (godot::Ref<Causal_Condition>(cond).is_valid()) {
                        causal_ids.push_back(c_slot);
                    }
                }
            }

            core::IncidentStateRowPOD row = narrative_dod_->get_incident_row(static_cast<uint32_t>(inc_slot));
            row.status = static_cast<int8_t>(inc->get_status());
            if (!prereq_ids.empty()) {
                auto* idx_col = narrative_dod_->get_column_ptr<uint32_t>(core::NarrativeBufferSlot::INCIDENT_COND_INDEX_TABLE);
                row.cond_slice_offset = static_cast<uint32_t>(inc_slot) * 4u;
                row.cond_slice_count  = static_cast<uint16_t>(prereq_ids.size());
                if (idx_col && row.cond_slice_offset + prereq_ids.size() <= narrative_dod_->get_capacities().max_incident_condition_refs) {
                    std::copy(prereq_ids.begin(), prereq_ids.end(), idx_col + row.cond_slice_offset);
                }
            }
            narrative_dod_->set_incident_row(static_cast<uint32_t>(inc_slot), row);
        }
    };
    bake_incident_group(get_incidents());
    bake_incident_group(get_moments());

    auto bake_plot_group = [&](const auto& p_plots) {
        for (size_t i = 0, n = p_plots.size(); i < n; ++i) {
            godot::Ref<Plot> plot = p_plots[i];
            if (plot.is_null()) continue;
            const int plot_slot = get_plot_dod_slot(plot);
            if (plot_slot < 0) continue;

            const godot::TypedArray<Plot_Event> p_events = plot->get_plot_events();
            std::vector<core::PlotEventPOD> dod_events;
            dod_events.reserve(p_events.size());

            for (size_t e = 0; e < p_events.size(); ++e) {
                godot::Ref<Plot_Event> pev = p_events[e];
                if (pev.is_null() || pev->get_incident().is_null()) continue;
                const int inc_slot = get_incident_dod_slot(pev->get_incident());
                if (inc_slot >= 0) {
                    core::PlotEventPOD pod{};
                    pod.incident_id      = static_cast<uint32_t>(inc_slot);
                    pod.requirement_type = static_cast<uint8_t>(pev->get_requirement());
                    dod_events.push_back(pod);
                }
            }
            if (!dod_events.empty()) {
                narrative_dod_->append_plot_events(static_cast<uint32_t>(plot_slot), dod_events);
            }
        }
    };
    bake_plot_group(get_plots());
    bake_plot_group(get_stories());

    narrative_dod_->sync_selection_ranges();
}

void DataOrientedNarrative::rebuild_dod_task_graph() {
    _ensure_narrative_dod_initialized();
    if (!narrative_dod_) return;

    switch (dod_execution_mode_) {
        case NARRATIVE_DOD_EXEC_DISABLED:
            return;

        case NARRATIVE_DOD_EXEC_COMPOSABLE_STAGES: {
            active_task_graph_ = narrative_dod_->get_default_task_graph();
            if (active_task_graph_) {
                active_task_graph_->clear();
                active_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
                narrative_dod_->inject_pipeline_stages(*active_task_graph_, pipeline_stage_mask_, auto_connect_pipeline_stages_);
            }
            break;
        }

        case NARRATIVE_DOD_EXEC_CUSTOM_GRAPH_RESOURCE: {
            if (custom_task_graph_resource_.is_valid() && narrative_dod_->get_memory_manager_raw()) {
                ui_to_dod_map_.clear();
                active_task_graph_ = custom_task_graph_resource_->compile_to_task_graph(
                    narrative_dod_->get_memory_manager_raw(),
                    ui_to_dod_map_
                );
                if (active_task_graph_) {
                    narrative_dod_->set_default_task_graph(active_task_graph_);
                }
            }
            break;
        }

        case NARRATIVE_DOD_EXEC_MANUAL_GRAPH: {
            active_task_graph_ = narrative_dod_->get_default_task_graph();
            if (active_task_graph_) {
                active_task_graph_->clear();
                active_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
            }
            break;
        }
    }
}

void DataOrientedNarrative::execute_dod_step(double p_delta) {
    if (!narrative_dod_ || dod_execution_mode_ == NARRATIVE_DOD_EXEC_DISABLED) return;

    narrative_dod_->execute_step(p_delta, active_task_graph_.get());

    if (dod_execution_mode_ == NARRATIVE_DOD_EXEC_CUSTOM_GRAPH_RESOURCE &&
        custom_task_graph_resource_.is_valid() && active_task_graph_) {
        custom_task_graph_resource_->sync_execution_snapshots(active_task_graph_, ui_to_dod_map_);
    }

    if (auto_sync_resources_from_dod_) {
        sync_oop_resources_from_dod();
    }
    if (auto_dispatch_signals_to_oop_) {
        dispatch_dod_signals_to_oop();
    }

    emit_signal("dod_step_completed", p_delta);
}

void DataOrientedNarrative::sync_oop_resources_from_dod() {
    if (!narrative_dod_) return;

    for (const auto& [slot, cond_res] : condition_slot_to_resource_) {
        if (cond_res.is_valid()) {
            const auto row = narrative_dod_->get_condition_row(slot);
            cond_res->set_last_status(static_cast<NarremeConditionStatus>(row.current_status));
        }
    }
    for (const auto& [slot, inc_res] : incident_slot_to_resource_) {
        if (inc_res.is_valid()) {
            const auto row = narrative_dod_->get_incident_row(slot);
            inc_res->set_status(static_cast<IncidentStatus>(row.status));
        }
    }
    for (const auto& [slot, plot_res] : plot_slot_to_resource_) {
        if (plot_res.is_valid()) {
            const auto row = narrative_dod_->get_plot_row(slot);
            plot_res->set_status(static_cast<PlotStatus>(row.status));
        }
    }
}

int DataOrientedNarrative::dispatch_dod_signals_to_oop() {
    if (!narrative_dod_) return 0;

    int dispatched = 0;

    // 1. Drain CharacterGoalEventPOD ring
    std::vector<core::CharacterGoalEventPOD> goal_events;
    narrative_dod_->drain_character_goal_events(goal_events);
    for (const auto& gev : goal_events) {
        godot::Ref<Character> chr = get_narreme_for_dod_slot(static_cast<int>(gev.character_id));
        godot::Ref<Plot> plot     = get_plot_for_dod_slot(static_cast<int>(gev.plot_id));
        if (chr.is_valid() && plot.is_valid()) {
            if ((gev.outcome_flags & core::GOAL_OUTCOME_ACHIEVED) != 0u) {
                chr->emit_signal("goal_achieved", plot);
                ++dispatched;
            } else if ((gev.outcome_flags & core::GOAL_OUTCOME_FAILED) != 0u) {
                chr->emit_signal("goal_failed", plot);
                ++dispatched;
            }
        }
    }

    // 2. Drain NarrativeSignalCommandPOD ring (emitted by N11 NarrativeResourceCommit)
    std::vector<core::NarrativeSignalCommandPOD> sig_cmds;
    narrative_dod_->drain_signal_commands(sig_cmds);

    for (const auto& cmd : sig_cmds) {
        emit_signal("dod_story_signal_emitted", static_cast<int>(cmd.signal_type), static_cast<int>(cmd.source_id), static_cast<int>(cmd.related_id), static_cast<int>(cmd.status_payload));

        switch (static_cast<core::NarrativeStorySignalType>(cmd.signal_type)) {
            case core::STORY_SIGNAL_INCIDENT_HAPPENED:
            case core::STORY_SIGNAL_INCIDENT_CANNOT_HAPPEN: {
                if (godot::Ref<Incident> inc = get_incident_for_dod_slot(static_cast<int>(cmd.source_id)); inc.is_valid()) {
                    inc->emit_signal(cmd.signal_type == core::STORY_SIGNAL_INCIDENT_HAPPENED ? "happened" : "cannot_happen");
                    ++dispatched;
                }
                break;
            }
            case core::STORY_SIGNAL_PLOT_BEGUN:
            case core::STORY_SIGNAL_PLOT_COMPLETED:
            case core::STORY_SIGNAL_PLOT_CANNOT_COMPLETE:
            case core::STORY_SIGNAL_PLOT_FAILED: {
                if (godot::Ref<Plot> plot = get_plot_for_dod_slot(static_cast<int>(cmd.source_id)); plot.is_valid()) {
                    const char* sig_name = "begun";
                    if (cmd.signal_type == core::STORY_SIGNAL_PLOT_COMPLETED)            sig_name = "completed";
                    else if (cmd.signal_type == core::STORY_SIGNAL_PLOT_CANNOT_COMPLETE) sig_name = "cannot_complete";
                    else if (cmd.signal_type == core::STORY_SIGNAL_PLOT_FAILED)          sig_name = "failed";
                    plot->emit_signal(sig_name);
                    ++dispatched;
                }
                break;
            }
            case core::STORY_SIGNAL_CONDITION_MET:
            case core::STORY_SIGNAL_CONDITION_UNMET:
            case core::STORY_SIGNAL_CONDITION_CANNOT_MEET:
            case core::STORY_SIGNAL_CONDITION_UNKNOWN: {
                if (godot::Ref<Incident_Condition> cond = get_condition_for_dod_slot(static_cast<int>(cmd.source_id)); cond.is_valid()) {
                    const char* sig_name = "met";
                    if (cmd.signal_type == core::STORY_SIGNAL_CONDITION_UNMET)            sig_name = "unmet";
                    else if (cmd.signal_type == core::STORY_SIGNAL_CONDITION_CANNOT_MEET) sig_name = "cannot_meet";
                    else if (cmd.signal_type == core::STORY_SIGNAL_CONDITION_UNKNOWN)     sig_name = "became_unknown";
                    cond->emit_signal(sig_name);
                    ++dispatched;
                }
                break;
            }
            case core::STORY_SIGNAL_ROLE_FILLED:
            case core::STORY_SIGNAL_ROLE_VACATED: {
                godot::Ref<Narreme> sym_res = get_symbolic_for_dod_slot(static_cast<int>(cmd.source_id));
                godot::Ref<Narreme> lit_res = get_narreme_for_dod_slot(static_cast<int>(cmd.related_id));
                const bool filled = (cmd.signal_type == core::STORY_SIGNAL_ROLE_FILLED);
                const char* sig_name = filled ? "role_filled" : "role_vacated";
                if (cmd.narreme_type == core::NARREME_TYPE_PROP)          sig_name = filled ? "symbol_filled" : "symbol_vacated";
                else if (cmd.narreme_type == core::NARREME_TYPE_LOCATION) sig_name = filled ? "place_filled"  : "place_vacated";
                else if (cmd.narreme_type == core::NARREME_TYPE_INCIDENT) sig_name = filled ? "moment_filled" : "moment_vacated";
                else if (cmd.narreme_type == core::NARREME_TYPE_PLOT)     sig_name = filled ? "story_filled"  : "story_vacated";
                emit_signal(sig_name, sym_res, lit_res);
                ++dispatched;
                break;
            }
            case core::STORY_SIGNAL_LOCATION_ENTERED:
            case core::STORY_SIGNAL_LOCATION_EXITED: {
                godot::Ref<Narreme> ent_res  = get_narreme_for_dod_slot(static_cast<int>(cmd.source_id));
                godot::Ref<Location> loc_res = get_narreme_for_dod_slot(static_cast<int>(cmd.related_id));
                const bool entered = (cmd.signal_type == core::STORY_SIGNAL_LOCATION_ENTERED);
                if (ent_res.is_valid()) {
                    ent_res->emit_signal(entered ? "location_entered" : "location_exited", loc_res);
                    ++dispatched;
                }
                if (loc_res.is_valid() && ent_res.is_valid()) {
                    if (godot::Ref<Character>(ent_res).is_valid()) {
                        loc_res->emit_signal(entered ? "character_entered" : "character_exited", ent_res);
                    } else if (godot::Ref<Prop>(ent_res).is_valid()) {
                        loc_res->emit_signal(entered ? "prop_entered" : "prop_exited", ent_res);
                    }
                }
                break;
            }
            case core::STORY_SIGNAL_PROP_TAKEN:
            case core::STORY_SIGNAL_PROP_RELEASED:
            case core::STORY_SIGNAL_PROP_LOST:
            case core::STORY_SIGNAL_PROP_FOUND: {
                godot::Ref<Prop> prop_res      = get_narreme_for_dod_slot(static_cast<int>(cmd.source_id));
                godot::Ref<Character> char_res = get_narreme_for_dod_slot(static_cast<int>(cmd.related_id));
                if (prop_res.is_valid()) {
                    if (cmd.signal_type == core::STORY_SIGNAL_PROP_TAKEN)         prop_res->emit_signal("taken", char_res);
                    else if (cmd.signal_type == core::STORY_SIGNAL_PROP_RELEASED) prop_res->emit_signal("released", char_res);
                    else if (cmd.signal_type == core::STORY_SIGNAL_PROP_LOST)     prop_res->emit_signal("became_lost");
                    else if (cmd.signal_type == core::STORY_SIGNAL_PROP_FOUND)    prop_res->emit_signal("found", char_res);
                    ++dispatched;
                }
                if (char_res.is_valid() && prop_res.is_valid()) {
                    if (cmd.signal_type == core::STORY_SIGNAL_PROP_TAKEN)         char_res->emit_signal("possession_taken", prop_res);
                    else if (cmd.signal_type == core::STORY_SIGNAL_PROP_RELEASED) char_res->emit_signal("possession_lost", prop_res);
                }
                break;
            }
            default:
                break;
        }
    }

    return dispatched;
}

// ============================================================================
// Buffer, Slot Lookup & Event Staging Helpers
// ============================================================================

int DataOrientedNarrative::get_buffer_id(const godot::StringName& p_semantic_name) const {
    if (!narrative_dod_) return -1;
    const godot::String s(p_semantic_name);
    const uint32_t id = narrative_dod_->get_buffer_id_by_name(s.utf8().get_data());
    return (id == core::INVALID_ID) ? -1 : static_cast<int>(id);
}

godot::Dictionary DataOrientedNarrative::get_buffer_manifest_dictionary() const {
    godot::Dictionary d;
    if (narrative_dod_) narrative_dod_->populate_task_buffer_dictionary(d, true);
    return d;
}

int DataOrientedNarrative::get_narreme_dod_slot(const godot::Ref<Narreme>& p_narreme) const {
    if (p_narreme.is_null()) return -1;
    auto it = object_id_to_narreme_slot_.find(p_narreme->get_instance_id());
    return (it != object_id_to_narreme_slot_.end()) ? static_cast<int>(it->second) : -1;
}

int DataOrientedNarrative::get_symbolic_dod_slot(const godot::Ref<Narreme>& p_symbolic) const {
    if (p_symbolic.is_null()) return -1;
    auto it = object_id_to_symbolic_slot_.find(p_symbolic->get_instance_id());
    return (it != object_id_to_symbolic_slot_.end()) ? static_cast<int>(it->second) : -1;
}

int DataOrientedNarrative::get_condition_dod_slot(const godot::Ref<Incident_Condition>& p_condition) const {
    if (p_condition.is_null()) return -1;
    auto it = object_id_to_condition_slot_.find(p_condition->get_instance_id());
    return (it != object_id_to_condition_slot_.end()) ? static_cast<int>(it->second) : -1;
}

int DataOrientedNarrative::get_incident_dod_slot(const godot::Ref<Incident>& p_incident) const {
    if (p_incident.is_null()) return -1;
    auto it = object_id_to_incident_slot_.find(p_incident->get_instance_id());
    return (it != object_id_to_incident_slot_.end()) ? static_cast<int>(it->second) : -1;
}

int DataOrientedNarrative::get_plot_dod_slot(const godot::Ref<Plot>& p_plot) const {
    if (p_plot.is_null()) return -1;
    auto it = object_id_to_plot_slot_.find(p_plot->get_instance_id());
    return (it != object_id_to_plot_slot_.end()) ? static_cast<int>(it->second) : -1;
}

godot::Ref<Narreme> DataOrientedNarrative::get_narreme_for_dod_slot(int p_slot) const {
    if (p_slot < 0) return {};
    auto it = narreme_slot_to_resource_.find(static_cast<uint32_t>(p_slot));
    return (it != narreme_slot_to_resource_.end()) ? it->second : godot::Ref<Narreme>();
}

godot::Ref<Narreme> DataOrientedNarrative::get_symbolic_for_dod_slot(int p_slot) const {
    if (p_slot < 0) return {};
    auto it = symbolic_slot_to_resource_.find(static_cast<uint32_t>(p_slot));
    return (it != symbolic_slot_to_resource_.end()) ? it->second : godot::Ref<Narreme>();
}

godot::Ref<Incident_Condition> DataOrientedNarrative::get_condition_for_dod_slot(int p_slot) const {
    if (p_slot < 0) return {};
    auto it = condition_slot_to_resource_.find(static_cast<uint32_t>(p_slot));
    return (it != condition_slot_to_resource_.end()) ? it->second : godot::Ref<Incident_Condition>();
}

godot::Ref<Incident> DataOrientedNarrative::get_incident_for_dod_slot(int p_slot) const {
    if (p_slot < 0) return {};
    auto it = incident_slot_to_resource_.find(static_cast<uint32_t>(p_slot));
    return (it != incident_slot_to_resource_.end()) ? it->second : godot::Ref<Incident>();
}

godot::Ref<Plot> DataOrientedNarrative::get_plot_for_dod_slot(int p_slot) const {
    if (p_slot < 0) return {};
    auto it = plot_slot_to_resource_.find(static_cast<uint32_t>(p_slot));
    return (it != plot_slot_to_resource_.end()) ? it->second : godot::Ref<Plot>();
}

bool DataOrientedNarrative::queue_spatial_transfer(
    const godot::Ref<Narreme>& p_narreme,
    const godot::Ref<Location>& p_new_location,
    const godot::Ref<Character>& p_new_possessor
) {
    if (!narrative_dod_ || p_narreme.is_null()) return false;
    const int nar_slot = get_narreme_dod_slot(p_narreme);
    if (nar_slot < 0) return false;

    core::NarremeSpatialTransferCommandPOD cmd{};
    cmd.narreme_id   = static_cast<uint32_t>(nar_slot);
    cmd.narreme_type = godot::Ref<Prop>(p_narreme).is_valid()
        ? static_cast<uint8_t>(core::NARREME_TYPE_PROP)
        : static_cast<uint8_t>(core::NARREME_TYPE_CHARACTER);

    if (p_new_possessor.is_valid()) {
        cmd.target_id   = static_cast<uint32_t>(std::max(0, get_narreme_dod_slot(p_new_possessor)));
        cmd.transfer_op = core::SPATIAL_OP_PROP_TAKEN_BY;
    } else if (p_new_location.is_valid()) {
        cmd.target_id   = static_cast<uint32_t>(std::max(0, get_narreme_dod_slot(p_new_location)));
        cmd.transfer_op = core::SPATIAL_OP_ENTER_LOCATION;
    } else {
        cmd.target_id   = core::INVALID_ID;
        cmd.transfer_op = core::SPATIAL_OP_LEAVE_LOCATION;
    }
    return narrative_dod_->push_spatial_transfer_command(cmd);
}

bool DataOrientedNarrative::queue_gameplay_condition_event(
    const godot::Ref<Incident_Condition>& p_condition,
    int p_new_status,
    int p_property_value
) {
    if (!narrative_dod_ || p_condition.is_null()) return false;
    const int cond_slot = get_condition_dod_slot(p_condition);
    if (cond_slot < 0) return false;

    core::GameplayConditionEventPOD ev{};
    ev.condition_id   = static_cast<uint32_t>(cond_slot);
    ev.target_status  = static_cast<int8_t>(std::clamp(p_new_status, -2, 2));
    ev.gameplay_value = p_property_value;
    return narrative_dod_->push_gameplay_condition_event(ev);
}

// ============================================================================
// Dynamic Task Node & Wave Co-Scheduling Wrappers
// ============================================================================

namespace {

core::NarrativesNodeExecutionConfig make_exec_config(int p_priority, bool p_factory, int p_view, int p_strategy, int p_type, int p_query_op = 0) noexcept {
    core::NarrativesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_factory;
    cfg.view                 = static_cast<core::MemoryView>(std::clamp(p_view, 0, static_cast<int>(core::MemoryView::Count) - 1));
    cfg.strategy             = static_cast<core::MemoryStrategy>(std::clamp(p_strategy, 0, static_cast<int>(core::MemoryStrategy::Count) - 1));
    cfg.element_type         = static_cast<core::MemoryTypes>(std::clamp(p_type, 0, static_cast<int>(core::MemoryTypes::Count) - 1));
    cfg.query_op             = (p_query_op == static_cast<int>(core::QueryOp::ADD)) ? core::QueryOp::ADD : core::QueryOp::CULL;
    return cfg;
}

} // anonymous namespace

int DataOrientedNarrative::add_narratives_transform_node(int p_logic_id, const godot::Dictionary& p_props, int p_priority, bool p_factory, int p_view, int p_strategy, int p_type) {
    _ensure_narrative_dod_initialized();
    if (!narrative_dod_ || !active_task_graph_) return -1;
    const core::NodeID id = narrative_dod_->add_transform_node_by_id(
        *active_task_graph_, static_cast<core::NarrativesTransformLogicID>(p_logic_id), p_props,
        make_exec_config(p_priority, p_factory, p_view, p_strategy, p_type)
    );
    return (id == core::INVALID_ID) ? -1 : static_cast<int>(id);
}

int DataOrientedNarrative::add_narratives_query_node(int p_logic_id, int p_query_op, const godot::Dictionary& p_props, int p_priority, bool p_factory, int p_view, int p_strategy, int p_type) {
    _ensure_narrative_dod_initialized();
    if (!narrative_dod_ || !active_task_graph_) return -1;
    const core::NodeID id = narrative_dod_->add_query_node_by_id(
        *active_task_graph_, static_cast<core::NarrativesQueryLogicID>(p_logic_id), p_props,
        make_exec_config(p_priority, p_factory, p_view, p_strategy, p_type, p_query_op)
    );
    return (id == core::INVALID_ID) ? -1 : static_cast<int>(id);
}

int DataOrientedNarrative::add_narratives_metadata_node(int p_logic_id, const godot::Dictionary& p_props, int p_priority, bool p_factory, int p_view, int p_strategy, int p_type) {
    _ensure_narrative_dod_initialized();
    if (!narrative_dod_ || !active_task_graph_) return -1;
    const core::NodeID id = narrative_dod_->add_metadata_node_by_id(
        *active_task_graph_, static_cast<core::NarrativesMetadataLogicID>(p_logic_id), p_props,
        make_exec_config(p_priority, p_factory, p_view, p_strategy, p_type)
    );
    return (id == core::INVALID_ID) ? -1 : static_cast<int>(id);
}

bool DataOrientedNarrative::schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream) {
    if (!narrative_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::NarrativeDOD::schedule_node_with_stage(
        *active_task_graph_,
        narrative_dod_->get_active_topology(),
        static_cast<core::NarrativesPipelineStageFlags>(p_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id),
        p_join_downstream
    );
}

bool DataOrientedNarrative::schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id) {
    if (!narrative_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::NarrativeDOD::schedule_node_between_stages(
        *active_task_graph_,
        narrative_dod_->get_active_topology(),
        static_cast<core::NarrativesPipelineStageFlags>(p_upstream_stage_flag),
        static_cast<core::NarrativesPipelineStageFlags>(p_downstream_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id)
    );
}

} // namespace ideam::godot_ext
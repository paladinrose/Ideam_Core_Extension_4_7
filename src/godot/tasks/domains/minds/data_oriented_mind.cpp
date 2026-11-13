#include "data_oriented_mind.h"

#include <algorithm>
#include <string>

namespace ideam::godot_ext {

void DataOrientedMind::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(MIND_DOD_EXEC_DISABLED);
    BIND_ENUM_CONSTANT(MIND_DOD_EXEC_COMPOSABLE_STAGES);
    BIND_ENUM_CONSTANT(MIND_DOD_EXEC_CUSTOM_GRAPH_RESOURCE);
    BIND_ENUM_CONSTANT(MIND_DOD_EXEC_MANUAL_GRAPH);

    ADD_GROUP("Minds & Epistemic Profiles", "");

    ClassDB::bind_method(D_METHOD("set_agent_minds", "minds"), &DataOrientedMind::set_agent_minds);
    ClassDB::bind_method(D_METHOD("get_agent_minds"), &DataOrientedMind::get_agent_minds);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "agent_minds", PROPERTY_HINT_ARRAY_TYPE, "AgentMindResource"),
        "set_agent_minds",
        "get_agent_minds"
    );

    ADD_GROUP("DOD Pipeline Configuration", "");

    ClassDB::bind_method(D_METHOD("set_dod_execution_mode", "mode"), &DataOrientedMind::set_dod_execution_mode);
    ClassDB::bind_method(D_METHOD("get_dod_execution_mode"), &DataOrientedMind::get_dod_execution_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "dod_execution_mode", PROPERTY_HINT_ENUM, "Disabled:0,Composable Stages:1,Custom TaskGraphResource:2,Manual Graph:3"),
        "set_dod_execution_mode",
        "get_dod_execution_mode"
    );

    ClassDB::bind_method(D_METHOD("set_pipeline_stage_mask", "mask"), &DataOrientedMind::set_pipeline_stage_mask);
    ClassDB::bind_method(D_METHOD("get_pipeline_stage_mask"), &DataOrientedMind::get_pipeline_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "pipeline_stage_mask"), "set_pipeline_stage_mask", "get_pipeline_stage_mask");

    ClassDB::bind_method(D_METHOD("set_auto_connect_pipeline_stages", "auto_connect"), &DataOrientedMind::set_auto_connect_pipeline_stages);
    ClassDB::bind_method(D_METHOD("get_auto_connect_pipeline_stages"), &DataOrientedMind::get_auto_connect_pipeline_stages);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_connect_pipeline_stages"), "set_auto_connect_pipeline_stages", "get_auto_connect_pipeline_stages");

    ClassDB::bind_method(D_METHOD("set_auto_sync_resources_from_dod", "auto_sync"), &DataOrientedMind::set_auto_sync_resources_from_dod);
    ClassDB::bind_method(D_METHOD("get_auto_sync_resources_from_dod"), &DataOrientedMind::get_auto_sync_resources_from_dod);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_sync_resources_from_dod"), "set_auto_sync_resources_from_dod", "get_auto_sync_resources_from_dod");

    ClassDB::bind_method(D_METHOD("set_custom_task_graph_resource", "resource"), &DataOrientedMind::set_custom_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_custom_task_graph_resource"), &DataOrientedMind::get_custom_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "custom_task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_custom_task_graph_resource",
        "get_custom_task_graph_resource"
    );

    ADD_GROUP("DOD Buffer Capacities", "");

    ClassDB::bind_method(D_METHOD("set_max_dod_akashic_tomes", "max"), &DataOrientedMind::set_max_dod_akashic_tomes);
    ClassDB::bind_method(D_METHOD("get_max_dod_akashic_tomes"), &DataOrientedMind::get_max_dod_akashic_tomes);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_akashic_tomes"), "set_max_dod_akashic_tomes", "get_max_dod_akashic_tomes");

    ClassDB::bind_method(D_METHOD("set_max_dod_facts", "max"), &DataOrientedMind::set_max_dod_facts);
    ClassDB::bind_method(D_METHOD("get_max_dod_facts"), &DataOrientedMind::get_max_dod_facts);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_facts"), "set_max_dod_facts", "get_max_dod_facts");

    ClassDB::bind_method(D_METHOD("set_max_dod_agents", "max"), &DataOrientedMind::set_max_dod_agents);
    ClassDB::bind_method(D_METHOD("get_max_dod_agents"), &DataOrientedMind::get_max_dod_agents);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_agents"), "set_max_dod_agents", "get_max_dod_agents");

    ClassDB::bind_method(D_METHOD("set_max_dod_beliefs", "max"), &DataOrientedMind::set_max_dod_beliefs);
    ClassDB::bind_method(D_METHOD("get_max_dod_beliefs"), &DataOrientedMind::get_max_dod_beliefs);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_beliefs"), "set_max_dod_beliefs", "get_max_dod_beliefs");

    // --- Methods ---
    ClassDB::bind_method(D_METHOD("setup_dod_pipeline"), &DataOrientedMind::setup_dod_pipeline);
    ClassDB::bind_method(D_METHOD("bake_to_dod"), &DataOrientedMind::bake_to_dod);
    ClassDB::bind_method(D_METHOD("rebuild_dod_task_graph"), &DataOrientedMind::rebuild_dod_task_graph);
    ClassDB::bind_method(D_METHOD("execute_dod_step", "delta"), &DataOrientedMind::execute_dod_step);
    ClassDB::bind_method(D_METHOD("sync_oop_resources_from_dod"), &DataOrientedMind::sync_oop_resources_from_dod);

    ClassDB::bind_method(D_METHOD("get_buffer_id", "semantic_name"), &DataOrientedMind::get_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_manifest_dictionary"), &DataOrientedMind::get_buffer_manifest_dictionary);
    ClassDB::bind_method(D_METHOD("get_agent_dod_slot", "mind"), &DataOrientedMind::get_agent_dod_slot);

    ClassDB::bind_method(
        D_METHOD("queue_sensory_observation_event", "sensor_agent_id", "target_fact_id", "sensed_value", "raw_clarity", "environmental_noise", "perceived_fact_id", "tome_id", "modality"),
        &DataOrientedMind::queue_sensory_observation_event,
        DEFVAL(1.0f),
        DEFVAL(0.0f),
        DEFVAL(0),
        DEFVAL(0),
        DEFVAL(0)
    );

    ClassDB::bind_method(
        D_METHOD("add_minds_transform_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedMind::add_minds_transform_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_minds_query_node", "logic_id", "query_op", "properties", "priority", "use_registry_factory"),
        &DataOrientedMind::add_minds_query_node,
        DEFVAL(0),
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_minds_metadata_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedMind::add_minds_metadata_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );

    ClassDB::bind_method(
        D_METHOD("schedule_node_with_stage", "stage_flag", "custom_node_id", "join_downstream"),
        &DataOrientedMind::schedule_node_with_stage,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("schedule_node_between_stages", "upstream_stage_flag", "downstream_stage_flag", "custom_node_id"),
        &DataOrientedMind::schedule_node_between_stages
    );
}

core::MindsBufferCapacities DataOrientedMind::_build_capacities() const noexcept {
    core::MindsBufferCapacities caps{};
    caps.max_akashic_tomes = static_cast<uint32_t>(std::max(4, max_dod_akashic_tomes_));
    caps.max_facts         = static_cast<uint32_t>(std::max(64, max_dod_facts_));
    caps.max_agents        = static_cast<uint32_t>(std::max(16, max_dod_agents_));
    caps.max_beliefs       = static_cast<uint32_t>(std::max(256, max_dod_beliefs_));
    caps.max_hypotheses    = static_cast<uint32_t>(std::max(64, max_dod_hypotheses_));
    caps.max_syllogisms    = static_cast<uint32_t>(std::max(64, max_dod_syllogisms_));
    return caps;
}

void DataOrientedMind::_ensure_mind_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager) {
    const core::MindsBufferCapacities caps = _build_capacities();
    if (!mind_dod_) {
        mind_dod_ = std::make_unique<core::MindDOD>(std::move(p_shared_manager), caps);
    } else if (p_shared_manager && mind_dod_->get_memory_manager() != p_shared_manager) {
        mind_dod_ = std::make_unique<core::MindDOD>(std::move(p_shared_manager), caps);
    }
    if (!active_task_graph_) {
        active_task_graph_ = mind_dod_->get_default_task_graph();
    }
}

void DataOrientedMind::_clear_resource_maps() noexcept {
    object_id_to_agent_slot_.clear();
    agent_slot_to_resource_.clear();
    baked_agent_ids_.clear();
}

void DataOrientedMind::set_agent_minds(const godot::TypedArray<AgentMindResource> &p_minds) {
    agent_minds_ = p_minds;
    emit_changed();
}

void DataOrientedMind::set_dod_execution_mode(MindDODExecutionMode p_mode) {
    dod_execution_mode_ = p_mode;
}

void DataOrientedMind::set_pipeline_stage_mask(int p_mask) {
    pipeline_stage_mask_ = static_cast<uint32_t>(p_mask) & core::MINDS_STAGE_ALL_MASK;
}

void DataOrientedMind::set_auto_connect_pipeline_stages(bool p_auto_connect) {
    auto_connect_pipeline_stages_ = p_auto_connect;
}

void DataOrientedMind::set_auto_sync_resources_from_dod(bool p_auto_sync) {
    auto_sync_resources_from_dod_ = p_auto_sync;
}

void DataOrientedMind::set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource) {
    custom_task_graph_resource_ = p_resource;
}

void DataOrientedMind::set_max_dod_akashic_tomes(int p_max) {
    max_dod_akashic_tomes_ = std::max(4, p_max);
}

void DataOrientedMind::set_max_dod_facts(int p_max) {
    max_dod_facts_ = std::max(64, p_max);
}

void DataOrientedMind::set_max_dod_agents(int p_max) {
    max_dod_agents_ = std::max(16, p_max);
}

void DataOrientedMind::set_max_dod_beliefs(int p_max) {
    max_dod_beliefs_ = std::max(256, p_max);
}

void DataOrientedMind::setup_dod_pipeline_with_manager(
    std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
    std::shared_ptr<core::TaskGraphDOD> p_shared_graph
) {
    _ensure_mind_dod_initialized(std::move(p_shared_manager));
    if (p_shared_graph) {
        active_task_graph_ = std::move(p_shared_graph);
        mind_dod_->set_default_task_graph(active_task_graph_);
    }
    bake_to_dod();
    rebuild_dod_task_graph();
}

void DataOrientedMind::setup_dod_pipeline() {
    setup_dod_pipeline_with_manager(nullptr, nullptr);
}

void DataOrientedMind::bake_to_dod() {
    _ensure_mind_dod_initialized();
    mind_dod_->reset_all_slots();
    _clear_resource_maps();

    // 1. Bake AgentMindResource profiles into AgentTraitAndMentalStateTable
    for (int64_t i = 0; i < agent_minds_.size(); ++i) {
        godot::Ref<AgentMindResource> mind = agent_minds_[i];
        if (mind.is_null()) continue;
        const uint32_t slot = mind_dod_->allocate_agent(
            mind->get_core_traits(),
            mind->get_attention_threshold(),
            mind->get_knowledge_threshold(),
            mind->get_excitation_level(),
            static_cast<uint32_t>(mind->get_active_emotion_mask()),
            static_cast<uint16_t>(mind->get_primary_tome_id())
        );
        if (slot != core::INVALID_ID) {
            object_id_to_agent_slot_[mind->get_instance_id()] = slot;
            agent_slot_to_resource_[slot]                     = mind;
            baked_agent_ids_.push_back(static_cast<uint32_t>(mind->get_agent_id()));
        }
    }

    // 2. Bake Akashic Tomes, Ground-Truth Facts, Hypotheses, Syllogisms & Reference Catalogues
    mind_dod_->bake_from_information_matrix(get_core_matrix(), baked_agent_ids_);
}

void DataOrientedMind::rebuild_dod_task_graph() {
    _ensure_mind_dod_initialized();
    if (!active_task_graph_) return;

    if (dod_execution_mode_ == MIND_DOD_EXEC_DISABLED ||
        dod_execution_mode_ == MIND_DOD_EXEC_MANUAL_GRAPH) {
        return;
    }

    if (dod_execution_mode_ == MIND_DOD_EXEC_CUSTOM_GRAPH_RESOURCE &&
        custom_task_graph_resource_.is_valid()) {
        ui_to_dod_map_.clear();
        custom_task_graph_resource_->compile_to_task_graph(*active_task_graph_, ui_to_dod_map_);
        return;
    }

    mind_dod_->inject_pipeline_stages(
        *active_task_graph_,
        pipeline_stage_mask_,
        auto_connect_pipeline_stages_
    );
}

void DataOrientedMind::execute_dod_step(double p_delta) {
    if (dod_execution_mode_ == MIND_DOD_EXEC_DISABLED || !mind_dod_) return;
    mind_dod_->execute_step(p_delta, active_task_graph_.get());
    if (auto_sync_resources_from_dod_) {
        sync_oop_resources_from_dod();
    }
}

void DataOrientedMind::sync_oop_resources_from_dod() {
    if (!mind_dod_) return;

    // 1. Sync Agent excitation & active emotions back to AgentMindResource
    const auto *exc = mind_dod_->get_column_ptr<float>(core::MindBufferSlot::MIND_AGENT_EXCITATION_LEVEL);
    const auto *emo = mind_dod_->get_column_ptr<uint32_t>(core::MindBufferSlot::MIND_AGENT_ACTIVE_EMOTION_MASK);
    for (const auto &kv : agent_slot_to_resource_) {
        const uint32_t slot                  = kv.first;
        godot::Ref<AgentMindResource> mind   = kv.second;
        if (mind.is_valid()) {
            if (exc) mind->set_excitation_level(exc[slot]);
            if (emo) mind->set_active_emotion_mask(static_cast<int>(emo[slot]));
        }
    }

    // 2. Sync Reference Catalogue beliefs back to InformationMatrixDOD
    mind_dod_->sync_to_information_matrix(get_core_matrix_mut(), baked_agent_ids_);
}

int DataOrientedMind::get_buffer_id(const godot::StringName &p_semantic_name) const {
    if (!mind_dod_) return -1;
    const godot::String s(p_semantic_name);
    const uint32_t id = mind_dod_->get_buffer_id_by_name(s.utf8().get_data());
    return (id != core::INVALID_ID) ? static_cast<int>(id) : -1;
}

godot::Dictionary DataOrientedMind::get_buffer_manifest_dictionary() const {
    godot::Dictionary d;
    if (mind_dod_) {
        mind_dod_->populate_task_buffer_dictionary(d, true);
    }
    return d;
}

int DataOrientedMind::get_agent_dod_slot(const godot::Ref<AgentMindResource> &p_mind) const {
    if (p_mind.is_null()) return -1;
    auto it = object_id_to_agent_slot_.find(p_mind->get_instance_id());
    return (it != object_id_to_agent_slot_.end()) ? static_cast<int>(it->second) : -1;
}

bool DataOrientedMind::queue_sensory_observation_event(
    int p_sensor_agent_id,
    int p_target_fact_id,
    int64_t p_sensed_value,
    float p_raw_clarity,
    float p_environmental_noise,
    int p_perceived_fact_id,
    int p_tome_id,
    int p_modality
) {
    _ensure_mind_dod_initialized();
    core::SensoryEventPod ev{};
    ev.sensor_agent_id     = static_cast<uint32_t>(std::max(0, p_sensor_agent_id));
    ev.target_fact_id      = static_cast<uint32_t>(std::max(0, p_target_fact_id));
    ev.sensed_value        = p_sensed_value;
    ev.raw_clarity         = std::clamp(p_raw_clarity, 0.0f, 1.0f);
    ev.environmental_noise = std::clamp(p_environmental_noise, 0.0f, 1.0f);
    ev.perceived_fact_id   = static_cast<uint32_t>(std::max(0, p_perceived_fact_id));
    ev.tome_id             = static_cast<uint16_t>(std::clamp(p_tome_id, 0, 65535));
    ev.modality            = static_cast<core::SensoryModality>(p_modality & 0x03);
    return mind_dod_->push_sensory_observation_event(ev);
}

int DataOrientedMind::add_minds_transform_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_mind_dod_initialized();
    if (!active_task_graph_) return -1;
    core::MindsNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = mind_dod_->add_transform_node_by_id(
        *active_task_graph_,
        static_cast<core::MindsTransformLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedMind::add_minds_query_node(
    int p_logic_id,
    int p_query_op,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_mind_dod_initialized();
    if (!active_task_graph_) return -1;
    core::MindsNodeExecutionConfig cfg{};
    cfg.query_op             = (p_query_op == 1) ? core::QueryOp::ADD : core::QueryOp::CULL;
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = mind_dod_->add_query_node_by_id(
        *active_task_graph_,
        static_cast<core::MindsQueryLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedMind::add_minds_metadata_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_mind_dod_initialized();
    if (!active_task_graph_) return -1;
    core::MindsNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = mind_dod_->add_metadata_node_by_id(
        *active_task_graph_,
        static_cast<core::MindsMetadataLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

bool DataOrientedMind::schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream) {
    if (!mind_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::MindDOD::schedule_node_with_stage(
        *active_task_graph_,
        mind_dod_->get_active_topology(),
        static_cast<core::MindsPipelineStageFlags>(p_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id),
        p_join_downstream
    );
}

bool DataOrientedMind::schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id) {
    if (!mind_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::MindDOD::schedule_node_between_stages(
        *active_task_graph_,
        mind_dod_->get_active_topology(),
        static_cast<core::MindsPipelineStageFlags>(p_upstream_stage_flag),
        static_cast<core::MindsPipelineStageFlags>(p_downstream_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id)
    );
}

} // namespace ideam::godot_ext

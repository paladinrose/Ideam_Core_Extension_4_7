#include "data_oriented_quantum_darwinism.h"
#include <algorithm>
#include <vector>

namespace ideam::godot_ext {

DataOrientedQuantumDarwinism::DataOrientedQuantumDarwinism()
    : dod_engine_(std::make_unique<core::QuantumDarwinismDOD>()) {}

void DataOrientedQuantumDarwinism::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_memory_manager_resource", "memory_manager"), &DataOrientedQuantumDarwinism::set_memory_manager_resource);
    ClassDB::bind_method(D_METHOD("get_memory_manager_resource"), &DataOrientedQuantumDarwinism::get_memory_manager_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "memory_manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"), "set_memory_manager_resource", "get_memory_manager_resource");

    ClassDB::bind_method(D_METHOD("set_task_graph_resource", "task_graph"), &DataOrientedQuantumDarwinism::set_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_task_graph_resource"), &DataOrientedQuantumDarwinism::get_task_graph_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"), "set_task_graph_resource", "get_task_graph_resource");

    ClassDB::bind_method(D_METHOD("set_stabilizer_resource", "stabilizer"), &DataOrientedQuantumDarwinism::set_stabilizer_resource);
    ClassDB::bind_method(D_METHOD("get_stabilizer_resource"), &DataOrientedQuantumDarwinism::get_stabilizer_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stabilizer_resource", PROPERTY_HINT_RESOURCE_TYPE, "QuantumDarwinismStabilizerResource"), "set_stabilizer_resource", "get_stabilizer_resource");

    ClassDB::bind_method(D_METHOD("set_generator_resource", "generator"), &DataOrientedQuantumDarwinism::set_generator_resource);
    ClassDB::bind_method(D_METHOD("get_generator_resource"), &DataOrientedQuantumDarwinism::get_generator_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "generator_resource", PROPERTY_HINT_RESOURCE_TYPE, "QuantumDarwinismGeneratorResource"), "set_generator_resource", "get_generator_resource");

    ClassDB::bind_method(D_METHOD("set_active_pattern", "pattern"), &DataOrientedQuantumDarwinism::set_active_pattern);
    ClassDB::bind_method(D_METHOD("get_active_pattern"), &DataOrientedQuantumDarwinism::get_active_pattern);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"), "set_active_pattern", "get_active_pattern");

    ClassDB::bind_method(D_METHOD("set_slot_count", "count"), &DataOrientedQuantumDarwinism::set_slot_count);
    ClassDB::bind_method(D_METHOD("get_slot_count"), &DataOrientedQuantumDarwinism::get_slot_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "slot_count", PROPERTY_HINT_RANGE, "1,4096,1"), "set_slot_count", "get_slot_count");

    ClassDB::bind_method(D_METHOD("initialize_dod_buffers"), &DataOrientedQuantumDarwinism::initialize_dod_buffers);
    ClassDB::bind_method(D_METHOD("set_slot_children", "parent_slot", "child_slots"), &DataOrientedQuantumDarwinism::set_slot_children);
    ClassDB::bind_method(D_METHOD("flag_slot_conflict", "slot_id"), &DataOrientedQuantumDarwinism::flag_slot_conflict);
    ClassDB::bind_method(D_METHOD("stage_environment_to_dod"), &DataOrientedQuantumDarwinism::stage_environment_to_dod);
    ClassDB::bind_method(D_METHOD("stage_channels_and_couplings_to_dod"), &DataOrientedQuantumDarwinism::stage_channels_and_couplings_to_dod);
    ClassDB::bind_method(D_METHOD("stage_perturbation_field_to_dod", "field"), &DataOrientedQuantumDarwinism::stage_perturbation_field_to_dod);
    ClassDB::bind_method(D_METHOD("execute_pipeline_direct", "stage_mask"), &DataOrientedQuantumDarwinism::execute_pipeline_direct, DEFVAL(static_cast<int>(core::QD_STAGE_DEFAULT_PIPELINE_MASK)));
    ClassDB::bind_method(D_METHOD("execute_generative_pipeline_direct"), &DataOrientedQuantumDarwinism::execute_generative_pipeline_direct);

    ClassDB::bind_method(D_METHOD("read_environment_slots_as_dicts"), &DataOrientedQuantumDarwinism::read_environment_slots_as_dicts);
    ClassDB::bind_method(D_METHOD("read_horizon_partitions_as_dicts"), &DataOrientedQuantumDarwinism::read_horizon_partitions_as_dicts);
    ClassDB::bind_method(D_METHOD("read_pointer_states_as_dicts"), &DataOrientedQuantumDarwinism::read_pointer_states_as_dicts);
    ClassDB::bind_method(D_METHOD("read_coherence_bridge_slots_as_dicts"), &DataOrientedQuantumDarwinism::read_coherence_bridge_slots_as_dicts);
    ClassDB::bind_method(D_METHOD("read_slot_channel_states_as_dicts"), &DataOrientedQuantumDarwinism::read_slot_channel_states_as_dicts);
    ClassDB::bind_method(D_METHOD("read_pointer_or_conflict_masks"), &DataOrientedQuantumDarwinism::read_pointer_or_conflict_masks);
}

void DataOrientedQuantumDarwinism::set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem) {
    memory_manager_resource_ = p_mem;
    if (dod_engine_) {
        dod_engine_->bind_execution_context(
            memory_manager_resource_.is_valid() ? memory_manager_resource_->get_dod_manager() : nullptr,
            task_graph_resource_.is_valid() ? task_graph_resource_->get_dod_graph() : nullptr
        );
    }
    emit_changed();
}

void DataOrientedQuantumDarwinism::set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph) {
    task_graph_resource_ = p_graph;
    if (dod_engine_) {
        dod_engine_->bind_execution_context(
            memory_manager_resource_.is_valid() ? memory_manager_resource_->get_dod_manager() : nullptr,
            task_graph_resource_.is_valid() ? task_graph_resource_->get_dod_graph() : nullptr
        );
    }
    emit_changed();
}

void DataOrientedQuantumDarwinism::set_stabilizer_resource(const godot::Ref<QuantumDarwinismStabilizerResource> &p_res) {
    stabilizer_resource_ = p_res;
    if (dod_engine_ && stabilizer_resource_.is_valid()) {
        dod_engine_->set_horizon_config(stabilizer_resource_->to_pod());
        if (stabilizer_resource_->get_pattern().is_valid()) active_pattern_ = stabilizer_resource_->get_pattern();
    }
    emit_changed();
}

void DataOrientedQuantumDarwinism::set_generator_resource(const godot::Ref<QuantumDarwinismGeneratorResource> &p_res) {
    generator_resource_ = p_res;
    if (dod_engine_ && generator_resource_.is_valid()) {
        generator_resource_->apply_to_solver(dod_engine_->solver());
        if (generator_resource_->get_pattern().is_valid()) active_pattern_ = generator_resource_->get_pattern();
    }
    emit_changed();
}

void DataOrientedQuantumDarwinism::set_slot_count(int p_count) {
    slot_count_ = std::clamp(p_count, 1, 4096);
    emit_changed();
}

bool DataOrientedQuantumDarwinism::initialize_dod_buffers() {
    if (!dod_engine_ || memory_manager_resource_.is_null()) return false;
    return dod_engine_->initialize_quantum_darwinism_memory(memory_manager_resource_->get_dod_manager());
}

void DataOrientedQuantumDarwinism::set_slot_children(int p_parent_slot, const godot::PackedInt32Array &p_child_slots) {
    if (!dod_engine_ || p_parent_slot < 0) return;
    std::vector<uint32_t> kids;
    kids.reserve(static_cast<size_t>(p_child_slots.size()));
    for (int i = 0; i < p_child_slots.size(); ++i) {
        if (p_child_slots[i] >= 0) kids.push_back(static_cast<uint32_t>(p_child_slots[i]));
    }
    dod_engine_->environment().set_slot_children(static_cast<uint32_t>(p_parent_slot), kids);
}

void DataOrientedQuantumDarwinism::flag_slot_conflict(int p_slot_id) {
    if (!dod_engine_ || p_slot_id < 0) return;
    auto &slots = dod_engine_->environment().slots();
    if (static_cast<size_t>(p_slot_id) < slots.size()) {
        slots[static_cast<size_t>(p_slot_id)].has_conflict = 1u;
    }
}

bool DataOrientedQuantumDarwinism::stage_environment_to_dod() {
    if (!dod_engine_) return false;
    if (stabilizer_resource_.is_valid()) dod_engine_->set_horizon_config(stabilizer_resource_->to_pod());
    if (generator_resource_.is_valid()) generator_resource_->apply_to_solver(dod_engine_->solver());
    if (active_pattern_.is_valid() && dod_engine_->environment().tokens().empty()) {
        active_pattern_->compile_to_environment(dod_engine_->environment(), static_cast<uint32_t>(slot_count_));
    }
    return dod_engine_->stage_environment_to_dod();
}

bool DataOrientedQuantumDarwinism::stage_channels_and_couplings_to_dod() {
    if (!dod_engine_) return false;
    if (generator_resource_.is_valid()) generator_resource_->apply_to_solver(dod_engine_->solver());
    return dod_engine_->stage_channels_and_couplings_to_dod();
}

bool DataOrientedQuantumDarwinism::stage_perturbation_field_to_dod(const godot::PackedFloat32Array &p_field) {
    if (!dod_engine_) return false;
    std::vector<float> tmp(static_cast<size_t>(p_field.size()));
    for (int i = 0; i < p_field.size(); ++i) tmp[static_cast<size_t>(i)] = p_field[i];
    return dod_engine_->stage_perturbation_field_to_dod(tmp);
}

bool DataOrientedQuantumDarwinism::execute_pipeline_direct(int p_stage_mask) {
    if (!dod_engine_) return false;
    return dod_engine_->execute_full_pipeline_direct(static_cast<uint32_t>(p_stage_mask));
}

bool DataOrientedQuantumDarwinism::execute_generative_pipeline_direct() {
    if (!dod_engine_) return false;
    if (generator_resource_.is_valid()) generator_resource_->apply_to_solver(dod_engine_->solver());
    return dod_engine_->execute_generative_pipeline_direct();
}

godot::Array DataOrientedQuantumDarwinism::read_environment_slots_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::EnvironmentSlotPOD> slots;
    dod_engine_->read_environment_slots(slots);
    const auto &env = dod_engine_->environment();
    for (const auto &s : slots) {
        godot::Dictionary d;
        d["slot_id"]            = static_cast<int>(s.slot_id);
        d["parent_slot_id"]     = (s.parent_slot_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(s.parent_slot_id);
        d["collapsed_token_id"] = s.collapsed_token_id;
        if (s.collapsed_token_id >= 0) {
            const std::string_view name = env.get_token_name(static_cast<uint32_t>(s.collapsed_token_id));
            d["token_name"] = godot::String::utf8(name.data(), static_cast<int>(name.size()));
        } else {
            d["token_name"] = godot::String();
        }
        d["einselection_score"] = s.einselection_score;
        d["is_pointer_state"]   = (s.is_pointer_state != 0u);
        d["has_conflict"]       = (s.has_conflict != 0u);
        out.push_back(d);
    }
    return out;
}

godot::Array DataOrientedQuantumDarwinism::read_horizon_partitions_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::HorizonSlotPartitionPOD> parts;
    dod_engine_->read_horizon_partitions(parts);
    for (const auto &p : parts) {
        if (p.hop_distance == 0xFFFFu) continue;
        godot::Dictionary d;
        d["slot_id"]                 = static_cast<int>(p.slot_id);
        d["nearest_conflict_slot"]   = (p.nearest_conflict_slot == 0xFFFFFFFFu) ? -1 : static_cast<int>(p.nearest_conflict_slot);
        d["predecessor_toward_conf"] = (p.predecessor_toward_conf == 0xFFFFFFFFu) ? -1 : static_cast<int>(p.predecessor_toward_conf);
        d["hop_distance"]            = static_cast<int>(p.hop_distance);
        d["role_flags"]              = static_cast<int>(p.role_flags);
        out.push_back(d);
    }
    return out;
}

godot::Array DataOrientedQuantumDarwinism::read_pointer_states_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::PointerStateRecordPOD> recs;
    dod_engine_->read_pointer_state_records(recs);
    for (const auto &r : recs) {
        if (r.einselection_redundancy <= 0.0f && r.preserved_trait_mask == 0u) continue;
        godot::Dictionary d;
        d["slot_id"]                  = static_cast<int>(r.slot_id);
        d["settled_token_id"]         = static_cast<int>(r.settled_token_id);
        d["einselection_redundancy"]  = r.einselection_redundancy;
        d["highest_satisfied_spec"]   = static_cast<int>(r.highest_satisfied_spec);
        d["preserved_trait_mask"]     = static_cast<int>(r.preserved_trait_mask);
        d["hop_distance_to_conflict"] = static_cast<int>(r.hop_distance_to_conflict);
        out.push_back(d);
    }
    return out;
}

godot::Array DataOrientedQuantumDarwinism::read_coherence_bridge_slots_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::CoherenceBridgeSlotPOD> bridges;
    dod_engine_->read_coherence_bridge_slots(bridges);
    for (const auto &b : bridges) {
        if (b.mutable_trait_mask == 0u && b.hops_from_pointer == 0u) continue;
        godot::Dictionary d;
        d["slot_id"]                 = static_cast<int>(b.slot_id);
        d["source_pointer_slot_id"]  = static_cast<int>(b.source_pointer_slot_id);
        d["target_conflict_slot_id"] = static_cast<int>(b.target_conflict_slot_id);
        d["hops_from_pointer"]       = static_cast<int>(b.hops_from_pointer);
        d["hops_to_conflict"]        = static_cast<int>(b.hops_to_conflict);
        d["mutable_trait_mask"]      = static_cast<int>(b.mutable_trait_mask);
        d["relax_down_to"]           = static_cast<int>(b.relax_down_to);
        out.push_back(d);
    }
    return out;
}

godot::Array DataOrientedQuantumDarwinism::read_slot_channel_states_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::SlotChannelStatePOD> states;
    dod_engine_->read_slot_channel_states(states);
    for (const auto &st : states) {
        godot::Dictionary d;
        d["slot_id"]                 = static_cast<int>(st.slot_id);
        d["channel_id"]              = static_cast<int>(st.channel_id);
        d["value"]                   = st.value;
        d["candidate_state_mask"]    = static_cast<int>(st.candidate_state_mask);
        d["redundancy_r_delta"]      = st.redundancy_r_delta;
        d["effective_threshold_tau"] = st.effective_threshold_tau;
        d["effective_permeability"]  = st.effective_permeability;
        d["is_locked"]               = (st.is_locked != 0u);
        d["is_pinned"]               = (st.is_pinned != 0u);
        d["is_nucleated"]            = (st.is_nucleated != 0u);
        d["locked_wave_index"]       = static_cast<int>(st.locked_wave_index);
        out.push_back(d);
    }
    return out;
}

godot::PackedInt32Array DataOrientedQuantumDarwinism::read_pointer_or_conflict_masks() const {
    godot::PackedInt32Array out;
    if (!dod_engine_) return out;
    std::vector<uint32_t> masks;
    dod_engine_->read_pointer_or_conflict_masks(masks);
    out.resize(static_cast<int64_t>(masks.size()));
    for (size_t i = 0; i < masks.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(masks[i]));
    }
    return out;
}

} // namespace ideam::godot_ext

#include "data_oriented_procedural_generation.h"
#include <algorithm>
#include <vector>

namespace ideam::godot_ext {

DataOrientedProceduralGeneration::DataOrientedProceduralGeneration()
    : dod_engine_(std::make_unique<core::ProceduralGenerationDOD>()) {}

void DataOrientedProceduralGeneration::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_memory_manager_resource", "memory_manager"), &DataOrientedProceduralGeneration::set_memory_manager_resource);
    ClassDB::bind_method(D_METHOD("get_memory_manager_resource"), &DataOrientedProceduralGeneration::get_memory_manager_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "memory_manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"),
        "set_memory_manager_resource",
        "get_memory_manager_resource"
    );

    ClassDB::bind_method(D_METHOD("set_task_graph_resource", "task_graph"), &DataOrientedProceduralGeneration::set_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_task_graph_resource"), &DataOrientedProceduralGeneration::get_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_task_graph_resource",
        "get_task_graph_resource"
    );

    ClassDB::bind_method(D_METHOD("set_active_pattern", "pattern"), &DataOrientedProceduralGeneration::set_active_pattern);
    ClassDB::bind_method(D_METHOD("get_active_pattern"), &DataOrientedProceduralGeneration::get_active_pattern);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"),
        "set_active_pattern",
        "get_active_pattern"
    );

    ClassDB::bind_method(D_METHOD("set_slot_count", "slot_count"), &DataOrientedProceduralGeneration::set_slot_count);
    ClassDB::bind_method(D_METHOD("get_slot_count"), &DataOrientedProceduralGeneration::get_slot_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "slot_count", PROPERTY_HINT_RANGE, "1,65536,1"), "set_slot_count", "get_slot_count");

    ClassDB::bind_method(D_METHOD("set_pointer_redundancy_threshold", "threshold"), &DataOrientedProceduralGeneration::set_pointer_redundancy_threshold);
    ClassDB::bind_method(D_METHOD("get_pointer_redundancy_threshold"), &DataOrientedProceduralGeneration::get_pointer_redundancy_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pointer_redundancy_threshold", PROPERTY_HINT_RANGE, "0.05,1.0,0.05"), "set_pointer_redundancy_threshold", "get_pointer_redundancy_threshold");

    ClassDB::bind_method(D_METHOD("set_max_propagation_sweeps", "sweeps"), &DataOrientedProceduralGeneration::set_max_propagation_sweeps);
    ClassDB::bind_method(D_METHOD("get_max_propagation_sweeps"), &DataOrientedProceduralGeneration::get_max_propagation_sweeps);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_propagation_sweeps", PROPERTY_HINT_RANGE, "1,128,1"), "set_max_propagation_sweeps", "get_max_propagation_sweeps");

    ClassDB::bind_method(D_METHOD("initialize_dod_buffers"), &DataOrientedProceduralGeneration::initialize_dod_buffers);
    ClassDB::bind_method(D_METHOD("set_slot_children", "parent_slot", "child_slots"), &DataOrientedProceduralGeneration::set_slot_children);
    ClassDB::bind_method(D_METHOD("pin_slot_token", "slot_id", "token_name"), &DataOrientedProceduralGeneration::pin_slot_token);
    ClassDB::bind_method(D_METHOD("stage_pattern_to_dod"), &DataOrientedProceduralGeneration::stage_pattern_to_dod);
    ClassDB::bind_method(
        D_METHOD("execute_pipeline_direct", "stage_mask"),
        &DataOrientedProceduralGeneration::execute_pipeline_direct,
        DEFVAL(static_cast<int>(core::PROCGEN_STAGE_DEFAULT_PIPELINE_MASK))
    );
    ClassDB::bind_method(D_METHOD("read_environment_slots_as_dicts"), &DataOrientedProceduralGeneration::read_environment_slots_as_dicts);
    ClassDB::bind_method(D_METHOD("read_pointer_or_conflict_masks"), &DataOrientedProceduralGeneration::read_pointer_or_conflict_masks);
}

void DataOrientedProceduralGeneration::set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem) {
    memory_manager_resource_ = p_mem;
    if (dod_engine_ && memory_manager_resource_.is_valid()) {
        dod_engine_->bind_execution_context(
            &memory_manager_resource_->get_dod_manager(),
            task_graph_resource_.is_valid() ? &task_graph_resource_->get_dod_graph() : nullptr
        );
    }
    emit_changed();
}

void DataOrientedProceduralGeneration::set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph) {
    task_graph_resource_ = p_graph;
    if (dod_engine_ && task_graph_resource_.is_valid()) {
        dod_engine_->bind_execution_context(
            memory_manager_resource_.is_valid() ? &memory_manager_resource_->get_dod_manager() : nullptr,
            &task_graph_resource_->get_dod_graph()
        );
    }
    emit_changed();
}

void DataOrientedProceduralGeneration::set_slot_count(int p_count) {
    slot_count_ = std::max(1, p_count);
    emit_changed();
}

void DataOrientedProceduralGeneration::set_pointer_redundancy_threshold(float p_thresh) {
    pointer_redundancy_threshold_ = std::clamp(p_thresh, 0.05f, 1.0f);
    if (dod_engine_) dod_engine_->set_pointer_redundancy_threshold(pointer_redundancy_threshold_);
    emit_changed();
}

void DataOrientedProceduralGeneration::set_max_propagation_sweeps(int p_sweeps) {
    max_propagation_sweeps_ = std::max(1, p_sweeps);
    if (dod_engine_) dod_engine_->set_max_propagation_sweeps(static_cast<uint32_t>(max_propagation_sweeps_));
    emit_changed();
}

bool DataOrientedProceduralGeneration::initialize_dod_buffers() {
    if (!dod_engine_ || memory_manager_resource_.is_null()) return false;
    return dod_engine_->initialize_procedural_generation_memory(&memory_manager_resource_->get_dod_manager());
}

void DataOrientedProceduralGeneration::set_slot_children(int p_parent_slot, const godot::PackedInt32Array &p_child_slots) {
    if (!dod_engine_ || p_parent_slot < 0) return;
    auto &env = dod_engine_->environment();
    if (env.slots().empty()) {
        env.configure_slots(static_cast<uint32_t>(slot_count_), std::max<uint32_t>(1u, static_cast<uint32_t>(env.tokens().size())));
    }
    std::vector<uint32_t> children;
    children.reserve(static_cast<size_t>(p_child_slots.size()));
    for (int i = 0; i < p_child_slots.size(); ++i) {
        if (p_child_slots[i] >= 0) {
            children.push_back(static_cast<uint32_t>(p_child_slots[i]));
        }
    }
    env.set_slot_children(static_cast<uint32_t>(p_parent_slot), children);
}

void DataOrientedProceduralGeneration::pin_slot_token(int p_slot_id, const godot::String &p_token_name) {
    if (!dod_engine_ || p_slot_id < 0) return;
    auto &env = dod_engine_->environment();
    const int32_t tok_id = env.find_token_by_name(p_token_name.utf8().get_data());
    if (tok_id >= 0 && static_cast<size_t>(p_slot_id) < env.slots().size()) {
        auto &slot = env.slots()[static_cast<size_t>(p_slot_id)];
        slot.domain.clear();
        slot.domain.set_bit(static_cast<uint32_t>(tok_id));
        slot.collapsed_token_id = tok_id;
        slot.locked_layer       = core::PatternCascadeLayer::LAYER_PINNED_OVERRIDE;
        slot.is_pointer_state   = 1u;
        slot.einselection_score = 1.0f;
    }
}

bool DataOrientedProceduralGeneration::stage_pattern_to_dod() {
    if (!dod_engine_) return false;
    auto &env = dod_engine_->environment();
    if (active_pattern_.is_valid()) {
        active_pattern_->compile_to_environment(env);
    }
    if (env.slots().empty()) {
        env.configure_slots(static_cast<uint32_t>(slot_count_), std::max<uint32_t>(1u, static_cast<uint32_t>(env.tokens().size())));
    }
    return dod_engine_->stage_environment_to_dod(env);
}

bool DataOrientedProceduralGeneration::execute_pipeline_direct(int p_stage_mask) {
    if (!dod_engine_) return false;
    return dod_engine_->execute_full_pipeline_direct(static_cast<uint32_t>(p_stage_mask));
}

godot::Array DataOrientedProceduralGeneration::read_environment_slots_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::EnvironmentSlotPOD> slots;
    dod_engine_->read_environment_slots(slots);
    for (const core::EnvironmentSlotPOD &s : slots) {
        godot::Dictionary d;
        d["slot_id"]            = static_cast<int>(s.slot_id);
        d["parent_slot_id"]     = (s.parent_slot_id != 0xFFFFFFFFu) ? static_cast<int>(s.parent_slot_id) : -1;
        d["children_count"]     = static_cast<int>(s.children_count);
        d["collapsed_token_id"] = s.collapsed_token_id;
        d["domain_popcount"]    = static_cast<int>(s.domain.popcount());
        d["einselection_score"] = s.einselection_score;
        d["is_pointer_state"]   = (s.is_pointer_state != 0u);
        d["has_conflict"]       = (s.has_conflict != 0u);
        out.push_back(d);
    }
    return out;
}

godot::PackedInt32Array DataOrientedProceduralGeneration::read_pointer_or_conflict_masks() const {
    godot::PackedInt32Array out;
    if (!dod_engine_) return out;
    std::vector<uint32_t> masks;
    dod_engine_->read_pointer_or_conflict_masks(masks);
    for (uint32_t m : masks) {
        out.push_back(static_cast<int32_t>(m));
    }
    return out;
}

} // namespace ideam::godot_ext

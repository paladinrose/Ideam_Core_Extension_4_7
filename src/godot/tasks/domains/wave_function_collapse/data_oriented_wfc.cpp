#include "data_oriented_wfc.h"
#include <algorithm>

namespace ideam::godot_ext {

DataOrientedWaveFunctionCollapse::DataOrientedWaveFunctionCollapse()
    : dod_engine_(std::make_unique<core::WaveFunctionCollapseDOD>()) {}

void DataOrientedWaveFunctionCollapse::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(
        D_METHOD("set_memory_manager_resource", "memory_manager"),
        &DataOrientedWaveFunctionCollapse::set_memory_manager_resource
    );
    ClassDB::bind_method(
        D_METHOD("get_memory_manager_resource"),
        &DataOrientedWaveFunctionCollapse::get_memory_manager_resource
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "memory_manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"),
        "set_memory_manager_resource",
        "get_memory_manager_resource"
    );

    ClassDB::bind_method(
        D_METHOD("set_task_graph_resource", "task_graph"),
        &DataOrientedWaveFunctionCollapse::set_task_graph_resource
    );
    ClassDB::bind_method(
        D_METHOD("get_task_graph_resource"),
        &DataOrientedWaveFunctionCollapse::get_task_graph_resource
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_task_graph_resource",
        "get_task_graph_resource"
    );

    ClassDB::bind_method(
        D_METHOD("set_rule_tensor_resource", "rule_tensor"),
        &DataOrientedWaveFunctionCollapse::set_rule_tensor_resource
    );
    ClassDB::bind_method(
        D_METHOD("get_rule_tensor_resource"),
        &DataOrientedWaveFunctionCollapse::get_rule_tensor_resource
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "rule_tensor_resource", PROPERTY_HINT_RESOURCE_TYPE, "WFCRuleTensorResource"),
        "set_rule_tensor_resource",
        "get_rule_tensor_resource"
    );

    ClassDB::bind_method(
        D_METHOD("set_active_pattern", "pattern"),
        &DataOrientedWaveFunctionCollapse::set_active_pattern
    );
    ClassDB::bind_method(
        D_METHOD("get_active_pattern"),
        &DataOrientedWaveFunctionCollapse::get_active_pattern
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"),
        "set_active_pattern",
        "get_active_pattern"
    );

    ClassDB::bind_method(D_METHOD("set_slot_count", "count"), &DataOrientedWaveFunctionCollapse::set_slot_count);
    ClassDB::bind_method(D_METHOD("get_slot_count"), &DataOrientedWaveFunctionCollapse::get_slot_count);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "slot_count", PROPERTY_HINT_RANGE, "1,4096,1"),
        "set_slot_count",
        "get_slot_count"
    );

    ClassDB::bind_method(
        D_METHOD("set_max_collapse_steps", "steps"),
        &DataOrientedWaveFunctionCollapse::set_max_collapse_steps
    );
    ClassDB::bind_method(
        D_METHOD("get_max_collapse_steps"),
        &DataOrientedWaveFunctionCollapse::get_max_collapse_steps
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "max_collapse_steps", PROPERTY_HINT_RANGE, "1,4096,1"),
        "set_max_collapse_steps",
        "get_max_collapse_steps"
    );

    ClassDB::bind_method(
        D_METHOD("set_pointer_redundancy_threshold", "threshold"),
        &DataOrientedWaveFunctionCollapse::set_pointer_redundancy_threshold
    );
    ClassDB::bind_method(
        D_METHOD("get_pointer_redundancy_threshold"),
        &DataOrientedWaveFunctionCollapse::get_pointer_redundancy_threshold
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "pointer_redundancy_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_pointer_redundancy_threshold",
        "get_pointer_redundancy_threshold"
    );

    ClassDB::bind_method(D_METHOD("set_horizon_radius", "radius"), &DataOrientedWaveFunctionCollapse::set_horizon_radius);
    ClassDB::bind_method(D_METHOD("get_horizon_radius"), &DataOrientedWaveFunctionCollapse::get_horizon_radius);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "horizon_radius", PROPERTY_HINT_RANGE, "1,16,1"),
        "set_horizon_radius",
        "get_horizon_radius"
    );

    ClassDB::bind_method(D_METHOD("initialize_dod_buffers"), &DataOrientedWaveFunctionCollapse::initialize_dod_buffers);
    ClassDB::bind_method(
        D_METHOD("set_slot_children", "parent_slot", "child_slots"),
        &DataOrientedWaveFunctionCollapse::set_slot_children
    );
    ClassDB::bind_method(
        D_METHOD("pin_slot_token", "slot_id", "token_name"),
        &DataOrientedWaveFunctionCollapse::pin_slot_token
    );
    ClassDB::bind_method(D_METHOD("stage_environment_to_dod"), &DataOrientedWaveFunctionCollapse::stage_environment_to_dod);
    ClassDB::bind_method(
        D_METHOD("execute_pipeline_direct", "stage_mask"),
        &DataOrientedWaveFunctionCollapse::execute_pipeline_direct,
        DEFVAL(static_cast<int>(core::WFC_STAGE_DEFAULT_PIPELINE_MASK))
    );
    ClassDB::bind_method(
        D_METHOD("read_environment_slots_as_dicts"),
        &DataOrientedWaveFunctionCollapse::read_environment_slots_as_dicts
    );
    ClassDB::bind_method(
        D_METHOD("read_contradiction_reports_as_dicts"),
        &DataOrientedWaveFunctionCollapse::read_contradiction_reports_as_dicts
    );
    ClassDB::bind_method(
        D_METHOD("read_pointer_or_conflict_masks"),
        &DataOrientedWaveFunctionCollapse::read_pointer_or_conflict_masks
    );
}

void DataOrientedWaveFunctionCollapse::set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem) {
    memory_manager_resource_ = p_mem;
    if (dod_engine_) {
        core::MemoryManagerDOD *raw_mem = memory_manager_resource_.is_valid()
            ? memory_manager_resource_->get_dod_manager()
            : nullptr;
        core::TaskGraphDOD *raw_graph = task_graph_resource_.is_valid()
            ? task_graph_resource_->get_dod_graph()
            : nullptr;
        dod_engine_->bind_execution_context(raw_mem, raw_graph);
    }
    emit_changed();
}

void DataOrientedWaveFunctionCollapse::set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph) {
    task_graph_resource_ = p_graph;
    if (dod_engine_) {
        core::MemoryManagerDOD *raw_mem = memory_manager_resource_.is_valid()
            ? memory_manager_resource_->get_dod_manager()
            : nullptr;
        core::TaskGraphDOD *raw_graph = task_graph_resource_.is_valid()
            ? task_graph_resource_->get_dod_graph()
            : nullptr;
        dod_engine_->bind_execution_context(raw_mem, raw_graph);
    }
    emit_changed();
}

void DataOrientedWaveFunctionCollapse::set_slot_count(int p_count) {
    slot_count_ = std::clamp(p_count, 1, 4096);
    emit_changed();
}

void DataOrientedWaveFunctionCollapse::set_max_collapse_steps(int p_steps) {
    max_collapse_steps_ = std::clamp(p_steps, 1, 4096);
    if (dod_engine_) {
        dod_engine_->set_max_collapse_steps(static_cast<uint32_t>(max_collapse_steps_));
    }
    emit_changed();
}

void DataOrientedWaveFunctionCollapse::set_pointer_redundancy_threshold(float p_thresh) {
    pointer_redundancy_threshold_ = std::clamp(p_thresh, 0.0f, 1.0f);
    if (dod_engine_) {
        dod_engine_->set_pointer_redundancy_threshold(pointer_redundancy_threshold_);
    }
    emit_changed();
}

void DataOrientedWaveFunctionCollapse::set_horizon_radius(int p_radius) {
    horizon_radius_ = std::clamp(p_radius, 1, 16);
    if (dod_engine_) {
        dod_engine_->set_horizon_radius(static_cast<uint32_t>(horizon_radius_));
    }
    emit_changed();
}

bool DataOrientedWaveFunctionCollapse::initialize_dod_buffers() {
    if (!dod_engine_ || memory_manager_resource_.is_null()) {
        return false;
    }
    return dod_engine_->initialize_wfc_memory(memory_manager_resource_->get_dod_manager());
}

void DataOrientedWaveFunctionCollapse::set_slot_children(int p_parent_slot, const godot::PackedInt32Array &p_child_slots) {
    if (!dod_engine_ || p_parent_slot < 0) {
        return;
    }
    std::vector<uint32_t> kids;
    kids.reserve(static_cast<size_t>(p_child_slots.size()));
    for (int i = 0; i < p_child_slots.size(); ++i) {
        if (p_child_slots[i] >= 0) {
            kids.push_back(static_cast<uint32_t>(p_child_slots[i]));
        }
    }
    dod_engine_->environment().set_slot_children(static_cast<uint32_t>(p_parent_slot), kids);
}

void DataOrientedWaveFunctionCollapse::pin_slot_token(int p_slot_id, const godot::String &p_token_name) {
    if (!dod_engine_ || p_slot_id < 0) {
        return;
    }
    auto &env = dod_engine_->environment();
    if (static_cast<size_t>(p_slot_id) >= env.slots().size()) {
        return;
    }
    const int32_t tok_id = env.find_token_by_name(p_token_name.utf8().get_data());
    if (tok_id >= 0) {
        auto &slot              = env.slots()[static_cast<size_t>(p_slot_id)];
        slot.domain.clear();
        slot.domain.set_bit(static_cast<uint32_t>(tok_id));
        slot.collapsed_token_id = tok_id;
        slot.locked_layer       = core::PatternCascadeLayer::LAYER_PINNED_OVERRIDE;
        slot.is_pointer_state   = 1u;
        slot.einselection_score = 1.0f;
    }
}

bool DataOrientedWaveFunctionCollapse::stage_environment_to_dod() {
    if (!dod_engine_) {
        return false;
    }
    if (rule_tensor_resource_.is_valid()) {
        rule_tensor_resource_->apply_to_solver_and_environment(
            dod_engine_->solver_core(),
            dod_engine_->environment(),
            slot_count_
        );
        dod_engine_->set_master_seed(static_cast<uint32_t>(rule_tensor_resource_->get_master_seed()));
    } else if (active_pattern_.is_valid()) {
        active_pattern_->compile_to_environment(
            dod_engine_->environment(),
            static_cast<uint32_t>(slot_count_)
        );
    }
    return dod_engine_->stage_environment_and_rules_to_dod();
}

bool DataOrientedWaveFunctionCollapse::execute_pipeline_direct(int p_stage_mask) {
    if (!dod_engine_) {
        return false;
    }
    return dod_engine_->execute_full_pipeline_direct(static_cast<uint32_t>(p_stage_mask));
}

godot::Array DataOrientedWaveFunctionCollapse::read_environment_slots_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) {
        return out;
    }
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
        d["popcount"]           = static_cast<int>(s.domain.popcount());
        d["einselection_score"] = s.einselection_score;
        d["is_pointer_state"]   = (s.is_pointer_state != 0u);
        d["has_conflict"]       = (s.has_conflict != 0u);
        d["entropy_bucket"]     = static_cast<int>(s.entropy_bucket);
        out.push_back(d);
    }
    return out;
}

godot::Array DataOrientedWaveFunctionCollapse::read_contradiction_reports_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) {
        return out;
    }
    std::vector<core::WFCContradictionReportPOD> reps;
    dod_engine_->read_contradiction_reports(reps);
    for (const auto &r : reps) {
        if (r.contradiction_flags == core::WFC_CONTRADICTION_NONE) {
            continue;
        }
        godot::Dictionary d;
        d["slot_id"]                    = static_cast<int>(r.slot_id);
        d["parent_slot_id"]             = (r.parent_slot_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(r.parent_slot_id);
        d["violated_rule_id"]           = (r.violated_rule_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(r.violated_rule_id);
        d["contradiction_flags"]        = static_cast<int>(r.contradiction_flags);
        d["deficit_or_excess_token_id"] = (r.deficit_or_excess_token_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(r.deficit_or_excess_token_id);
        d["recommended_horizon_radius"] = static_cast<int>(r.recommended_horizon_radius);
        d["local_einselection_score"]   = r.local_einselection_score;
        d["failed_cascade_layer"]       = static_cast<int>(r.failed_cascade_layer);
        out.push_back(d);
    }
    return out;
}

godot::PackedInt32Array DataOrientedWaveFunctionCollapse::read_pointer_or_conflict_masks() const {
    godot::PackedInt32Array out;
    if (!dod_engine_) {
        return out;
    }
    std::vector<uint32_t> masks;
    dod_engine_->read_pointer_or_conflict_masks(masks);
    out.resize(static_cast<int64_t>(masks.size()));
    for (size_t i = 0; i < masks.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(masks[i]));
    }
    return out;
}

} // namespace ideam::godot_ext

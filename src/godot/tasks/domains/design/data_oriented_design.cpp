#include "data_oriented_design.h"
#include <algorithm>

namespace ideam::godot_ext {

DataOrientedDesign::DataOrientedDesign()
    : dod_engine_(std::make_unique<core::DesignDOD>()) {}

void DataOrientedDesign::set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem) {
    memory_manager_resource_ = p_mem;
    emit_changed();
}

void DataOrientedDesign::set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph) {
    task_graph_resource_ = p_graph;
    emit_changed();
}

void DataOrientedDesign::set_active_theme_3d(const godot::Ref<Theme3DResource> &p_theme) {
    active_theme_3d_ = p_theme;
    emit_changed();
}

void DataOrientedDesign::set_active_recipe(const godot::Ref<ParametricAssemblyRecipeResource> &p_recipe) {
    active_recipe_ = p_recipe;
    emit_changed();
}

void DataOrientedDesign::set_active_blueprint(const godot::Ref<BlueprintResource> &p_blueprint) {
    active_blueprint_ = p_blueprint;
    emit_changed();
}

void DataOrientedDesign::set_max_structure_nodes(int p_count) {
    max_structure_nodes_ = std::clamp(p_count, 8, 4096);
    if (dod_engine_) {
        core::DesignBufferCapacities caps = dod_engine_->get_capacities();
        caps.max_structure_nodes          = static_cast<size_t>(max_structure_nodes_);
        caps.max_sockets                  = static_cast<size_t>(max_structure_nodes_);
        dod_engine_->set_capacities(caps);
    }
    emit_changed();
}

void DataOrientedDesign::set_dream_morph_factor(float p_factor) {
    dream_morph_factor_ = std::clamp(p_factor, 0.0f, 1.0f);
    if (dod_engine_) {
        dod_engine_->set_dream_morph_factor(dream_morph_factor_);
    }
    emit_changed();
}

bool DataOrientedDesign::initialize_dod_buffers() {
    if (!dod_engine_ || memory_manager_resource_.is_null()) {
        return false;
    }
    core::MemoryManagerDOD *mem = memory_manager_resource_->get_dod_manager();
    if (!mem) {
        return false;
    }
    core::TaskGraphDOD *graph = task_graph_resource_.is_valid()
        ? task_graph_resource_->get_dod_graph()
        : nullptr;
    dod_engine_->bind_execution_context(mem, graph);
    return dod_engine_->initialize_design_memory(mem);
}

bool DataOrientedDesign::stage_active_sources_to_dod() {
    if (!dod_engine_) {
        return false;
    }
    core::StructureLayoutSolver &solver = dod_engine_->solver();
    solver.clear();

    if (active_theme_3d_.is_valid()) {
        std::vector<core::Style3DRulePod> rules;
        active_theme_3d_->compile_to_pod_rules(rules);
        for (const auto &r : rules) {
            solver.add_style_rule(r);
        }
    }

    if (active_blueprint_.is_valid()) {
        active_blueprint_->get_core_blueprint().extrude_to_structure_layout(solver);
    }

    if (active_recipe_.is_valid()) {
        active_recipe_->get_core_recipe().synthesize_to_layout_solver(solver, dream_morph_factor_);
    }

    return dod_engine_->stage_solver_to_dod();
}

bool DataOrientedDesign::execute_pipeline_direct(int p_stage_mask) {
    if (!dod_engine_) {
        return false;
    }
    return dod_engine_->execute_full_pipeline_direct(static_cast<uint32_t>(p_stage_mask));
}

godot::Array DataOrientedDesign::read_structure_nodes_as_dicts() const {
    godot::Array arr;
    if (!dod_engine_) return arr;
    std::vector<core::StructureNodePod> nodes;
    dod_engine_->read_structure_nodes(nodes);
    for (const auto &n : nodes) {
        godot::Dictionary d;
        d["node_id"]          = static_cast<int>(n.node_id);
        d["parent_id"]        = static_cast<int>(n.parent_id);
        d["semantic_tag_id"]  = static_cast<int>(n.semantic_tag_id);
        d["style_slot_id"]    = static_cast<int>(n.style_slot_id);
        d["rest_obb_extents"] = godot::Vector3(n.rest_obb_extents.x, n.rest_obb_extents.y, n.rest_obb_extents.z);
        d["position"]         = godot::Vector3(n.fitted_transform[3][0], n.fitted_transform[3][1], n.fitted_transform[3][2]);
        arr.push_back(d);
    }
    return arr;
}

godot::Array DataOrientedDesign::read_sockets_as_dicts() const {
    godot::Array arr;
    if (!dod_engine_) return arr;
    std::vector<core::SocketPod> sockets;
    dod_engine_->read_sockets(sockets);
    for (const auto &s : sockets) {
        godot::Dictionary d;
        d["socket_id"]            = static_cast<int>(s.socket_id);
        d["owner_node_id"]        = static_cast<int>(s.owner_node_id);
        d["mated_socket_id"]      = static_cast<int>(s.mated_socket_id);
        d["physics_binding_mode"] = static_cast<int>(s.physics_binding_mode);
        d["is_occupied"]          = s.is_occupied != 0u;
        arr.push_back(d);
    }
    return arr;
}

godot::Array DataOrientedDesign::read_aperture_cutouts_as_dicts() const {
    godot::Array arr;
    if (!dod_engine_) return arr;
    for (const auto &c : dod_engine_->solver().aperture_cutouts()) {
        godot::Dictionary d;
        d["aperture_node_id"]        = static_cast<int>(c.aperture_node_id);
        d["host_wall_node_id"]       = static_cast<int>(c.host_wall_node_id);
        d["aperture_semantic_role"]  = static_cast<int>(c.aperture_semantic_role);
        d["resulting_segment_count"] = static_cast<int>(c.resulting_segment_count);
        d["spawns_hinge_socket"]     = c.spawns_hinge_socket != 0u;
        arr.push_back(d);
    }
    return arr;
}

godot::PackedInt32Array DataOrientedDesign::read_node_status_masks() const {
    godot::PackedInt32Array out;
    if (!dod_engine_) return out;
    std::vector<uint32_t> masks;
    dod_engine_->read_node_status_masks(masks);
    out.resize(static_cast<int>(masks.size()));
    for (size_t i = 0; i < masks.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(masks[i]));
    }
    return out;
}

void DataOrientedDesign::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_memory_manager_resource", "mem"), &DataOrientedDesign::set_memory_manager_resource);
    ClassDB::bind_method(D_METHOD("get_memory_manager_resource"), &DataOrientedDesign::get_memory_manager_resource);

    ClassDB::bind_method(D_METHOD("set_task_graph_resource", "graph"), &DataOrientedDesign::set_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_task_graph_resource"), &DataOrientedDesign::get_task_graph_resource);

    ClassDB::bind_method(D_METHOD("set_active_theme_3d", "theme"), &DataOrientedDesign::set_active_theme_3d);
    ClassDB::bind_method(D_METHOD("get_active_theme_3d"), &DataOrientedDesign::get_active_theme_3d);

    ClassDB::bind_method(D_METHOD("set_active_recipe", "recipe"), &DataOrientedDesign::set_active_recipe);
    ClassDB::bind_method(D_METHOD("get_active_recipe"), &DataOrientedDesign::get_active_recipe);

    ClassDB::bind_method(D_METHOD("set_active_blueprint", "blueprint"), &DataOrientedDesign::set_active_blueprint);
    ClassDB::bind_method(D_METHOD("get_active_blueprint"), &DataOrientedDesign::get_active_blueprint);

    ClassDB::bind_method(D_METHOD("set_max_structure_nodes", "count"), &DataOrientedDesign::set_max_structure_nodes);
    ClassDB::bind_method(D_METHOD("get_max_structure_nodes"), &DataOrientedDesign::get_max_structure_nodes);

    ClassDB::bind_method(D_METHOD("set_dream_morph_factor", "factor"), &DataOrientedDesign::set_dream_morph_factor);
    ClassDB::bind_method(D_METHOD("get_dream_morph_factor"), &DataOrientedDesign::get_dream_morph_factor);

    ClassDB::bind_method(D_METHOD("initialize_dod_buffers"), &DataOrientedDesign::initialize_dod_buffers);
    ClassDB::bind_method(D_METHOD("stage_active_sources_to_dod"), &DataOrientedDesign::stage_active_sources_to_dod);
    ClassDB::bind_method(
        D_METHOD("execute_pipeline_direct", "stage_mask"),
        &DataOrientedDesign::execute_pipeline_direct,
        DEFVAL(static_cast<int>(core::DESIGN_STAGE_DEFAULT_PIPELINE_MASK))
    );

    ClassDB::bind_method(D_METHOD("read_structure_nodes_as_dicts"), &DataOrientedDesign::read_structure_nodes_as_dicts);
    ClassDB::bind_method(D_METHOD("read_sockets_as_dicts"), &DataOrientedDesign::read_sockets_as_dicts);
    ClassDB::bind_method(D_METHOD("read_aperture_cutouts_as_dicts"), &DataOrientedDesign::read_aperture_cutouts_as_dicts);
    ClassDB::bind_method(D_METHOD("read_node_status_masks"), &DataOrientedDesign::read_node_status_masks);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "memory_manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"), "set_memory_manager_resource", "get_memory_manager_resource");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"), "set_task_graph_resource", "get_task_graph_resource");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_theme_3d", PROPERTY_HINT_RESOURCE_TYPE, "Theme3DResource"), "set_active_theme_3d", "get_active_theme_3d");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_recipe", PROPERTY_HINT_RESOURCE_TYPE, "ParametricAssemblyRecipeResource"), "set_active_recipe", "get_active_recipe");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_blueprint", PROPERTY_HINT_RESOURCE_TYPE, "BlueprintResource"), "set_active_blueprint", "get_active_blueprint");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_structure_nodes", PROPERTY_HINT_RANGE, "8,4096,1"), "set_max_structure_nodes", "get_max_structure_nodes");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dream_morph_factor", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_dream_morph_factor", "get_dream_morph_factor");
}

} // namespace ideam::godot_ext

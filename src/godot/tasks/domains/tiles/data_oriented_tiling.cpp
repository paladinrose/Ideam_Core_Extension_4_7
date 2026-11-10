#include "data_oriented_tiling.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ideam::godot_ext {

void DataOrientedTiling::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(TILES_DOD_EXEC_DISABLED);
    BIND_ENUM_CONSTANT(TILES_DOD_EXEC_COMPOSABLE_STAGES);
    BIND_ENUM_CONSTANT(TILES_DOD_EXEC_CUSTOM_GRAPH_RESOURCE);
    BIND_ENUM_CONSTANT(TILES_DOD_EXEC_MANUAL_GRAPH);

    ADD_GROUP("Tiling Resources & Domain Bridges", "");

    ClassDB::bind_method(D_METHOD("set_tilings", "tilings"), &DataOrientedTiling::set_tilings);
    ClassDB::bind_method(D_METHOD("get_tilings"), &DataOrientedTiling::get_tilings);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::ARRAY,
            "tilings",
            PROPERTY_HINT_ARRAY_TYPE,
            String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":TilingResource"
        ),
        "set_tilings",
        "get_tilings"
    );

    ClassDB::bind_method(D_METHOD("set_linked_geometry_resource", "geometry"), &DataOrientedTiling::set_linked_geometry_resource);
    ClassDB::bind_method(D_METHOD("get_linked_geometry_resource"), &DataOrientedTiling::get_linked_geometry_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "linked_geometry_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedGeometry"),
        "set_linked_geometry_resource",
        "get_linked_geometry_resource"
    );

    ClassDB::bind_method(D_METHOD("set_linked_splines_resource", "splines"), &DataOrientedTiling::set_linked_splines_resource);
    ClassDB::bind_method(D_METHOD("get_linked_splines_resource"), &DataOrientedTiling::get_linked_splines_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "linked_splines_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedSplines"),
        "set_linked_splines_resource",
        "get_linked_splines_resource"
    );

    ClassDB::bind_method(D_METHOD("bind_data_oriented_geometry", "geometry"), &DataOrientedTiling::bind_data_oriented_geometry);
    ClassDB::bind_method(D_METHOD("bind_data_oriented_splines", "splines"), &DataOrientedTiling::bind_data_oriented_splines);

    ADD_GROUP("DOD Pipeline Configuration", "");

    ClassDB::bind_method(D_METHOD("set_dod_execution_mode", "mode"), &DataOrientedTiling::set_dod_execution_mode);
    ClassDB::bind_method(D_METHOD("get_dod_execution_mode"), &DataOrientedTiling::get_dod_execution_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "dod_execution_mode",
            PROPERTY_HINT_ENUM,
            "Disabled:0,Composable Stages:1,Custom TaskGraphResource:2,Manual Graph:3"
        ),
        "set_dod_execution_mode",
        "get_dod_execution_mode"
    );

    ClassDB::bind_method(D_METHOD("set_pipeline_stage_mask", "mask"), &DataOrientedTiling::set_pipeline_stage_mask);
    ClassDB::bind_method(D_METHOD("get_pipeline_stage_mask"), &DataOrientedTiling::get_pipeline_stage_mask);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "pipeline_stage_mask",
            PROPERTY_HINT_FLAGS,
            "Generation & Hex Partition (T_M1):1,Lattice & Spline Mask Query (T_Q1):2,Instance & Seam Transform (T_T1+T_T2):4,Hexize & Border Extract (T_T3):8,Navigation & Flow-Field (T_T4):16"
        ),
        "set_pipeline_stage_mask",
        "get_pipeline_stage_mask"
    );

    ClassDB::bind_method(D_METHOD("set_auto_connect_pipeline_stages", "auto_connect"), &DataOrientedTiling::set_auto_connect_pipeline_stages);
    ClassDB::bind_method(D_METHOD("get_auto_connect_pipeline_stages"), &DataOrientedTiling::get_auto_connect_pipeline_stages);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_connect_pipeline_stages"), "set_auto_connect_pipeline_stages", "get_auto_connect_pipeline_stages");

    ClassDB::bind_method(D_METHOD("set_default_edge_subdivisions", "subdivs"), &DataOrientedTiling::set_default_edge_subdivisions);
    ClassDB::bind_method(D_METHOD("get_default_edge_subdivisions"), &DataOrientedTiling::get_default_edge_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "default_edge_subdivisions", PROPERTY_HINT_RANGE, "2,48,1"),
        "set_default_edge_subdivisions",
        "get_default_edge_subdivisions"
    );

    ClassDB::bind_method(D_METHOD("set_custom_task_graph_resource", "resource"), &DataOrientedTiling::set_custom_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_custom_task_graph_resource"), &DataOrientedTiling::get_custom_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "custom_task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_custom_task_graph_resource",
        "get_custom_task_graph_resource"
    );

    ADD_GROUP("Buffer Capacities", "max_dod_");

    ClassDB::bind_method(D_METHOD("set_max_dod_prototiles", "max"), &DataOrientedTiling::set_max_dod_prototiles);
    ClassDB::bind_method(D_METHOD("get_max_dod_prototiles"), &DataOrientedTiling::get_max_dod_prototiles);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_prototiles", PROPERTY_HINT_RANGE, "4,512,1"), "set_max_dod_prototiles", "get_max_dod_prototiles");

    ClassDB::bind_method(D_METHOD("set_max_dod_cells", "max"), &DataOrientedTiling::set_max_dod_cells);
    ClassDB::bind_method(D_METHOD("get_max_dod_cells"), &DataOrientedTiling::get_max_dod_cells);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_cells", PROPERTY_HINT_RANGE, "64,262144,64"), "set_max_dod_cells", "get_max_dod_cells");

    ClassDB::bind_method(D_METHOD("set_max_dod_edges", "max"), &DataOrientedTiling::set_max_dod_edges);
    ClassDB::bind_method(D_METHOD("get_max_dod_edges"), &DataOrientedTiling::get_max_dod_edges);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_edges", PROPERTY_HINT_RANGE, "128,524288,128"), "set_max_dod_edges", "get_max_dod_edges");

    ClassDB::bind_method(D_METHOD("set_max_dod_hex_super_cells", "max"), &DataOrientedTiling::set_max_dod_hex_super_cells);
    ClassDB::bind_method(D_METHOD("get_max_dod_hex_super_cells"), &DataOrientedTiling::get_max_dod_hex_super_cells);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_hex_super_cells", PROPERTY_HINT_RANGE, "32,65536,32"), "set_max_dod_hex_super_cells", "get_max_dod_hex_super_cells");

    ClassDB::bind_method(D_METHOD("set_max_dod_nav_requests", "max"), &DataOrientedTiling::set_max_dod_nav_requests);
    ClassDB::bind_method(D_METHOD("get_max_dod_nav_requests"), &DataOrientedTiling::get_max_dod_nav_requests);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_nav_requests", PROPERTY_HINT_RANGE, "16,16384,16"), "set_max_dod_nav_requests", "get_max_dod_nav_requests");

    // --- Lifecycle & Execution ---
    ClassDB::bind_method(D_METHOD("setup_dod_pipeline"), &DataOrientedTiling::setup_dod_pipeline);
    ClassDB::bind_method(D_METHOD("bake_to_dod"), &DataOrientedTiling::bake_to_dod);
    ClassDB::bind_method(D_METHOD("rebuild_dod_task_graph"), &DataOrientedTiling::rebuild_dod_task_graph);
    ClassDB::bind_method(D_METHOD("execute_dod_step", "delta"), &DataOrientedTiling::execute_dod_step);

    // --- Navigation & Readback ---
    ClassDB::bind_method(
        D_METHOD("stage_navigation_requests", "start_cells", "goal_cells", "manhattan_heuristic", "emit_flow_goals"),
        &DataOrientedTiling::stage_navigation_requests,
        DEFVAL(false),
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_navigation_path_cells", "request_index"), &DataOrientedTiling::get_navigation_path_cells);
    ClassDB::bind_method(D_METHOD("get_flow_field_costs"), &DataOrientedTiling::get_flow_field_costs);
    ClassDB::bind_method(D_METHOD("get_flow_field_vectors"), &DataOrientedTiling::get_flow_field_vectors);

    ClassDB::bind_method(D_METHOD("get_active_cell_count"), &DataOrientedTiling::get_active_cell_count);
    ClassDB::bind_method(D_METHOD("get_active_edge_count"), &DataOrientedTiling::get_active_edge_count);
    ClassDB::bind_method(D_METHOD("get_active_hex_super_cell_count"), &DataOrientedTiling::get_active_hex_super_cell_count);
    ClassDB::bind_method(D_METHOD("get_cell_transforms_2d"), &DataOrientedTiling::get_cell_transforms_2d);
    ClassDB::bind_method(D_METHOD("get_cell_centers_2d"), &DataOrientedTiling::get_cell_centers_2d);
    ClassDB::bind_method(D_METHOD("get_hex_super_cell_centers_2d"), &DataOrientedTiling::get_hex_super_cell_centers_2d);

    ClassDB::bind_method(D_METHOD("get_tiling_dod_slot", "tiling"), &DataOrientedTiling::get_tiling_dod_slot);
    ClassDB::bind_method(D_METHOD("get_buffer_id", "semantic_name"), &DataOrientedTiling::get_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_manifest_dictionary"), &DataOrientedTiling::get_buffer_manifest_dictionary);

    // --- Dynamic Task Nodes & Stage Co-Scheduling ---
    ClassDB::bind_method(
        D_METHOD("add_tiles_transform_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedTiling::add_tiles_transform_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_tiles_query_node", "logic_id", "query_op", "properties", "priority", "use_registry_factory"),
        &DataOrientedTiling::add_tiles_query_node,
        DEFVAL(0),
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_tiles_metadata_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedTiling::add_tiles_metadata_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );

    ClassDB::bind_method(
        D_METHOD("schedule_node_with_stage", "stage_flag", "custom_node_id", "join_downstream"),
        &DataOrientedTiling::schedule_node_with_stage,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("schedule_node_between_stages", "upstream_stage_flag", "downstream_stage_flag", "custom_node_id"),
        &DataOrientedTiling::schedule_node_between_stages
    );
}

// ============================================================================
// Property Setters & Getters
// ============================================================================

void DataOrientedTiling::set_tilings(const godot::TypedArray<TilingResource> &p_tilings) {
    tilings_ = p_tilings;
    emit_changed();
}

void DataOrientedTiling::set_linked_geometry_resource(const godot::Ref<DataOrientedGeometry> &p_geometry) {
    linked_geometry_resource_ = p_geometry;
    if (linked_geometry_resource_.is_valid() && tile_dod_) {
        bind_data_oriented_geometry(linked_geometry_resource_);
    }
    emit_changed();
}

bool DataOrientedTiling::bind_data_oriented_geometry(const godot::Ref<DataOrientedGeometry> &p_geometry) {
    if (p_geometry.is_null()) return false;
    linked_geometry_resource_ = p_geometry;
    if (!p_geometry->get_geometry_dod()) {
        p_geometry->setup_dod_pipeline();
    }
    core::GeometryDOD *geom_dod = p_geometry->get_geometry_dod();
    if (!geom_dod) return false;

    _ensure_tile_dod_initialized(p_geometry->get_memory_manager_shared());
    if (!active_task_graph_ && p_geometry->get_active_task_graph_shared()) {
        active_task_graph_ = p_geometry->get_active_task_graph_shared();
        tile_dod_->set_default_task_graph(active_task_graph_);
    }
    tile_dod_->bind_external_geometry_buffers(geom_dod->get_buffer_manifest());
    return true;
}

void DataOrientedTiling::set_linked_splines_resource(const godot::Ref<DataOrientedSplines> &p_splines) {
    linked_splines_resource_ = p_splines;
    if (linked_splines_resource_.is_valid() && tile_dod_) {
        bind_data_oriented_splines(linked_splines_resource_);
    }
    emit_changed();
}

bool DataOrientedTiling::bind_data_oriented_splines(const godot::Ref<DataOrientedSplines> &p_splines) {
    if (p_splines.is_null()) return false;
    linked_splines_resource_ = p_splines;
    if (!p_splines->get_spline_dod()) {
        p_splines->setup_dod_pipeline();
    }
    core::SplineDOD *sp_dod = p_splines->get_spline_dod();
    if (!sp_dod) return false;

    _ensure_tile_dod_initialized(p_splines->get_memory_manager_shared());
    if (!active_task_graph_ && p_splines->get_active_task_graph_shared()) {
        active_task_graph_ = p_splines->get_active_task_graph_shared();
        tile_dod_->set_default_task_graph(active_task_graph_);
    }
    tile_dod_->bind_external_splines_buffers(sp_dod->get_buffer_manifest());
    return true;
}

void DataOrientedTiling::set_dod_execution_mode(TilesDODExecutionMode p_mode) {
    dod_execution_mode_ = p_mode;
}

void DataOrientedTiling::set_pipeline_stage_mask(int p_mask) {
    pipeline_stage_mask_ = static_cast<uint32_t>(p_mask) & core::TILES_STAGE_ALL_MASK;
}

void DataOrientedTiling::set_auto_connect_pipeline_stages(bool p_auto_connect) {
    auto_connect_pipeline_stages_ = p_auto_connect;
}

void DataOrientedTiling::set_default_edge_subdivisions(int p_subdivs) {
    default_edge_subdivisions_ = std::clamp(p_subdivs, 2, 48);
}

void DataOrientedTiling::set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource) {
    custom_task_graph_resource_ = p_resource;
}

void DataOrientedTiling::set_max_dod_prototiles(int p_max) {
    max_dod_prototiles_ = std::max(4, p_max);
}

void DataOrientedTiling::set_max_dod_cells(int p_max) {
    max_dod_cells_ = std::max(64, p_max);
}

void DataOrientedTiling::set_max_dod_edges(int p_max) {
    max_dod_edges_ = std::max(128, p_max);
}

void DataOrientedTiling::set_max_dod_hex_super_cells(int p_max) {
    max_dod_hex_super_cells_ = std::max(32, p_max);
}

void DataOrientedTiling::set_max_dod_nav_requests(int p_max) {
    max_dod_nav_requests_ = std::max(16, p_max);
}

// ============================================================================
// Pipeline Setup, Baking & Execution
// ============================================================================

core::TilesBufferCapacities DataOrientedTiling::_build_capacities() const noexcept {
    core::TilesBufferCapacities caps{};
    caps.max_prototiles        = static_cast<uint32_t>(std::max(4, max_dod_prototiles_));
    caps.max_cells             = static_cast<uint32_t>(std::max(64, max_dod_cells_));
    caps.max_cell_csr_indices  = caps.max_cells * 6u;
    caps.max_edges             = static_cast<uint32_t>(std::max(128, max_dod_edges_));
    caps.max_hex_super_cells   = static_cast<uint32_t>(std::max(32, max_dod_hex_super_cells_));
    caps.max_hex_child_csr     = caps.max_hex_super_cells * 12u;
    caps.max_nav_requests      = static_cast<uint32_t>(std::max(16, max_dod_nav_requests_));
    caps.max_nav_path_cells    = caps.max_nav_requests * 64u;
    return caps;
}

void DataOrientedTiling::_ensure_tile_dod_initialized(
    std::shared_ptr<core::MemoryManagerDOD> p_shared_manager
) {
    const core::TilesBufferCapacities caps = _build_capacities();
    if (!tile_dod_) {
        tile_dod_ = std::make_unique<core::TileDOD>(std::move(p_shared_manager), caps);
    } else if (p_shared_manager && tile_dod_->get_memory_manager() != p_shared_manager) {
        tile_dod_ = std::make_unique<core::TileDOD>(std::move(p_shared_manager), caps);
    }
    if (!active_task_graph_ && tile_dod_) {
        active_task_graph_ = tile_dod_->get_default_task_graph();
    }
}

void DataOrientedTiling::_clear_resource_maps() noexcept {
    object_id_to_prototile_slot_.clear();
    prototile_slot_to_resource_.clear();
}

void DataOrientedTiling::setup_dod_pipeline_with_manager(
    std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
    std::shared_ptr<core::TaskGraphDOD> p_shared_graph
) {
    _ensure_tile_dod_initialized(std::move(p_shared_manager));
    if (p_shared_graph) {
        active_task_graph_ = std::move(p_shared_graph);
        tile_dod_->set_default_task_graph(active_task_graph_);
    } else {
        active_task_graph_ = tile_dod_->get_default_task_graph();
    }

    if (linked_geometry_resource_.is_valid()) {
        bind_data_oriented_geometry(linked_geometry_resource_);
    }
    if (linked_splines_resource_.is_valid()) {
        bind_data_oriented_splines(linked_splines_resource_);
    }

    bake_to_dod();
    rebuild_dod_task_graph();
}

void DataOrientedTiling::setup_dod_pipeline() {
    std::shared_ptr<core::MemoryManagerDOD> shared_mgr = nullptr;
    std::shared_ptr<core::TaskGraphDOD> shared_graph   = nullptr;
    if (linked_geometry_resource_.is_valid()) {
        if (!linked_geometry_resource_->get_geometry_dod()) {
            linked_geometry_resource_->setup_dod_pipeline();
        }
        shared_mgr   = linked_geometry_resource_->get_memory_manager_shared();
        shared_graph = linked_geometry_resource_->get_active_task_graph_shared();
    } else if (linked_splines_resource_.is_valid()) {
        if (!linked_splines_resource_->get_spline_dod()) {
            linked_splines_resource_->setup_dod_pipeline();
        }
        shared_mgr   = linked_splines_resource_->get_memory_manager_shared();
        shared_graph = linked_splines_resource_->get_active_task_graph_shared();
    }
    setup_dod_pipeline_with_manager(std::move(shared_mgr), std::move(shared_graph));
}

void DataOrientedTiling::bake_to_dod() {
    _ensure_tile_dod_initialized();
    if (!tile_dod_) return;

    tile_dod_->reset_all_slots();
    _clear_resource_maps();

    if (linked_geometry_resource_.is_valid() && linked_geometry_resource_->get_geometry_dod()) {
        tile_dod_->bind_external_geometry_buffers(
            linked_geometry_resource_->get_geometry_dod()->get_buffer_manifest()
        );
    } else if (linked_splines_resource_.is_valid() && linked_splines_resource_->get_spline_dod()) {
        tile_dod_->bind_external_splines_buffers(
            linked_splines_resource_->get_spline_dod()->get_buffer_manifest()
        );
    }

    core::GeometryDOD *geom_dod = (linked_geometry_resource_.is_valid())
        ? linked_geometry_resource_->get_geometry_dod()
        : nullptr;

    for (int i = 0; i < tilings_.size(); ++i) {
        godot::Ref<TilingResource> res = tilings_[i];
        if (res.is_null()) continue;

        uint32_t slot = core::INVALID_ID;
        if (auto *comp = godot::Object::cast_to<CompositeTilingResource>(res.ptr())) {
            slot = tile_dod_->bake_region(comp->get_core_region(), geom_dod);
        } else if (auto *wp = godot::Object::cast_to<WallpaperTilingResource>(res.ptr())) {
            slot = tile_dod_->bake_prototile(wp->get_core_tiling());
        }

        if (slot != core::INVALID_ID) {
            object_id_to_prototile_slot_[res->get_instance_id()] = slot;
            prototile_slot_to_resource_[slot]                   = res;
        }
    }

    tile_dod_->sync_selection_ranges();
}

void DataOrientedTiling::rebuild_dod_task_graph() {
    _ensure_tile_dod_initialized();
    if (!tile_dod_ || !active_task_graph_) return;

    if (dod_execution_mode_ == TILES_DOD_EXEC_DISABLED ||
        dod_execution_mode_ == TILES_DOD_EXEC_MANUAL_GRAPH) {
        return;
    }

    // Only clear if we are not sharing a multi-domain TaskGraphDOD with Geometry or Splines
    const bool shares_graph =
        (linked_geometry_resource_.is_valid() &&
         linked_geometry_resource_->get_active_task_graph_shared() == active_task_graph_) ||
        (linked_splines_resource_.is_valid() &&
         linked_splines_resource_->get_active_task_graph_shared() == active_task_graph_);
    if (!shares_graph) {
        active_task_graph_->clear_graph();
    }

    ui_to_dod_map_.clear();

    if (dod_execution_mode_ == TILES_DOD_EXEC_CUSTOM_GRAPH_RESOURCE &&
        custom_task_graph_resource_.is_valid()) {
        custom_task_graph_resource_->bake_to_dod(
            *active_task_graph_,
            tile_dod_->get_memory_manager_raw(),
            ui_to_dod_map_
        );
        return;
    }

    tile_dod_->inject_pipeline_stages(
        *active_task_graph_,
        pipeline_stage_mask_,
        auto_connect_pipeline_stages_
    );
}

void DataOrientedTiling::execute_dod_step(double p_delta) {
    if (!tile_dod_ || dod_execution_mode_ == TILES_DOD_EXEC_DISABLED) return;
    tile_dod_->execute_step(p_delta, active_task_graph_.get());
}

// ============================================================================
// Navigation Staging & Readback
// ============================================================================

void DataOrientedTiling::stage_navigation_requests(
    const godot::PackedInt32Array &p_start_cells,
    const godot::PackedInt32Array &p_goal_cells,
    bool p_manhattan_heuristic,
    bool p_emit_flow_goals
) {
    _ensure_tile_dod_initialized();
    if (!tile_dod_) return;

    const int count = std::min(p_start_cells.size(), p_goal_cells.size());
    std::vector<core::TileNavRequestPOD> reqs(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        core::TileNavRequestPOD r{};
        r.start_cell_id     = static_cast<uint32_t>(std::max<int32_t>(0, p_start_cells[i]));
        r.goal_cell_id      = static_cast<uint32_t>(std::max<int32_t>(0, p_goal_cells[i]));
        r.max_path_capacity = 64u;
        r.use_manhattan     = p_manhattan_heuristic ? 1u : 0u;
        r.emit_flow_goal    = p_emit_flow_goals ? 1u : 0u;
        reqs[static_cast<size_t>(i)] = r;
    }
    tile_dod_->stage_navigation_requests(reqs);
}

godot::PackedInt32Array DataOrientedTiling::get_navigation_path_cells(int p_request_index) const {
    godot::PackedInt32Array out;
    if (!tile_dod_ || p_request_index < 0) return out;
    const auto span = tile_dod_->get_navigation_path_cells(static_cast<uint32_t>(p_request_index));
    out.resize(static_cast<int>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(span[i]));
    }
    return out;
}

godot::PackedFloat32Array DataOrientedTiling::get_flow_field_costs() const {
    godot::PackedFloat32Array out;
    if (!tile_dod_) return out;
    const auto span = tile_dod_->get_flow_field_costs();
    out.resize(static_cast<int>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int>(i), span[i]);
    }
    return out;
}

godot::PackedVector2Array DataOrientedTiling::get_flow_field_vectors() const {
    godot::PackedVector2Array out;
    if (!tile_dod_) return out;
    const auto span = tile_dod_->get_flow_field_vectors();
    out.resize(static_cast<int>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector2(span[i].x, span[i].y));
    }
    return out;
}

int DataOrientedTiling::get_active_cell_count() const {
    return tile_dod_ ? static_cast<int>(tile_dod_->get_active_cell_count()) : 0;
}

int DataOrientedTiling::get_active_edge_count() const {
    return tile_dod_ ? static_cast<int>(tile_dod_->get_active_edge_count()) : 0;
}

int DataOrientedTiling::get_active_hex_super_cell_count() const {
    return tile_dod_ ? static_cast<int>(tile_dod_->get_active_hex_super_cell_count()) : 0;
}

godot::TypedArray<godot::Transform2D> DataOrientedTiling::get_cell_transforms_2d() const {
    godot::TypedArray<godot::Transform2D> out;
    if (!tile_dod_) return out;
    const auto xforms = tile_dod_->get_active_instance_transforms();
    out.resize(static_cast<int>(xforms.size()));
    for (size_t i = 0; i < xforms.size(); ++i) {
        const core::TileTransformPOD &xf = xforms[i];
        godot::Transform2D t(
            xf.angle_rad,
            godot::Vector2(xf.scale.x, xf.scale.y),
            0.0f,
            godot::Vector2(xf.position.x, xf.position.y)
        );
        out[static_cast<int>(i)] = t;
    }
    return out;
}

godot::PackedVector2Array DataOrientedTiling::get_cell_centers_2d() const {
    godot::PackedVector2Array out;
    if (!tile_dod_) return out;
    const auto cells = tile_dod_->get_active_cells();
    out.resize(static_cast<int>(cells.size()));
    for (size_t i = 0; i < cells.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector2(cells[i].center.x, cells[i].center.y));
    }
    return out;
}

godot::PackedVector2Array DataOrientedTiling::get_hex_super_cell_centers_2d() const {
    godot::PackedVector2Array out;
    if (!tile_dod_) return out;
    const auto hexes = tile_dod_->get_active_hex_super_cells();
    out.resize(static_cast<int>(hexes.size()));
    for (size_t i = 0; i < hexes.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector2(hexes[i].center.x, hexes[i].center.y));
    }
    return out;
}

int DataOrientedTiling::get_tiling_dod_slot(const godot::Ref<TilingResource> &p_tiling) const {
    if (p_tiling.is_null()) return -1;
    auto it = object_id_to_prototile_slot_.find(p_tiling->get_instance_id());
    return (it != object_id_to_prototile_slot_.end()) ? static_cast<int>(it->second) : -1;
}

int DataOrientedTiling::get_buffer_id(const godot::StringName &p_semantic_name) const {
    if (!tile_dod_) return -1;
    const godot::CharString utf8 = godot::String(p_semantic_name).utf8();
    const uint32_t id = tile_dod_->get_buffer_id_by_name(std::string_view(utf8.get_data(), utf8.length()));
    return (id != core::INVALID_ID) ? static_cast<int>(id) : -1;
}

godot::Dictionary DataOrientedTiling::get_buffer_manifest_dictionary() const {
    godot::Dictionary d;
    if (tile_dod_) {
        tile_dod_->populate_task_buffer_dictionary(d, true);
    }
    return d;
}

// ============================================================================
// Dynamic Task Node & Wave Co-Scheduling API
// ============================================================================

int DataOrientedTiling::add_tiles_transform_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_tile_dod_initialized();
    if (!tile_dod_ || !active_task_graph_) return -1;

    core::TilesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid   = tile_dod_->add_transform_node_by_id(
        *active_task_graph_,
        static_cast<core::TilesTransformLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedTiling::add_tiles_query_node(
    int p_logic_id,
    int p_query_op,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_tile_dod_initialized();
    if (!tile_dod_ || !active_task_graph_) return -1;

    core::TilesNodeExecutionConfig cfg{};
    cfg.query_op             = (p_query_op == 1) ? core::QueryOp::ADD : core::QueryOp::CULL;
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid   = tile_dod_->add_query_node_by_id(
        *active_task_graph_,
        static_cast<core::TilesQueryLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedTiling::add_tiles_metadata_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_tile_dod_initialized();
    if (!tile_dod_ || !active_task_graph_) return -1;

    core::TilesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid   = tile_dod_->add_metadata_node_by_id(
        *active_task_graph_,
        static_cast<core::TilesMetadataLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

bool DataOrientedTiling::schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream) {
    if (!tile_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::TileDOD::schedule_node_with_stage(
        *active_task_graph_,
        tile_dod_->get_active_topology(),
        static_cast<core::TilesPipelineStageFlags>(p_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id),
        p_join_downstream
    );
}

bool DataOrientedTiling::schedule_node_between_stages(
    int p_upstream_stage_flag,
    int p_downstream_stage_flag,
    int p_custom_node_id
) {
    if (!tile_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::TileDOD::schedule_node_between_stages(
        *active_task_graph_,
        tile_dod_->get_active_topology(),
        static_cast<core::TilesPipelineStageFlags>(p_upstream_stage_flag),
        static_cast<core::TilesPipelineStageFlags>(p_downstream_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id)
    );
}

} // namespace ideam::godot_ext

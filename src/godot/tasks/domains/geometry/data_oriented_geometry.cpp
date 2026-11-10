#include "data_oriented_geometry.h"

#include <algorithm>
#include <string>

namespace ideam::godot_ext {

void DataOrientedGeometry::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(GEOMETRY_DOD_EXEC_DISABLED);
    BIND_ENUM_CONSTANT(GEOMETRY_DOD_EXEC_COMPOSABLE_STAGES);
    BIND_ENUM_CONSTANT(GEOMETRY_DOD_EXEC_CUSTOM_GRAPH_RESOURCE);
    BIND_ENUM_CONSTANT(GEOMETRY_DOD_EXEC_MANUAL_GRAPH);

    ADD_GROUP("Geometry Resources", "");

    ClassDB::bind_method(D_METHOD("set_polygons", "polygons"), &DataOrientedGeometry::set_polygons);
    ClassDB::bind_method(D_METHOD("get_polygons"), &DataOrientedGeometry::get_polygons);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "polygons", PROPERTY_HINT_ARRAY_TYPE, "Polygon2DResource"),
        "set_polygons",
        "get_polygons"
    );

    ClassDB::bind_method(D_METHOD("set_voronoi_diagram", "voronoi"), &DataOrientedGeometry::set_voronoi_diagram);
    ClassDB::bind_method(D_METHOD("get_voronoi_diagram"), &DataOrientedGeometry::get_voronoi_diagram);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "voronoi_diagram", PROPERTY_HINT_RESOURCE_TYPE, "VoronoiDiagram2DResource"),
        "set_voronoi_diagram",
        "get_voronoi_diagram"
    );

    ClassDB::bind_method(D_METHOD("set_sdf_grids", "grids"), &DataOrientedGeometry::set_sdf_grids);
    ClassDB::bind_method(D_METHOD("get_sdf_grids"), &DataOrientedGeometry::get_sdf_grids);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "sdf_grids", PROPERTY_HINT_ARRAY_TYPE, "SDFGrid2DResource"),
        "set_sdf_grids",
        "get_sdf_grids"
    );

    ADD_GROUP("DOD Pipeline Configuration", "");

    ClassDB::bind_method(D_METHOD("set_dod_execution_mode", "mode"), &DataOrientedGeometry::set_dod_execution_mode);
    ClassDB::bind_method(D_METHOD("get_dod_execution_mode"), &DataOrientedGeometry::get_dod_execution_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "dod_execution_mode", PROPERTY_HINT_ENUM, "Disabled:0,Composable Stages:1,Custom TaskGraphResource:2,Manual Graph:3"),
        "set_dod_execution_mode",
        "get_dod_execution_mode"
    );

    ClassDB::bind_method(D_METHOD("set_pipeline_stage_mask", "mask"), &DataOrientedGeometry::set_pipeline_stage_mask);
    ClassDB::bind_method(D_METHOD("get_pipeline_stage_mask"), &DataOrientedGeometry::get_pipeline_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "pipeline_stage_mask"), "set_pipeline_stage_mask", "get_pipeline_stage_mask");

    ClassDB::bind_method(D_METHOD("set_auto_connect_pipeline_stages", "auto_connect"), &DataOrientedGeometry::set_auto_connect_pipeline_stages);
    ClassDB::bind_method(D_METHOD("get_auto_connect_pipeline_stages"), &DataOrientedGeometry::get_auto_connect_pipeline_stages);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_connect_pipeline_stages"), "set_auto_connect_pipeline_stages", "get_auto_connect_pipeline_stages");

    ClassDB::bind_method(D_METHOD("set_auto_sync_resources_from_dod", "auto_sync"), &DataOrientedGeometry::set_auto_sync_resources_from_dod);
    ClassDB::bind_method(D_METHOD("get_auto_sync_resources_from_dod"), &DataOrientedGeometry::get_auto_sync_resources_from_dod);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_sync_resources_from_dod"), "set_auto_sync_resources_from_dod", "get_auto_sync_resources_from_dod");

    ClassDB::bind_method(D_METHOD("set_custom_task_graph_resource", "resource"), &DataOrientedGeometry::set_custom_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_custom_task_graph_resource"), &DataOrientedGeometry::get_custom_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "custom_task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_custom_task_graph_resource",
        "get_custom_task_graph_resource"
    );

    ADD_GROUP("DOD Buffer Capacities", "");

    ClassDB::bind_method(D_METHOD("set_max_dod_polylines", "max"), &DataOrientedGeometry::set_max_dod_polylines);
    ClassDB::bind_method(D_METHOD("get_max_dod_polylines"), &DataOrientedGeometry::get_max_dod_polylines);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_polylines"), "set_max_dod_polylines", "get_max_dod_polylines");

    ClassDB::bind_method(D_METHOD("set_max_dod_polyline_vertices", "max"), &DataOrientedGeometry::set_max_dod_polyline_vertices);
    ClassDB::bind_method(D_METHOD("get_max_dod_polyline_vertices"), &DataOrientedGeometry::get_max_dod_polyline_vertices);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_polyline_vertices"), "set_max_dod_polyline_vertices", "get_max_dod_polyline_vertices");

    ClassDB::bind_method(D_METHOD("set_max_dod_voronoi_sites", "max"), &DataOrientedGeometry::set_max_dod_voronoi_sites);
    ClassDB::bind_method(D_METHOD("get_max_dod_voronoi_sites"), &DataOrientedGeometry::get_max_dod_voronoi_sites);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_voronoi_sites"), "set_max_dod_voronoi_sites", "get_max_dod_voronoi_sites");

    ClassDB::bind_method(D_METHOD("set_max_dod_sdf_cells", "max"), &DataOrientedGeometry::set_max_dod_sdf_cells);
    ClassDB::bind_method(D_METHOD("get_max_dod_sdf_cells"), &DataOrientedGeometry::get_max_dod_sdf_cells);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_sdf_cells"), "set_max_dod_sdf_cells", "get_max_dod_sdf_cells");

    // --- Methods ---
    ClassDB::bind_method(D_METHOD("setup_dod_pipeline"), &DataOrientedGeometry::setup_dod_pipeline);
    ClassDB::bind_method(D_METHOD("bake_to_dod"), &DataOrientedGeometry::bake_to_dod);
    ClassDB::bind_method(D_METHOD("rebuild_dod_task_graph"), &DataOrientedGeometry::rebuild_dod_task_graph);
    ClassDB::bind_method(D_METHOD("execute_dod_step", "delta"), &DataOrientedGeometry::execute_dod_step);
    ClassDB::bind_method(D_METHOD("sync_oop_resources_from_dod"), &DataOrientedGeometry::sync_oop_resources_from_dod);

    ClassDB::bind_method(D_METHOD("get_buffer_id", "semantic_name"), &DataOrientedGeometry::get_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_manifest_dictionary"), &DataOrientedGeometry::get_buffer_manifest_dictionary);
    ClassDB::bind_method(D_METHOD("get_polygon_dod_slot", "polygon"), &DataOrientedGeometry::get_polygon_dod_slot);
    ClassDB::bind_method(D_METHOD("get_extracted_marching_squares_segments"), &DataOrientedGeometry::get_extracted_marching_squares_segments);

    ClassDB::bind_method(
        D_METHOD("add_geometry_transform_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedGeometry::add_geometry_transform_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_geometry_query_node", "logic_id", "query_op", "properties", "priority", "use_registry_factory"),
        &DataOrientedGeometry::add_geometry_query_node,
        DEFVAL(0),
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_geometry_metadata_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedGeometry::add_geometry_metadata_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );

    ClassDB::bind_method(
        D_METHOD("schedule_node_with_stage", "stage_flag", "custom_node_id", "join_downstream"),
        &DataOrientedGeometry::schedule_node_with_stage,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("schedule_node_between_stages", "upstream_stage_flag", "downstream_stage_flag", "custom_node_id"),
        &DataOrientedGeometry::schedule_node_between_stages
    );
}

core::GeometryBufferCapacities DataOrientedGeometry::_build_capacities() const noexcept {
    core::GeometryBufferCapacities caps{};
    caps.max_polylines            = static_cast<uint32_t>(std::max(16, max_dod_polylines_));
    caps.max_polyline_vertices_2d = static_cast<uint32_t>(std::max(256, max_dod_polyline_vertices_));
    caps.max_polygon_meshes       = static_cast<uint32_t>(std::max(16, max_dod_polygon_meshes_));
    caps.max_polygon_vertices_2d  = caps.max_polyline_vertices_2d;
    caps.max_polygon_indices      = caps.max_polyline_vertices_2d * 3u;
    caps.max_voronoi_sites        = static_cast<uint32_t>(std::max(16, max_dod_voronoi_sites_));
    caps.max_sdf_grids            = static_cast<uint32_t>(std::max(1, max_dod_sdf_grids_));
    caps.max_sdf_cells            = static_cast<uint32_t>(std::max(256, max_dod_sdf_cells_));
    return caps;
}

void DataOrientedGeometry::_ensure_geometry_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager) {
    const core::GeometryBufferCapacities caps = _build_capacities();
    if (!geometry_dod_) {
        geometry_dod_ = std::make_unique<core::GeometryDOD>(std::move(p_shared_manager), caps);
    } else if (p_shared_manager && geometry_dod_->get_memory_manager() != p_shared_manager) {
        geometry_dod_ = std::make_unique<core::GeometryDOD>(std::move(p_shared_manager), caps);
    }
    if (!active_task_graph_) {
        active_task_graph_ = geometry_dod_->get_default_task_graph();
    }
}

void DataOrientedGeometry::_clear_resource_maps() noexcept {
    object_id_to_polyline_slot_.clear();
    polyline_slot_to_resource_.clear();
    object_id_to_sdf_slot_.clear();
    sdf_slot_to_resource_.clear();
}

void DataOrientedGeometry::set_polygons(const godot::TypedArray<Polygon2DResource> &p_polygons) {
    polygons_ = p_polygons;
    emit_changed();
}

void DataOrientedGeometry::set_voronoi_diagram(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi) {
    voronoi_diagram_ = p_voronoi;
    emit_changed();
}

void DataOrientedGeometry::set_sdf_grids(const godot::TypedArray<SDFGrid2DResource> &p_grids) {
    sdf_grids_ = p_grids;
    emit_changed();
}

void DataOrientedGeometry::set_dod_execution_mode(GeometryDODExecutionMode p_mode) {
    dod_execution_mode_ = p_mode;
}

void DataOrientedGeometry::set_pipeline_stage_mask(int p_mask) {
    pipeline_stage_mask_ = static_cast<uint32_t>(p_mask) & core::GEOMETRY_STAGE_ALL_MASK;
}

void DataOrientedGeometry::set_auto_connect_pipeline_stages(bool p_auto_connect) {
    auto_connect_pipeline_stages_ = p_auto_connect;
}

void DataOrientedGeometry::set_auto_sync_resources_from_dod(bool p_auto_sync) {
    auto_sync_resources_from_dod_ = p_auto_sync;
}

void DataOrientedGeometry::set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource) {
    custom_task_graph_resource_ = p_resource;
}

void DataOrientedGeometry::set_max_dod_polylines(int p_max) {
    max_dod_polylines_ = std::max(16, p_max);
}

void DataOrientedGeometry::set_max_dod_polyline_vertices(int p_max) {
    max_dod_polyline_vertices_ = std::max(256, p_max);
}

void DataOrientedGeometry::set_max_dod_voronoi_sites(int p_max) {
    max_dod_voronoi_sites_ = std::max(16, p_max);
}

void DataOrientedGeometry::set_max_dod_sdf_cells(int p_max) {
    max_dod_sdf_cells_ = std::max(256, p_max);
}

void DataOrientedGeometry::setup_dod_pipeline_with_manager(
    std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
    std::shared_ptr<core::TaskGraphDOD> p_shared_graph
) {
    _ensure_geometry_dod_initialized(std::move(p_shared_manager));
    if (p_shared_graph) {
        active_task_graph_ = std::move(p_shared_graph);
        geometry_dod_->set_default_task_graph(active_task_graph_);
    }
    bake_to_dod();
    rebuild_dod_task_graph();
}

void DataOrientedGeometry::setup_dod_pipeline() {
    setup_dod_pipeline_with_manager(nullptr, nullptr);
}

void DataOrientedGeometry::bake_to_dod() {
    _ensure_geometry_dod_initialized();
    geometry_dod_->reset_all_slots();
    _clear_resource_maps();

    // 1. Bake Polygon2DResource array into GEOM_POLYLINE_SLICE_POD & GEOM_POLYLINE_VERTEX_2D
    for (int64_t i = 0; i < polygons_.size(); ++i) {
        godot::Ref<Polygon2DResource> poly = polygons_[i];
        if (poly.is_null()) continue;
        const auto v_span = poly->get_vertices_span();
        std::vector<glm::vec2> f_verts;
        f_verts.reserve(v_span.size());
        for (const auto &v : v_span) {
            f_verts.emplace_back(static_cast<float>(v.x), static_cast<float>(v.y));
        }
        const uint32_t slot = geometry_dod_->allocate_polyline_2d(f_verts, poly->is_closed());
        if (slot != core::INVALID_ID) {
            object_id_to_polyline_slot_[poly->get_instance_id()] = slot;
            polyline_slot_to_resource_[slot]                     = poly;
        }
    }

    // 2. Bake VoronoiDiagram2DResource into GEOM_VORONOI_SITE_2D
    if (voronoi_diagram_.is_valid()) {
        const auto s_span = voronoi_diagram_->get_sites_span();
        std::vector<glm::vec2> f_sites;
        f_sites.reserve(s_span.size());
        for (const auto &s : s_span) {
            f_sites.emplace_back(static_cast<float>(s.x), static_cast<float>(s.y));
        }
        geometry_dod_->stage_voronoi_sites(f_sites);
    }

    // 3. Bake SDFGrid2DResource array into GEOM_SDF_GRID_HEADER_POD & GEOM_SDF_GRID_FLOAT32
    for (int64_t i = 0; i < sdf_grids_.size(); ++i) {
        godot::Ref<SDFGrid2DResource> sdf = sdf_grids_[i];
        if (sdf.is_null()) continue;
        const godot::Vector2i res   = sdf->get_resolution();
        const godot::Rect2 bounds   = sdf->get_world_bounds();
        const glm::vec2 origin(bounds.position.x, bounds.position.y);
        const glm::vec2 cell_sz(
            bounds.size.x / static_cast<float>(std::max(1, res.x - 1)),
            bounds.size.y / static_cast<float>(std::max(1, res.y - 1))
        );
        const uint32_t slot = geometry_dod_->allocate_sdf_grid_2d(
            static_cast<uint32_t>(res.x),
            static_cast<uint32_t>(res.y),
            origin,
            cell_sz,
            sdf->get_grid_span(),
            0.0f
        );
        if (slot != core::INVALID_ID) {
            object_id_to_sdf_slot_[sdf->get_instance_id()] = slot;
            sdf_slot_to_resource_[slot]                    = sdf;
        }
    }

    geometry_dod_->sync_selection_ranges();
}

void DataOrientedGeometry::rebuild_dod_task_graph() {
    _ensure_geometry_dod_initialized();
    if (!active_task_graph_) return;

    if (dod_execution_mode_ == GEOMETRY_DOD_EXEC_DISABLED ||
        dod_execution_mode_ == GEOMETRY_DOD_EXEC_MANUAL_GRAPH) {
        return;
    }

    if (dod_execution_mode_ == GEOMETRY_DOD_EXEC_CUSTOM_GRAPH_RESOURCE &&
        custom_task_graph_resource_.is_valid()) {
        ui_to_dod_map_.clear();
        custom_task_graph_resource_->compile_to_task_graph(*active_task_graph_, ui_to_dod_map_);
        return;
    }

    geometry_dod_->inject_pipeline_stages(
        *active_task_graph_,
        pipeline_stage_mask_,
        auto_connect_pipeline_stages_
    );
}

void DataOrientedGeometry::execute_dod_step(double p_delta) {
    if (dod_execution_mode_ == GEOMETRY_DOD_EXEC_DISABLED || !geometry_dod_) return;
    geometry_dod_->execute_step(p_delta, active_task_graph_.get());
    if (auto_sync_resources_from_dod_) {
        sync_oop_resources_from_dod();
    }
}

void DataOrientedGeometry::sync_oop_resources_from_dod() {
    if (!geometry_dod_) return;

    // Sync Voronoi sites if Lloyd relaxation updated them in DOD
    if (voronoi_diagram_.is_valid()) {
        const auto dod_sites = geometry_dod_->get_voronoi_sites();
        if (!dod_sites.empty() && static_cast<int>(dod_sites.size()) == voronoi_diagram_->get_site_count()) {
            godot::PackedVector2Array updated;
            updated.resize(static_cast<int64_t>(dod_sites.size()));
            for (size_t i = 0; i < dod_sites.size(); ++i) {
                updated.set(static_cast<int64_t>(i), godot::Vector2(dod_sites[i].x, dod_sites[i].y));
            }
            voronoi_diagram_->set_sites(updated);
        }
    }

    // Sync SDFGrid2DResource cells if updated by SDFFieldAndContourTransformLogic
    for (const auto &kv : sdf_slot_to_resource_) {
        const uint32_t slot                  = kv.first;
        godot::Ref<SDFGrid2DResource> sdf    = kv.second;
        const std::span<const float> dod_buf = geometry_dod_->get_sdf_grid_cells(slot);
        if (sdf.is_valid() && !dod_buf.empty()) {
            std::span<float> dst = sdf->get_grid_span_mut();
            if (dst.size() == dod_buf.size()) {
                std::copy(dod_buf.begin(), dod_buf.end(), dst.begin());
            }
        }
    }
}

int DataOrientedGeometry::get_buffer_id(const godot::StringName &p_semantic_name) const {
    if (!geometry_dod_) return -1;
    const godot::String s(p_semantic_name);
    const uint32_t id = geometry_dod_->get_buffer_id_by_name(s.utf8().get_data());
    return (id != core::INVALID_ID) ? static_cast<int>(id) : -1;
}

godot::Dictionary DataOrientedGeometry::get_buffer_manifest_dictionary() const {
    godot::Dictionary d;
    if (geometry_dod_) {
        geometry_dod_->populate_task_buffer_dictionary(d, true);
    }
    return d;
}

int DataOrientedGeometry::get_polygon_dod_slot(const godot::Ref<Polygon2DResource> &p_polygon) const {
    if (p_polygon.is_null()) return -1;
    auto it = object_id_to_polyline_slot_.find(p_polygon->get_instance_id());
    return (it != object_id_to_polyline_slot_.end()) ? static_cast<int>(it->second) : -1;
}

godot::PackedVector2Array DataOrientedGeometry::get_extracted_marching_squares_segments() const {
    godot::PackedVector2Array out;
    if (!geometry_dod_) return out;
    const auto segs = geometry_dod_->get_extracted_marching_squares_segments();
    out.resize(static_cast<int64_t>(segs.size() * 2u));
    for (size_t i = 0; i < segs.size(); ++i) {
        out.set(static_cast<int64_t>(i * 2u), godot::Vector2(segs[i].start.x, segs[i].start.y));
        out.set(static_cast<int64_t>(i * 2u + 1u), godot::Vector2(segs[i].end.x, segs[i].end.y));
    }
    return out;
}

int DataOrientedGeometry::add_geometry_transform_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_geometry_dod_initialized();
    if (!active_task_graph_) return -1;
    core::GeometryNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = geometry_dod_->add_transform_node_by_id(
        *active_task_graph_,
        static_cast<core::GeometryTransformLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedGeometry::add_geometry_query_node(
    int p_logic_id,
    int p_query_op,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_geometry_dod_initialized();
    if (!active_task_graph_) return -1;
    core::GeometryNodeExecutionConfig cfg{};
    cfg.query_op             = (p_query_op == 1) ? core::QueryOp::ADD : core::QueryOp::CULL;
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = geometry_dod_->add_query_node_by_id(
        *active_task_graph_,
        static_cast<core::GeometryQueryLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedGeometry::add_geometry_metadata_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_geometry_dod_initialized();
    if (!active_task_graph_) return -1;
    core::GeometryNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = geometry_dod_->add_metadata_node_by_id(
        *active_task_graph_,
        static_cast<core::GeometryMetadataLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

bool DataOrientedGeometry::schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream) {
    if (!geometry_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::GeometryDOD::schedule_node_with_stage(
        *active_task_graph_,
        geometry_dod_->get_active_topology(),
        static_cast<core::GeometryPipelineStageFlags>(p_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id),
        p_join_downstream
    );
}

bool DataOrientedGeometry::schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id) {
    if (!geometry_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::GeometryDOD::schedule_node_between_stages(
        *active_task_graph_,
        geometry_dod_->get_active_topology(),
        static_cast<core::GeometryPipelineStageFlags>(p_upstream_stage_flag),
        static_cast<core::GeometryPipelineStageFlags>(p_downstream_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id)
    );
}

} // namespace ideam::godot_ext

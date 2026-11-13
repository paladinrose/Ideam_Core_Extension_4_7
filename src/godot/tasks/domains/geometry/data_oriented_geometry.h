#pragma once

/**
 * data_oriented_geometry.h
 * Godot-facing Resource bridge class in `src/godot/tasks/domains/geometry/`.
 *
 * Extends `ideam::godot_ext::GeometryResource` to unite the Object-Oriented authoring
 * Resources in `src/godot/geometry/` (`Polygon2DResource`, `VoronoiDiagram2DResource`,
 * `SDFGrid2DResource`) with the pure C++26 Data-Oriented Design (DOD) memory tables and
 * Kahn-sorted `TaskGraphDOD` pipeline in `src/core/tasks/domains/geometry/`.
 */

#include "../../../../core/tasks/domains/geometry/geometry_dod.h"
#include "../../../geometry/geometry_resource.h"
#include "../../../geometry/resources/polygon_2d_resource.h"
#include "../../../geometry/resources/sdf_grid_2d_resource.h"
#include "../../../geometry/resources/voronoi_diagram_2d_resource.h"
#include "../../task_graph_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace ideam::godot_ext {

enum GeometryDODExecutionMode : int32_t {
    GEOMETRY_DOD_EXEC_DISABLED              = 0, // Pure OOP Resource evaluation fallback
    GEOMETRY_DOD_EXEC_COMPOSABLE_STAGES     = 1, // Bitmask-configurable stage pipeline + custom wave co-scheduling
    GEOMETRY_DOD_EXEC_CUSTOM_GRAPH_RESOURCE = 2, // User-authored TaskGraphResource bound to GeometryDOD buffers
    GEOMETRY_DOD_EXEC_MANUAL_GRAPH          = 3  // Buffers allocated; graph left open for manual node construction
};

class DataOrientedGeometry : public GeometryResource {
    GDCLASS(DataOrientedGeometry, GeometryResource)

protected:
    static void _bind_methods();

private:
    std::unique_ptr<core::GeometryDOD> geometry_dod_;
    std::shared_ptr<core::TaskGraphDOD> active_task_graph_;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map_;

    // --- Managed Geometry Resources ---
    godot::TypedArray<Polygon2DResource> polygons_;
    godot::Ref<VoronoiDiagram2DResource> voronoi_diagram_;
    godot::TypedArray<SDFGrid2DResource> sdf_grids_;
    TypedArray<TensorField2DResource> tensor_fields_;

    // --- Inspector Configuration ---
    GeometryDODExecutionMode dod_execution_mode_ = GEOMETRY_DOD_EXEC_COMPOSABLE_STAGES;
    uint32_t pipeline_stage_mask_                = core::GEOMETRY_STAGE_DEFAULT_PIPELINE_MASK;
    bool auto_connect_pipeline_stages_           = true;
    bool auto_sync_resources_from_dod_           = true;

    godot::Ref<TaskGraphResource> custom_task_graph_resource_;

    // Configurable Buffer Capacities
    int max_dod_polylines_           = 1024;
    int max_dod_polyline_vertices_   = 32768;
    int max_dod_polygon_meshes_      = 1024;
    int max_dod_voronoi_sites_       = 2048;
    int max_dod_sdf_grids_           = 16;
    int max_dod_sdf_cells_           = 65536;

    // --- Bidirectional ObjectID <-> DOD Slot Registries ---
    std::unordered_map<uint64_t, uint32_t> object_id_to_polyline_slot_;
    std::unordered_map<uint32_t, godot::Ref<Polygon2DResource>> polyline_slot_to_resource_;

    std::unordered_map<uint64_t, uint32_t> object_id_to_sdf_slot_;
    std::unordered_map<uint32_t, godot::Ref<SDFGrid2DResource>> sdf_slot_to_resource_;

    core::GeometryBufferCapacities _build_capacities() const noexcept;
    void _ensure_geometry_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager = nullptr);
    void _clear_resource_maps() noexcept;

public:
    DataOrientedGeometry() = default;
    ~DataOrientedGeometry() override = default;

    // --- Direct C++ Accessors ---
    [[nodiscard]] core::GeometryDOD *get_geometry_dod() const noexcept { return geometry_dod_.get(); }
    [[nodiscard]] std::shared_ptr<core::TaskGraphDOD> get_active_task_graph_shared() const noexcept { return active_task_graph_; }
    [[nodiscard]] core::TaskGraphDOD *get_active_task_graph_dod() const noexcept { return active_task_graph_.get(); }
    [[nodiscard]] std::shared_ptr<core::MemoryManagerDOD> get_memory_manager_shared() const noexcept {
        return geometry_dod_ ? geometry_dod_->get_memory_manager() : nullptr;
    }
    [[nodiscard]] core::MemoryManagerDOD *get_memory_manager_dod() const noexcept {
        return geometry_dod_ ? geometry_dod_->get_memory_manager_raw() : nullptr;
    }

    // --- Resource Collection Setters / Getters ---
    void set_polygons(const godot::TypedArray<Polygon2DResource> &p_polygons);
    [[nodiscard]] godot::TypedArray<Polygon2DResource> get_polygons() const { return polygons_; }

    void set_voronoi_diagram(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi);
    [[nodiscard]] godot::Ref<VoronoiDiagram2DResource> get_voronoi_diagram() const { return voronoi_diagram_; }

    void set_sdf_grids(const godot::TypedArray<SDFGrid2DResource> &p_grids);
    [[nodiscard]] godot::TypedArray<SDFGrid2DResource> get_sdf_grids() const { return sdf_grids_; }

    // --- Inspector Execution Setters / Getters ---
    void set_dod_execution_mode(GeometryDODExecutionMode p_mode);
    [[nodiscard]] GeometryDODExecutionMode get_dod_execution_mode() const noexcept { return dod_execution_mode_; }

    void set_pipeline_stage_mask(int p_mask);
    [[nodiscard]] int get_pipeline_stage_mask() const noexcept { return static_cast<int>(pipeline_stage_mask_); }

    void set_auto_connect_pipeline_stages(bool p_auto_connect);
    [[nodiscard]] bool get_auto_connect_pipeline_stages() const noexcept { return auto_connect_pipeline_stages_; }

    void set_auto_sync_resources_from_dod(bool p_auto_sync);
    [[nodiscard]] bool get_auto_sync_resources_from_dod() const noexcept { return auto_sync_resources_from_dod_; }

    void set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_custom_task_graph_resource() const { return custom_task_graph_resource_; }

    void set_max_dod_polylines(int p_max);
    [[nodiscard]] int get_max_dod_polylines() const noexcept { return max_dod_polylines_; }

    void set_max_dod_polyline_vertices(int p_max);
    [[nodiscard]] int get_max_dod_polyline_vertices() const noexcept { return max_dod_polyline_vertices_; }

    void set_max_dod_voronoi_sites(int p_max);
    [[nodiscard]] int get_max_dod_voronoi_sites() const noexcept { return max_dod_voronoi_sites_; }

    void set_max_dod_sdf_cells(int p_max);
    [[nodiscard]] int get_max_dod_sdf_cells() const noexcept { return max_dod_sdf_cells_; }

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

    // --- Buffer & Slot Lookup API ---
    [[nodiscard]] int get_buffer_id(const godot::StringName &p_semantic_name) const;
    [[nodiscard]] godot::Dictionary get_buffer_manifest_dictionary() const;
    [[nodiscard]] int get_polygon_dod_slot(const godot::Ref<Polygon2DResource> &p_polygon) const;
    [[nodiscard]] godot::PackedVector2Array get_extracted_marching_squares_segments() const;

    // --- Dynamic Task Node & Wave Co-Scheduling API ---
    int add_geometry_transform_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_geometry_query_node(
        int p_logic_id,
        int p_query_op = 0,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_geometry_metadata_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );

    bool schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream = false);
    bool schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::GeometryDODExecutionMode);

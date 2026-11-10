#pragma once

/**
 * data_oriented_tiling.h
 * Godot-facing Resource bridge class in `src/godot/tasks/domains/tiles/`.
 *
 * Extends `ideam::godot_ext::GeometryResource` to unite `TilingResource` objects
 * (`WallpaperTilingResource` and `CompositeTilingResource`) with `ideam::core::TileDOD`,
 * `ideam::core::GeometryDOD` (`DataOrientedGeometry`), and `ideam::core::SplineDOD`
 * (`DataOrientedSplines`).
 *
 * Key Responsibilities:
 *   1. Bakes any collection of `WallpaperTilingResource` prototiles and `CompositeTilingResource`
 *      bounded regions into `TileDOD`'s contiguous tables (`BakedPrototilePOD<float>`,
 *      `IsoCellPOD`, `TileTransformPOD`, `TileEdgePOD`, `TileEdgeMetadataPOD`,
 *      `HexSuperCellPOD`, `BridgeQuadCellPOD`, and CSR vertex/edge/neighbor slices).
 *   2. Automatically bridges to `DataOrientedGeometry` (`bind_data_oriented_geometry`) and
 *      `DataOrientedSplines` (`bind_data_oriented_splines`) via `TileDOD::bind_external_geometry_buffers()`,
 *      sharing the same `MemoryManagerDOD` and `TaskGraphDOD` so Wave 1 `SplineFlattenToPolyline`
 *      and Wave 2/3 `TilingLatticeCullAndMask` + `TilingBoundaryClipAndSeamStitch` read and
 *      write the exact same `GeometryDOD` polyline and polygon mesh buffers with zero copying.
 */

#include "../../../../core/tasks/domains/tiles/tile_dod.h"
#include "../../../geometry/geometry_resource.h"
#include "../../../tiles/composite_tiling_resource.h"
#include "../../../tiles/tiling_resource.h"
#include "../../../tiles/wallpaper_tiling_resource.h"
#include "../geometry/data_oriented_geometry.h"
#include "../splines/data_oriented_splines.h"
#include "../../task_graph_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace ideam::godot_ext {

enum TilesDODExecutionMode : int32_t {
    TILES_DOD_EXEC_DISABLED              = 0,
    TILES_DOD_EXEC_COMPOSABLE_STAGES     = 1,
    TILES_DOD_EXEC_CUSTOM_GRAPH_RESOURCE = 2,
    TILES_DOD_EXEC_MANUAL_GRAPH          = 3
};

class DataOrientedTiling : public GeometryResource {
    GDCLASS(DataOrientedTiling, GeometryResource)

protected:
    static void _bind_methods();

private:
    std::unique_ptr<core::TileDOD> tile_dod_;
    std::shared_ptr<core::TaskGraphDOD> active_task_graph_;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map_;

    // --- Managed Tiling Resources & Optional Linked Geometry / Splines DOD Resources ---
    godot::TypedArray<TilingResource> tilings_;
    godot::Ref<DataOrientedGeometry> linked_geometry_resource_;
    godot::Ref<DataOrientedSplines> linked_splines_resource_;

    // --- Inspector Configuration ---
    TilesDODExecutionMode dod_execution_mode_ = TILES_DOD_EXEC_COMPOSABLE_STAGES;
    uint32_t pipeline_stage_mask_             = core::TILES_STAGE_DEFAULT_PIPELINE_MASK;
    bool auto_connect_pipeline_stages_        = true;
    int default_edge_subdivisions_            = 12;

    godot::Ref<TaskGraphResource> custom_task_graph_resource_;

    // Configurable Buffer Capacities
    int max_dod_prototiles_      = 64;
    int max_dod_cells_           = 16384;
    int max_dod_edges_           = 49152;
    int max_dod_hex_super_cells_ = 4096;
    int max_dod_nav_requests_    = 512;

    // --- Bidirectional ObjectID <-> DOD Prototile Slot Registries ---
    std::unordered_map<uint64_t, uint32_t> object_id_to_prototile_slot_;
    std::unordered_map<uint32_t, godot::Ref<TilingResource>> prototile_slot_to_resource_;

    core::TilesBufferCapacities _build_capacities() const noexcept;
    void _ensure_tile_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager = nullptr);
    void _clear_resource_maps() noexcept;

public:
    DataOrientedTiling() = default;
    ~DataOrientedTiling() override = default;

    // --- Direct C++ Accessors ---
    [[nodiscard]] core::TileDOD *get_tile_dod() const noexcept { return tile_dod_.get(); }
    [[nodiscard]] std::shared_ptr<core::TaskGraphDOD> get_active_task_graph_shared() const noexcept { return active_task_graph_; }
    [[nodiscard]] core::TaskGraphDOD *get_active_task_graph_dod() const noexcept { return active_task_graph_.get(); }
    [[nodiscard]] std::shared_ptr<core::MemoryManagerDOD> get_memory_manager_shared() const noexcept {
        return tile_dod_ ? tile_dod_->get_memory_manager() : nullptr;
    }
    [[nodiscard]] core::MemoryManagerDOD *get_memory_manager_dod() const noexcept {
        return tile_dod_ ? tile_dod_->get_memory_manager_raw() : nullptr;
    }

    // --- Resource Collection & Cross-Domain Bridging ---
    void set_tilings(const godot::TypedArray<TilingResource> &p_tilings);
    [[nodiscard]] godot::TypedArray<TilingResource> get_tilings() const { return tilings_; }

    void set_linked_geometry_resource(const godot::Ref<DataOrientedGeometry> &p_geometry);
    [[nodiscard]] godot::Ref<DataOrientedGeometry> get_linked_geometry_resource() const { return linked_geometry_resource_; }
    bool bind_data_oriented_geometry(const godot::Ref<DataOrientedGeometry> &p_geometry);

    void set_linked_splines_resource(const godot::Ref<DataOrientedSplines> &p_splines);
    [[nodiscard]] godot::Ref<DataOrientedSplines> get_linked_splines_resource() const { return linked_splines_resource_; }
    bool bind_data_oriented_splines(const godot::Ref<DataOrientedSplines> &p_splines);

    // --- Inspector Configuration Setters / Getters ---
    void set_dod_execution_mode(TilesDODExecutionMode p_mode);
    [[nodiscard]] TilesDODExecutionMode get_dod_execution_mode() const noexcept { return dod_execution_mode_; }

    void set_pipeline_stage_mask(int p_mask);
    [[nodiscard]] int get_pipeline_stage_mask() const noexcept { return static_cast<int>(pipeline_stage_mask_); }

    void set_auto_connect_pipeline_stages(bool p_auto_connect);
    [[nodiscard]] bool get_auto_connect_pipeline_stages() const noexcept { return auto_connect_pipeline_stages_; }

    void set_default_edge_subdivisions(int p_subdivs);
    [[nodiscard]] int get_default_edge_subdivisions() const noexcept { return default_edge_subdivisions_; }

    void set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_custom_task_graph_resource() const { return custom_task_graph_resource_; }

    void set_max_dod_prototiles(int p_max);
    [[nodiscard]] int get_max_dod_prototiles() const noexcept { return max_dod_prototiles_; }

    void set_max_dod_cells(int p_max);
    [[nodiscard]] int get_max_dod_cells() const noexcept { return max_dod_cells_; }

    void set_max_dod_edges(int p_max);
    [[nodiscard]] int get_max_dod_edges() const noexcept { return max_dod_edges_; }

    void set_max_dod_hex_super_cells(int p_max);
    [[nodiscard]] int get_max_dod_hex_super_cells() const noexcept { return max_dod_hex_super_cells_; }

    void set_max_dod_nav_requests(int p_max);
    [[nodiscard]] int get_max_dod_nav_requests() const noexcept { return max_dod_nav_requests_; }

    // --- Pipeline Setup, Baking & Execution API ---
    void setup_dod_pipeline_with_manager(
        std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
        std::shared_ptr<core::TaskGraphDOD> p_shared_graph = nullptr
    );
    void setup_dod_pipeline();
    void bake_to_dod();
    void rebuild_dod_task_graph();
    void execute_dod_step(double p_delta);

    // --- Batch Navigation Staging & Instance Transform / Flow-Field Readback ---
    void stage_navigation_requests(
        const godot::PackedInt32Array &p_start_cells,
        const godot::PackedInt32Array &p_goal_cells,
        bool p_manhattan_heuristic = false,
        bool p_emit_flow_goals = true
    );
    [[nodiscard]] godot::PackedInt32Array get_navigation_path_cells(int p_request_index) const;
    [[nodiscard]] godot::PackedFloat32Array get_flow_field_costs() const;
    [[nodiscard]] godot::PackedVector2Array get_flow_field_vectors() const;

    [[nodiscard]] int get_active_cell_count() const;
    [[nodiscard]] int get_active_edge_count() const;
    [[nodiscard]] int get_active_hex_super_cell_count() const;
    [[nodiscard]] godot::TypedArray<godot::Transform2D> get_cell_transforms_2d() const;
    [[nodiscard]] godot::PackedVector2Array get_cell_centers_2d() const;
    [[nodiscard]] godot::PackedVector2Array get_hex_super_cell_centers_2d() const;

    [[nodiscard]] int get_tiling_dod_slot(const godot::Ref<TilingResource> &p_tiling) const;
    [[nodiscard]] int get_buffer_id(const godot::StringName &p_semantic_name) const;
    [[nodiscard]] godot::Dictionary get_buffer_manifest_dictionary() const;

    // --- Dynamic Task Node & Wave Co-Scheduling API ---
    int add_tiles_transform_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_tiles_query_node(
        int p_logic_id,
        int p_query_op = 0,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_tiles_metadata_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );

    bool schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream = false);
    bool schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TilesDODExecutionMode);

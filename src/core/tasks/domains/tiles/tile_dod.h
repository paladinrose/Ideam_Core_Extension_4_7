#pragma once

/**
 * tile_dod.h
 * Lean, table-driven C++26 Data-Oriented Design (DOD) coordinator for the IdeamTiles
 * domain (`src/core/tasks/domains/tiles/`).
 *
 * Responsibilities:
 *   1. Loop-driven allocation, selection-range synchronization, semantic name lookup, and
 *      TaskGraphResource dictionary population across `TILE_BUFFER_SPECS`.
 *   2. Baking of `IsohedralTiling2D<T>` prototiles (`BakedPrototilePOD<float>`) and
 *      `TilingRegion2D<T>` bounded multi-layer regions (`IsoCellPOD`, `TileTransformPOD`,
 *      `TileEdgePOD`, `TileEdgeMetadataPOD`, `HexSuperCellPOD`, `BridgeQuadCellPOD`, and
 *      CSR topology slices) into contiguous `MemoryManagerDOD` buffers.
 *   3. Zero-copy cross-domain bridging to `GeometryDOD` via `bind_external_geometry_buffers()`
 *      (and by extension `SplineDOD`), allowing `TilingLatticeCullAndMaskQueryLogic` and
 *      `TilingBoundaryClipAndSeamStitchTransformLogic` to read flattened `Spline2D` boundary
 *      contours directly from `GeometryDOD`'s `GEOM_POLYLINE_SLICE_POD` and write deduplicated
 *      tile polygon meshes directly into `GeometryDOD`'s `GEOM_POLYGON_VERTEX_2D` and
 *      `GEOM_POLYGON_INDEX_U32` buffers!
 */

#include "tiles_dod_common.h"
#include "../geometry/geometry_dod.h"
#include "../splines/spline_dod.h"
#include "../../task_graph_dod.h"
#include "../../transform_task.h"
#include "../../query_task.h"
#include "../../metadata_task.h"

#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

class TileDOD {
private:
    std::shared_ptr<MemoryManagerDOD> memory_manager_;
    std::shared_ptr<TaskGraphDOD> default_task_graph_;

    TilesBufferCapacities capacities_{};
    TilesBufferManifest manifest_{};
    TilesPipelineTopology active_topology_{};

    // --- O(1) Slot Allocators & Contiguous Bump Cursors ---
    SlotIndexAllocator prototile_allocator_;
    uint32_t cell_cursor_                     = 0u;
    uint32_t cell_vertex_csr_cursor_          = 0u;
    uint32_t cell_edge_csr_cursor_            = 0u;
    uint32_t edge_cursor_                     = 0u;
    uint32_t bridge_quad_cursor_              = 0u;
    uint32_t hex_super_cell_cursor_           = 0u;
    uint32_t hex_child_csr_cursor_            = 0u;
    uint32_t nav_request_cursor_              = 0u;
    uint32_t nav_path_cursor_                 = 0u;
    uint32_t bridged_polyline_cursor_         = 0u;
    uint32_t bridged_polyline_vertex_cursor_  = 0u;
    uint32_t bridged_polygon_mesh_cursor_     = 0u;
    uint32_t bridged_polygon_vertex_cursor_   = 0u;
    uint32_t bridged_polygon_index_cursor_    = 0u;
    bool has_external_geometry_buffers_       = false;

    uint32_t _active_count_for_group(TileTableGroup p_group) const noexcept;
    void _bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept;

    template <FloatingPointScalar T>
    static BakedPrototilePOD<float> _convert_prototile_to_float(const BakedPrototilePOD<T> &p_src) noexcept;

public:
    explicit TileDOD(
        std::shared_ptr<MemoryManagerDOD> p_memory_manager = nullptr,
        const TilesBufferCapacities &p_capacities = TilesBufferCapacities{}
    );
    ~TileDOD() = default;

    TileDOD(const TileDOD &) = delete;
    TileDOD &operator=(const TileDOD &) = delete;
    TileDOD(TileDOD &&) noexcept = default;
    TileDOD &operator=(TileDOD &&) noexcept = default;

    // --- Core Manager & Table-Driven Manifest Lifecycle ---
    void initialize_buffers(const TilesBufferCapacities &p_capacities = TilesBufferCapacities{});
    void sync_selection_ranges() noexcept;
    void reset_all_slots() noexcept;

    [[nodiscard]] std::shared_ptr<MemoryManagerDOD> get_memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] MemoryManagerDOD *get_memory_manager_raw() const noexcept { return memory_manager_.get(); }

    [[nodiscard]] std::shared_ptr<TaskGraphDOD> get_default_task_graph() const noexcept { return default_task_graph_; }
    void set_default_task_graph(std::shared_ptr<TaskGraphDOD> p_graph) noexcept { default_task_graph_ = std::move(p_graph); }

    [[nodiscard]] const TilesBufferCapacities &get_capacities() const noexcept { return capacities_; }
    [[nodiscard]] const TilesBufferManifest &get_buffer_manifest() const noexcept { return manifest_; }
    [[nodiscard]] const TilesPipelineTopology &get_active_topology() const noexcept { return active_topology_; }

    [[nodiscard]] constexpr uint32_t get_buffer_id(TileBufferSlot p_slot) const noexcept { return manifest_.get(p_slot); }
    [[nodiscard]] constexpr uint32_t get_buffer_id_by_name(std::string_view p_name) const noexcept { return manifest_.get_by_name(p_name); }

    void populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing = false) const;
    [[nodiscard]] GrantPartPOD make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept;
    [[nodiscard]] GrantPartPOD make_slot_requirement(TileBufferSlot p_slot, BufferAccessMode p_access) const noexcept {
        return make_buffer_requirement(manifest_.get(p_slot), p_access);
    }

    // --- Direct Typed Column Accessors ---
    template <typename T>
    [[nodiscard]] T *get_column_ptr(TileBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return nullptr;
        MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr) return nullptr;
        return reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset);
    }

    template <typename T>
    [[nodiscard]] std::span<T> get_column_span(TileBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return {};
        MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr || buf->max_elements <= 0) return {};
        return std::span<T>(
            reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset),
            static_cast<size_t>(buf->max_elements)
        );
    }

    // ========================================================================
    // Cross-Domain GeometryDOD & SplineDOD Buffer Aliasing (`bind_external_geometry_buffers`)
    // ========================================================================

    void bind_external_geometry_buffers(
        uint32_t p_polyline_slice_buf_id,
        uint32_t p_polyline_vertex_2d_buf_id,
        uint32_t p_polygon_mesh_slice_buf_id = INVALID_ID,
        uint32_t p_polygon_vertex_2d_buf_id = INVALID_ID,
        uint32_t p_polygon_index_u32_buf_id = INVALID_ID,
        uint32_t p_clip_classification_buf_id = INVALID_ID
    ) noexcept;

    void bind_external_geometry_buffers(const GeometryBufferManifest &p_geom_manifest) noexcept {
        bind_external_geometry_buffers(
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYLINE_SLICE_POD),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_2D),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYGON_MESH_SLICE_POD),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYGON_VERTEX_2D),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYGON_INDEX_U32),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_CLIP_CLASSIFICATION_FLAGS)
        );
    }

    void bind_external_splines_buffers(const SplinesBufferManifest &p_splines_manifest) noexcept {
        bind_external_geometry_buffers(
            p_splines_manifest.get(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD),
            p_splines_manifest.get(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_2D)
        );
    }

    [[nodiscard]] bool has_external_geometry_buffers() const noexcept { return has_external_geometry_buffers_; }

    // ========================================================================
    // Templated Prototile & Region Baking (`bake_prototile`, `bake_region`)
    // ========================================================================

    template <FloatingPointScalar T>
    uint32_t bake_prototile(const IsohedralTiling2D<T> &p_tiling);

    template <FloatingPointScalar T>
    uint32_t bake_region(
        const TilingRegion2D<T> &p_region,
        GeometryDOD *r_optional_linked_geometry_dod = nullptr
    );

    [[nodiscard]] uint32_t get_active_prototile_count() const noexcept { return prototile_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_active_cell_count() const noexcept { return cell_cursor_; }
    [[nodiscard]] uint32_t get_active_edge_count() const noexcept { return edge_cursor_; }
    [[nodiscard]] uint32_t get_active_hex_super_cell_count() const noexcept { return hex_super_cell_cursor_; }
    [[nodiscard]] uint32_t get_active_bridge_quad_count() const noexcept { return bridge_quad_cursor_; }

    [[nodiscard]] std::span<const IsoCellPOD> get_active_cells() const noexcept;
    [[nodiscard]] std::span<const TileTransformPOD> get_active_instance_transforms() const noexcept;
    [[nodiscard]] std::span<const HexSuperCellPOD> get_active_hex_super_cells() const noexcept;

    // --- Batch Navigation Request Staging & Flow-Field Readback ---
    void stage_navigation_requests(std::span<const TileNavRequestPOD> p_requests);
    [[nodiscard]] std::span<const TileNavRequestPOD> get_navigation_requests() const noexcept;
    [[nodiscard]] std::span<const uint32_t> get_navigation_path_cells(uint32_t p_request_index) const noexcept;
    [[nodiscard]] std::span<const float> get_flow_field_costs() const noexcept;
    [[nodiscard]] std::span<const glm::vec2> get_flow_field_vectors() const noexcept;

    // ========================================================================
    // TEMPLATED TASK NODE BUILDER & PIPELINE STAGE INJECTION
    // ========================================================================

    template <typename T_Logic>
    NodeID add_tile_task_node(
        TaskGraphDOD &r_graph,
        const T_Logic &p_logic_override = T_Logic{},
        const TilesNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_generation_partition_metadata_node(TaskGraphDOD &g, const TilingGenerationAndHexPartitionMetadataLogic<> &l = {}, const TilesNodeExecutionConfig &c = {}) const {
        return add_tile_task_node(g, l, c);
    }
    NodeID add_lattice_cull_and_mask_query_node(TaskGraphDOD &g, const TilingLatticeCullAndMaskQueryLogic<> &l = {}, const TilesNodeExecutionConfig &c = {}) const {
        return add_tile_task_node(g, l, c);
    }
    NodeID add_instance_transform_node(TaskGraphDOD &g, const TilingInstanceTransformLogic<> &l = {}, const TilesNodeExecutionConfig &c = {}) const {
        return add_tile_task_node(g, l, c);
    }
    NodeID add_boundary_clip_and_seam_stitch_node(TaskGraphDOD &g, const TilingBoundaryClipAndSeamStitchTransformLogic<> &l = {}, const TilesNodeExecutionConfig &c = {}) const {
        return add_tile_task_node(g, l, c);
    }
    NodeID add_hexize_and_border_extract_node(TaskGraphDOD &g, const TilingHexizeAndBorderExtractTransformLogic<> &l = {}, const TilesNodeExecutionConfig &c = {}) const {
        return add_tile_task_node(g, l, c);
    }
    NodeID add_quotient_astar_and_flow_field_node(TaskGraphDOD &g, const TilingQuotientAStarAndFlowFieldTransformLogic<> &l = {}, const TilesNodeExecutionConfig &c = {}) const {
        return add_tile_task_node(g, l, c);
    }

    NodeID add_transform_node_by_id(
        TaskGraphDOD &r_graph,
        TilesTransformLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const TilesNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_query_node_by_id(
        TaskGraphDOD &r_graph,
        TilesQueryLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const TilesNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_metadata_node_by_id(
        TaskGraphDOD &r_graph,
        TilesMetadataLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const TilesNodeExecutionConfig &p_config = {}
    ) const;

    TilesPipelineTopology inject_pipeline_stages(
        TaskGraphDOD &r_graph,
        uint32_t p_stage_mask = TILES_STAGE_DEFAULT_PIPELINE_MASK,
        bool p_auto_connect_stages = true
    );

    static bool schedule_node_with_stage(
        TaskGraphDOD &r_graph,
        const TilesPipelineTopology &p_topology,
        TilesPipelineStageFlags p_target_stage,
        NodeID p_custom_node,
        bool p_join_downstream = false
    );

    static bool schedule_node_between_stages(
        TaskGraphDOD &r_graph,
        const TilesPipelineTopology &p_topology,
        TilesPipelineStageFlags p_upstream_stage,
        TilesPipelineStageFlags p_downstream_stage,
        NodeID p_custom_node
    );

    void execute_step(double p_delta, TaskGraphDOD *p_override_graph = nullptr);
};

} // namespace ideam::core

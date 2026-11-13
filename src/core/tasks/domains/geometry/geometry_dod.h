#pragma once

/**
 * geometry_dod.h
 * Lean, table-driven C++26 Data-Oriented Design (DOD) coordinator for the IdeamGeometry
 * substrate (`src/core/tasks/domains/geometry/`).
 *
 * Responsibilities:
 *   1. Loop-driven allocation, selection-range synchronization, semantic name lookup, and
 *      TaskGraphResource dictionary population across the 28 GeometryDOD buffer slots via
 *      `GEOMETRY_BUFFER_SPECS`.
 *   2. O(1) slot allocation and zero-copy contiguous staging for 2D/3D Polylines, 3D Bishop
 *      Rotation-Minimizing Frames, Triangulated Polygon Meshes, Planar Half-Edge Graphs,
 *      Voronoi Generator Sites & CSR Dual Adjacency, Spatial Hash Entries, 2D SDF Grids,
 *      and 2D Symmetric Tensor Fields (`TensorGridHeaderPod`, `TensorBasisElementPod`,
 *      `SymmetricTensor2DPod`, `TensorSingularityPod`, `TensorStreamlineSeedPod`).
 *   3. Single templated `add_geometry_task_node<T_Logic>()` builder + bitmask-driven
 *      `inject_pipeline_stages()` for all Geometry domain tasks, plus cross-domain buffer
 *      sharing with `SplineDOD`, `TileDOD`, and `NarrativeDOD`.
 */

#include "geometry_dod_common.h"
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

class GeometryDOD {
private:
    std::shared_ptr<MemoryManagerDOD> memory_manager_;
    std::shared_ptr<TaskGraphDOD> default_task_graph_;

    GeometryBufferCapacities capacities_{};
    GeometryBufferManifest manifest_{};
    GeometryPipelineTopology active_topology_{};

    // --- O(1) Slot Allocators per Primary Table ---
    SlotIndexAllocator polyline_allocator_;
    SlotIndexAllocator polygon_mesh_allocator_;
    SlotIndexAllocator voronoi_site_allocator_;
    SlotIndexAllocator sdf_grid_allocator_;
    SlotIndexAllocator tensor_grid_allocator_;

    // --- Contiguous Vertex / Index / CSR / Tensor Bump Cursors ---
    uint32_t polyline_vertex_2d_cursor_ = 0u;
    uint32_t polyline_vertex_3d_cursor_ = 0u;
    uint32_t curve_frame_3d_cursor_     = 0u;
    uint32_t polygon_vertex_2d_cursor_  = 0u;
    uint32_t polygon_index_cursor_      = 0u;
    uint32_t planar_half_edge_cursor_   = 0u;
    uint32_t voronoi_csr_edge_cursor_   = 0u;
    uint32_t spatial_hash_entry_cursor_ = 0u;
    uint32_t sdf_cell_cursor_           = 0u;
    uint32_t tensor_basis_cursor_       = 0u;
    uint32_t tensor_cell_cursor_        = 0u;
    uint32_t tensor_seed_cursor_        = 0u;

    uint32_t _active_count_for_group(GeometryTableGroup p_group) const noexcept;
    void _bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept;

public:
    explicit GeometryDOD(
        std::shared_ptr<MemoryManagerDOD> p_memory_manager = nullptr,
        const GeometryBufferCapacities &p_capacities = GeometryBufferCapacities{}
    );
    ~GeometryDOD() = default;

    GeometryDOD(const GeometryDOD &) = delete;
    GeometryDOD &operator=(const GeometryDOD &) = delete;
    GeometryDOD(GeometryDOD &&) noexcept = default;
    GeometryDOD &operator=(GeometryDOD &&) noexcept = default;

    // --- Core Manager & Table-Driven Manifest Lifecycle ---
    void initialize_buffers(const GeometryBufferCapacities &p_capacities = GeometryBufferCapacities{});
    void sync_selection_ranges() noexcept;
    void reset_all_slots() noexcept;

    [[nodiscard]] std::shared_ptr<MemoryManagerDOD> get_memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] MemoryManagerDOD *get_memory_manager_raw() const noexcept { return memory_manager_.get(); }

    [[nodiscard]] std::shared_ptr<TaskGraphDOD> get_default_task_graph() const noexcept { return default_task_graph_; }
    void set_default_task_graph(std::shared_ptr<TaskGraphDOD> p_graph) noexcept { default_task_graph_ = std::move(p_graph); }

    [[nodiscard]] const GeometryBufferCapacities &get_capacities() const noexcept { return capacities_; }
    [[nodiscard]] const GeometryBufferManifest &get_buffer_manifest() const noexcept { return manifest_; }
    [[nodiscard]] const GeometryPipelineTopology &get_active_topology() const noexcept { return active_topology_; }

    [[nodiscard]] constexpr uint32_t get_buffer_id(GeometryBufferSlot p_slot) const noexcept { return manifest_.get(p_slot); }
    [[nodiscard]] constexpr uint32_t get_buffer_id_by_name(std::string_view p_name) const noexcept { return manifest_.get_by_name(p_name); }

    void populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing = false) const;
    [[nodiscard]] GrantPartPOD make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept;
    [[nodiscard]] GrantPartPOD make_slot_requirement(GeometryBufferSlot p_slot, BufferAccessMode p_access) const noexcept {
        return make_buffer_requirement(manifest_.get(p_slot), p_access);
    }

    // --- Direct Typed Column Accessors ---
    template <typename T>
    [[nodiscard]] T *get_column_ptr(GeometryBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return nullptr;
        MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr) return nullptr;
        return reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset);
    }

    template <typename T>
    [[nodiscard]] std::span<T> get_column_span(GeometryBufferSlot p_slot) const noexcept {
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
    // Table 1: Polyline & 3D Bishop Frame Staging / Readback
    // ========================================================================

    uint32_t allocate_polyline_2d(
        std::span<const glm::vec2> p_vertices,
        bool p_closed = true
    );
    uint32_t allocate_polyline_3d(
        std::span<const glm::vec3> p_vertices,
        bool p_closed = false
    );
    [[nodiscard]] bool is_polyline_slot_valid(uint32_t p_slot) const noexcept { return polyline_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_polyline_count() const noexcept { return polyline_allocator_.get_active_count(); }

    [[nodiscard]] PolylineSlicePOD get_polyline_slice(uint32_t p_slot) const noexcept;
    [[nodiscard]] std::span<const glm::vec2> get_polyline_vertices_2d(uint32_t p_slot) const noexcept;
    [[nodiscard]] std::span<const glm::vec3> get_polyline_vertices_3d(uint32_t p_slot) const noexcept;

    uint32_t append_curve_frames_3d(std::span<const CurveFrame3DPOD<float>> p_frames);
    [[nodiscard]] std::span<const CurveFrame3DPOD<float>> get_curve_frames_3d(uint32_t p_offset, uint32_t p_count) const noexcept;

    // ========================================================================
    // Table 2: Polygon Mesh & CSG Classification Staging / Readback
    // ========================================================================

    uint32_t allocate_polygon_mesh_2d(
        std::span<const glm::vec2> p_vertices,
        std::span<const uint32_t> p_indices = {},
        uint32_t p_material_or_group_id = 0u
    );
    [[nodiscard]] bool is_polygon_mesh_slot_valid(uint32_t p_slot) const noexcept { return polygon_mesh_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_polygon_mesh_count() const noexcept { return polygon_mesh_allocator_.get_active_count(); }

    [[nodiscard]] PolygonMeshSlicePOD get_polygon_mesh_slice(uint32_t p_slot) const noexcept;
    [[nodiscard]] std::span<const glm::vec2> get_polygon_mesh_vertices_2d(uint32_t p_slot) const noexcept;
    [[nodiscard]] std::span<const uint32_t> get_polygon_mesh_indices(uint32_t p_slot) const noexcept;
    [[nodiscard]] ClipClassification2D get_clip_classification(uint32_t p_polyline_slot) const noexcept;

    // ========================================================================
    // Table 3: Voronoi Sites, Cell Polygons & CSR Dual Adjacency
    // ========================================================================

    void stage_voronoi_sites(std::span<const glm::vec2> p_sites);
    [[nodiscard]] uint32_t get_active_voronoi_site_count() const noexcept { return voronoi_site_allocator_.get_active_count(); }
    [[nodiscard]] std::span<const glm::vec2> get_voronoi_sites() const noexcept;
    [[nodiscard]] std::span<const glm::vec2> get_voronoi_cell_vertices(uint32_t p_site_slot) const noexcept;
    [[nodiscard]] std::span<const uint32_t> get_voronoi_csr_row_offsets() const noexcept;
    [[nodiscard]] std::span<const uint32_t> get_voronoi_csr_col_neighbors() const noexcept;

    // ========================================================================
    // Table 5: 2D SDF Grids & Marching Squares Contours
    // ========================================================================

    uint32_t allocate_sdf_grid_2d(
        uint32_t p_width,
        uint32_t p_height,
        const glm::vec2 &p_origin,
        const glm::vec2 &p_cell_size,
        std::span<const float> p_initial_values = {},
        float p_iso_value = 0.0f
    );
    [[nodiscard]] SDFGridHeaderPOD get_sdf_grid_header(uint32_t p_grid_slot) const noexcept;
    [[nodiscard]] std::span<const float> get_sdf_grid_cells(uint32_t p_grid_slot) const noexcept;
    [[nodiscard]] std::span<float> get_sdf_grid_cells_mut(uint32_t p_grid_slot) noexcept;
    [[nodiscard]] std::span<const MarchingSquaresSegment2D<float>> get_extracted_marching_squares_segments() const noexcept;

    // ========================================================================
    // Table 6: 2D Symmetric Tensor Fields, Eigenvectors & Hyperstreamline Seeds
    // ========================================================================

    uint32_t allocate_tensor_grid_2d(
        uint32_t p_width,
        uint32_t p_height,
        const glm::vec2 &p_origin,
        const glm::vec2 &p_cell_size,
        std::span<const TensorBasisElementPod> p_bases = {},
        uint32_t p_sdf_grid_slot = INVALID_ID
    );
    [[nodiscard]] TensorGridHeaderPod get_tensor_grid_header(uint32_t p_grid_slot) const noexcept;
    [[nodiscard]] std::span<const SymmetricTensor2DPod> get_tensor_grid_cells(uint32_t p_grid_slot) const noexcept;
    [[nodiscard]] std::span<const glm::vec2> get_tensor_major_eigenvectors(uint32_t p_grid_slot) const noexcept;
    [[nodiscard]] std::span<const glm::vec2> get_tensor_minor_eigenvectors(uint32_t p_grid_slot) const noexcept;
    [[nodiscard]] std::span<const TensorSingularityPod> get_detected_tensor_singularities() const noexcept;
    void stage_tensor_streamline_seeds(std::span<const TensorStreamlineSeedPod> p_seeds);

    // --- Signal Command Ring Push / Drain ---
    bool push_signal_command(const GeometrySignalCommandPOD &p_cmd) noexcept;
    uint32_t drain_signal_commands(std::vector<GeometrySignalCommandPOD> &r_out, uint32_t p_max_drain = 512u) noexcept;

    // ========================================================================
    // TEMPLATED TASK NODE BUILDER & PIPELINE STAGE INJECTION
    // ========================================================================

    template <typename T_Logic>
    NodeID add_geometry_task_node(
        TaskGraphDOD &r_graph,
        const T_Logic &p_logic_override = T_Logic{},
        const GeometryNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_spatial_hash_partition_node(TaskGraphDOD &g, const SpatialHashPartitionMetadataLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_polygon_boundary_cull_node(TaskGraphDOD &g, const PolygonBoundaryCullQueryLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_polygon_triangulate_node(TaskGraphDOD &g, const PolygonTriangulateAndOffsetTransformLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_csg_clip_and_seam_weld_node(TaskGraphDOD &g, const CSGClipAndSeamWeldTransformLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_voronoi_delaunay_dual_node(TaskGraphDOD &g, const VoronoiDelaunayDualTransformLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_sdf_field_and_contour_node(TaskGraphDOD &g, const SDFFieldAndContourTransformLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_tensor_field_evaluate_node(TaskGraphDOD &g, const TensorFieldEvaluateAndEigenTransformLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_tensor_singularity_and_seed_node(TaskGraphDOD &g, const TensorSingularityAndSeedQueryLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }
    NodeID add_tensor_hyperstreamline_network_node(TaskGraphDOD &g, const TensorHyperstreamlineNetworkTransformLogic<> &l = {}, const GeometryNodeExecutionConfig &c = {}) const {
        return add_geometry_task_node(g, l, c);
    }

    NodeID add_transform_node_by_id(
        TaskGraphDOD &r_graph,
        GeometryTransformLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const GeometryNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_query_node_by_id(
        TaskGraphDOD &r_graph,
        GeometryQueryLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const GeometryNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_metadata_node_by_id(
        TaskGraphDOD &r_graph,
        GeometryMetadataLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const GeometryNodeExecutionConfig &p_config = {}
    ) const;

    GeometryPipelineTopology inject_pipeline_stages(
        TaskGraphDOD &r_graph,
        uint32_t p_stage_mask = GEOMETRY_STAGE_DEFAULT_PIPELINE_MASK,
        bool p_auto_connect_stages = true
    );

    static bool schedule_node_with_stage(
        TaskGraphDOD &r_graph,
        const GeometryPipelineTopology &p_topology,
        GeometryPipelineStageFlags p_target_stage,
        NodeID p_custom_node,
        bool p_join_downstream = false
    );

    static bool schedule_node_between_stages(
        TaskGraphDOD &r_graph,
        const GeometryPipelineTopology &p_topology,
        GeometryPipelineStageFlags p_upstream_stage,
        GeometryPipelineStageFlags p_downstream_stage,
        NodeID p_custom_node
    );

    void execute_step(double p_delta, TaskGraphDOD *p_override_graph = nullptr);
};

} // namespace ideam::core

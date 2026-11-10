#pragma once

/**
 * spline_dod.h
 * Lean, table-driven C++26 Data-Oriented Design (DOD) coordinator for the IdeamSplines
 * domain (`src/core/tasks/domains/splines/`).
 *
 * Responsibilities:
 *   1. Loop-driven allocation, selection-range synchronization, semantic name lookup, and
 *      TaskGraphResource dictionary population across `SPLINE_BUFFER_SPECS`.
 *   2. Dimension-agnostic (`SplineBase<N, T>`) baking of 1D, 2D, 3D, and 4D splines into
 *      uniform 64-byte `SplineSegmentCoeffsPOD<4, float>` Horner segments and 64-bin
 *      weighted & arc-length `SplineParametricLUTEntryPOD<float>` tables.
 *   3. Zero-copy cross-domain bridging to `GeometryDOD` via `bind_external_geometry_buffers()`,
 *      allowing `SplineFlattenToPolylineTransformLogic` and `SplineBishopFrameSweepTransformLogic`
 *      to write flattened 2D/3D polylines and 3D Rotation-Minimizing Bishop Frames directly into
 *      `GeometryDOD`'s `GEOM_POLYLINE_SLICE_POD` and `GEOM_CURVE_FRAME_3D_POD` buffers!
 */

#include "splines_dod_common.h"
#include "../geometry/geometry_dod.h"
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

class SplineDOD {
private:
    std::shared_ptr<MemoryManagerDOD> memory_manager_;
    std::shared_ptr<TaskGraphDOD> default_task_graph_;

    SplinesBufferCapacities capacities_{};
    SplinesBufferManifest manifest_{};
    SplinesPipelineTopology active_topology_{};

    // --- O(1) Slot Allocators & Contiguous Bump Cursors ---
    SlotIndexAllocator spline_allocator_;
    uint32_t segment_cursor_             = 0u;
    uint32_t weight_lut_cursor_          = 0u;
    uint32_t distance_lut_cursor_        = 0u;
    uint32_t sample_request_cursor_      = 0u;
    uint32_t query_point_cursor_         = 0u;
    uint32_t bridged_polyline_cursor_    = 0u;
    uint32_t bridged_vertex_2d_cursor_   = 0u;
    uint32_t bridged_vertex_3d_cursor_   = 0u;
    uint32_t bridged_frame_3d_cursor_    = 0u;
    bool has_external_geometry_buffers_  = false;

    uint32_t _active_count_for_group(SplineTableGroup p_group) const noexcept;
    void _bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept;

    template <size_t N, FloatingPointScalar T>
    static SplineSegmentCoeffsPOD<4, float> _promote_coeffs_to_4d_float(
        const SplineSegmentCoeffsPOD<N, T> &p_src
    ) noexcept {
        SplineSegmentCoeffsPOD<4, float> out{};
        for (size_t c = 0; c < N && c < 4; ++c) {
            out.c3[static_cast<glm::length_t>(c)] = static_cast<float>(p_src.c3[static_cast<glm::length_t>(c)]);
            out.c2[static_cast<glm::length_t>(c)] = static_cast<float>(p_src.c2[static_cast<glm::length_t>(c)]);
            out.c1[static_cast<glm::length_t>(c)] = static_cast<float>(p_src.c1[static_cast<glm::length_t>(c)]);
            out.c0[static_cast<glm::length_t>(c)] = static_cast<float>(p_src.c0[static_cast<glm::length_t>(c)]);
        }
        return out;
    }

public:
    explicit SplineDOD(
        std::shared_ptr<MemoryManagerDOD> p_memory_manager = nullptr,
        const SplinesBufferCapacities &p_capacities = SplinesBufferCapacities{}
    );
    ~SplineDOD() = default;

    SplineDOD(const SplineDOD &) = delete;
    SplineDOD &operator=(const SplineDOD &) = delete;
    SplineDOD(SplineDOD &&) noexcept = default;
    SplineDOD &operator=(SplineDOD &&) noexcept = default;

    // --- Core Manager & Table-Driven Manifest Lifecycle ---
    void initialize_buffers(const SplinesBufferCapacities &p_capacities = SplinesBufferCapacities{});
    void sync_selection_ranges() noexcept;
    void reset_all_slots() noexcept;

    [[nodiscard]] std::shared_ptr<MemoryManagerDOD> get_memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] MemoryManagerDOD *get_memory_manager_raw() const noexcept { return memory_manager_.get(); }

    [[nodiscard]] std::shared_ptr<TaskGraphDOD> get_default_task_graph() const noexcept { return default_task_graph_; }
    void set_default_task_graph(std::shared_ptr<TaskGraphDOD> p_graph) noexcept { default_task_graph_ = std::move(p_graph); }

    [[nodiscard]] const SplinesBufferCapacities &get_capacities() const noexcept { return capacities_; }
    [[nodiscard]] const SplinesBufferManifest &get_buffer_manifest() const noexcept { return manifest_; }
    [[nodiscard]] const SplinesPipelineTopology &get_active_topology() const noexcept { return active_topology_; }

    [[nodiscard]] constexpr uint32_t get_buffer_id(SplineBufferSlot p_slot) const noexcept { return manifest_.get(p_slot); }
    [[nodiscard]] constexpr uint32_t get_buffer_id_by_name(std::string_view p_name) const noexcept { return manifest_.get_by_name(p_name); }

    void populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing = false) const;
    [[nodiscard]] GrantPartPOD make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept;
    [[nodiscard]] GrantPartPOD make_slot_requirement(SplineBufferSlot p_slot, BufferAccessMode p_access) const noexcept {
        return make_buffer_requirement(manifest_.get(p_slot), p_access);
    }

    // --- Direct Typed Column Accessors ---
    template <typename T>
    [[nodiscard]] T *get_column_ptr(SplineBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return nullptr;
        MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr) return nullptr;
        return reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset);
    }

    template <typename T>
    [[nodiscard]] std::span<T> get_column_span(SplineBufferSlot p_slot) const noexcept {
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
    // Cross-Domain GeometryDOD Buffer Aliasing (`bind_external_geometry_buffers`)
    // ========================================================================

    /**
     * Aliases `SplineDOD`'s bridged polyline and 3D frame buffer slots directly to an
     * external `GeometryDOD` instance's `GeometryBufferManifest` in the shared `MemoryManagerDOD`.
     */
    void bind_external_geometry_buffers(
        uint32_t p_polyline_slice_buf_id,
        uint32_t p_polyline_vertex_2d_buf_id,
        uint32_t p_polyline_vertex_3d_buf_id = INVALID_ID,
        uint32_t p_curve_frame_3d_buf_id = INVALID_ID
    ) noexcept;

    void bind_external_geometry_buffers(const GeometryBufferManifest &p_geom_manifest) noexcept {
        bind_external_geometry_buffers(
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYLINE_SLICE_POD),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_2D),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_3D),
            p_geom_manifest.get(GeometryBufferSlot::GEOM_CURVE_FRAME_3D_POD)
        );
    }

    [[nodiscard]] bool has_external_geometry_buffers() const noexcept { return has_external_geometry_buffers_; }

    // ========================================================================
    // Templated 1D-4D Spline Baking (`bake_spline`) & Batch Request Staging
    // ========================================================================

    template <size_t N, FloatingPointScalar T>
    uint32_t bake_spline(
        const SplineBase<N, T> &p_spline,
        uint8_t p_subdivisions_per_section = 12u,
        uint8_t p_w_channel_mode = 0u,
        uint32_t p_lut_bins = 64u,
        GeometryDOD *r_optional_linked_geometry_dod = nullptr
    );

    [[nodiscard]] bool is_spline_slot_valid(uint32_t p_slot) const noexcept { return spline_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_spline_count() const noexcept { return spline_allocator_.get_active_count(); }
    [[nodiscard]] SplineHeaderPOD get_spline_header(uint32_t p_slot) const noexcept;
    [[nodiscard]] std::span<const SplineSegmentCoeffsPOD<4, float>> get_spline_segments(uint32_t p_slot) const noexcept;

    // --- Batch Evaluation Request & Proximity Query Staging ---
    void stage_sample_requests(std::span<const SplineSampleRequestPOD> p_requests);
    [[nodiscard]] std::span<const glm::vec4> get_evaluated_outputs() const noexcept;
    [[nodiscard]] std::span<const glm::vec4> get_evaluated_tangents() const noexcept;

    void stage_proximity_query_points(std::span<const glm::vec4> p_query_points);
    [[nodiscard]] std::span<const float> get_closest_percents() const noexcept;
    [[nodiscard]] std::span<const float> get_closest_distances() const noexcept;
    [[nodiscard]] std::span<const int32_t> get_closest_anchors() const noexcept;

    // --- Flattened Polyline & 3D Bishop Frame Readback (From Internal or Bridged GeometryDOD Buffers) ---
    [[nodiscard]] PolylineSlicePOD get_flattened_polyline_slice(uint32_t p_spline_slot) const noexcept;
    [[nodiscard]] std::span<const glm::vec2> get_flattened_polyline_vertices_2d(uint32_t p_spline_slot) const noexcept;
    [[nodiscard]] std::span<const glm::vec3> get_flattened_polyline_vertices_3d(uint32_t p_spline_slot) const noexcept;
    [[nodiscard]] std::span<const CurveFrame3DPOD<float>> get_swept_bishop_frames_3d(uint32_t p_spline_slot) const noexcept;

    // ========================================================================
    // TEMPLATED TASK NODE BUILDER & PIPELINE STAGE INJECTION
    // ========================================================================

    template <typename T_Logic>
    NodeID add_spline_task_node(
        TaskGraphDOD &r_graph,
        const T_Logic &p_logic_override = T_Logic{},
        const SplinesNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_arc_length_metadata_node(TaskGraphDOD &g, const SplineArcLengthLUTMetadataLogic<> &l = {}, const SplinesNodeExecutionConfig &c = {}) const {
        return add_spline_task_node(g, l, c);
    }
    NodeID add_threshold_and_corridor_query_node(TaskGraphDOD &g, const SplineThresholdAndCorridorQueryLogic<> &l = {}, const SplinesNodeExecutionConfig &c = {}) const {
        return add_spline_task_node(g, l, c);
    }
    NodeID add_batch_evaluate_node(TaskGraphDOD &g, const SplineBatchEvaluateTransformLogic<> &l = {}, const SplinesNodeExecutionConfig &c = {}) const {
        return add_spline_task_node(g, l, c);
    }
    NodeID add_flatten_to_polyline_node(TaskGraphDOD &g, const SplineFlattenToPolylineTransformLogic<> &l = {}, const SplinesNodeExecutionConfig &c = {}) const {
        return add_spline_task_node(g, l, c);
    }
    NodeID add_bishop_frame_sweep_node(TaskGraphDOD &g, const SplineBishopFrameSweepTransformLogic<> &l = {}, const SplinesNodeExecutionConfig &c = {}) const {
        return add_spline_task_node(g, l, c);
    }
    NodeID add_closest_point_project_node(TaskGraphDOD &g, const SplineClosestPointProjectTransformLogic<> &l = {}, const SplinesNodeExecutionConfig &c = {}) const {
        return add_spline_task_node(g, l, c);
    }

    NodeID add_transform_node_by_id(
        TaskGraphDOD &r_graph,
        SplinesTransformLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const SplinesNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_query_node_by_id(
        TaskGraphDOD &r_graph,
        SplinesQueryLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const SplinesNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_metadata_node_by_id(
        TaskGraphDOD &r_graph,
        SplinesMetadataLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const SplinesNodeExecutionConfig &p_config = {}
    ) const;

    SplinesPipelineTopology inject_pipeline_stages(
        TaskGraphDOD &r_graph,
        uint32_t p_stage_mask = SPLINES_STAGE_DEFAULT_PIPELINE_MASK,
        bool p_auto_connect_stages = true
    );

    static bool schedule_node_with_stage(
        TaskGraphDOD &r_graph,
        const SplinesPipelineTopology &p_topology,
        SplinesPipelineStageFlags p_target_stage,
        NodeID p_custom_node,
        bool p_join_downstream = false
    );

    static bool schedule_node_between_stages(
        TaskGraphDOD &r_graph,
        const SplinesPipelineTopology &p_topology,
        SplinesPipelineStageFlags p_upstream_stage,
        SplinesPipelineStageFlags p_downstream_stage,
        NodeID p_custom_node
    );

    void execute_step(double p_delta, TaskGraphDOD *p_override_graph = nullptr);
};

} // namespace ideam::core

#pragma once

/**
 * splines_dod_common.h
 * Foundational pure C++26 data structures, table-driven buffer manifest descriptors,
 * capacity configurations, and composable TaskGraph pipeline stage flags for the
 * IdeamSplines <-> IdeamTasks & GeometryDOD Data-Oriented Design (DOD) bridge.
 *
 * Implements the 3 core CSR multi-spline packing tables + batch query/evaluation tables
 * + bridged GeometryDOD polyline and 3D Bishop frame slots:
 *   1. SplineHeaderBuffer        (`SplineHeaderPOD`, 64B AoS)
 *   2. SplineSegmentCoeffsBuffer (`SplineSegmentCoeffsPOD<4, float>`, 64B AoS — uniform 16B SIMD lanes for 1D-4D)
 *   3. SplineParametricLUTBuffer (`SplineParametricLUTEntryPOD<float>`, 12B AoS — weighted & arc-length LUTs)
 *   4. Batch Evaluation & Proximity Projection Tables (`SplineSampleRequestPOD`, `glm::vec4` outputs & tangents,
 *      closest-point query & result columns)
 *   5. Bridged GeometryDOD Buffer Slots (`BRIDGED_GEOM_POLYLINE_SLICE_POD`, `BRIDGED_GEOM_POLYLINE_VERTEX_2D`,
 *      `BRIDGED_GEOM_POLYLINE_VERTEX_3D`, `BRIDGED_GEOM_CURVE_FRAME_3D_POD`) bound via
 *      `SplineDOD::bind_external_geometry_buffers()`.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include "../domain_dod_common.h"
#include "../geometry/geometry_dod_common.h"
#include "../../../geometry/geometry_common.h"
#include "../../../geometry/parallel_transport_3d.h"
#include "../../../splines/spline_1d.h"
#include "../../../splines/spline_2d.h"
#include "../../../splines/spline_3d.h"
#include "../../../splines/spline_4d.h"
#include "../../../splines/spline_math.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

// Include the 3 Splines task registry headers
#include "registration/splines_metadata_task_registry.h"
#include "registration/splines_query_task_registry.h"
#include "registration/splines_transform_task_registry.h"

// Include the 6 Splines domain logic headers
#include "metadata_logic/spline_arc_length_lut_metadata_logic.h"
#include "query_logic/spline_threshold_and_corridor_query_logic.h"
#include "transform_logic/spline_batch_evaluate_transform_logic.h"
#include "transform_logic/spline_flatten_to_polyline_transform_logic.h"
#include "transform_logic/spline_bishop_frame_sweep_transform_logic.h"
#include "transform_logic/spline_closest_point_project_transform_logic.h"

namespace ideam::core {

/**
 * SplinesPipelineStageFlags
 * Composable bitmask flags representing logical stages in the SplineDOD evaluation pipeline.
 */
enum SplinesPipelineStageFlags : uint32_t {
    SPLINES_STAGE_NONE                      = 0u,

    // Stage 1: S_M1 SplineArcLengthLUTMetadataLogic (Dimension Partitioning & Complexity LODs)
    SPLINES_STAGE_ARC_LENGTH_METADATA       = 1u << 0,

    // Stage 2: S_T2 SplineFlattenToPolyline + S_T3 SplineBishopFrameSweep (Wave 1 GeometryDOD Bridge!)
    SPLINES_STAGE_GEOMETRY_FLATTEN_AND_SWEEP= 1u << 1,

    // Stage 3: S_T1 SplineBatchEvaluate + S_T4 SplineClosestPointProject (Parallel Batch Evaluation)
    SPLINES_STAGE_BATCH_EVAL_AND_PROJECT    = 1u << 2,

    // Stage 4: S_Q1 SplineThresholdAndCorridorQueryLogic (Corridor / Threshold / Polygon Culling)
    SPLINES_STAGE_CORRIDOR_AND_THRESHOLD_QUERY = 1u << 3,

    // Preset masks:
    SPLINES_STAGE_GEOMETRY_BRIDGE_MASK      =
        SPLINES_STAGE_ARC_LENGTH_METADATA       |
        SPLINES_STAGE_GEOMETRY_FLATTEN_AND_SWEEP,

    SPLINES_STAGE_DEFAULT_PIPELINE_MASK     =
        SPLINES_STAGE_GEOMETRY_BRIDGE_MASK      |
        SPLINES_STAGE_BATCH_EVAL_AND_PROJECT    |
        SPLINES_STAGE_CORRIDOR_AND_THRESHOLD_QUERY,

    SPLINES_STAGE_ALL_MASK                  = 0x000Fu
};

/**
 * SplinesNodeExecutionConfig
 * Optional overrides when instantiating any Splines task node in a TaskGraphDOD.
 */
struct SplinesNodeExecutionConfig {
    MemoryView view            = MemoryView::SingleElementView;
    MemoryStrategy strategy    = MemoryStrategy::FlatStrategy;
    MemoryTypes element_type   = MemoryTypes::INT32;
    QueryOp query_op           = QueryOp::CULL;
    int32_t execution_priority = 0;
    bool use_registry_factory  = false;
    std::span<const GrantPartPOD> custom_requirements{};
};

/**
 * SplinesStageAnchor
 * Topological handle returned for each injected stage in a TaskGraphDOD.
 */
struct SplinesStageAnchor {
    SplinesPipelineStageFlags stage = SPLINES_STAGE_NONE;
    NodeID entry_node               = INVALID_ID;
    NodeID exit_node                = INVALID_ID;
    std::vector<NodeID> nodes;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return entry_node != INVALID_ID && exit_node != INVALID_ID;
    }
};

/**
 * SplinesPipelineTopology
 * Describes the set of SplinesStageAnchors injected into a TaskGraphDOD.
 */
struct SplinesPipelineTopology {
    NodeID first_node = INVALID_ID;
    NodeID last_node  = INVALID_ID;
    std::vector<SplinesStageAnchor> stages;

    [[nodiscard]] const SplinesStageAnchor *get_stage_anchor(SplinesPipelineStageFlags p_stage) const noexcept {
        for (const auto &anchor : stages) {
            if (anchor.stage == p_stage) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool has_stage(SplinesPipelineStageFlags p_stage) const noexcept {
        const SplinesStageAnchor *anchor = get_stage_anchor(p_stage);
        return anchor != nullptr && anchor->is_valid();
    }
};

/**
 * SplineCapacityKind
 * Identifies which configurable capacity field in SplinesBufferCapacities governs a slot.
 */
enum class SplineCapacityKind : uint8_t {
    SPLINES = 0,
    SEGMENTS,
    LUT_ENTRIES,
    SAMPLE_REQUESTS,
    QUERY_POINTS,
    BRIDGED_GEOM_POLYLINES,
    BRIDGED_GEOM_VERTICES_2D,
    BRIDGED_GEOM_VERTICES_3D,
    BRIDGED_GEOM_FRAMES_3D
};

/**
 * SplineTableGroup
 * Logical table ownership tag for each buffer slot, enabling SplineDOD to synchronize
 * active selection counts in a single loop over SPLINE_BUFFER_SPECS.
 */
enum class SplineTableGroup : uint8_t {
    SPLINE_HEADER_TABLE = 0,
    SPLINE_SEGMENT_SLICE,
    SPLINE_WEIGHT_LUT_SLICE,
    SPLINE_DISTANCE_LUT_SLICE,
    SAMPLE_REQUEST_TABLE,
    QUERY_PROJECTION_TABLE,
    BRIDGED_GEOM_POLYLINE_SLICE,
    BRIDGED_GEOM_VERTEX_2D_SLICE,
    BRIDGED_GEOM_VERTEX_3D_SLICE,
    BRIDGED_GEOM_FRAME_3D_SLICE
};

/**
 * SplinesBufferCapacities
 * Configurable row and slice capacities for SplineDOD tables.
 */
struct SplinesBufferCapacities {
    uint32_t max_splines                  = 512;
    uint32_t max_segments                 = 8192;
    uint32_t max_lut_entries              = 32768; // 512 splines * 64 bins
    uint32_t max_sample_requests          = 16384;
    uint32_t max_query_points             = 8192;
    uint32_t max_bridged_polylines        = 1024;
    uint32_t max_bridged_vertices_2d      = 32768;
    uint32_t max_bridged_vertices_3d      = 16384;
    uint32_t max_bridged_frames_3d        = 8192;

    [[nodiscard]] constexpr uint32_t get_capacity(SplineCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case SplineCapacityKind::SPLINES:                   return max_splines;
            case SplineCapacityKind::SEGMENTS:                  return max_segments;
            case SplineCapacityKind::LUT_ENTRIES:               return max_lut_entries;
            case SplineCapacityKind::SAMPLE_REQUESTS:           return max_sample_requests;
            case SplineCapacityKind::QUERY_POINTS:              return max_query_points;
            case SplineCapacityKind::BRIDGED_GEOM_POLYLINES:    return max_bridged_polylines;
            case SplineCapacityKind::BRIDGED_GEOM_VERTICES_2D:  return max_bridged_vertices_2d;
            case SplineCapacityKind::BRIDGED_GEOM_VERTICES_3D:  return max_bridged_vertices_3d;
            case SplineCapacityKind::BRIDGED_GEOM_FRAMES_3D:    return max_bridged_frames_3d;
            default:                                            return max_splines;
        }
    }
};

/**
 * SplineBufferSlot
 * Enumerates every DOD buffer column in SplineDOD (including the 4 bridged GeometryDOD slots).
 */
enum class SplineBufferSlot : uint8_t {
    // --- Table 1: CSR Multi-Spline Header, 4D Horner Coefficients & O(1) LUTs ---
    SPLINE_HEADER_POD = 0,               // SplineHeaderPOD (64B AoS)
    SPLINE_SEGMENT_COEFFS_4D_POD,        // SplineSegmentCoeffsPOD<4, float> (64B AoS)
    SPLINE_WEIGHT_LUT_POD,               // SplineParametricLUTEntryPOD<float> (12B AoS)
    SPLINE_DISTANCE_LUT_POD,             // SplineParametricLUTEntryPOD<float> (12B AoS)

    // --- Table 2: Batch Parametric Evaluation Buffers ---
    SPLINE_SAMPLE_REQUEST_POD,           // SplineSampleRequestPOD (16B AoS)
    SPLINE_EVAL_OUTPUT_VEC4,             // glm::vec4 (16B Flat: evaluated 1D..4D position/value)
    SPLINE_EVAL_TANGENT_VEC4,            // glm::vec4 (16B Flat: evaluated unit tangent / derivative)

    // --- Table 3: Batch Proximity Projection (`get_closest_point_on_spline`) Buffers ---
    SPLINE_QUERY_POINT_VEC4,             // glm::vec4 (16B Flat: xyz = query pos, w = optional spline_slot)
    SPLINE_CLOSEST_PERCENT_FLOAT32,      // float (4B Flat: closest global u in [0, 1])
    SPLINE_CLOSEST_DISTANCE_FLOAT32,     // float (4B Flat: unsigned Euclidean distance to spline)
    SPLINE_CLOSEST_ANCHOR_INT32,         // int32_t (4B Flat: anchor index immediately before closest point)

    // --- Table 4: Bridged GeometryDOD Buffers (Bound via `bind_external_geometry_buffers`) ---
    BRIDGED_GEOM_POLYLINE_SLICE_POD,     // GeometryDOD::GEOM_POLYLINE_SLICE_POD (PolylineSlicePOD, 32B AoS)
    BRIDGED_GEOM_POLYLINE_VERTEX_2D,     // GeometryDOD::GEOM_POLYLINE_VERTEX_2D (glm::vec2, 8B Flat)
    BRIDGED_GEOM_POLYLINE_VERTEX_3D,     // GeometryDOD::GEOM_POLYLINE_VERTEX_3D (glm::vec3, 12B Flat)
    BRIDGED_GEOM_CURVE_FRAME_3D_POD,     // GeometryDOD::GEOM_CURVE_FRAME_3D_POD (CurveFrame3DPOD<float>, 64B AoS)

    COUNT
};

static constexpr size_t SPLINE_BUFFER_SLOT_COUNT = static_cast<size_t>(SplineBufferSlot::COUNT);

/**
 * SplineBufferSpec
 * Compile-time descriptor row in SPLINE_BUFFER_SPECS.
 */
struct SplineBufferSpec {
    SplineBufferSlot slot                   = SplineBufferSlot::COUNT;
    std::string_view canonical_name         = {};
    std::array<std::string_view, 3> aliases = {};
    uint32_t element_stride                 = sizeof(uint32_t);
    BufferLayoutType layout                 = BufferLayoutType::FLAT;
    SelectionMode selection_mode            = SelectionMode::RANGE;
    SplineCapacityKind capacity_kind        = SplineCapacityKind::SPLINES;
    SplineTableGroup table_group            = SplineTableGroup::SPLINE_HEADER_TABLE;

    [[nodiscard]] constexpr bool matches_name(std::string_view p_query) const noexcept {
        if (p_query.empty()) return false;
        if (canonical_name == p_query) return true;
        for (const auto &alias : aliases) {
            if (!alias.empty() && alias == p_query) {
                return true;
            }
        }
        return false;
    }
};

/**
 * SPLINE_BUFFER_SPECS
 * Compile-time manifest covering all 15 SplineDOD buffer slots and property aliases.
 */
inline constexpr std::array<SplineBufferSpec, SPLINE_BUFFER_SLOT_COUNT> SPLINE_BUFFER_SPECS = {{
    // --- Table 1: CSR Multi-Spline Header, 4D Horner Coefficients & O(1) LUTs ---
    {
        SplineBufferSlot::SPLINE_HEADER_POD,
        "spline_header_buffer_id",
        {"spline_header_pod_buffer_id", "", ""},
        sizeof(SplineHeaderPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        SplineCapacityKind::SPLINES, SplineTableGroup::SPLINE_HEADER_TABLE
    },
    {
        SplineBufferSlot::SPLINE_SEGMENT_COEFFS_4D_POD,
        "segment_coeffs_4d_buffer_id",
        {"spline_segment_coeffs_4d_pod_buffer_id", "", ""},
        sizeof(SplineSegmentCoeffsPOD<4, float>), BufferLayoutType::AOS, SelectionMode::RANGE,
        SplineCapacityKind::SEGMENTS, SplineTableGroup::SPLINE_SEGMENT_SLICE
    },
    {
        SplineBufferSlot::SPLINE_WEIGHT_LUT_POD,
        "weight_lut_buffer_id",
        {"spline_weight_lut_pod_buffer_id", "", ""},
        sizeof(SplineParametricLUTEntryPOD<float>), BufferLayoutType::AOS, SelectionMode::RANGE,
        SplineCapacityKind::LUT_ENTRIES, SplineTableGroup::SPLINE_WEIGHT_LUT_SLICE
    },
    {
        SplineBufferSlot::SPLINE_DISTANCE_LUT_POD,
        "distance_lut_buffer_id",
        {"spline_distance_lut_pod_buffer_id", "", ""},
        sizeof(SplineParametricLUTEntryPOD<float>), BufferLayoutType::AOS, SelectionMode::RANGE,
        SplineCapacityKind::LUT_ENTRIES, SplineTableGroup::SPLINE_DISTANCE_LUT_SLICE
    },

    // --- Table 2: Batch Parametric Evaluation Buffers ---
    {
        SplineBufferSlot::SPLINE_SAMPLE_REQUEST_POD,
        "sample_request_buffer_id",
        {"spline_sample_request_pod_buffer_id", "", ""},
        sizeof(SplineSampleRequestPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        SplineCapacityKind::SAMPLE_REQUESTS, SplineTableGroup::SAMPLE_REQUEST_TABLE
    },
    {
        SplineBufferSlot::SPLINE_EVAL_OUTPUT_VEC4,
        "eval_output_vec4_buffer_id",
        {"spline_eval_output_vec4_buffer_id", "", ""},
        sizeof(glm::vec4), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::SAMPLE_REQUESTS, SplineTableGroup::SAMPLE_REQUEST_TABLE
    },
    {
        SplineBufferSlot::SPLINE_EVAL_TANGENT_VEC4,
        "eval_tangent_vec4_buffer_id",
        {"spline_eval_tangent_vec4_buffer_id", "", ""},
        sizeof(glm::vec4), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::SAMPLE_REQUESTS, SplineTableGroup::SAMPLE_REQUEST_TABLE
    },

    // --- Table 3: Batch Proximity Projection Buffers ---
    {
        SplineBufferSlot::SPLINE_QUERY_POINT_VEC4,
        "query_point_vec4_buffer_id",
        {"spline_query_point_vec4_buffer_id", "", ""},
        sizeof(glm::vec4), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::QUERY_POINTS, SplineTableGroup::QUERY_PROJECTION_TABLE
    },
    {
        SplineBufferSlot::SPLINE_CLOSEST_PERCENT_FLOAT32,
        "closest_percent_buffer_id",
        {"spline_closest_percent_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::QUERY_POINTS, SplineTableGroup::QUERY_PROJECTION_TABLE
    },
    {
        SplineBufferSlot::SPLINE_CLOSEST_DISTANCE_FLOAT32,
        "closest_distance_buffer_id",
        {"spline_closest_distance_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::QUERY_POINTS, SplineTableGroup::QUERY_PROJECTION_TABLE
    },
    {
        SplineBufferSlot::SPLINE_CLOSEST_ANCHOR_INT32,
        "closest_anchor_buffer_id",
        {"spline_closest_anchor_buffer_id", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::QUERY_POINTS, SplineTableGroup::QUERY_PROJECTION_TABLE
    },

    // --- Table 4: Bridged GeometryDOD Buffers (Matches GeometryDOD canonical names!) ---
    {
        SplineBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD,
        "polyline_slice_buffer_id",
        {"geom_polyline_slice_pod_buffer_id", "bridged_geom_polyline_slice_buffer_id", ""},
        sizeof(PolylineSlicePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        SplineCapacityKind::BRIDGED_GEOM_POLYLINES, SplineTableGroup::BRIDGED_GEOM_POLYLINE_SLICE
    },
    {
        SplineBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_2D,
        "polyline_vertex_2d_buffer_id",
        {"geom_polyline_vertex_2d_buffer_id", "bridged_geom_polyline_vertex_2d_buffer_id", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::BRIDGED_GEOM_VERTICES_2D, SplineTableGroup::BRIDGED_GEOM_VERTEX_2D_SLICE
    },
    {
        SplineBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_3D,
        "polyline_vertex_3d_buffer_id",
        {"geom_polyline_vertex_3d_buffer_id", "bridged_geom_polyline_vertex_3d_buffer_id", ""},
        sizeof(glm::vec3), BufferLayoutType::FLAT, SelectionMode::RANGE,
        SplineCapacityKind::BRIDGED_GEOM_VERTICES_3D, SplineTableGroup::BRIDGED_GEOM_VERTEX_3D_SLICE
    },
    {
        SplineBufferSlot::BRIDGED_GEOM_CURVE_FRAME_3D_POD,
        "curve_frame_3d_buffer_id",
        {"geom_curve_frame_3d_pod_buffer_id", "bridged_geom_curve_frame_3d_buffer_id", ""},
        sizeof(CurveFrame3DPOD<float>), BufferLayoutType::AOS, SelectionMode::RANGE,
        SplineCapacityKind::BRIDGED_GEOM_FRAMES_3D, SplineTableGroup::BRIDGED_GEOM_FRAME_3D_SLICE
    }
}};

[[nodiscard]] consteval bool validate_spline_buffer_specs() noexcept {
    for (size_t i = 0; i < SPLINE_BUFFER_SLOT_COUNT; ++i) {
        if (static_cast<size_t>(SPLINE_BUFFER_SPECS[i].slot) != i) {
            return false;
        }
        if (SPLINE_BUFFER_SPECS[i].canonical_name.empty() || SPLINE_BUFFER_SPECS[i].element_stride == 0u) {
            return false;
        }
    }
    return true;
}

static_assert(validate_spline_buffer_specs(), "SPLINE_BUFFER_SPECS indices must match SplineBufferSlot enum order!");

/**
 * SplinesBufferManifest
 * Table-driven directory of all MemoryManagerDOD buffer IDs backing SplineDOD.
 */
struct SplinesBufferManifest {
    std::array<uint32_t, SPLINE_BUFFER_SLOT_COUNT> buffer_ids = []() consteval {
        std::array<uint32_t, SPLINE_BUFFER_SLOT_COUNT> arr{};
        arr.fill(INVALID_ID);
        return arr;
    }();

    [[nodiscard]] constexpr uint32_t &operator[](SplineBufferSlot p_slot) noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr const uint32_t &operator[](SplineBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr uint32_t get(SplineBufferSlot p_slot) const noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        return (idx < SPLINE_BUFFER_SLOT_COUNT) ? buffer_ids[idx] : INVALID_ID;
    }

    constexpr void set(SplineBufferSlot p_slot, uint32_t p_buffer_id) noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        if (idx < SPLINE_BUFFER_SLOT_COUNT) {
            buffer_ids[idx] = p_buffer_id;
        }
    }

    constexpr void reset() noexcept {
        buffer_ids.fill(INVALID_ID);
    }

    [[nodiscard]] constexpr uint32_t get_by_name(std::string_view p_name) const noexcept {
        for (size_t i = 0; i < SPLINE_BUFFER_SLOT_COUNT; ++i) {
            if (SPLINE_BUFFER_SPECS[i].matches_name(p_name)) {
                return buffer_ids[i];
            }
        }
        return INVALID_ID;
    }

    [[nodiscard]] constexpr bool is_initialized() const noexcept {
        return get(SplineBufferSlot::SPLINE_HEADER_POD) != INVALID_ID &&
               get(SplineBufferSlot::SPLINE_SEGMENT_COEFFS_4D_POD) != INVALID_ID &&
               get(SplineBufferSlot::SPLINE_WEIGHT_LUT_POD) != INVALID_ID &&
               get(SplineBufferSlot::SPLINE_DISTANCE_LUT_POD) != INVALID_ID;
    }
};

using SplineBufferManifest = SplinesBufferManifest;

} // namespace ideam::core

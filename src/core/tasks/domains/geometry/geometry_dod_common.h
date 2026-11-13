#pragma once

/**
 * geometry_dod_common.h
 * Foundational pure C++26 data structures, table-driven buffer manifest descriptors,
 * capacity configurations, and composable TaskGraph pipeline stage flags for the
 * IdeamGeometry / IdeamSplines / IdeamTiles <-> IdeamTasks Data-Oriented Design (DOD) substrate.
 *
 * Implements the canonical GeometryDOD contiguous memory tables in MemoryManagerDOD:
 *   1. PolylineBuffer        (PolylineSlicePOD + shared 2D/3D vertex arrays + 3D Bishop frames)
 *   2. PolygonMeshBuffer     (PolygonMeshSlicePOD + 2D vertices + triangle index buffer + ClipClassification2D flags)
 *   3. PlanarGraphAndVoronoi (PlanarHalfEdgePOD + Voronoi sites + Voronoi cell slices + CSR dual adjacency)
 *   4. SpatialHashBuffer     (SpatialHashEntryPOD key-value cell hashes)
 *   5. SDFAndContourBuffer   (SDFGridHeaderPOD + row-major float32 SDF grid + MarchingSquaresSegment2D array)
 *   6. TensorFieldAndStreamlineBuffer (TensorGridHeaderPod + TensorBasisElementPod + SymmetricTensor2DPod +
 *      major/minor eigenvectors + TensorSingularityPod + TensorStreamlineSeedPod)
 *   + GeometrySignalCommandPOD ring for deferred main-thread Godot Resource synchronization.
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
#include "../../../geometry/csg_2d.h"
#include "../../../geometry/geometry_common.h"
#include "../../../geometry/parallel_transport_3d.h"
#include "../../../geometry/polygon_math_2d.h"
#include "../../../geometry/sdf_math.h"
#include "../../../geometry/symmetry_2d.h"
#include "../../../geometry/tensor_field_2d.h"
#include "../../../geometry/voronoi_2d.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

// Include the 3 Geometry task registry headers for compile-time LogicIDs
#include "registration/geometry_metadata_task_registry.h"
#include "registration/geometry_query_task_registry.h"
#include "registration/geometry_transform_task_registry.h"

// Include the Geometry domain logic headers
#include "metadata_logic/spatial_hash_partition_metadata_logic.h"
#include "query_logic/polygon_boundary_cull_query_logic.h"
#include "query_logic/tensor_singularity_and_seed_query_logic.h"
#include "transform_logic/polygon_triangulate_and_offset_transform_logic.h"
#include "transform_logic/csg_clip_and_seam_weld_transform_logic.h"
#include "transform_logic/voronoi_delaunay_dual_transform_logic.h"
#include "transform_logic/sdf_field_and_contour_transform_logic.h"
#include "transform_logic/tensor_field_evaluate_and_eigen_transform_logic.h"
#include "transform_logic/tensor_hyperstreamline_network_transform_logic.h"

namespace ideam::core {

/**
 * SDFGridHeaderPOD (32 Bytes, Cache-Line Aligned)
 * Descriptor for a 2D scalar Signed Distance Field or Chladni cymatic grid slice in `GEOM_SDF_GRID_FLOAT32`.
 */
struct alignas(16) SDFGridHeaderPOD {
    uint32_t cell_offset   = 0u;    // Start index in GEOM_SDF_GRID_FLOAT32
    uint32_t width         = 64u;   // Grid width in samples
    uint32_t height        = 64u;   // Grid height in samples
    float iso_value        = 0.0f;  // Iso-contour threshold
    glm::vec2 origin{-5.0f, -5.0f}; // World-space min corner
    glm::vec2 cell_size{0.15873f, 0.15873f}; // World-space step per cell
};

static_assert(sizeof(SDFGridHeaderPOD) == 32, "SDFGridHeaderPOD must be 32 bytes!");

/**
 * GeometrySignalCommandPOD (16 Bytes)
 * Queued event record emitted when a GeometryDOD task updates a bound `Polygon2DResource`,
 * `VoronoiDiagram2DResource`, `SDFGrid2DResource`, or `TensorField2DResource`.
 */
enum GeometrySignalKind : uint8_t {
    GEOM_SIGNAL_POLYGON_UPDATED      = 0,
    GEOM_SIGNAL_VORONOI_UPDATED      = 1,
    GEOM_SIGNAL_SDF_UPDATED          = 2,
    GEOM_SIGNAL_TENSOR_FIELD_UPDATED = 3
};

struct alignas(16) GeometrySignalCommandPOD {
    uint32_t slot_index  = INVALID_ID;
    uint32_t payload_aux = 0u;
    uint8_t signal_kind  = GEOM_SIGNAL_POLYGON_UPDATED;
    uint8_t flags        = 0u;
    uint16_t reserved    = 0u;
    float metric_value   = 0.0f;
};

static_assert(sizeof(GeometrySignalCommandPOD) == 16, "GeometrySignalCommandPOD must be 16 bytes!");

/**
 * GeometryPipelineStageFlags
 * Composable bitmask flags representing logical stages in the GeometryDOD execution pipeline.
 */
enum GeometryPipelineStageFlags : uint32_t {
    GEOMETRY_STAGE_NONE                         = 0u,

    // Stage 1: G_M1 SpatialHashPartitionMetadataLogic (AABB & Spatial Hash Grid Indexing)
    GEOMETRY_STAGE_SPATIAL_HASH_METADATA        = 1u << 0,

    // Stage 2: G_Q1 PolygonBoundaryCullQueryLogic (Wave 2 Interior vs. Straddling Classification)
    GEOMETRY_STAGE_BOUNDARY_CULL_CLASSIFY       = 1u << 1,

    // Stage 3: G_T1 PolygonTriangulateAndOffset + G_T2 CSGClipAndSeamWeld
    GEOMETRY_STAGE_POLYGON_TRIANGULATE_CSG      = 1u << 2,

    // Stage 4: G_T3 VoronoiDelaunayDualTransformLogic (Delaunay, Clipped Voronoi & CSR Dual Graph)
    GEOMETRY_STAGE_VORONOI_DELAUNAY_DUAL        = 1u << 3,

    // Stage 5: G_T4 SDFFieldAndContourTransformLogic (2D SDF / Chladni Field & Marching Squares)
    GEOMETRY_STAGE_SDF_AND_CONTOURS             = 1u << 4,

    // Stage 6: G_T5 TensorFieldEvaluateAndEigen + G_Q2 TensorSingularityAndSeed + G_T6 TensorHyperstreamlineNetwork
    GEOMETRY_STAGE_TENSOR_FIELD_AND_STREAMLINES = 1u << 5,

    // Preset masks:
    GEOMETRY_STAGE_POLYGON_PIPELINE_MASK        =
        GEOMETRY_STAGE_SPATIAL_HASH_METADATA  |
        GEOMETRY_STAGE_BOUNDARY_CULL_CLASSIFY |
        GEOMETRY_STAGE_POLYGON_TRIANGULATE_CSG,

    GEOMETRY_STAGE_DEFAULT_PIPELINE_MASK        =
        GEOMETRY_STAGE_POLYGON_PIPELINE_MASK  |
        GEOMETRY_STAGE_VORONOI_DELAUNAY_DUAL  |
        GEOMETRY_STAGE_SDF_AND_CONTOURS       |
        GEOMETRY_STAGE_TENSOR_FIELD_AND_STREAMLINES,

    GEOMETRY_STAGE_ALL_MASK                     = 0x003Fu
};

/**
 * GeometryNodeExecutionConfig
 * Optional overrides when instantiating any Geometry task node in a TaskGraphDOD.
 */
struct GeometryNodeExecutionConfig {
    MemoryView view            = MemoryView::SingleElementView;
    MemoryStrategy strategy    = MemoryStrategy::FlatStrategy;
    MemoryTypes element_type   = MemoryTypes::INT32;
    QueryOp query_op           = QueryOp::CULL;
    int32_t execution_priority = 0;
    bool use_registry_factory  = false;
    std::span<const GrantPartPOD> custom_requirements{};
};

/**
 * GeometryStageAnchor
 * Topological handle returned for each injected stage in a TaskGraphDOD.
 */
struct GeometryStageAnchor {
    GeometryPipelineStageFlags stage = GEOMETRY_STAGE_NONE;
    NodeID entry_node                = INVALID_ID;
    NodeID exit_node                 = INVALID_ID;
    std::vector<NodeID> nodes;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return entry_node != INVALID_ID && exit_node != INVALID_ID;
    }
};

/**
 * GeometryPipelineTopology
 * Describes the set of GeometryStageAnchors injected into a TaskGraphDOD.
 */
struct GeometryPipelineTopology {
    NodeID first_node = INVALID_ID;
    NodeID last_node  = INVALID_ID;
    std::vector<GeometryStageAnchor> stages;

    [[nodiscard]] const GeometryStageAnchor *get_stage_anchor(GeometryPipelineStageFlags p_stage) const noexcept {
        for (const auto &anchor : stages) {
            if (anchor.stage == p_stage) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool has_stage(GeometryPipelineStageFlags p_stage) const noexcept {
        const GeometryStageAnchor *anchor = get_stage_anchor(p_stage);
        return anchor != nullptr && anchor->is_valid();
    }
};

/**
 * GeometryCapacityKind
 * Identifies which configurable capacity field in GeometryBufferCapacities governs a slot.
 */
enum class GeometryCapacityKind : uint8_t {
    POLYLINES = 0,
    POLYLINE_VERTICES_2D,
    POLYLINE_VERTICES_3D,
    CURVE_FRAMES_3D,
    POLYGON_MESHES,
    POLYGON_VERTICES_2D,
    POLYGON_INDICES,
    HALF_EDGES,
    VORONOI_SITES,
    VORONOI_CSR_OFFSETS,
    VORONOI_CSR_NEIGHBORS,
    SPATIAL_HASH_ENTRIES,
    SDF_GRIDS,
    SDF_CELLS,
    MARCHING_SQUARES_SEGMENTS,
    TENSOR_GRIDS,
    TENSOR_BASES,
    TENSOR_CELLS,
    TENSOR_SINGULARITIES,
    TENSOR_SEEDS,
    SIGNAL_COMMANDS,
    RING_CURSOR_OR_COUNTER
};

/**
 * GeometryTableGroup
 * Logical table ownership tag for each buffer slot, enabling GeometryDOD to synchronize
 * active selection counts in a single loop over GEOMETRY_BUFFER_SPECS.
 */
enum class GeometryTableGroup : uint8_t {
    POLYLINE_SLICE = 0,
    POLYLINE_VERTEX_2D_SLICE,
    POLYLINE_VERTEX_3D_SLICE,
    CURVE_FRAME_3D_SLICE,
    POLYGON_MESH_SLICE,
    POLYGON_VERTEX_2D_SLICE,
    POLYGON_INDEX_SLICE,
    PLANAR_HALF_EDGE_SLICE,
    VORONOI_SITE_TABLE,
    VORONOI_CSR_ROW,
    VORONOI_CSR_EDGE,
    SPATIAL_HASH_TABLE,
    SDF_GRID_HEADER,
    SDF_CELL_SLICE,
    MARCHING_SQUARES_SLICE,
    TENSOR_GRID_HEADER,
    TENSOR_BASIS_SLICE,
    TENSOR_CELL_SLICE,
    TENSOR_SINGULARITY_SLICE,
    TENSOR_SEED_SLICE,
    EVENT_RING_OR_COUNTER
};

/**
 * GeometryBufferCapacities
 * Configurable row and vertex capacities for GeometryDOD tables.
 */
struct GeometryBufferCapacities {
    uint32_t max_polylines                 = 1024;
    uint32_t max_polyline_vertices_2d      = 32768;
    uint32_t max_polyline_vertices_3d      = 16384;
    uint32_t max_curve_frames_3d           = 8192;
    uint32_t max_polygon_meshes            = 1024;
    uint32_t max_polygon_vertices_2d       = 32768;
    uint32_t max_polygon_indices           = 98304;
    uint32_t max_half_edges                = 32768;
    uint32_t max_voronoi_sites             = 2048;
    uint32_t max_voronoi_csr_neighbors     = 12288;
    uint32_t max_spatial_hash_entries      = 4096;
    uint32_t max_sdf_grids                 = 16;
    uint32_t max_sdf_cells                 = 65536; // e.g., 256x256 or 16 x 64x64 grids
    uint32_t max_marching_squares_segments = 16384;
    uint32_t max_tensor_grids              = 16;
    uint32_t max_tensor_bases              = 256;
    uint32_t max_tensor_cells              = 65536;
    uint32_t max_tensor_singularities      = 256;
    uint32_t max_tensor_seeds              = 2048;
    uint32_t max_signal_commands           = 512;
    uint32_t max_ring_cursors              = 4;

    [[nodiscard]] constexpr uint32_t get_capacity(GeometryCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case GeometryCapacityKind::POLYLINES:                 return max_polylines;
            case GeometryCapacityKind::POLYLINE_VERTICES_2D:      return max_polyline_vertices_2d;
            case GeometryCapacityKind::POLYLINE_VERTICES_3D:      return max_polyline_vertices_3d;
            case GeometryCapacityKind::CURVE_FRAMES_3D:           return max_curve_frames_3d;
            case GeometryCapacityKind::POLYGON_MESHES:            return max_polygon_meshes;
            case GeometryCapacityKind::POLYGON_VERTICES_2D:       return max_polygon_vertices_2d;
            case GeometryCapacityKind::POLYGON_INDICES:           return max_polygon_indices;
            case GeometryCapacityKind::HALF_EDGES:                return max_half_edges;
            case GeometryCapacityKind::VORONOI_SITES:             return max_voronoi_sites;
            case GeometryCapacityKind::VORONOI_CSR_OFFSETS:       return max_voronoi_sites + 1u;
            case GeometryCapacityKind::VORONOI_CSR_NEIGHBORS:     return max_voronoi_csr_neighbors;
            case GeometryCapacityKind::SPATIAL_HASH_ENTRIES:      return max_spatial_hash_entries;
            case GeometryCapacityKind::SDF_GRIDS:                 return max_sdf_grids;
            case GeometryCapacityKind::SDF_CELLS:                 return max_sdf_cells;
            case GeometryCapacityKind::MARCHING_SQUARES_SEGMENTS: return max_marching_squares_segments;
            case GeometryCapacityKind::TENSOR_GRIDS:              return max_tensor_grids;
            case GeometryCapacityKind::TENSOR_BASES:              return max_tensor_bases;
            case GeometryCapacityKind::TENSOR_CELLS:              return max_tensor_cells;
            case GeometryCapacityKind::TENSOR_SINGULARITIES:      return max_tensor_singularities;
            case GeometryCapacityKind::TENSOR_SEEDS:              return max_tensor_seeds;
            case GeometryCapacityKind::SIGNAL_COMMANDS:           return max_signal_commands;
            case GeometryCapacityKind::RING_CURSOR_OR_COUNTER:    return max_ring_cursors;
            default:                                              return max_polylines;
        }
    }
};

/**
 * GeometryBufferSlot
 * Enumerates every DOD buffer column in GeometryDOD. Indexed 1:1 into GEOMETRY_BUFFER_SPECS.
 */
enum class GeometryBufferSlot : uint8_t {
    // --- Table 1: Polyline & 3D Frame Buffers (Shared with SplineDOD & TileDOD) ---
    GEOM_POLYLINE_SLICE_POD = 0,         // PolylineSlicePOD (32B AoS)
    GEOM_POLYLINE_VERTEX_2D,             // glm::vec2 (8B Flat)
    GEOM_POLYLINE_VERTEX_3D,             // glm::vec3 (12B Flat)
    GEOM_CURVE_FRAME_3D_POD,             // CurveFrame3DPOD<float> (64B AoS)

    // --- Table 2: Polygon Mesh & CSG Classification Buffers ---
    GEOM_POLYGON_MESH_SLICE_POD,         // PolygonMeshSlicePOD (32B AoS)
    GEOM_POLYGON_VERTEX_2D,              // glm::vec2 (8B Flat)
    GEOM_POLYGON_INDEX_U32,              // uint32_t (4B Flat)
    GEOM_CLIP_CLASSIFICATION_FLAGS,      // uint8_t (1B Flat: ClipClassification2D)

    // --- Table 3: Planar Graph DCEL & Voronoi Dual Adjacency Buffers ---
    GEOM_PLANAR_HALF_EDGE_POD,           // PlanarHalfEdgePOD (24B AoS)
    GEOM_VORONOI_SITE_2D,                // glm::vec2 (8B Flat)
    GEOM_VORONOI_CELL_SLICE_POD,         // PolylineSlicePOD (32B AoS)
    GEOM_VORONOI_CSR_ROW_OFFSETS,        // uint32_t (4B Flat: N+1 CSR row offsets)
    GEOM_VORONOI_CSR_COL_NEIGHBORS,      // uint32_t (4B Flat: CSR neighbor site IDs)

    // --- Table 4: Spatial Hash Acceleration Buffer ---
    GEOM_SPATIAL_HASH_ENTRY_POD,         // SpatialHashEntryPOD (8B AoS)

    // --- Table 5: 2D SDF Grid & Marching Squares Iso-Contour Buffers ---
    GEOM_SDF_GRID_HEADER_POD,            // SDFGridHeaderPOD (32B AoS)
    GEOM_SDF_GRID_FLOAT32,               // float (4B Flat)
    GEOM_MARCHING_SQUARES_SEGMENT_POD,   // MarchingSquaresSegment2D<float> (24B AoS)
    GEOM_MARCHING_SQUARES_COUNT,         // uint32_t (4B Flat counter)

    // --- Table 6: 2D Tensor Field, Eigenvector, Singularity & Hyperstreamline Seed Buffers ---
    GEOM_TENSOR_GRID_HEADER_POD,         // TensorGridHeaderPod (32B AoS)
    GEOM_TENSOR_BASIS_POD,               // TensorBasisElementPod (32B AoS)
    GEOM_TENSOR_CELL_POD,                // SymmetricTensor2DPod (16B AoS)
    GEOM_TENSOR_EIGEN_MAJOR_2D,          // glm::vec2 (8B Flat: Major eigenvector field)
    GEOM_TENSOR_EIGEN_MINOR_2D,          // glm::vec2 (8B Flat: Minor eigenvector field)
    GEOM_TENSOR_SINGULARITY_POD,         // TensorSingularityPod (16B AoS)
    GEOM_TENSOR_SINGULARITY_COUNT,       // uint32_t (4B Flat counter)
    GEOM_TENSOR_STREAMLINE_SEED_POD,     // TensorStreamlineSeedPod (16B AoS)

    // --- Ring 1: Deferred Godot Resource Signal Command Ring ---
    GEOM_SIGNAL_COMMAND_RING,            // GeometrySignalCommandPOD (16B Ring)
    GEOM_SIGNAL_RING_CURSOR,             // uint32_t[2] cursor

    COUNT
};

static constexpr size_t GEOMETRY_BUFFER_SLOT_COUNT = static_cast<size_t>(GeometryBufferSlot::COUNT);

/**
 * GeometryBufferSpec
 * Compile-time descriptor row in GEOMETRY_BUFFER_SPECS.
 */
struct GeometryBufferSpec {
    GeometryBufferSlot slot                 = GeometryBufferSlot::COUNT;
    std::string_view canonical_name         = {};
    std::array<std::string_view, 3> aliases = {};
    uint32_t element_stride                 = sizeof(uint32_t);
    BufferLayoutType layout                 = BufferLayoutType::FLAT;
    SelectionMode selection_mode            = SelectionMode::RANGE;
    GeometryCapacityKind capacity_kind      = GeometryCapacityKind::POLYLINES;
    GeometryTableGroup table_group          = GeometryTableGroup::POLYLINE_SLICE;

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
 * GEOMETRY_BUFFER_SPECS
 * Compile-time manifest covering all 28 GeometryDOD buffer slots and property aliases.
 */
inline constexpr std::array<GeometryBufferSpec, GEOMETRY_BUFFER_SLOT_COUNT> GEOMETRY_BUFFER_SPECS = {{
    // --- Table 1: Polyline & 3D Frame Buffers ---
    {
        GeometryBufferSlot::GEOM_POLYLINE_SLICE_POD,
        "polyline_slice_buffer_id",
        {"geom_polyline_slice_pod_buffer_id", "", ""},
        sizeof(PolylineSlicePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::POLYLINES, GeometryTableGroup::POLYLINE_SLICE
    },
    {
        GeometryBufferSlot::GEOM_POLYLINE_VERTEX_2D,
        "polyline_vertex_2d_buffer_id",
        {"geom_polyline_vertex_2d_buffer_id", "", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::POLYLINE_VERTICES_2D, GeometryTableGroup::POLYLINE_VERTEX_2D_SLICE
    },
    {
        GeometryBufferSlot::GEOM_POLYLINE_VERTEX_3D,
        "polyline_vertex_3d_buffer_id",
        {"geom_polyline_vertex_3d_buffer_id", "", ""},
        sizeof(glm::vec3), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::POLYLINE_VERTICES_3D, GeometryTableGroup::POLYLINE_VERTEX_3D_SLICE
    },
    {
        GeometryBufferSlot::GEOM_CURVE_FRAME_3D_POD,
        "curve_frame_3d_buffer_id",
        {"geom_curve_frame_3d_pod_buffer_id", "", ""},
        sizeof(CurveFrame3DPOD<float>), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::CURVE_FRAMES_3D, GeometryTableGroup::CURVE_FRAME_3D_SLICE
    },

    // --- Table 2: Polygon Mesh & CSG Classification Buffers ---
    {
        GeometryBufferSlot::GEOM_POLYGON_MESH_SLICE_POD,
        "polygon_mesh_slice_buffer_id",
        {"geom_polygon_mesh_slice_pod_buffer_id", "", ""},
        sizeof(PolygonMeshSlicePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::POLYGON_MESHES, GeometryTableGroup::POLYGON_MESH_SLICE
    },
    {
        GeometryBufferSlot::GEOM_POLYGON_VERTEX_2D,
        "polygon_vertex_2d_buffer_id",
        {"geom_polygon_vertex_2d_buffer_id", "", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::POLYGON_VERTICES_2D, GeometryTableGroup::POLYGON_VERTEX_2D_SLICE
    },
    {
        GeometryBufferSlot::GEOM_POLYGON_INDEX_U32,
        "polygon_index_u32_buffer_id",
        {"geom_polygon_index_u32_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::POLYGON_INDICES, GeometryTableGroup::POLYGON_INDEX_SLICE
    },
    {
        GeometryBufferSlot::GEOM_CLIP_CLASSIFICATION_FLAGS,
        "clip_classification_buffer_id",
        {"geom_clip_classification_flags_buffer_id", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::POLYLINES, GeometryTableGroup::POLYLINE_SLICE
    },

    // --- Table 3: Planar Graph DCEL & Voronoi Dual Adjacency Buffers ---
    {
        GeometryBufferSlot::GEOM_PLANAR_HALF_EDGE_POD,
        "planar_half_edge_buffer_id",
        {"geom_planar_half_edge_pod_buffer_id", "", ""},
        sizeof(PlanarHalfEdgePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::HALF_EDGES, GeometryTableGroup::PLANAR_HALF_EDGE_SLICE
    },
    {
        GeometryBufferSlot::GEOM_VORONOI_SITE_2D,
        "voronoi_site_2d_buffer_id",
        {"geom_voronoi_site_2d_buffer_id", "", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::VORONOI_SITES, GeometryTableGroup::VORONOI_SITE_TABLE
    },
    {
        GeometryBufferSlot::GEOM_VORONOI_CELL_SLICE_POD,
        "voronoi_cell_slice_buffer_id",
        {"geom_voronoi_cell_slice_pod_buffer_id", "", ""},
        sizeof(PolylineSlicePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::VORONOI_SITES, GeometryTableGroup::VORONOI_SITE_TABLE
    },
    {
        GeometryBufferSlot::GEOM_VORONOI_CSR_ROW_OFFSETS,
        "voronoi_csr_offsets_buffer_id",
        {"location_csr_offsets_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::VORONOI_CSR_OFFSETS, GeometryTableGroup::VORONOI_CSR_ROW
    },
    {
        GeometryBufferSlot::GEOM_VORONOI_CSR_COL_NEIGHBORS,
        "voronoi_csr_neighbors_buffer_id",
        {"location_csr_neighbors_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::VORONOI_CSR_NEIGHBORS, GeometryTableGroup::VORONOI_CSR_EDGE
    },

    // --- Table 4: Spatial Hash Acceleration Buffer ---
    {
        GeometryBufferSlot::GEOM_SPATIAL_HASH_ENTRY_POD,
        "spatial_hash_entry_buffer_id",
        {"geom_spatial_hash_entry_pod_buffer_id", "", ""},
        sizeof(SpatialHashEntryPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::SPATIAL_HASH_ENTRIES, GeometryTableGroup::SPATIAL_HASH_TABLE
    },

    // --- Table 5: 2D SDF Grid & Marching Squares Iso-Contour Buffers ---
    {
        GeometryBufferSlot::GEOM_SDF_GRID_HEADER_POD,
        "sdf_grid_header_buffer_id",
        {"geom_sdf_grid_header_pod_buffer_id", "", ""},
        sizeof(SDFGridHeaderPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::SDF_GRIDS, GeometryTableGroup::SDF_GRID_HEADER
    },
    {
        GeometryBufferSlot::GEOM_SDF_GRID_FLOAT32,
        "sdf_grid_float32_buffer_id",
        {"geom_sdf_grid_float32_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::SDF_CELLS, GeometryTableGroup::SDF_CELL_SLICE
    },
    {
        GeometryBufferSlot::GEOM_MARCHING_SQUARES_SEGMENT_POD,
        "marching_squares_segment_buffer_id",
        {"geom_marching_squares_segment_pod_buffer_id", "", ""},
        sizeof(MarchingSquaresSegment2D<float>), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::MARCHING_SQUARES_SEGMENTS, GeometryTableGroup::MARCHING_SQUARES_SLICE
    },
    {
        GeometryBufferSlot::GEOM_MARCHING_SQUARES_COUNT,
        "marching_squares_count_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::RING_CURSOR_OR_COUNTER, GeometryTableGroup::EVENT_RING_OR_COUNTER
    },

    // --- Table 6: 2D Tensor Field, Eigenvector, Singularity & Hyperstreamline Seed Buffers ---
    {
        GeometryBufferSlot::GEOM_TENSOR_GRID_HEADER_POD,
        "tensor_grid_header_buffer_id",
        {"geom_tensor_grid_header_pod_buffer_id", "", ""},
        sizeof(TensorGridHeaderPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::TENSOR_GRIDS, GeometryTableGroup::TENSOR_GRID_HEADER
    },
    {
        GeometryBufferSlot::GEOM_TENSOR_BASIS_POD,
        "tensor_basis_buffer_id",
        {"geom_tensor_basis_pod_buffer_id", "", ""},
        sizeof(TensorBasisElementPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::TENSOR_BASES, GeometryTableGroup::TENSOR_BASIS_SLICE
    },
    {
        GeometryBufferSlot::GEOM_TENSOR_CELL_POD,
        "tensor_cell_buffer_id",
        {"geom_tensor_cell_pod_buffer_id", "symmetric_tensor_2d_buffer_id", ""},
        sizeof(SymmetricTensor2DPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::TENSOR_CELLS, GeometryTableGroup::TENSOR_CELL_SLICE
    },
    {
        GeometryBufferSlot::GEOM_TENSOR_EIGEN_MAJOR_2D,
        "tensor_eigen_major_buffer_id",
        {"geom_tensor_eigen_major_2d_buffer_id", "major_eigenvector_buffer_id", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::TENSOR_CELLS, GeometryTableGroup::TENSOR_CELL_SLICE
    },
    {
        GeometryBufferSlot::GEOM_TENSOR_EIGEN_MINOR_2D,
        "tensor_eigen_minor_buffer_id",
        {"geom_tensor_eigen_minor_2d_buffer_id", "minor_eigenvector_buffer_id", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::TENSOR_CELLS, GeometryTableGroup::TENSOR_CELL_SLICE
    },
    {
        GeometryBufferSlot::GEOM_TENSOR_SINGULARITY_POD,
        "tensor_singularity_buffer_id",
        {"geom_tensor_singularity_pod_buffer_id", "", ""},
        sizeof(TensorSingularityPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        GeometryCapacityKind::TENSOR_SINGULARITIES, GeometryTableGroup::TENSOR_SINGULARITY_SLICE
    },
    {
        GeometryBufferSlot::GEOM_TENSOR_SINGULARITY_COUNT,
        "tensor_singularity_count_buf_id",
        {"geom_tensor_singularity_count_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::RING_CURSOR_OR_COUNTER, GeometryTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        GeometryBufferSlot::GEOM_TENSOR_STREAMLINE_SEED_POD,
        "tensor_seed_buffer_id",
        {"geom_tensor_streamline_seed_pod_buffer_id", "streamline_seed_buffer_id", ""},
        sizeof(TensorStreamlineSeedPod), BufferLayoutType::AOS, SelectionMode::SPARSE_SET,
        GeometryCapacityKind::TENSOR_SEEDS, GeometryTableGroup::TENSOR_SEED_SLICE
    },

    // --- Ring 1: Deferred Godot Resource Signal Command Ring ---
    {
        GeometryBufferSlot::GEOM_SIGNAL_COMMAND_RING,
        "geometry_signal_command_ring_buffer_id",
        {"signal_command_ring_buffer_id", "", ""},
        sizeof(GeometrySignalCommandPOD), BufferLayoutType::RING, SelectionMode::RANGE,
        GeometryCapacityKind::SIGNAL_COMMANDS, GeometryTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        GeometryBufferSlot::GEOM_SIGNAL_RING_CURSOR,
        "geometry_signal_ring_cursor_buffer_id",
        {"signal_ring_cursor_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        GeometryCapacityKind::RING_CURSOR_OR_COUNTER, GeometryTableGroup::EVENT_RING_OR_COUNTER
    }
}};

[[nodiscard]] consteval bool validate_geometry_buffer_specs() noexcept {
    for (size_t i = 0; i < GEOMETRY_BUFFER_SLOT_COUNT; ++i) {
        if (static_cast<size_t>(GEOMETRY_BUFFER_SPECS[i].slot) != i) {
            return false;
        }
        if (GEOMETRY_BUFFER_SPECS[i].canonical_name.empty() || GEOMETRY_BUFFER_SPECS[i].element_stride == 0u) {
            return false;
        }
    }
    return true;
}

static_assert(validate_geometry_buffer_specs(), "GEOMETRY_BUFFER_SPECS indices must match GeometryBufferSlot enum order!");

/**
 * GeometryBufferManifest
 * Table-driven directory of all MemoryManagerDOD buffer IDs backing GeometryDOD.
 */
struct GeometryBufferManifest {
    std::array<uint32_t, GEOMETRY_BUFFER_SLOT_COUNT> buffer_ids = []() consteval {
        std::array<uint32_t, GEOMETRY_BUFFER_SLOT_COUNT> arr{};
        arr.fill(INVALID_ID);
        return arr;
    }();

    [[nodiscard]] constexpr uint32_t &operator[](GeometryBufferSlot p_slot) noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr const uint32_t &operator[](GeometryBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr uint32_t get(GeometryBufferSlot p_slot) const noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        return (idx < GEOMETRY_BUFFER_SLOT_COUNT) ? buffer_ids[idx] : INVALID_ID;
    }

    constexpr void set(GeometryBufferSlot p_slot, uint32_t p_buffer_id) noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        if (idx < GEOMETRY_BUFFER_SLOT_COUNT) {
            buffer_ids[idx] = p_buffer_id;
        }
    }

    constexpr void reset() noexcept {
        buffer_ids.fill(INVALID_ID);
    }

    [[nodiscard]] constexpr uint32_t get_by_name(std::string_view p_name) const noexcept {
        for (size_t i = 0; i < GEOMETRY_BUFFER_SLOT_COUNT; ++i) {
            if (GEOMETRY_BUFFER_SPECS[i].matches_name(p_name)) {
                return buffer_ids[i];
            }
        }
        return INVALID_ID;
    }

    [[nodiscard]] constexpr bool is_initialized() const noexcept {
        return get(GeometryBufferSlot::GEOM_POLYLINE_SLICE_POD) != INVALID_ID &&
               get(GeometryBufferSlot::GEOM_POLYGON_MESH_SLICE_POD) != INVALID_ID &&
               get(GeometryBufferSlot::GEOM_VORONOI_SITE_2D) != INVALID_ID &&
               get(GeometryBufferSlot::GEOM_SDF_GRID_FLOAT32) != INVALID_ID &&
               get(GeometryBufferSlot::GEOM_TENSOR_CELL_POD) != INVALID_ID;
    }
};

} // namespace ideam::core

#pragma once

/**
 * tiles_dod_common.h
 * Foundational pure C++26 data structures, table-driven buffer manifest descriptors,
 * capacity configurations, and composable TaskGraph pipeline stage flags for the
 * IdeamTiles <-> IdeamTasks & GeometryDOD Data-Oriented Design (DOD) bridge.
 *
 * Implements the core CSR multi-layer tiling tables + symmetry navigation tables +
 * bridged GeometryDOD polyline and polygon mesh slots:
 *   1. PrototileDescriptorTable (`BakedPrototilePOD<float>`, AoS — 17 Wallpaper / 81 Tactile IH
 *      basis vectors, aspect transforms, 3-coloring lookup, and 12x6 quotient adjacency table)
 *   2. FundamentalCellTables    (`IsoCellPOD` 32B AoS, `TileTransformPOD` 32B AoS, plus CSR
 *      vertex, edge, and neighbor index slices)
 *   3. TopologicalEdgeTables    (`TileEdgePOD` 16B AoS, `TileEdgeMetadataPOD` 16B AoS,
 *      `BridgeQuadCellPOD` 32B AoS)
 *   4. HexSuperCellTables       (`HexSuperCellPOD` 64B AoS, `uint32_t` child cell CSR)
 *   5. NavigationAndFlowTables  (`TileNavRequestPOD` 32B AoS, `uint32_t` path cells,
 *      `float` wavefront costs, `glm::vec2` flow-field steering vectors)
 *   6. Bridged GeometryDOD Slots (`BRIDGED_GEOM_POLYLINE_SLICE_POD`, `BRIDGED_GEOM_POLYLINE_VERTEX_2D`,
 *      `BRIDGED_GEOM_POLYGON_MESH_SLICE_POD`, `BRIDGED_GEOM_POLYGON_VERTEX_2D`,
 *      `BRIDGED_GEOM_POLYGON_INDEX_U32`, `BRIDGED_GEOM_CLIP_CLASSIFICATION_FLAGS`) bound via
 *      `TileDOD::bind_external_geometry_buffers()`.
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
#include "../../../geometry/csg_2d.h"
#include "../../../geometry/geometry_common.h"
#include "../../../geometry/polygon_math_2d.h"
#include "../../../geometry/symmetry_2d.h"
#include "../../../tiles/isohedral_tiling_2d.h"
#include "../../../tiles/tile_common.h"
#include "../../../tiles/tiling_navigation_2d.h"
#include "../../../tiles/tiling_region_2d.h"
#include "../../../tiles/tiling_seam_stitcher_2d.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

// Include the 3 Tiles task registry headers
#include "registration/tiles_metadata_task_registry.h"
#include "registration/tiles_query_task_registry.h"
#include "registration/tiles_transform_task_registry.h"

// Include the 6 Tiles domain logic headers
#include "metadata_logic/tiling_generation_and_hex_partition_metadata_logic.h"
#include "query_logic/tiling_lattice_cull_and_mask_query_logic.h"
#include "transform_logic/tiling_instance_transform_logic.h"
#include "transform_logic/tiling_boundary_clip_and_seam_stitch_transform_logic.h"
#include "transform_logic/tiling_hexize_and_border_extract_transform_logic.h"
#include "transform_logic/tiling_quotient_astar_and_flow_field_transform_logic.h"

namespace ideam::core {

/**
 * TilesPipelineStageFlags
 * Composable bitmask flags representing logical stages in the TileDOD evaluation pipeline.
 */
enum TilesPipelineStageFlags : uint32_t {
    TILES_STAGE_NONE                         = 0u,

    // Stage 1: T_M1 TilingGenerationAndHexPartitionMetadataLogic (Generation & Boundary Partitioning)
    TILES_STAGE_GENERATION_PARTITION_METADATA= 1u << 0,

    // Stage 2: T_Q1 TilingLatticeCullAndMaskQueryLogic (Wave 2 Lattice & Spline2D Boundary Classification)
    TILES_STAGE_LATTICE_CULL_AND_MASK_QUERY  = 1u << 1,

    // Stage 3: T_T1 TilingInstanceTransform + T_T2 TilingBoundaryClipAndSeamStitch (Wave 3 Instance & Seam Clip)
    TILES_STAGE_INSTANCE_AND_SEAM_TRANSFORM  = 1u << 2,

    // Stage 4: T_T3 TilingHexizeAndBorderExtractTransformLogic (Macro-Hexagon & Border Loop Sync)
    TILES_STAGE_HEXIZE_AND_BORDER_EXTRACT    = 1u << 3,

    // Stage 5: T_T4 TilingQuotientAStarAndFlowFieldTransformLogic (Quotient A* & Eikonal Flow Field)
    TILES_STAGE_NAVIGATION_AND_FLOW_FIELD    = 1u << 4,

    // Preset masks:
    TILES_STAGE_GEOMETRY_FILL_MASK           =
        TILES_STAGE_GENERATION_PARTITION_METADATA |
        TILES_STAGE_LATTICE_CULL_AND_MASK_QUERY   |
        TILES_STAGE_INSTANCE_AND_SEAM_TRANSFORM   |
        TILES_STAGE_HEXIZE_AND_BORDER_EXTRACT,

    TILES_STAGE_DEFAULT_PIPELINE_MASK        =
        TILES_STAGE_GEOMETRY_FILL_MASK            |
        TILES_STAGE_NAVIGATION_AND_FLOW_FIELD,

    TILES_STAGE_ALL_MASK                     = 0x001Fu
};

/**
 * TilesNodeExecutionConfig
 * Optional overrides when instantiating any Tiles task node in a TaskGraphDOD.
 */
struct TilesNodeExecutionConfig {
    MemoryView view            = MemoryView::SingleElementView;
    MemoryStrategy strategy    = MemoryStrategy::FlatStrategy;
    MemoryTypes element_type   = MemoryTypes::INT32;
    QueryOp query_op           = QueryOp::CULL;
    int32_t execution_priority = 0;
    bool use_registry_factory  = false;
    std::span<const GrantPartPOD> custom_requirements{};
};

/**
 * TilesStageAnchor
 * Topological handle returned for each injected stage in a TaskGraphDOD.
 */
struct TilesStageAnchor {
    TilesPipelineStageFlags stage = TILES_STAGE_NONE;
    NodeID entry_node             = INVALID_ID;
    NodeID exit_node              = INVALID_ID;
    std::vector<NodeID> nodes;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return entry_node != INVALID_ID && exit_node != INVALID_ID;
    }
};

/**
 * TilesPipelineTopology
 * Describes the set of TilesStageAnchors injected into a TaskGraphDOD.
 */
struct TilesPipelineTopology {
    NodeID first_node = INVALID_ID;
    NodeID last_node  = INVALID_ID;
    std::vector<TilesStageAnchor> stages;

    [[nodiscard]] const TilesStageAnchor *get_stage_anchor(TilesPipelineStageFlags p_stage) const noexcept {
        for (const auto &anchor : stages) {
            if (anchor.stage == p_stage) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool has_stage(TilesPipelineStageFlags p_stage) const noexcept {
        const TilesStageAnchor *anchor = get_stage_anchor(p_stage);
        return anchor != nullptr && anchor->is_valid();
    }
};

/**
 * TileCapacityKind
 * Identifies which configurable capacity field in TilesBufferCapacities governs a slot.
 */
enum class TileCapacityKind : uint8_t {
    PROTOTILES = 0,
    CELLS,
    CELL_CSR_INDICES,
    EDGES,
    BRIDGE_QUADS,
    HEX_SUPER_CELLS,
    HEX_CHILD_CSR,
    NAV_REQUESTS,
    NAV_PATH_CELLS,
    BRIDGED_GEOM_POLYLINES,
    BRIDGED_GEOM_POLYLINE_VERTICES_2D,
    BRIDGED_GEOM_POLYGON_MESHES,
    BRIDGED_GEOM_POLYGON_VERTICES_2D,
    BRIDGED_GEOM_POLYGON_INDICES
};

/**
 * TileTableGroup
 * Logical table ownership tag for each buffer slot, enabling TileDOD to synchronize
 * active selection counts in a single loop over TILE_BUFFER_SPECS.
 */
enum class TileTableGroup : uint8_t {
    PROTOTILE_DESCRIPTOR_TABLE = 0,
    ISO_CELL_TABLE,
    CELL_VERTEX_CSR_SLICE,
    CELL_EDGE_CSR_SLICE,
    EDGE_TABLE,
    BRIDGE_QUAD_TABLE,
    HEX_SUPER_CELL_TABLE,
    HEX_CHILD_CSR_SLICE,
    NAV_REQUEST_TABLE,
    NAV_PATH_SLICE,
    BRIDGED_GEOM_POLYLINE_SLICE,
    BRIDGED_GEOM_POLYLINE_VERTEX_2D_SLICE,
    BRIDGED_GEOM_POLYGON_MESH_SLICE,
    BRIDGED_GEOM_POLYGON_VERTEX_2D_SLICE,
    BRIDGED_GEOM_POLYGON_INDEX_SLICE
};

/**
 * TilesBufferCapacities
 * Configurable row and slice capacities for TileDOD tables.
 */
struct TilesBufferCapacities {
    uint32_t max_prototiles                  = 64;
    uint32_t max_cells                       = 16384;
    uint32_t max_cell_csr_indices            = 98304; // 16384 cells * up to 6 vertices/edges
    uint32_t max_edges                       = 49152;
    uint32_t max_bridge_quads                = 2048;
    uint32_t max_hex_super_cells             = 4096;
    uint32_t max_hex_child_csr               = 49152;
    uint32_t max_nav_requests                = 512;
    uint32_t max_nav_path_cells              = 16384;
    uint32_t max_bridged_polylines           = 1024;
    uint32_t max_bridged_polyline_vertices   = 32768;
    uint32_t max_bridged_polygon_meshes      = 1024;
    uint32_t max_bridged_polygon_vertices    = 32768;
    uint32_t max_bridged_polygon_indices     = 98304;

    [[nodiscard]] constexpr uint32_t get_capacity(TileCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case TileCapacityKind::PROTOTILES:                        return max_prototiles;
            case TileCapacityKind::CELLS:                             return max_cells;
            case TileCapacityKind::CELL_CSR_INDICES:                  return max_cell_csr_indices;
            case TileCapacityKind::EDGES:                             return max_edges;
            case TileCapacityKind::BRIDGE_QUADS:                      return max_bridge_quads;
            case TileCapacityKind::HEX_SUPER_CELLS:                   return max_hex_super_cells;
            case TileCapacityKind::HEX_CHILD_CSR:                     return max_hex_child_csr;
            case TileCapacityKind::NAV_REQUESTS:                      return max_nav_requests;
            case TileCapacityKind::NAV_PATH_CELLS:                    return max_nav_path_cells;
            case TileCapacityKind::BRIDGED_GEOM_POLYLINES:            return max_bridged_polylines;
            case TileCapacityKind::BRIDGED_GEOM_POLYLINE_VERTICES_2D: return max_bridged_polyline_vertices;
            case TileCapacityKind::BRIDGED_GEOM_POLYGON_MESHES:       return max_bridged_polygon_meshes;
            case TileCapacityKind::BRIDGED_GEOM_POLYGON_VERTICES_2D:  return max_bridged_polygon_vertices;
            case TileCapacityKind::BRIDGED_GEOM_POLYGON_INDICES:      return max_bridged_polygon_indices;
            default:                                                  return max_cells;
        }
    }
};

/**
 * TileBufferSlot
 * Enumerates every DOD buffer column in TileDOD (including the 6 bridged GeometryDOD slots).
 */
enum class TileBufferSlot : uint8_t {
    // --- Table 1: Prototile & Symmetry Group Descriptor Table ---
    TILE_PROTOTILE_DESCRIPTOR_POD = 0,       // BakedPrototilePOD<float> (AoS)

    // --- Table 2: Fundamental Cell, Instance Transform & CSR Topology Slices ---
    TILE_ISO_CELL_POD,                       // IsoCellPOD (32B AoS)
    TILE_INSTANCE_TRANSFORM_POD,             // TileTransformPOD (32B AoS)
    TILE_CELL_VERTEX_INDEX_CSR,              // uint32_t (4B Flat: indices into BRIDGED_GEOM_POLYGON_VERTEX_2D)
    TILE_CELL_EDGE_INDEX_CSR,                // uint32_t (4B Flat: indices into TILE_EDGE_POD)
    TILE_CELL_NEIGHBOR_INDEX_CSR,            // int32_t  (4B Flat: neighbor IsoCellPOD indices)

    // --- Table 3: Shared Topological Edges, Edge Metadata & Seam Bridge Quads ---
    TILE_EDGE_POD,                           // TileEdgePOD (16B AoS)
    TILE_EDGE_METADATA_POD,                  // TileEdgeMetadataPOD (16B AoS)
    TILE_BRIDGE_QUAD_POD,                    // BridgeQuadCellPOD (32B AoS)

    // --- Table 4: Dual-Level Hexagonal Super-Cells (`Hexize`) ---
    TILE_HEX_SUPER_CELL_POD,                 // HexSuperCellPOD (64B AoS)
    TILE_HEX_CHILD_CELL_CSR,                 // uint32_t (4B Flat: child IsoCellPOD indices)

    // --- Table 5: Symmetry-Accelerated Navigation, A* Pathfinding & Eikonal Flow Field ---
    TILE_NAV_REQUEST_POD,                    // TileNavRequestPOD (32B AoS)
    TILE_NAV_PATH_CELL_U32,                  // uint32_t (4B Flat: output path cell IDs)
    TILE_FLOW_COST_FLOAT32,                  // float (4B Flat: per-cell wavefront cost)
    TILE_FLOW_VECTOR_VEC2,                   // glm::vec2 (8B Flat: per-cell normalized steering vector)

    // --- Table 6: Bridged GeometryDOD Buffers (Bound via `bind_external_geometry_buffers`) ---
    BRIDGED_GEOM_POLYLINE_SLICE_POD,         // GeometryDOD::GEOM_POLYLINE_SLICE_POD (PolylineSlicePOD, 32B AoS)
    BRIDGED_GEOM_POLYLINE_VERTEX_2D,         // GeometryDOD::GEOM_POLYLINE_VERTEX_2D (glm::vec2, 8B Flat)
    BRIDGED_GEOM_POLYGON_MESH_SLICE_POD,     // GeometryDOD::GEOM_POLYGON_MESH_SLICE_POD (PolygonMeshSlicePOD, 32B AoS)
    BRIDGED_GEOM_POLYGON_VERTEX_2D,          // GeometryDOD::GEOM_POLYGON_VERTEX_2D (glm::vec2, 8B Flat)
    BRIDGED_GEOM_POLYGON_INDEX_U32,          // GeometryDOD::GEOM_POLYGON_INDEX_U32 (uint32_t, 4B Flat)
    BRIDGED_GEOM_CLIP_CLASSIFICATION_FLAGS,  // GeometryDOD::GEOM_CLIP_CLASSIFICATION_FLAGS (uint8_t, 1B Flat)

    COUNT
};

static constexpr size_t TILE_BUFFER_SLOT_COUNT = static_cast<size_t>(TileBufferSlot::COUNT);

/**
 * TileBufferSpec
 * Compile-time descriptor row in TILE_BUFFER_SPECS.
 */
struct TileBufferSpec {
    TileBufferSlot slot                     = TileBufferSlot::COUNT;
    std::string_view canonical_name         = {};
    std::array<std::string_view, 3> aliases = {};
    uint32_t element_stride                 = sizeof(uint32_t);
    BufferLayoutType layout                 = BufferLayoutType::FLAT;
    SelectionMode selection_mode            = SelectionMode::RANGE;
    TileCapacityKind capacity_kind          = TileCapacityKind::CELLS;
    TileTableGroup table_group              = TileTableGroup::ISO_CELL_TABLE;

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
 * TILE_BUFFER_SPECS
 * Compile-time manifest covering all 21 TileDOD buffer slots and property aliases.
 */
inline constexpr std::array<TileBufferSpec, TILE_BUFFER_SLOT_COUNT> TILE_BUFFER_SPECS = {{
    // --- Table 1: Prototile Descriptor Table ---
    {
        TileBufferSlot::TILE_PROTOTILE_DESCRIPTOR_POD,
        "prototile_descriptor_buffer_id",
        {"tile_prototile_descriptor_pod_buffer_id", "", ""},
        sizeof(BakedPrototilePOD<float>), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::PROTOTILES, TileTableGroup::PROTOTILE_DESCRIPTOR_TABLE
    },

    // --- Table 2: Fundamental Cell, Instance Transform & CSR Topology Slices ---
    {
        TileBufferSlot::TILE_ISO_CELL_POD,
        "iso_cell_buffer_id",
        {"tile_iso_cell_pod_buffer_id", "", ""},
        sizeof(IsoCellPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::CELLS, TileTableGroup::ISO_CELL_TABLE
    },
    {
        TileBufferSlot::TILE_INSTANCE_TRANSFORM_POD,
        "instance_transform_buffer_id",
        {"tile_instance_transform_pod_buffer_id", "", ""},
        sizeof(TileTransformPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::CELLS, TileTableGroup::ISO_CELL_TABLE
    },
    {
        TileBufferSlot::TILE_CELL_VERTEX_INDEX_CSR,
        "cell_vertex_csr_buffer_id",
        {"tile_cell_vertex_index_csr_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::CELL_CSR_INDICES, TileTableGroup::CELL_VERTEX_CSR_SLICE
    },
    {
        TileBufferSlot::TILE_CELL_EDGE_INDEX_CSR,
        "cell_edge_csr_buffer_id",
        {"tile_cell_edge_index_csr_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::CELL_CSR_INDICES, TileTableGroup::CELL_EDGE_CSR_SLICE
    },
    {
        TileBufferSlot::TILE_CELL_NEIGHBOR_INDEX_CSR,
        "cell_neighbor_csr_buffer_id",
        {"tile_cell_neighbor_index_csr_buffer_id", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::CELL_CSR_INDICES, TileTableGroup::CELL_EDGE_CSR_SLICE
    },

    // --- Table 3: Shared Topological Edges, Edge Metadata & Seam Bridge Quads ---
    {
        TileBufferSlot::TILE_EDGE_POD,
        "tile_edge_buffer_id",
        {"tile_edge_pod_buffer_id", "", ""},
        sizeof(TileEdgePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::EDGES, TileTableGroup::EDGE_TABLE
    },
    {
        TileBufferSlot::TILE_EDGE_METADATA_POD,
        "tile_edge_metadata_buffer_id",
        {"tile_edge_metadata_pod_buffer_id", "", ""},
        sizeof(TileEdgeMetadataPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::EDGES, TileTableGroup::EDGE_TABLE
    },
    {
        TileBufferSlot::TILE_BRIDGE_QUAD_POD,
        "bridge_quad_buffer_id",
        {"tile_bridge_quad_pod_buffer_id", "", ""},
        sizeof(BridgeQuadCellPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::BRIDGE_QUADS, TileTableGroup::BRIDGE_QUAD_TABLE
    },

    // --- Table 4: Dual-Level Hexagonal Super-Cells (`Hexize`) ---
    {
        TileBufferSlot::TILE_HEX_SUPER_CELL_POD,
        "hex_super_cell_buffer_id",
        {"tile_hex_super_cell_pod_buffer_id", "", ""},
        sizeof(HexSuperCellPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::HEX_SUPER_CELLS, TileTableGroup::HEX_SUPER_CELL_TABLE
    },
    {
        TileBufferSlot::TILE_HEX_CHILD_CELL_CSR,
        "hex_child_cell_csr_buffer_id",
        {"tile_hex_child_cell_csr_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::HEX_CHILD_CSR, TileTableGroup::HEX_CHILD_CSR_SLICE
    },

    // --- Table 5: Symmetry-Accelerated Navigation, A* Pathfinding & Eikonal Flow Field ---
    {
        TileBufferSlot::TILE_NAV_REQUEST_POD,
        "nav_request_buffer_id",
        {"tile_nav_request_pod_buffer_id", "", ""},
        sizeof(TileNavRequestPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::NAV_REQUESTS, TileTableGroup::NAV_REQUEST_TABLE
    },
    {
        TileBufferSlot::TILE_NAV_PATH_CELL_U32,
        "nav_path_cell_u32_buffer_id",
        {"tile_nav_path_cell_u32_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::NAV_PATH_CELLS, TileTableGroup::NAV_PATH_SLICE
    },
    {
        TileBufferSlot::TILE_FLOW_COST_FLOAT32,
        "flow_cost_float32_buffer_id",
        {"tile_flow_cost_float32_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::CELLS, TileTableGroup::ISO_CELL_TABLE
    },
    {
        TileBufferSlot::TILE_FLOW_VECTOR_VEC2,
        "flow_vector_vec2_buffer_id",
        {"tile_flow_vector_vec2_buffer_id", "", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::CELLS, TileTableGroup::ISO_CELL_TABLE
    },

    // --- Table 6: Bridged GeometryDOD Buffers (Matches GeometryDOD canonical names!) ---
    {
        TileBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD,
        "polyline_slice_buffer_id",
        {"geom_polyline_slice_pod_buffer_id", "bridged_geom_polyline_slice_buffer_id", ""},
        sizeof(PolylineSlicePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::BRIDGED_GEOM_POLYLINES, TileTableGroup::BRIDGED_GEOM_POLYLINE_SLICE
    },
    {
        TileBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_2D,
        "polyline_vertex_2d_buffer_id",
        {"geom_polyline_vertex_2d_buffer_id", "bridged_geom_polyline_vertex_2d_buffer_id", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::BRIDGED_GEOM_POLYLINE_VERTICES_2D, TileTableGroup::BRIDGED_GEOM_POLYLINE_VERTEX_2D_SLICE
    },
    {
        TileBufferSlot::BRIDGED_GEOM_POLYGON_MESH_SLICE_POD,
        "polygon_mesh_slice_buffer_id",
        {"geom_polygon_mesh_slice_pod_buffer_id", "bridged_geom_polygon_mesh_slice_buffer_id", ""},
        sizeof(PolygonMeshSlicePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        TileCapacityKind::BRIDGED_GEOM_POLYGON_MESHES, TileTableGroup::BRIDGED_GEOM_POLYGON_MESH_SLICE
    },
    {
        TileBufferSlot::BRIDGED_GEOM_POLYGON_VERTEX_2D,
        "polygon_vertex_2d_buffer_id",
        {"geom_polygon_vertex_2d_buffer_id", "bridged_geom_polygon_vertex_2d_buffer_id", ""},
        sizeof(glm::vec2), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::BRIDGED_GEOM_POLYGON_VERTICES_2D, TileTableGroup::BRIDGED_GEOM_POLYGON_VERTEX_2D_SLICE
    },
    {
        TileBufferSlot::BRIDGED_GEOM_POLYGON_INDEX_U32,
        "polygon_index_u32_buffer_id",
        {"geom_polygon_index_u32_buffer_id", "bridged_geom_polygon_index_u32_buffer_id", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::BRIDGED_GEOM_POLYGON_INDICES, TileTableGroup::BRIDGED_GEOM_POLYGON_INDEX_SLICE
    },
    {
        TileBufferSlot::BRIDGED_GEOM_CLIP_CLASSIFICATION_FLAGS,
        "clip_classification_buffer_id",
        {"geom_clip_classification_flags_buffer_id", "bridged_geom_clip_classification_buffer_id", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        TileCapacityKind::CELLS, TileTableGroup::ISO_CELL_TABLE
    }
}};

[[nodiscard]] consteval bool validate_tile_buffer_specs() noexcept {
    for (size_t i = 0; i < TILE_BUFFER_SLOT_COUNT; ++i) {
        if (static_cast<size_t>(TILE_BUFFER_SPECS[i].slot) != i) {
            return false;
        }
        if (TILE_BUFFER_SPECS[i].canonical_name.empty() || TILE_BUFFER_SPECS[i].element_stride == 0u) {
            return false;
        }
    }
    return true;
}

static_assert(validate_tile_buffer_specs(), "TILE_BUFFER_SPECS indices must match TileBufferSlot enum order!");

/**
 * TilesBufferManifest
 * Table-driven directory of all MemoryManagerDOD buffer IDs backing TileDOD.
 */
struct TilesBufferManifest {
    std::array<uint32_t, TILE_BUFFER_SLOT_COUNT> buffer_ids = []() consteval {
        std::array<uint32_t, TILE_BUFFER_SLOT_COUNT> arr{};
        arr.fill(INVALID_ID);
        return arr;
    }();

    [[nodiscard]] constexpr uint32_t &operator[](TileBufferSlot p_slot) noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr const uint32_t &operator[](TileBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr uint32_t get(TileBufferSlot p_slot) const noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        return (idx < TILE_BUFFER_SLOT_COUNT) ? buffer_ids[idx] : INVALID_ID;
    }

    constexpr void set(TileBufferSlot p_slot, uint32_t p_buffer_id) noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        if (idx < TILE_BUFFER_SLOT_COUNT) {
            buffer_ids[idx] = p_buffer_id;
        }
    }

    constexpr void reset() noexcept {
        buffer_ids.fill(INVALID_ID);
    }

    [[nodiscard]] constexpr uint32_t get_by_name(std::string_view p_name) const noexcept {
        for (size_t i = 0; i < TILE_BUFFER_SLOT_COUNT; ++i) {
            if (TILE_BUFFER_SPECS[i].matches_name(p_name)) {
                return buffer_ids[i];
            }
        }
        return INVALID_ID;
    }

    [[nodiscard]] constexpr bool is_initialized() const noexcept {
        return get(TileBufferSlot::TILE_PROTOTILE_DESCRIPTOR_POD) != INVALID_ID &&
               get(TileBufferSlot::TILE_ISO_CELL_POD) != INVALID_ID &&
               get(TileBufferSlot::TILE_INSTANCE_TRANSFORM_POD) != INVALID_ID &&
               get(TileBufferSlot::TILE_EDGE_POD) != INVALID_ID;
    }
};

using TileBufferManifest = TilesBufferManifest;

} // namespace ideam::core

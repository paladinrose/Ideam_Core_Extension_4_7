#include "tile_dod.h"

#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

constexpr size_t DEFAULT_TILES_MASTER_BLOCK_BYTES    = 32ULL * 1024ULL * 1024ULL; // 32 MB
constexpr size_t DEFAULT_TILES_TRANSIENT_BLOCK_BYTES = 8ULL * 1024ULL * 1024ULL;  // 8 MB

struct TileSlotAccessSpec {
    TileBufferSlot slot     = TileBufferSlot::COUNT;
    BufferAccessMode access = BufferAccessMode::READ;
};

template <typename T_Logic>
struct TileTaskDescriptor;

#define DEFINE_TILE_TASK_DESCRIPTOR(LogicStruct, EnumVal, S0, A0, S1, A1, S2, A2, S3, A3) \
    template <typename T>                                                                 \
    struct TileTaskDescriptor<LogicStruct<T>> {                                           \
        static constexpr auto logic_id = EnumVal;                                         \
        static constexpr std::array<TileSlotAccessSpec, 4> default_grants = {{            \
            {TileBufferSlot::S0, BufferAccessMode::A0},                                   \
            {TileBufferSlot::S1, BufferAccessMode::A1},                                   \
            {TileBufferSlot::S2, BufferAccessMode::A2},                                   \
            {TileBufferSlot::S3, BufferAccessMode::A3}                                    \
        }};                                                                               \
    }

DEFINE_TILE_TASK_DESCRIPTOR(
    TilingGenerationAndHexPartitionMetadataLogic,
    TilesMetadataLogicID::TilingGenerationAndHexPartition,
    TILE_ISO_CELL_POD, READ,
    TILE_INSTANCE_TRANSFORM_POD, READ,
    TILE_HEX_SUPER_CELL_POD, READ,
    TILE_BRIDGE_QUAD_POD, READ
);

DEFINE_TILE_TASK_DESCRIPTOR(
    TilingLatticeCullAndMaskQueryLogic,
    TilesQueryLogicID::TilingLatticeCullAndMask,
    TILE_ISO_CELL_POD, READ,
    TILE_CELL_VERTEX_INDEX_CSR, READ,
    BRIDGED_GEOM_POLYGON_VERTEX_2D, READ,
    BRIDGED_GEOM_CLIP_CLASSIFICATION_FLAGS, READ_WRITE
);

DEFINE_TILE_TASK_DESCRIPTOR(
    TilingInstanceTransformLogic,
    TilesTransformLogicID::TilingInstanceTransform,
    TILE_INSTANCE_TRANSFORM_POD, READ_WRITE,
    TILE_PROTOTILE_DESCRIPTOR_POD, READ,
    TILE_ISO_CELL_POD, READ,
    BRIDGED_GEOM_CLIP_CLASSIFICATION_FLAGS, READ
);

DEFINE_TILE_TASK_DESCRIPTOR(
    TilingBoundaryClipAndSeamStitchTransformLogic,
    TilesTransformLogicID::TilingBoundaryClipAndSeamStitch,
    TILE_ISO_CELL_POD, READ_WRITE,
    TILE_CELL_VERTEX_INDEX_CSR, READ,
    BRIDGED_GEOM_POLYGON_VERTEX_2D, READ_WRITE,
    TILE_BRIDGE_QUAD_POD, READ
);

DEFINE_TILE_TASK_DESCRIPTOR(
    TilingHexizeAndBorderExtractTransformLogic,
    TilesTransformLogicID::TilingHexizeAndBorderExtract,
    TILE_HEX_SUPER_CELL_POD, READ_WRITE,
    TILE_ISO_CELL_POD, READ,
    TILE_HEX_CHILD_CELL_CSR, READ,
    BRIDGED_GEOM_POLYGON_VERTEX_2D, READ
);

DEFINE_TILE_TASK_DESCRIPTOR(
    TilingQuotientAStarAndFlowFieldTransformLogic,
    TilesTransformLogicID::TilingQuotientAStarAndFlowField,
    TILE_NAV_REQUEST_POD, READ_WRITE,
    TILE_ISO_CELL_POD, READ,
    TILE_CELL_NEIGHBOR_INDEX_CSR, READ,
    TILE_NAV_PATH_CELL_U32, READ_WRITE
);

#undef DEFINE_TILE_TASK_DESCRIPTOR

} // anonymous namespace

TileDOD::TileDOD(
    std::shared_ptr<MemoryManagerDOD> p_memory_manager,
    const TilesBufferCapacities &p_capacities
)
    : memory_manager_(std::move(p_memory_manager)),
      capacities_(p_capacities) {
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_TILES_MASTER_BLOCK_BYTES,
            DEFAULT_TILES_TRANSIENT_BLOCK_BYTES
        );
    }
    default_task_graph_ = std::make_shared<TaskGraphDOD>(memory_manager_.get());
    default_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
    initialize_buffers(capacities_);
}

void TileDOD::initialize_buffers(const TilesBufferCapacities &p_capacities) {
    capacities_ = p_capacities;
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_TILES_MASTER_BLOCK_BYTES,
            DEFAULT_TILES_TRANSIENT_BLOCK_BYTES
        );
    }

    prototile_allocator_.initialize(capacities_.max_prototiles);

    for (size_t i = 0; i < TILE_BUFFER_SLOT_COUNT; ++i) {
        const TileBufferSpec &spec = TILE_BUFFER_SPECS[i];
        const size_t max_elems = std::max<size_t>(1u, capacities_.get_capacity(spec.capacity_kind));
        const size_t bytes     = max_elems * spec.element_stride;
        const uint32_t buf_id  = memory_manager_->create_shadowed_buffer(
            spec.layout, bytes, static_cast<int64_t>(max_elems), spec.selection_mode
        );
        if (MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id)) {
            buf->element_stride = spec.element_stride;
            buf->current_count  = 0;
            if (buf->master_block_ptr) {
                std::memset(buf->master_block_ptr + buf->memory_offset, 0, bytes);
            }
        }
        manifest_.buffer_ids[i] = buf_id;
    }

    has_external_geometry_buffers_ = false;
    reset_all_slots();
}

uint32_t TileDOD::_active_count_for_group(TileTableGroup p_group) const noexcept {
    switch (p_group) {
        case TileTableGroup::PROTOTILE_DESCRIPTOR_TABLE:            return prototile_allocator_.get_high_water_mark();
        case TileTableGroup::ISO_CELL_TABLE:                        return cell_cursor_;
        case TileTableGroup::CELL_VERTEX_CSR_SLICE:                 return cell_vertex_csr_cursor_;
        case TileTableGroup::CELL_EDGE_CSR_SLICE:                   return cell_edge_csr_cursor_;
        case TileTableGroup::EDGE_TABLE:                            return edge_cursor_;
        case TileTableGroup::BRIDGE_QUAD_TABLE:                     return bridge_quad_cursor_;
        case TileTableGroup::HEX_SUPER_CELL_TABLE:                  return hex_super_cell_cursor_;
        case TileTableGroup::HEX_CHILD_CSR_SLICE:                   return hex_child_csr_cursor_;
        case TileTableGroup::NAV_REQUEST_TABLE:                     return nav_request_cursor_;
        case TileTableGroup::NAV_PATH_SLICE:                        return nav_path_cursor_;
        case TileTableGroup::BRIDGED_GEOM_POLYLINE_SLICE:           return bridged_polyline_cursor_;
        case TileTableGroup::BRIDGED_GEOM_POLYLINE_VERTEX_2D_SLICE: return bridged_polyline_vertex_cursor_;
        case TileTableGroup::BRIDGED_GEOM_POLYGON_MESH_SLICE:       return bridged_polygon_mesh_cursor_;
        case TileTableGroup::BRIDGED_GEOM_POLYGON_VERTEX_2D_SLICE:  return bridged_polygon_vertex_cursor_;
        case TileTableGroup::BRIDGED_GEOM_POLYGON_INDEX_SLICE:      return bridged_polygon_index_cursor_;
        default:                                                    return 0u;
    }
}

void TileDOD::sync_selection_ranges() noexcept {
    if (!memory_manager_) return;
    for (size_t i = 0; i < TILE_BUFFER_SLOT_COUNT; ++i) {
        MemoryBufferPOD *buf = memory_manager_->get_buffer(manifest_.buffer_ids[i]);
        if (!buf) continue;
        const TileBufferSpec &spec = TILE_BUFFER_SPECS[i];
        const int64_t desired = static_cast<int64_t>(_active_count_for_group(spec.table_group));
        if (has_external_geometry_buffers_ &&
            (spec.table_group == TileTableGroup::BRIDGED_GEOM_POLYLINE_SLICE ||
             spec.table_group == TileTableGroup::BRIDGED_GEOM_POLYLINE_VERTEX_2D_SLICE ||
             spec.table_group == TileTableGroup::BRIDGED_GEOM_POLYGON_MESH_SLICE ||
             spec.table_group == TileTableGroup::BRIDGED_GEOM_POLYGON_VERTEX_2D_SLICE ||
             spec.table_group == TileTableGroup::BRIDGED_GEOM_POLYGON_INDEX_SLICE)) {
            buf->current_count = std::min<int64_t>(std::max<int64_t>(buf->current_count, desired), buf->max_elements);
        } else {
            buf->current_count = std::min<int64_t>(desired, buf->max_elements);
        }
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void TileDOD::reset_all_slots() noexcept {
    prototile_allocator_.clear();
    cell_cursor_                    = 0u;
    cell_vertex_csr_cursor_         = 0u;
    cell_edge_csr_cursor_           = 0u;
    edge_cursor_                    = 0u;
    bridge_quad_cursor_             = 0u;
    hex_super_cell_cursor_          = 0u;
    hex_child_csr_cursor_           = 0u;
    nav_request_cursor_             = 0u;
    nav_path_cursor_                = 0u;
    bridged_polyline_cursor_        = 0u;
    bridged_polyline_vertex_cursor_ = 0u;
    bridged_polygon_mesh_cursor_    = 0u;
    bridged_polygon_vertex_cursor_  = 0u;
    bridged_polygon_index_cursor_   = 0u;

    sync_selection_ranges();
}

void TileDOD::populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing) const {
    auto set_key = [&](std::string_view p_key, uint32_t p_val) {
        if (p_key.empty() || p_val == INVALID_ID) return;
        const godot::String key_str(std::string(p_key).c_str());
        if (p_overwrite_existing || !r_props.has(key_str)) {
            r_props[key_str] = static_cast<int64_t>(p_val);
        }
    };
    for (size_t i = 0; i < TILE_BUFFER_SLOT_COUNT; ++i) {
        const uint32_t buf_id = manifest_.buffer_ids[i];
        set_key(TILE_BUFFER_SPECS[i].canonical_name, buf_id);
        for (std::string_view alias : TILE_BUFFER_SPECS[i].aliases) {
            set_key(alias, buf_id);
        }
    }
}

GrantPartPOD TileDOD::make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept {
    if (memory_manager_ && p_buffer_id != INVALID_ID) {
        if (const MemoryBufferPOD *buf = memory_manager_->get_buffer(p_buffer_id)) {
            return make_grant_requirement(p_buffer_id, p_access, buf->element_stride, buf->max_elements);
        }
    }
    return make_grant_requirement(p_buffer_id, p_access, sizeof(uint32_t), 0);
}

void TileDOD::bind_external_geometry_buffers(
    uint32_t p_polyline_slice_buf_id,
    uint32_t p_polyline_vertex_2d_buf_id,
    uint32_t p_polygon_mesh_slice_buf_id,
    uint32_t p_polygon_vertex_2d_buf_id,
    uint32_t p_polygon_index_u32_buf_id,
    uint32_t p_clip_classification_buf_id
) noexcept {
    if (p_polyline_slice_buf_id != INVALID_ID) {
        manifest_.set(TileBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD, p_polyline_slice_buf_id);
        has_external_geometry_buffers_ = true;
    }
    if (p_polyline_vertex_2d_buf_id != INVALID_ID) {
        manifest_.set(TileBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_2D, p_polyline_vertex_2d_buf_id);
        has_external_geometry_buffers_ = true;
    }
    if (p_polygon_mesh_slice_buf_id != INVALID_ID) {
        manifest_.set(TileBufferSlot::BRIDGED_GEOM_POLYGON_MESH_SLICE_POD, p_polygon_mesh_slice_buf_id);
        has_external_geometry_buffers_ = true;
    }
    if (p_polygon_vertex_2d_buf_id != INVALID_ID) {
        manifest_.set(TileBufferSlot::BRIDGED_GEOM_POLYGON_VERTEX_2D, p_polygon_vertex_2d_buf_id);
        has_external_geometry_buffers_ = true;
    }
    if (p_polygon_index_u32_buf_id != INVALID_ID) {
        manifest_.set(TileBufferSlot::BRIDGED_GEOM_POLYGON_INDEX_U32, p_polygon_index_u32_buf_id);
        has_external_geometry_buffers_ = true;
    }
    if (p_clip_classification_buf_id != INVALID_ID) {
        manifest_.set(TileBufferSlot::BRIDGED_GEOM_CLIP_CLASSIFICATION_FLAGS, p_clip_classification_buf_id);
    }
}

// ============================================================================
// Prototile & Region Baking (`bake_prototile`, `bake_region`)
// ============================================================================

template <FloatingPointScalar T>
BakedPrototilePOD<float> TileDOD::_convert_prototile_to_float(const BakedPrototilePOD<T> &p_src) noexcept {
    if constexpr (std::is_same_v<T, float>) {
        return p_src;
    } else {
        BakedPrototilePOD<float> out{};
        out.wallpaper_group = p_src.wallpaper_group;
        out.tactile_ih_type = p_src.tactile_ih_type;
        out.num_vertices    = p_src.num_vertices;
        out.num_aspects     = p_src.num_aspects;
        out.num_params      = p_src.num_params;
        out.num_edge_shapes = p_src.num_edge_shapes;
        out.shape_preset    = p_src.shape_preset;

        out.lattice_basis.type        = p_src.lattice_basis.type;
        out.lattice_basis.u_axis      = glm::vec2(p_src.lattice_basis.u_axis);
        out.lattice_basis.v_axis      = glm::vec2(p_src.lattice_basis.v_axis);
        out.lattice_basis.origin      = glm::vec2(p_src.lattice_basis.origin);
        out.lattice_basis.basis_mat   = glm::mat2(p_src.lattice_basis.basis_mat);
        out.lattice_basis.inv_basis_mat = glm::mat2(p_src.lattice_basis.inv_basis_mat);

        for (size_t i = 0; i < BakedPrototilePOD<float>::MAX_PARAMETERS; ++i) {
            out.parameters[i] = static_cast<float>(p_src.parameters[i]);
        }
        for (size_t i = 0; i < BakedPrototilePOD<float>::MAX_VERTICES; ++i) {
            out.base_vertices[i]              = glm::vec2(p_src.base_vertices[i]);
            out.edges[i].edge_shape_id        = p_src.edges[i].edge_shape_id;
            out.edges[i].shape                = p_src.edges[i].shape;
            out.edges[i].reversed             = p_src.edges[i].reversed;
            out.edges[i].start_vertex         = glm::vec2(p_src.edges[i].start_vertex);
            out.edges[i].end_vertex           = glm::vec2(p_src.edges[i].end_vertex);
            out.edges[i].canonical_to_edge_xform = glm::mat3(p_src.edges[i].canonical_to_edge_xform);
        }
        for (size_t a = 0; a < BakedPrototilePOD<float>::MAX_ASPECTS; ++a) {
            out.aspect_xforms[a]        = glm::mat3(p_src.aspect_xforms[a]);
            out.inv_aspect_xforms[a]    = glm::mat3(p_src.inv_aspect_xforms[a]);
            out.aspect_local_centers[a] = glm::vec2(p_src.aspect_local_centers[a]);
            out.aspect_angles_rad[a]    = static_cast<float>(p_src.aspect_angles_rad[a]);
            out.aspect_scales[a]        = glm::vec2(p_src.aspect_scales[a]);
            out.aspect_orientations[a]  = p_src.aspect_orientations[a];
        }
        out.colouring_table    = p_src.colouring_table;
        out.quotient_adjacency = p_src.quotient_adjacency;
        return out;
    }
}

template <FloatingPointScalar T>
uint32_t TileDOD::bake_prototile(const IsohedralTiling2D<T> &p_tiling) {
    const uint32_t slot = prototile_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    auto protos = get_column_span<BakedPrototilePOD<float>>(TileBufferSlot::TILE_PROTOTILE_DESCRIPTOR_POD);
    if (slot < protos.size()) {
        protos[slot] = _convert_prototile_to_float(p_tiling.get_baked_prototile());
    }
    sync_selection_ranges();
    return slot;
}

template <FloatingPointScalar T>
uint32_t TileDOD::bake_region(
    const TilingRegion2D<T> &p_region,
    GeometryDOD *r_optional_linked_geometry_dod
) {
    (void)r_optional_linked_geometry_dod;
    const uint32_t proto_slot = bake_prototile(p_region.get_tiling());
    if (proto_slot == INVALID_ID) return INVALID_ID;

    // 1. Copy shared 2D vertices into BRIDGED_GEOM_POLYGON_VERTEX_2D
    const uint32_t base_vtx_offset = bridged_polygon_vertex_cursor_;
    const auto src_verts           = p_region.get_vertices();
    auto dst_verts                 = get_column_span<glm::vec2>(TileBufferSlot::BRIDGED_GEOM_POLYGON_VERTEX_2D);
    const uint32_t write_verts     = (base_vtx_offset < dst_verts.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_verts.size()), static_cast<uint32_t>(dst_verts.size()) - base_vtx_offset)
        : 0u;
    for (uint32_t i = 0; i < write_verts; ++i) {
        dst_verts[base_vtx_offset + i] = glm::vec2(static_cast<float>(src_verts[i].x), static_cast<float>(src_verts[i].y));
    }
    bridged_polygon_vertex_cursor_ += write_verts;

    // 2. Copy triangle indices into BRIDGED_GEOM_POLYGON_INDEX_U32 and record a PolygonMeshSlicePOD
    const uint32_t base_idx_offset = bridged_polygon_index_cursor_;
    const auto src_tris            = p_region.get_triangle_indices();
    auto dst_tris                  = get_column_span<uint32_t>(TileBufferSlot::BRIDGED_GEOM_POLYGON_INDEX_U32);
    const uint32_t write_tris      = (base_idx_offset < dst_tris.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_tris.size()), static_cast<uint32_t>(dst_tris.size()) - base_idx_offset)
        : 0u;
    for (uint32_t i = 0; i < write_tris; ++i) {
        dst_tris[base_idx_offset + i] = src_tris[i] + base_vtx_offset;
    }
    bridged_polygon_index_cursor_ += write_tris;

    auto mesh_slices = get_column_span<PolygonMeshSlicePOD>(TileBufferSlot::BRIDGED_GEOM_POLYGON_MESH_SLICE_POD);
    if (bridged_polygon_mesh_cursor_ < mesh_slices.size()) {
        PolygonMeshSlicePOD ms{};
        ms.vertex_offset  = base_vtx_offset;
        ms.vertex_count   = write_verts;
        ms.index_offset   = base_idx_offset;
        ms.triangle_count = write_tris / 3u;
        mesh_slices[bridged_polygon_mesh_cursor_] = ms;
        ++bridged_polygon_mesh_cursor_;
    }

    // 3. Copy CSR cell vertex indices, edge indices, and neighbor indices
    const uint32_t base_vcsr_offset = cell_vertex_csr_cursor_;
    const auto src_vcsr             = p_region.get_cell_vertex_indices();
    auto dst_vcsr                   = get_column_span<uint32_t>(TileBufferSlot::TILE_CELL_VERTEX_INDEX_CSR);
    const uint32_t write_vcsr       = (base_vcsr_offset < dst_vcsr.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_vcsr.size()), static_cast<uint32_t>(dst_vcsr.size()) - base_vcsr_offset)
        : 0u;
    for (uint32_t i = 0; i < write_vcsr; ++i) {
        dst_vcsr[base_vcsr_offset + i] = src_vcsr[i] + base_vtx_offset;
    }
    cell_vertex_csr_cursor_ += write_vcsr;

    const uint32_t base_ecsr_offset = cell_edge_csr_cursor_;
    const uint32_t base_edge_offset = edge_cursor_;
    const uint32_t base_cell_offset = cell_cursor_;

    const auto src_ecsr = p_region.get_cell_edge_indices();
    const auto src_ncsr = p_region.get_cell_neighbor_indices();
    auto dst_ecsr       = get_column_span<uint32_t>(TileBufferSlot::TILE_CELL_EDGE_INDEX_CSR);
    auto dst_ncsr       = get_column_span<int32_t>(TileBufferSlot::TILE_CELL_NEIGHBOR_INDEX_CSR);
    const uint32_t write_ecsr = (base_ecsr_offset < dst_ecsr.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_ecsr.size()), static_cast<uint32_t>(dst_ecsr.size()) - base_ecsr_offset)
        : 0u;
    for (uint32_t i = 0; i < write_ecsr; ++i) {
        dst_ecsr[base_ecsr_offset + i] = src_ecsr[i] + base_edge_offset;
        if (i < src_ncsr.size() && base_ecsr_offset + i < dst_ncsr.size()) {
            dst_ncsr[base_ecsr_offset + i] = (src_ncsr[i] >= 0)
                ? (src_ncsr[i] + static_cast<int32_t>(base_cell_offset))
                : -1;
        }
    }
    cell_edge_csr_cursor_ += write_ecsr;

    // 4. Copy IsoCellPOD & TileTransformPOD
    const auto src_cells  = p_region.get_cells();
    const auto src_xforms = p_region.get_cell_transforms();
    auto dst_cells        = get_column_span<IsoCellPOD>(TileBufferSlot::TILE_ISO_CELL_POD);
    auto dst_xforms       = get_column_span<TileTransformPOD>(TileBufferSlot::TILE_INSTANCE_TRANSFORM_POD);
    const uint32_t write_cells = (base_cell_offset < dst_cells.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_cells.size()), static_cast<uint32_t>(dst_cells.size()) - base_cell_offset)
        : 0u;

    for (uint32_t i = 0; i < write_cells; ++i) {
        IsoCellPOD c = src_cells[i];
        c.group_slot = static_cast<uint8_t>(proto_slot);
        if (c.first_vertex_idx != INVALID_ID) c.first_vertex_idx += base_vcsr_offset;
        if (c.first_edge_idx != INVALID_ID)   c.first_edge_idx   += base_ecsr_offset;
        if (c.super_hex_id != INVALID_ID)     c.super_hex_id     += hex_super_cell_cursor_;
        dst_cells[base_cell_offset + i] = c;

        if (i < src_xforms.size() && base_cell_offset + i < dst_xforms.size()) {
            TileTransformPOD xf = src_xforms[i];
            xf.cell_id          = base_cell_offset + i;
            dst_xforms[base_cell_offset + i] = xf;
        }
    }
    cell_cursor_ += write_cells;

    // 5. Copy TileEdgePOD & TileEdgeMetadataPOD
    const auto src_edges = p_region.get_edges();
    const auto src_metas = p_region.get_edge_metadata();
    auto dst_edges       = get_column_span<TileEdgePOD>(TileBufferSlot::TILE_EDGE_POD);
    auto dst_metas       = get_column_span<TileEdgeMetadataPOD>(TileBufferSlot::TILE_EDGE_METADATA_POD);
    const uint32_t write_edges = (base_edge_offset < dst_edges.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_edges.size()), static_cast<uint32_t>(dst_edges.size()) - base_edge_offset)
        : 0u;
    for (uint32_t i = 0; i < write_edges; ++i) {
        TileEdgePOD e = src_edges[i];
        e.vertex_a += base_vtx_offset;
        e.vertex_b += base_vtx_offset;
        if (e.cell_a >= 0) e.cell_a += static_cast<int32_t>(base_cell_offset);
        if (e.cell_b >= 0) e.cell_b += static_cast<int32_t>(base_cell_offset);
        dst_edges[base_edge_offset + i] = e;
        if (i < src_metas.size() && base_edge_offset + i < dst_metas.size()) {
            dst_metas[base_edge_offset + i] = src_metas[i];
        }
    }
    edge_cursor_ += write_edges;

    // 6. Copy HexSuperCellPOD & child cell CSR
    const uint32_t base_hex_offset  = hex_super_cell_cursor_;
    const uint32_t base_hcsr_offset = hex_child_csr_cursor_;
    const auto src_hcsr             = p_region.get_hex_child_cell_indices();
    auto dst_hcsr                   = get_column_span<uint32_t>(TileBufferSlot::TILE_HEX_CHILD_CELL_CSR);
    const uint32_t write_hcsr       = (base_hcsr_offset < dst_hcsr.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_hcsr.size()), static_cast<uint32_t>(dst_hcsr.size()) - base_hcsr_offset)
        : 0u;
    for (uint32_t i = 0; i < write_hcsr; ++i) {
        dst_hcsr[base_hcsr_offset + i] = src_hcsr[i] + base_cell_offset;
    }
    hex_child_csr_cursor_ += write_hcsr;

    const auto src_hex = p_region.get_hex_super_cells();
    auto dst_hex       = get_column_span<HexSuperCellPOD>(TileBufferSlot::TILE_HEX_SUPER_CELL_POD);
    const uint32_t write_hex = (base_hex_offset < dst_hex.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_hex.size()), static_cast<uint32_t>(dst_hex.size()) - base_hex_offset)
        : 0u;
    for (uint32_t i = 0; i < write_hex; ++i) {
        HexSuperCellPOD h = src_hex[i];
        if (h.center_vertex_id != INVALID_ID) h.center_vertex_id += base_vtx_offset;
        h.set_child_slice(h.get_child_offset() + base_hcsr_offset, h.get_child_count());
        for (size_t k = 0; k < 6; ++k) {
            if (h.perimeter_vertex_ids[k] != INVALID_ID) h.perimeter_vertex_ids[k] += base_vtx_offset;
            if (h.neighbor_hex_ids[k] >= 0) h.neighbor_hex_ids[k] += static_cast<int32_t>(base_hex_offset);
        }
        dst_hex[base_hex_offset + i] = h;
    }
    hex_super_cell_cursor_ += write_hex;

    // 7. Copy BridgeQuadCellPODs
    const uint32_t base_bq_offset = bridge_quad_cursor_;
    const auto src_bq             = p_region.get_bridge_quads();
    auto dst_bq                   = get_column_span<BridgeQuadCellPOD>(TileBufferSlot::TILE_BRIDGE_QUAD_POD);
    const uint32_t write_bq       = (base_bq_offset < dst_bq.size())
        ? std::min<uint32_t>(static_cast<uint32_t>(src_bq.size()), static_cast<uint32_t>(dst_bq.size()) - base_bq_offset)
        : 0u;
    for (uint32_t i = 0; i < write_bq; ++i) {
        BridgeQuadCellPOD bq = src_bq[i];
        if (bq.cell_id != INVALID_ID)             bq.cell_id             += base_cell_offset;
        if (bq.dominant_cell_id != INVALID_ID)    bq.dominant_cell_id    += base_cell_offset;
        if (bq.subordinate_cell_id != INVALID_ID) bq.subordinate_cell_id += base_cell_offset;
        for (size_t k = 0; k < 4; ++k) {
            if (bq.quad_vertex_ids[k] != INVALID_ID) bq.quad_vertex_ids[k] += base_vtx_offset;
        }
        dst_bq[base_bq_offset + i] = bq;
    }
    bridge_quad_cursor_ += write_bq;

    sync_selection_ranges();
    return proto_slot;
}

template uint32_t TileDOD::bake_prototile<float>(const IsohedralTiling2D<float> &);
template uint32_t TileDOD::bake_prototile<double>(const IsohedralTiling2D<double> &);
template uint32_t TileDOD::bake_region<float>(const TilingRegion2D<float> &, GeometryDOD *);
template uint32_t TileDOD::bake_region<double>(const TilingRegion2D<double> &, GeometryDOD *);

// ============================================================================
// Readback & Navigation Staging Helpers
// ============================================================================

std::span<const IsoCellPOD> TileDOD::get_active_cells() const noexcept {
    auto sp = get_column_span<const IsoCellPOD>(TileBufferSlot::TILE_ISO_CELL_POD);
    return sp.subspan(0, std::min<size_t>(cell_cursor_, sp.size()));
}

std::span<const TileTransformPOD> TileDOD::get_active_instance_transforms() const noexcept {
    auto sp = get_column_span<const TileTransformPOD>(TileBufferSlot::TILE_INSTANCE_TRANSFORM_POD);
    return sp.subspan(0, std::min<size_t>(cell_cursor_, sp.size()));
}

std::span<const HexSuperCellPOD> TileDOD::get_active_hex_super_cells() const noexcept {
    auto sp = get_column_span<const HexSuperCellPOD>(TileBufferSlot::TILE_HEX_SUPER_CELL_POD);
    return sp.subspan(0, std::min<size_t>(hex_super_cell_cursor_, sp.size()));
}

void TileDOD::stage_navigation_requests(std::span<const TileNavRequestPOD> p_requests) {
    auto dst = get_column_span<TileNavRequestPOD>(TileBufferSlot::TILE_NAV_REQUEST_POD);
    const uint32_t count = std::min<uint32_t>(static_cast<uint32_t>(p_requests.size()), static_cast<uint32_t>(dst.size()));
    uint32_t path_off = 0u;
    for (uint32_t i = 0; i < count; ++i) {
        TileNavRequestPOD req = p_requests[i];
        if (req.max_path_capacity == 0u) req.max_path_capacity = 64u;
        req.path_slice_offset = path_off;
        path_off += req.max_path_capacity;
        dst[i] = req;
    }
    nav_request_cursor_ = count;
    nav_path_cursor_    = std::min<uint32_t>(path_off, capacities_.max_nav_path_cells);
    sync_selection_ranges();
}

std::span<const TileNavRequestPOD> TileDOD::get_navigation_requests() const noexcept {
    auto sp = get_column_span<const TileNavRequestPOD>(TileBufferSlot::TILE_NAV_REQUEST_POD);
    return sp.subspan(0, std::min<size_t>(nav_request_cursor_, sp.size()));
}

std::span<const uint32_t> TileDOD::get_navigation_path_cells(uint32_t p_request_index) const noexcept {
    auto reqs  = get_navigation_requests();
    auto paths = get_column_span<const uint32_t>(TileBufferSlot::TILE_NAV_PATH_CELL_U32);
    if (p_request_index >= reqs.size()) return {};
    const TileNavRequestPOD &r = reqs[p_request_index];
    if (r.out_path_length == 0u || r.path_slice_offset + r.out_path_length > paths.size()) return {};
    return paths.subspan(r.path_slice_offset, r.out_path_length);
}

std::span<const float> TileDOD::get_flow_field_costs() const noexcept {
    auto sp = get_column_span<const float>(TileBufferSlot::TILE_FLOW_COST_FLOAT32);
    return sp.subspan(0, std::min<size_t>(cell_cursor_, sp.size()));
}

std::span<const glm::vec2> TileDOD::get_flow_field_vectors() const noexcept {
    auto sp = get_column_span<const glm::vec2>(TileBufferSlot::TILE_FLOW_VECTOR_VEC2);
    return sp.subspan(0, std::min<size_t>(cell_cursor_, sp.size()));
}

// ============================================================================
// Task Node Builders & Pipeline Stage Injection
// ============================================================================

void TileDOD::_bind_default_logic_buffers(
    void *r_logic_bytes,
    size_t p_size,
    const void *p_default_bytes
) const noexcept {
    if (!r_logic_bytes || !p_default_bytes || p_size == 0) return;

    godot::Dictionary dict;
    populate_task_buffer_dictionary(dict, true);

    TilingGenerationAndHexPartitionMetadataLogic<> dummy_m{};
    (void)dummy_m;
}

template <typename T_Logic>
NodeID TileDOD::add_tile_task_node(
    TaskGraphDOD &r_graph,
    const T_Logic &p_logic_override,
    const TilesNodeExecutionConfig &p_config
) const {
    using Desc = TileTaskDescriptor<T_Logic>;
    T_Logic configured_logic = p_logic_override;

    godot::Dictionary auto_props;
    populate_task_buffer_dictionary(auto_props, false);
    configured_logic.apply_properties(auto_props);

    std::vector<GrantPartPOD> reqs;
    if (!p_config.custom_requirements.empty()) {
        reqs.assign(p_config.custom_requirements.begin(), p_config.custom_requirements.end());
    } else {
        for (const TileSlotAccessSpec &gs : Desc::default_grants) {
            if (gs.slot != TileBufferSlot::COUNT) {
                const uint32_t bid = manifest_.get(gs.slot);
                if (bid != INVALID_ID) {
                    reqs.push_back(make_buffer_requirement(bid, gs.access));
                }
            }
        }
    }

    std::unique_ptr<INativeTask> task;
    using LogicIDType = std::decay_t<decltype(Desc::logic_id)>;
    if constexpr (std::is_same_v<LogicIDType, TilesMetadataLogicID>) {
        auto typed = std::make_unique<MetadataTask<T_Logic>>(configured_logic);
        task       = std::move(typed);
    } else if constexpr (std::is_same_v<LogicIDType, TilesQueryLogicID>) {
        if (p_config.query_op == QueryOp::ADD) {
            auto typed = std::make_unique<QueryTask<T_Logic, QueryOp::ADD>>(configured_logic);
            task       = std::move(typed);
        } else {
            auto typed = std::make_unique<QueryTask<T_Logic, QueryOp::CULL>>(configured_logic);
            task       = std::move(typed);
        }
    } else {
        auto typed = std::make_unique<TransformTask<T_Logic>>(configured_logic);
        task       = std::move(typed);
    }

    if (!task) return INVALID_ID;
    return r_graph.add_task_node(std::move(task), reqs, p_config.execution_priority);
}

template NodeID TileDOD::add_tile_task_node<TilingGenerationAndHexPartitionMetadataLogic<>>(
    TaskGraphDOD &, const TilingGenerationAndHexPartitionMetadataLogic<> &, const TilesNodeExecutionConfig &
) const;
template NodeID TileDOD::add_tile_task_node<TilingLatticeCullAndMaskQueryLogic<>>(
    TaskGraphDOD &, const TilingLatticeCullAndMaskQueryLogic<> &, const TilesNodeExecutionConfig &
) const;
template NodeID TileDOD::add_tile_task_node<TilingInstanceTransformLogic<>>(
    TaskGraphDOD &, const TilingInstanceTransformLogic<> &, const TilesNodeExecutionConfig &
) const;
template NodeID TileDOD::add_tile_task_node<TilingBoundaryClipAndSeamStitchTransformLogic<>>(
    TaskGraphDOD &, const TilingBoundaryClipAndSeamStitchTransformLogic<> &, const TilesNodeExecutionConfig &
) const;
template NodeID TileDOD::add_tile_task_node<TilingHexizeAndBorderExtractTransformLogic<>>(
    TaskGraphDOD &, const TilingHexizeAndBorderExtractTransformLogic<> &, const TilesNodeExecutionConfig &
) const;
template NodeID TileDOD::add_tile_task_node<TilingQuotientAStarAndFlowFieldTransformLogic<>>(
    TaskGraphDOD &, const TilingQuotientAStarAndFlowFieldTransformLogic<> &, const TilesNodeExecutionConfig &
) const;

NodeID TileDOD::add_transform_node_by_id(
    TaskGraphDOD &r_graph,
    TilesTransformLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const TilesNodeExecutionConfig &p_config
) const {
    switch (p_logic_id) {
        case TilesTransformLogicID::TilingInstanceTransform: {
            TilingInstanceTransformLogic<> l{};
            l.apply_properties(p_custom_props);
            return add_tile_task_node(r_graph, l, p_config);
        }
        case TilesTransformLogicID::TilingBoundaryClipAndSeamStitch: {
            TilingBoundaryClipAndSeamStitchTransformLogic<> l{};
            l.apply_properties(p_custom_props);
            return add_tile_task_node(r_graph, l, p_config);
        }
        case TilesTransformLogicID::TilingHexizeAndBorderExtract: {
            TilingHexizeAndBorderExtractTransformLogic<> l{};
            l.apply_properties(p_custom_props);
            return add_tile_task_node(r_graph, l, p_config);
        }
        case TilesTransformLogicID::TilingQuotientAStarAndFlowField: {
            TilingQuotientAStarAndFlowFieldTransformLogic<> l{};
            l.apply_properties(p_custom_props);
            return add_tile_task_node(r_graph, l, p_config);
        }
        default:
            return INVALID_ID;
    }
}

NodeID TileDOD::add_query_node_by_id(
    TaskGraphDOD &r_graph,
    TilesQueryLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const TilesNodeExecutionConfig &p_config
) const {
    if (p_logic_id == TilesQueryLogicID::TilingLatticeCullAndMask) {
        TilingLatticeCullAndMaskQueryLogic<> l{};
        l.apply_properties(p_custom_props);
        return add_tile_task_node(r_graph, l, p_config);
    }
    return INVALID_ID;
}

NodeID TileDOD::add_metadata_node_by_id(
    TaskGraphDOD &r_graph,
    TilesMetadataLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const TilesNodeExecutionConfig &p_config
) const {
    if (p_logic_id == TilesMetadataLogicID::TilingGenerationAndHexPartition) {
        TilingGenerationAndHexPartitionMetadataLogic<> l{};
        l.apply_properties(p_custom_props);
        return add_tile_task_node(r_graph, l, p_config);
    }
    return INVALID_ID;
}

TilesPipelineTopology TileDOD::inject_pipeline_stages(
    TaskGraphDOD &r_graph,
    uint32_t p_stage_mask,
    bool p_auto_connect_stages
) {
    TilesPipelineTopology topo{};

    auto push_stage = [&](TilesPipelineStageFlags p_flag, const std::vector<NodeID> &p_nodes) {
        TilesStageAnchor anchor{};
        anchor.stage = p_flag;
        for (NodeID nid : p_nodes) {
            if (nid != INVALID_ID) anchor.nodes.push_back(nid);
        }
        if (anchor.nodes.empty()) return;

        anchor.entry_node = anchor.nodes.front();
        anchor.exit_node  = anchor.nodes.back();

        for (size_t i = 0; i + 1 < anchor.nodes.size(); ++i) {
            r_graph.connect_Execution_flow(anchor.nodes[i], anchor.nodes[i + 1]);
        }
        if (p_auto_connect_stages && topo.last_node != INVALID_ID) {
            r_graph.connect_Execution_flow(topo.last_node, anchor.entry_node);
        }
        if (topo.first_node == INVALID_ID) {
            topo.first_node = anchor.entry_node;
        }
        topo.last_node = anchor.exit_node;
        topo.stages.push_back(std::move(anchor));
    };

    if ((p_stage_mask & TILES_STAGE_GENERATION_PARTITION_METADATA) != 0u) {
        push_stage(TILES_STAGE_GENERATION_PARTITION_METADATA, {add_generation_partition_metadata_node(r_graph)});
    }
    if ((p_stage_mask & TILES_STAGE_LATTICE_CULL_AND_MASK_QUERY) != 0u) {
        push_stage(TILES_STAGE_LATTICE_CULL_AND_MASK_QUERY, {add_lattice_cull_and_mask_query_node(r_graph)});
    }
    if ((p_stage_mask & TILES_STAGE_INSTANCE_AND_SEAM_TRANSFORM) != 0u) {
        push_stage(
            TILES_STAGE_INSTANCE_AND_SEAM_TRANSFORM,
            {add_instance_transform_node(r_graph), add_boundary_clip_and_seam_stitch_node(r_graph)}
        );
    }
    if ((p_stage_mask & TILES_STAGE_HEXIZE_AND_BORDER_EXTRACT) != 0u) {
        push_stage(TILES_STAGE_HEXIZE_AND_BORDER_EXTRACT, {add_hexize_and_border_extract_node(r_graph)});
    }
    if ((p_stage_mask & TILES_STAGE_NAVIGATION_AND_FLOW_FIELD) != 0u) {
        push_stage(TILES_STAGE_NAVIGATION_AND_FLOW_FIELD, {add_quotient_astar_and_flow_field_node(r_graph)});
    }

    active_topology_ = topo;
    return topo;
}

bool TileDOD::schedule_node_with_stage(
    TaskGraphDOD &r_graph,
    const TilesPipelineTopology &p_topology,
    TilesPipelineStageFlags p_target_stage,
    NodeID p_custom_node,
    bool p_join_downstream
) {
    if (p_custom_node == INVALID_ID) return false;
    for (size_t i = 0; i < p_topology.stages.size(); ++i) {
        if (p_topology.stages[i].stage == p_target_stage && p_topology.stages[i].is_valid()) {
            if (i > 0 && p_topology.stages[i - 1].is_valid()) {
                r_graph.connect_Execution_flow(p_topology.stages[i - 1].exit_node, p_custom_node);
            }
            if (p_join_downstream && i + 1 < p_topology.stages.size() && p_topology.stages[i + 1].is_valid()) {
                r_graph.connect_Execution_flow(p_custom_node, p_topology.stages[i + 1].entry_node);
            }
            return true;
        }
    }
    return false;
}

bool TileDOD::schedule_node_between_stages(
    TaskGraphDOD &r_graph,
    const TilesPipelineTopology &p_topology,
    TilesPipelineStageFlags p_upstream_stage,
    TilesPipelineStageFlags p_downstream_stage,
    NodeID p_custom_node
) {
    if (p_custom_node == INVALID_ID) return false;
    const TilesStageAnchor *up   = p_topology.get_stage_anchor(p_upstream_stage);
    const TilesStageAnchor *down = p_topology.get_stage_anchor(p_downstream_stage);
    if (!up || !down || !up->is_valid() || !down->is_valid()) return false;
    r_graph.connect_Execution_flow(up->exit_node, p_custom_node);
    r_graph.connect_Execution_flow(p_custom_node, down->entry_node);
    return true;
}

void TileDOD::execute_step(double p_delta, TaskGraphDOD *p_override_graph) {
    (void)p_delta;
    sync_selection_ranges();
    TaskGraphDOD *graph = p_override_graph ? p_override_graph : default_task_graph_.get();
    if (graph) {
        graph->execute_graph();
    }
}

} // namespace ideam::core

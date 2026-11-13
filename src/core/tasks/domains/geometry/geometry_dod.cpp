#include "geometry_dod.h"

#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

constexpr size_t DEFAULT_GEOMETRY_MASTER_BLOCK_BYTES    = 16ULL * 1024ULL * 1024ULL; // 16 MB
constexpr size_t DEFAULT_GEOMETRY_TRANSIENT_BLOCK_BYTES = 4ULL * 1024ULL * 1024ULL;  // 4 MB

struct GeometrySlotAccessSpec {
    GeometryBufferSlot slot = GeometryBufferSlot::COUNT;
    BufferAccessMode access = BufferAccessMode::READ;
};

template <typename T_Logic>
struct GeometryTaskDescriptor;

#define DEFINE_GEOMETRY_TASK_DESCRIPTOR(LogicStruct, EnumVal, S0, A0, S1, A1, S2, A2, S3, A3) \
    template <typename T>                                                                     \
    struct GeometryTaskDescriptor<LogicStruct<T>> {                                           \
        static constexpr auto logic_id = EnumVal;                                             \
        static constexpr std::array<GeometrySlotAccessSpec, 4> default_grants = {{            \
            {GeometryBufferSlot::S0, BufferAccessMode::A0},                                   \
            {GeometryBufferSlot::S1, BufferAccessMode::A1},                                   \
            {GeometryBufferSlot::S2, BufferAccessMode::A2},                                   \
            {GeometryBufferSlot::S3, BufferAccessMode::A3}                                    \
        }};                                                                                   \
    }

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    SpatialHashPartitionMetadataLogic,
    GeometryMetadataLogicID::SpatialHashPartition,
    GEOM_POLYLINE_SLICE_POD, READ,
    GEOM_SPATIAL_HASH_ENTRY_POD, READ_WRITE,
    GEOM_POLYLINE_VERTEX_2D, READ,
    GEOM_CLIP_CLASSIFICATION_FLAGS, READ
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    PolygonBoundaryCullQueryLogic,
    GeometryQueryLogicID::PolygonBoundaryCull,
    GEOM_CLIP_CLASSIFICATION_FLAGS, READ_WRITE,
    GEOM_POLYLINE_SLICE_POD, READ,
    GEOM_POLYLINE_VERTEX_2D, READ,
    GEOM_POLYGON_MESH_SLICE_POD, READ
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    PolygonTriangulateAndOffsetTransformLogic,
    GeometryTransformLogicID::PolygonTriangulateAndOffset,
    GEOM_POLYGON_MESH_SLICE_POD, READ_WRITE,
    GEOM_POLYLINE_SLICE_POD, READ,
    GEOM_POLYLINE_VERTEX_2D, READ,
    GEOM_POLYGON_INDEX_U32, READ_WRITE
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    CSGClipAndSeamWeldTransformLogic,
    GeometryTransformLogicID::CSGClipAndSeamWeld,
    GEOM_POLYGON_MESH_SLICE_POD, READ_WRITE,
    GEOM_POLYLINE_SLICE_POD, READ,
    GEOM_POLYLINE_VERTEX_2D, READ,
    GEOM_CLIP_CLASSIFICATION_FLAGS, READ
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    VoronoiDelaunayDualTransformLogic,
    GeometryTransformLogicID::VoronoiDelaunayDual,
    GEOM_VORONOI_SITE_2D, READ_WRITE,
    GEOM_VORONOI_CELL_SLICE_POD, READ_WRITE,
    GEOM_VORONOI_CSR_ROW_OFFSETS, READ_WRITE,
    GEOM_VORONOI_CSR_COL_NEIGHBORS, READ_WRITE
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    SDFFieldAndContourTransformLogic,
    GeometryTransformLogicID::SDFFieldAndContour,
    GEOM_SDF_GRID_FLOAT32, READ_WRITE,
    GEOM_MARCHING_SQUARES_SEGMENT_POD, READ_WRITE,
    GEOM_MARCHING_SQUARES_COUNT, READ_WRITE,
    GEOM_SDF_GRID_HEADER_POD, READ
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    TensorFieldEvaluateAndEigenTransformLogic,
    GeometryTransformLogicID::TensorFieldEvaluateAndEigen,
    GEOM_TENSOR_CELL_POD, READ_WRITE,
    GEOM_TENSOR_GRID_HEADER_POD, READ,
    GEOM_TENSOR_BASIS_POD, READ,
    GEOM_TENSOR_EIGEN_MAJOR_2D, READ_WRITE
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    TensorSingularityAndSeedQueryLogic,
    GeometryQueryLogicID::TensorSingularityAndSeed,
    GEOM_TENSOR_STREAMLINE_SEED_POD, READ,
    GEOM_TENSOR_CELL_POD, READ,
    GEOM_TENSOR_SINGULARITY_POD, READ_WRITE,
    GEOM_TENSOR_SINGULARITY_COUNT, READ_WRITE
);

DEFINE_GEOMETRY_TASK_DESCRIPTOR(
    TensorHyperstreamlineNetworkTransformLogic,
    GeometryTransformLogicID::TensorHyperstreamlineNetwork,
    GEOM_POLYLINE_SLICE_POD, READ_WRITE,
    GEOM_TENSOR_CELL_POD, READ,
    GEOM_PLANAR_HALF_EDGE_POD, READ_WRITE,
    GEOM_POLYGON_MESH_SLICE_POD, READ_WRITE
);

#undef DEFINE_GEOMETRY_TASK_DESCRIPTOR

} // anonymous namespace

GeometryDOD::GeometryDOD(
    std::shared_ptr<MemoryManagerDOD> p_memory_manager,
    const GeometryBufferCapacities &p_capacities
)
    : memory_manager_(std::move(p_memory_manager)),
      capacities_(p_capacities) {
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_GEOMETRY_MASTER_BLOCK_BYTES,
            DEFAULT_GEOMETRY_TRANSIENT_BLOCK_BYTES
        );
    }
    default_task_graph_ = std::make_shared<TaskGraphDOD>(memory_manager_.get());
    default_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
    initialize_buffers(capacities_);
}

void GeometryDOD::initialize_buffers(const GeometryBufferCapacities &p_capacities) {
    capacities_ = p_capacities;
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_GEOMETRY_MASTER_BLOCK_BYTES,
            DEFAULT_GEOMETRY_TRANSIENT_BLOCK_BYTES
        );
    }

    polyline_allocator_.initialize(capacities_.max_polylines);
    polygon_mesh_allocator_.initialize(capacities_.max_polygon_meshes);
    voronoi_site_allocator_.initialize(capacities_.max_voronoi_sites);
    sdf_grid_allocator_.initialize(capacities_.max_sdf_grids);
    tensor_grid_allocator_.initialize(capacities_.max_tensor_grids);

    for (size_t i = 0; i < GEOMETRY_BUFFER_SLOT_COUNT; ++i) {
        const GeometryBufferSpec &spec = GEOMETRY_BUFFER_SPECS[i];
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

    reset_all_slots();
}

uint32_t GeometryDOD::_active_count_for_group(GeometryTableGroup p_group) const noexcept {
    switch (p_group) {
        case GeometryTableGroup::POLYLINE_SLICE:            return polyline_allocator_.get_high_water_mark();
        case GeometryTableGroup::POLYLINE_VERTEX_2D_SLICE:  return polyline_vertex_2d_cursor_;
        case GeometryTableGroup::POLYLINE_VERTEX_3D_SLICE:  return polyline_vertex_3d_cursor_;
        case GeometryTableGroup::CURVE_FRAME_3D_SLICE:      return curve_frame_3d_cursor_;
        case GeometryTableGroup::POLYGON_MESH_SLICE:        return std::max(polygon_mesh_allocator_.get_high_water_mark(), polyline_allocator_.get_high_water_mark());
        case GeometryTableGroup::POLYGON_VERTEX_2D_SLICE:   return polygon_vertex_2d_cursor_;
        case GeometryTableGroup::POLYGON_INDEX_SLICE:       return polygon_index_cursor_;
        case GeometryTableGroup::PLANAR_HALF_EDGE_SLICE:    return planar_half_edge_cursor_;
        case GeometryTableGroup::VORONOI_SITE_TABLE:        return voronoi_site_allocator_.get_high_water_mark();
        case GeometryTableGroup::VORONOI_CSR_ROW:           return voronoi_site_allocator_.get_high_water_mark() ? (voronoi_site_allocator_.get_high_water_mark() + 1u) : 0u;
        case GeometryTableGroup::VORONOI_CSR_EDGE:          return voronoi_csr_edge_cursor_;
        case GeometryTableGroup::SPATIAL_HASH_TABLE:        return std::max(spatial_hash_entry_cursor_, polyline_allocator_.get_high_water_mark());
        case GeometryTableGroup::SDF_GRID_HEADER:           return sdf_grid_allocator_.get_high_water_mark();
        case GeometryTableGroup::SDF_CELL_SLICE:            return sdf_cell_cursor_;
        case GeometryTableGroup::MARCHING_SQUARES_SLICE:    return capacities_.max_marching_squares_segments;
        case GeometryTableGroup::TENSOR_GRID_HEADER:        return tensor_grid_allocator_.get_high_water_mark();
        case GeometryTableGroup::TENSOR_BASIS_SLICE:        return tensor_basis_cursor_;
        case GeometryTableGroup::TENSOR_CELL_SLICE:         return tensor_cell_cursor_;
        case GeometryTableGroup::TENSOR_SINGULARITY_SLICE:  return capacities_.max_tensor_singularities;
        case GeometryTableGroup::TENSOR_SEED_SLICE:         return tensor_seed_cursor_;
        case GeometryTableGroup::EVENT_RING_OR_COUNTER:
        default:                                            return 0u;
    }
}

void GeometryDOD::sync_selection_ranges() noexcept {
    if (!memory_manager_) return;
    for (size_t i = 0; i < GEOMETRY_BUFFER_SLOT_COUNT; ++i) {
        MemoryBufferPOD *buf = memory_manager_->get_buffer(manifest_.buffer_ids[i]);
        if (!buf) continue;
        const GeometryBufferSpec &spec = GEOMETRY_BUFFER_SPECS[i];
        buf->current_count = (spec.table_group == GeometryTableGroup::EVENT_RING_OR_COUNTER ||
                              spec.table_group == GeometryTableGroup::MARCHING_SQUARES_SLICE ||
                              spec.table_group == GeometryTableGroup::TENSOR_SINGULARITY_SLICE)
            ? buf->max_elements
            : std::min<int64_t>(static_cast<int64_t>(_active_count_for_group(spec.table_group)), buf->max_elements);
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void GeometryDOD::reset_all_slots() noexcept {
    polyline_allocator_.clear();
    polygon_mesh_allocator_.clear();
    voronoi_site_allocator_.clear();
    sdf_grid_allocator_.clear();
    tensor_grid_allocator_.clear();

    polyline_vertex_2d_cursor_ = 0u;
    polyline_vertex_3d_cursor_ = 0u;
    curve_frame_3d_cursor_     = 0u;
    polygon_vertex_2d_cursor_  = 0u;
    polygon_index_cursor_      = 0u;
    planar_half_edge_cursor_   = 0u;
    voronoi_csr_edge_cursor_   = 0u;
    spatial_hash_entry_cursor_ = 0u;
    sdf_cell_cursor_           = 0u;
    tensor_basis_cursor_       = 0u;
    tensor_cell_cursor_        = 0u;
    tensor_seed_cursor_        = 0u;

    auto cursor_span = get_column_span<uint32_t>(GeometryBufferSlot::GEOM_SIGNAL_RING_CURSOR);
    std::fill(cursor_span.begin(), cursor_span.end(), 0u);

    auto ms_cnt_span = get_column_span<uint32_t>(GeometryBufferSlot::GEOM_MARCHING_SQUARES_COUNT);
    std::fill(ms_cnt_span.begin(), ms_cnt_span.end(), 0u);

    auto ts_cnt_span = get_column_span<uint32_t>(GeometryBufferSlot::GEOM_TENSOR_SINGULARITY_COUNT);
    std::fill(ts_cnt_span.begin(), ts_cnt_span.end(), 0u);

    sync_selection_ranges();
}

void GeometryDOD::populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing) const {
    auto set_key = [&](std::string_view p_key, uint32_t p_val) {
        if (p_key.empty() || p_val == INVALID_ID) return;
        const godot::String key_str(std::string(p_key).c_str());
        if (p_overwrite_existing || !r_props.has(key_str)) {
            r_props[key_str] = static_cast<int64_t>(p_val);
        }
    };
    for (size_t i = 0; i < GEOMETRY_BUFFER_SLOT_COUNT; ++i) {
        const uint32_t buf_id = manifest_.buffer_ids[i];
        set_key(GEOMETRY_BUFFER_SPECS[i].canonical_name, buf_id);
        for (std::string_view alias : GEOMETRY_BUFFER_SPECS[i].aliases) {
            set_key(alias, buf_id);
        }
    }
}

GrantPartPOD GeometryDOD::make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept {
    if (memory_manager_ && p_buffer_id != INVALID_ID) {
        if (const MemoryBufferPOD *buf = memory_manager_->get_buffer(p_buffer_id)) {
            return make_grant_requirement(p_buffer_id, p_access, buf->element_stride, buf->max_elements);
        }
    }
    return make_grant_requirement(p_buffer_id, p_access, sizeof(uint32_t), 0);
}

// ============================================================================
// Table 1: Polyline & 3D Bishop Frame Staging / Readback
// ============================================================================

uint32_t GeometryDOD::allocate_polyline_2d(std::span<const glm::vec2> p_vertices, bool p_closed) {
    if (polyline_vertex_2d_cursor_ + p_vertices.size() > capacities_.max_polyline_vertices_2d) {
        return INVALID_ID;
    }
    const uint32_t slot = polyline_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    auto *slices = get_column_ptr<PolylineSlicePOD>(GeometryBufferSlot::GEOM_POLYLINE_SLICE_POD);
    auto *verts  = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_2D);
    if (!slices || !verts) return INVALID_ID;

    const uint32_t start = polyline_vertex_2d_cursor_;
    const uint32_t count = static_cast<uint32_t>(p_vertices.size());
    if (count > 0u) {
        std::copy(p_vertices.begin(), p_vertices.end(), verts + start);
        polyline_vertex_2d_cursor_ += count;
    }

    const AABB2D<float> box    = PolygonMath2D<float>::compute_aabb(p_vertices);
    const WindingOrder winding = PolygonMath2D<float>::get_winding_order(p_vertices);

    PolylineSlicePOD slice{};
    slice.vertex_offset = start;
    slice.vertex_count  = count;
    slice.total_length  = PolygonMath2D<float>::perimeter(p_vertices, p_closed);
    slice.dimension     = 2u;
    slice.is_closed     = p_closed ? 1u : 0u;
    slice.winding_order = (winding == WindingOrder::CLOCKWISE) ? 0u : 1u;
    slice.bounds_min    = box.is_valid() ? box.min_bounds : glm::vec2(0.0f);
    slice.bounds_max    = box.is_valid() ? box.max_bounds : glm::vec2(0.0f);
    slices[slot]        = slice;

    while (polygon_mesh_allocator_.get_high_water_mark() <= slot) {
        (void)polygon_mesh_allocator_.allocate();
    }

    sync_selection_ranges();
    return slot;
}

uint32_t GeometryDOD::allocate_polyline_3d(std::span<const glm::vec3> p_vertices, bool p_closed) {
    if (polyline_vertex_3d_cursor_ + p_vertices.size() > capacities_.max_polyline_vertices_3d) {
        return INVALID_ID;
    }
    const uint32_t slot = polyline_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    auto *slices = get_column_ptr<PolylineSlicePOD>(GeometryBufferSlot::GEOM_POLYLINE_SLICE_POD);
    auto *verts  = get_column_ptr<glm::vec3>(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_3D);
    if (!slices || !verts) return INVALID_ID;

    const uint32_t start = polyline_vertex_3d_cursor_;
    const uint32_t count = static_cast<uint32_t>(p_vertices.size());
    float total_len      = 0.0f;
    AABB2D<float> box;

    for (size_t i = 0; i < p_vertices.size(); ++i) {
        verts[start + i] = p_vertices[i];
        box.expand_to(glm::vec2(p_vertices[i].x, p_vertices[i].y));
        if (i + 1 < p_vertices.size()) {
            total_len += glm::distance(p_vertices[i], p_vertices[i + 1]);
        }
    }
    if (p_closed && p_vertices.size() > 2u) {
        total_len += glm::distance(p_vertices.back(), p_vertices.front());
    }
    polyline_vertex_3d_cursor_ += count;

    PolylineSlicePOD slice{};
    slice.vertex_offset = start;
    slice.vertex_count  = count;
    slice.total_length  = total_len;
    slice.dimension     = 3u;
    slice.is_closed     = p_closed ? 1u : 0u;
    slice.bounds_min    = box.is_valid() ? box.min_bounds : glm::vec2(0.0f);
    slice.bounds_max    = box.is_valid() ? box.max_bounds : glm::vec2(0.0f);
    slices[slot]        = slice;

    sync_selection_ranges();
    return slot;
}

PolylineSlicePOD GeometryDOD::get_polyline_slice(uint32_t p_slot) const noexcept {
    if (p_slot >= capacities_.max_polylines) return PolylineSlicePOD{};
    const auto *slices = get_column_ptr<PolylineSlicePOD>(GeometryBufferSlot::GEOM_POLYLINE_SLICE_POD);
    return slices ? slices[p_slot] : PolylineSlicePOD{};
}

std::span<const glm::vec2> GeometryDOD::get_polyline_vertices_2d(uint32_t p_slot) const noexcept {
    const PolylineSlicePOD s = get_polyline_slice(p_slot);
    const auto *verts        = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_2D);
    if (!verts || s.vertex_count == 0u || s.vertex_offset + s.vertex_count > capacities_.max_polyline_vertices_2d) {
        return {};
    }
    return std::span<const glm::vec2>(verts + s.vertex_offset, s.vertex_count);
}

std::span<const glm::vec3> GeometryDOD::get_polyline_vertices_3d(uint32_t p_slot) const noexcept {
    const PolylineSlicePOD s = get_polyline_slice(p_slot);
    const auto *verts        = get_column_ptr<glm::vec3>(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_3D);
    if (!verts || s.vertex_count == 0u || s.vertex_offset + s.vertex_count > capacities_.max_polyline_vertices_3d) {
        return {};
    }
    return std::span<const glm::vec3>(verts + s.vertex_offset, s.vertex_count);
}

uint32_t GeometryDOD::append_curve_frames_3d(std::span<const CurveFrame3DPOD<float>> p_frames) {
    if (p_frames.empty() || curve_frame_3d_cursor_ + p_frames.size() > capacities_.max_curve_frames_3d) {
        return INVALID_ID;
    }
    auto *frames = get_column_ptr<CurveFrame3DPOD<float>>(GeometryBufferSlot::GEOM_CURVE_FRAME_3D_POD);
    if (!frames) return INVALID_ID;

    const uint32_t start = curve_frame_3d_cursor_;
    std::copy(p_frames.begin(), p_frames.end(), frames + start);
    curve_frame_3d_cursor_ += static_cast<uint32_t>(p_frames.size());
    sync_selection_ranges();
    return start;
}

std::span<const CurveFrame3DPOD<float>> GeometryDOD::get_curve_frames_3d(uint32_t p_offset, uint32_t p_count) const noexcept {
    const auto *frames = get_column_ptr<CurveFrame3DPOD<float>>(GeometryBufferSlot::GEOM_CURVE_FRAME_3D_POD);
    if (!frames || p_count == 0u || p_offset + p_count > capacities_.max_curve_frames_3d) {
        return {};
    }
    return std::span<const CurveFrame3DPOD<float>>(frames + p_offset, p_count);
}

// ============================================================================
// Table 2: Polygon Mesh & CSG Classification Staging / Readback
// ============================================================================

uint32_t GeometryDOD::allocate_polygon_mesh_2d(
    std::span<const glm::vec2> p_vertices,
    std::span<const uint32_t> p_indices,
    uint32_t p_material_or_group_id
) {
    if (polygon_vertex_2d_cursor_ + p_vertices.size() > capacities_.max_polygon_vertices_2d ||
        polygon_index_cursor_ + p_indices.size() > capacities_.max_polygon_indices) {
        return INVALID_ID;
    }
    const uint32_t slot = polygon_mesh_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    auto *slices  = get_column_ptr<PolygonMeshSlicePOD>(GeometryBufferSlot::GEOM_POLYGON_MESH_SLICE_POD);
    auto *verts   = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_POLYGON_VERTEX_2D);
    auto *indices = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_POLYGON_INDEX_U32);
    if (!slices || !verts || !indices) return INVALID_ID;

    PolygonMeshSlicePOD ms{};
    ms.vertex_offset        = polygon_vertex_2d_cursor_;
    ms.vertex_count         = static_cast<uint32_t>(p_vertices.size());
    ms.index_offset         = polygon_index_cursor_;
    ms.index_count          = static_cast<uint32_t>(p_indices.size());
    ms.signed_area          = PolygonMath2D<float>::signed_area(p_vertices);
    ms.centroid             = PolygonMath2D<float>::centroid(p_vertices);
    ms.material_or_group_id = p_material_or_group_id;

    if (!p_vertices.empty()) {
        std::copy(p_vertices.begin(), p_vertices.end(), verts + polygon_vertex_2d_cursor_);
        polygon_vertex_2d_cursor_ += ms.vertex_count;
    }
    if (!p_indices.empty()) {
        std::copy(p_indices.begin(), p_indices.end(), indices + polygon_index_cursor_);
        polygon_index_cursor_ += ms.index_count;
    } else if (p_vertices.size() >= 3u) {
        const std::vector<uint32_t> auto_tris = PolygonMath2D<float>::triangulate(p_vertices);
        if (polygon_index_cursor_ + auto_tris.size() <= capacities_.max_polygon_indices) {
            ms.index_offset = polygon_index_cursor_;
            ms.index_count  = static_cast<uint32_t>(auto_tris.size());
            std::copy(auto_tris.begin(), auto_tris.end(), indices + polygon_index_cursor_);
            polygon_index_cursor_ += ms.index_count;
        }
    }

    slices[slot] = ms;
    sync_selection_ranges();
    return slot;
}

PolygonMeshSlicePOD GeometryDOD::get_polygon_mesh_slice(uint32_t p_slot) const noexcept {
    if (p_slot >= capacities_.max_polygon_meshes) return PolygonMeshSlicePOD{};
    const auto *slices = get_column_ptr<PolygonMeshSlicePOD>(GeometryBufferSlot::GEOM_POLYGON_MESH_SLICE_POD);
    return slices ? slices[p_slot] : PolygonMeshSlicePOD{};
}

std::span<const glm::vec2> GeometryDOD::get_polygon_mesh_vertices_2d(uint32_t p_slot) const noexcept {
    const PolygonMeshSlicePOD ms = get_polygon_mesh_slice(p_slot);
    const auto *verts            = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_POLYGON_VERTEX_2D);
    if (!verts || ms.vertex_count == 0u || ms.vertex_offset + ms.vertex_count > capacities_.max_polygon_vertices_2d) {
        return {};
    }
    return std::span<const glm::vec2>(verts + ms.vertex_offset, ms.vertex_count);
}

std::span<const uint32_t> GeometryDOD::get_polygon_mesh_indices(uint32_t p_slot) const noexcept {
    const PolygonMeshSlicePOD ms = get_polygon_mesh_slice(p_slot);
    const auto *indices          = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_POLYGON_INDEX_U32);
    if (!indices || ms.index_count == 0u || ms.index_offset + ms.index_count > capacities_.max_polygon_indices) {
        return {};
    }
    return std::span<const uint32_t>(indices + ms.index_offset, ms.index_count);
}

ClipClassification2D GeometryDOD::get_clip_classification(uint32_t p_polyline_slot) const noexcept {
    if (p_polyline_slot >= capacities_.max_polylines) return ClipClassification2D::FULLY_OUTSIDE;
    const auto *flags = get_column_ptr<uint8_t>(GeometryBufferSlot::GEOM_CLIP_CLASSIFICATION_FLAGS);
    return flags ? static_cast<ClipClassification2D>(flags[p_polyline_slot]) : ClipClassification2D::FULLY_OUTSIDE;
}

// ============================================================================
// Table 3: Voronoi Sites, Cell Polygons & CSR Dual Adjacency
// ============================================================================

void GeometryDOD::stage_voronoi_sites(std::span<const glm::vec2> p_sites) {
    voronoi_site_allocator_.clear();
    auto *sites = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_VORONOI_SITE_2D);
    if (!sites) return;

    const size_t limit = std::min<size_t>(p_sites.size(), capacities_.max_voronoi_sites);
    for (size_t i = 0; i < limit; ++i) {
        const uint32_t slot = voronoi_site_allocator_.allocate();
        if (slot != INVALID_ID) {
            sites[slot] = p_sites[i];
        }
    }
    sync_selection_ranges();
}

std::span<const glm::vec2> GeometryDOD::get_voronoi_sites() const noexcept {
    const auto *sites = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_VORONOI_SITE_2D);
    const uint32_t n  = voronoi_site_allocator_.get_high_water_mark();
    return (sites && n > 0u) ? std::span<const glm::vec2>(sites, n) : std::span<const glm::vec2>{};
}

std::span<const glm::vec2> GeometryDOD::get_voronoi_cell_vertices(uint32_t p_site_slot) const noexcept {
    if (p_site_slot >= voronoi_site_allocator_.get_high_water_mark()) return {};
    const auto *cells = get_column_ptr<PolylineSlicePOD>(GeometryBufferSlot::GEOM_VORONOI_CELL_SLICE_POD);
    const auto *verts = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_POLYLINE_VERTEX_2D);
    if (!cells || !verts) return {};
    const PolylineSlicePOD &cs = cells[p_site_slot];
    if (cs.vertex_count == 0u || cs.vertex_offset + cs.vertex_count > capacities_.max_polyline_vertices_2d) {
        return {};
    }
    return std::span<const glm::vec2>(verts + cs.vertex_offset, cs.vertex_count);
}

std::span<const uint32_t> GeometryDOD::get_voronoi_csr_row_offsets() const noexcept {
    const auto *roff = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_VORONOI_CSR_ROW_OFFSETS);
    const uint32_t n = voronoi_site_allocator_.get_high_water_mark();
    return (roff && n > 0u) ? std::span<const uint32_t>(roff, n + 1u) : std::span<const uint32_t>{};
}

std::span<const uint32_t> GeometryDOD::get_voronoi_csr_col_neighbors() const noexcept {
    const auto roff  = get_voronoi_csr_row_offsets();
    const auto *cnbr = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_VORONOI_CSR_COL_NEIGHBORS);
    if (roff.empty() || !cnbr) return {};
    const uint32_t total_edges = std::min(roff.back(), capacities_.max_voronoi_csr_neighbors);
    return std::span<const uint32_t>(cnbr, total_edges);
}

// ============================================================================
// Table 5: 2D SDF Grids & Marching Squares Contours
// ============================================================================

uint32_t GeometryDOD::allocate_sdf_grid_2d(
    uint32_t p_width,
    uint32_t p_height,
    const glm::vec2 &p_origin,
    const glm::vec2 &p_cell_size,
    std::span<const float> p_initial_values,
    float p_iso_value
) {
    const uint32_t w     = std::max(2u, p_width);
    const uint32_t h     = std::max(2u, p_height);
    const uint32_t cells = w * h;
    if (sdf_cell_cursor_ + cells > capacities_.max_sdf_cells) {
        return INVALID_ID;
    }
    const uint32_t slot = sdf_grid_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    auto *headers   = get_column_ptr<SDFGridHeaderPOD>(GeometryBufferSlot::GEOM_SDF_GRID_HEADER_POD);
    auto *grid_data = get_column_ptr<float>(GeometryBufferSlot::GEOM_SDF_GRID_FLOAT32);
    if (!headers || !grid_data) return INVALID_ID;

    SDFGridHeaderPOD hdr{};
    hdr.cell_offset = sdf_cell_cursor_;
    hdr.width       = w;
    hdr.height      = h;
    hdr.iso_value   = p_iso_value;
    hdr.origin      = p_origin;
    hdr.cell_size   = p_cell_size;
    headers[slot]   = hdr;

    if (!p_initial_values.empty()) {
        const size_t copy_n = std::min<size_t>(cells, p_initial_values.size());
        std::copy_n(p_initial_values.data(), copy_n, grid_data + sdf_cell_cursor_);
    } else {
        std::fill_n(grid_data + sdf_cell_cursor_, cells, 1000.0f);
    }
    sdf_cell_cursor_ += cells;
    sync_selection_ranges();
    return slot;
}

SDFGridHeaderPOD GeometryDOD::get_sdf_grid_header(uint32_t p_grid_slot) const noexcept {
    if (p_grid_slot >= capacities_.max_sdf_grids) return SDFGridHeaderPOD{};
    const auto *headers = get_column_ptr<SDFGridHeaderPOD>(GeometryBufferSlot::GEOM_SDF_GRID_HEADER_POD);
    return headers ? headers[p_grid_slot] : SDFGridHeaderPOD{};
}

std::span<const float> GeometryDOD::get_sdf_grid_cells(uint32_t p_grid_slot) const noexcept {
    const SDFGridHeaderPOD hdr = get_sdf_grid_header(p_grid_slot);
    const auto *grid_data      = get_column_ptr<float>(GeometryBufferSlot::GEOM_SDF_GRID_FLOAT32);
    const uint32_t total       = hdr.width * hdr.height;
    if (!grid_data || total == 0u || hdr.cell_offset + total > capacities_.max_sdf_cells) {
        return {};
    }
    return std::span<const float>(grid_data + hdr.cell_offset, total);
}

std::span<float> GeometryDOD::get_sdf_grid_cells_mut(uint32_t p_grid_slot) noexcept {
    const SDFGridHeaderPOD hdr = get_sdf_grid_header(p_grid_slot);
    auto *grid_data            = get_column_ptr<float>(GeometryBufferSlot::GEOM_SDF_GRID_FLOAT32);
    const uint32_t total       = hdr.width * hdr.height;
    if (!grid_data || total == 0u || hdr.cell_offset + total > capacities_.max_sdf_cells) {
        return {};
    }
    return std::span<float>(grid_data + hdr.cell_offset, total);
}

std::span<const MarchingSquaresSegment2D<float>> GeometryDOD::get_extracted_marching_squares_segments() const noexcept {
    const auto *segs = get_column_ptr<MarchingSquaresSegment2D<float>>(GeometryBufferSlot::GEOM_MARCHING_SQUARES_SEGMENT_POD);
    const auto *cnt  = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_MARCHING_SQUARES_COUNT);
    if (!segs || !cnt) return {};
    const uint32_t n = std::min(*cnt, capacities_.max_marching_squares_segments);
    return std::span<const MarchingSquaresSegment2D<float>>(segs, n);
}

// ============================================================================
// Table 6: 2D Symmetric Tensor Fields, Eigenvectors & Hyperstreamline Seeds
// ============================================================================

uint32_t GeometryDOD::allocate_tensor_grid_2d(
    uint32_t p_width,
    uint32_t p_height,
    const glm::vec2 &p_origin,
    const glm::vec2 &p_cell_size,
    std::span<const TensorBasisElementPod> p_bases,
    uint32_t p_sdf_grid_slot
) {
    const uint32_t w     = std::max(2u, p_width);
    const uint32_t h     = std::max(2u, p_height);
    const uint32_t cells = w * h;
    if (tensor_cell_cursor_ + cells > capacities_.max_tensor_cells ||
        tensor_basis_cursor_ + p_bases.size() > capacities_.max_tensor_bases) {
        return INVALID_ID;
    }
    const uint32_t slot = tensor_grid_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    auto *headers = get_column_ptr<TensorGridHeaderPod>(GeometryBufferSlot::GEOM_TENSOR_GRID_HEADER_POD);
    auto *bases   = get_column_ptr<TensorBasisElementPod>(GeometryBufferSlot::GEOM_TENSOR_BASIS_POD);
    if (!headers || !bases) return INVALID_ID;

    TensorGridHeaderPod hdr{};
    hdr.cell_offset   = tensor_cell_cursor_;
    hdr.width         = w;
    hdr.height        = h;
    hdr.basis_offset  = static_cast<uint16_t>(tensor_basis_cursor_);
    hdr.basis_count   = static_cast<uint16_t>(p_bases.size());
    hdr.sdf_grid_slot = p_sdf_grid_slot;
    hdr.origin        = p_origin;
    hdr.cell_size     = p_cell_size;
    headers[slot]     = hdr;

    if (!p_bases.empty()) {
        std::copy(p_bases.begin(), p_bases.end(), bases + tensor_basis_cursor_);
        tensor_basis_cursor_ += static_cast<uint32_t>(p_bases.size());
    }
    tensor_cell_cursor_ += cells;
    sync_selection_ranges();
    return slot;
}

TensorGridHeaderPod GeometryDOD::get_tensor_grid_header(uint32_t p_grid_slot) const noexcept {
    if (p_grid_slot >= capacities_.max_tensor_grids) return TensorGridHeaderPod{};
    const auto *headers = get_column_ptr<TensorGridHeaderPod>(GeometryBufferSlot::GEOM_TENSOR_GRID_HEADER_POD);
    return headers ? headers[p_grid_slot] : TensorGridHeaderPod{};
}

std::span<const SymmetricTensor2DPod> GeometryDOD::get_tensor_grid_cells(uint32_t p_grid_slot) const noexcept {
    const TensorGridHeaderPod hdr = get_tensor_grid_header(p_grid_slot);
    const auto *cells = get_column_ptr<SymmetricTensor2DPod>(GeometryBufferSlot::GEOM_TENSOR_CELL_POD);
    const uint32_t total = hdr.width * hdr.height;
    if (!cells || total == 0u || hdr.cell_offset + total > capacities_.max_tensor_cells) {
        return {};
    }
    return std::span<const SymmetricTensor2DPod>(cells + hdr.cell_offset, total);
}

std::span<const glm::vec2> GeometryDOD::get_tensor_major_eigenvectors(uint32_t p_grid_slot) const noexcept {
    const TensorGridHeaderPod hdr = get_tensor_grid_header(p_grid_slot);
    const auto *vecs = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_TENSOR_EIGEN_MAJOR_2D);
    const uint32_t total = hdr.width * hdr.height;
    if (!vecs || total == 0u || hdr.cell_offset + total > capacities_.max_tensor_cells) {
        return {};
    }
    return std::span<const glm::vec2>(vecs + hdr.cell_offset, total);
}

std::span<const glm::vec2> GeometryDOD::get_tensor_minor_eigenvectors(uint32_t p_grid_slot) const noexcept {
    const TensorGridHeaderPod hdr = get_tensor_grid_header(p_grid_slot);
    const auto *vecs = get_column_ptr<glm::vec2>(GeometryBufferSlot::GEOM_TENSOR_EIGEN_MINOR_2D);
    const uint32_t total = hdr.width * hdr.height;
    if (!vecs || total == 0u || hdr.cell_offset + total > capacities_.max_tensor_cells) {
        return {};
    }
    return std::span<const glm::vec2>(vecs + hdr.cell_offset, total);
}

std::span<const TensorSingularityPod> GeometryDOD::get_detected_tensor_singularities() const noexcept {
    const auto *sings = get_column_ptr<TensorSingularityPod>(GeometryBufferSlot::GEOM_TENSOR_SINGULARITY_POD);
    const auto *cnt   = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_TENSOR_SINGULARITY_COUNT);
    if (!sings || !cnt) return {};
    const uint32_t n = std::min(*cnt, capacities_.max_tensor_singularities);
    return std::span<const TensorSingularityPod>(sings, n);
}

void GeometryDOD::stage_tensor_streamline_seeds(std::span<const TensorStreamlineSeedPod> p_seeds) {
    auto *seeds = get_column_ptr<TensorStreamlineSeedPod>(GeometryBufferSlot::GEOM_TENSOR_STREAMLINE_SEED_POD);
    if (!seeds) return;
    const size_t copy_n = std::min<size_t>(p_seeds.size(), capacities_.max_tensor_seeds);
    if (copy_n > 0u) {
        std::copy_n(p_seeds.data(), copy_n, seeds);
    }
    tensor_seed_cursor_ = static_cast<uint32_t>(copy_n);
    sync_selection_ranges();
}

bool GeometryDOD::push_signal_command(const GeometrySignalCommandPOD &p_cmd) noexcept {
    auto *ring    = get_column_ptr<GeometrySignalCommandPOD>(GeometryBufferSlot::GEOM_SIGNAL_COMMAND_RING);
    auto *cursors = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_SIGNAL_RING_CURSOR);
    if (!ring || !cursors || capacities_.max_signal_commands == 0u) return false;

    const uint32_t write_idx = cursors[1];
    if (write_idx >= capacities_.max_signal_commands) return false;
    ring[write_idx] = p_cmd;
    cursors[1]      = write_idx + 1u;
    return true;
}

uint32_t GeometryDOD::drain_signal_commands(std::vector<GeometrySignalCommandPOD> &r_out, uint32_t p_max_drain) noexcept {
    const auto *ring = get_column_ptr<GeometrySignalCommandPOD>(GeometryBufferSlot::GEOM_SIGNAL_COMMAND_RING);
    auto *cursors    = get_column_ptr<uint32_t>(GeometryBufferSlot::GEOM_SIGNAL_RING_CURSOR);
    if (!ring || !cursors) return 0u;

    const uint32_t write_idx = std::min(cursors[1], capacities_.max_signal_commands);
    uint32_t read_idx        = std::min(cursors[0], write_idx);
    uint32_t drained         = 0u;

    while (read_idx < write_idx && drained < p_max_drain) {
        r_out.push_back(ring[read_idx++]);
        ++drained;
    }
    if (read_idx >= write_idx) {
        cursors[0] = 0u;
        cursors[1] = 0u;
    } else {
        cursors[0] = read_idx;
    }
    return drained;
}

// ============================================================================
// Templated Task Node Builder & Pipeline Stage Injection
// ============================================================================

void GeometryDOD::_bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept {
    auto *dst       = static_cast<uint32_t *>(r_logic_bytes);
    const auto *src = static_cast<const uint32_t *>(p_default_bytes);
    for (size_t i = 0, n = p_size / sizeof(uint32_t); i < n; ++i) {
        if (dst[i] == INVALID_ID && src[i] != INVALID_ID) {
            dst[i] = src[i];
        }
    }
}

template <typename T_Logic>
NodeID GeometryDOD::add_geometry_task_node(
    TaskGraphDOD &r_graph,
    const T_Logic &p_logic_override,
    const GeometryNodeExecutionConfig &p_config
) const {
    using Descriptor = GeometryTaskDescriptor<T_Logic>;

    T_Logic logic = p_logic_override;
    T_Logic defaults{};
    godot::Dictionary manifest_props;
    populate_task_buffer_dictionary(manifest_props, true);
    defaults.apply_properties(manifest_props);
    _bind_default_logic_buffers(&logic, sizeof(T_Logic), &defaults);

    const NodeID node_id = r_graph.add_task_node(
        IsQueryLogic<T_Logic> ? TaskTypeDOD::QUERY_CULLER : TaskTypeDOD::NATIVE_CPU
    );
    r_graph.set_node_priority(node_id, p_config.execution_priority);

    std::unique_ptr<INativeTask> native_task;
    if (p_config.use_registry_factory) {
        if constexpr (IsTransformLogic<T_Logic>) {
            native_task = GeometryTransformTaskRegistry::create(
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = GeometryQueryTaskRegistry::create(
                static_cast<uint32_t>(p_config.query_op),
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = GeometryMetadataTaskRegistry::create(
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        }
        if (native_task) {
            manifest_props["target_buffer_id"] = static_cast<int64_t>(logic.get_target_buffer_id());
            native_task->apply_properties(manifest_props);
        }
    }

    if (!native_task) {
        if constexpr (IsTransformLogic<T_Logic>) {
            native_task = std::make_unique<TransformTask<T_Logic>>(logic);
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = (p_config.query_op == QueryOp::ADD)
                ? std::unique_ptr<INativeTask>(std::make_unique<QueryTask<T_Logic, QueryOp::ADD>>(logic))
                : std::unique_ptr<INativeTask>(std::make_unique<QueryTask<T_Logic, QueryOp::CULL>>(logic));
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = std::make_unique<MetadataTask<T_Logic>>(logic);
        }
    }

    r_graph.configure_native_interface(node_id, std::move(native_task));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        std::array<GrantPartPOD, 4> reqs = {{
            make_slot_requirement(Descriptor::default_grants[0].slot, Descriptor::default_grants[0].access),
            make_slot_requirement(Descriptor::default_grants[1].slot, Descriptor::default_grants[1].access),
            make_slot_requirement(Descriptor::default_grants[2].slot, Descriptor::default_grants[2].access),
            make_slot_requirement(Descriptor::default_grants[3].slot, Descriptor::default_grants[3].access)
        }};
        const uint32_t target_id = logic.get_target_buffer_id();
        if (target_id != INVALID_ID && reqs[0].buffer_id != target_id) {
            reqs[0] = make_buffer_requirement(target_id, Descriptor::default_grants[0].access);
        }
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

#define INSTANTIATE_GEOMETRY_TASK_NODE(LogicType) \
    template NodeID GeometryDOD::add_geometry_task_node<LogicType>(TaskGraphDOD &, const LogicType &, const GeometryNodeExecutionConfig &) const

INSTANTIATE_GEOMETRY_TASK_NODE(SpatialHashPartitionMetadataLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(PolygonBoundaryCullQueryLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(PolygonTriangulateAndOffsetTransformLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(CSGClipAndSeamWeldTransformLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(VoronoiDelaunayDualTransformLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(SDFFieldAndContourTransformLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(TensorFieldEvaluateAndEigenTransformLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(TensorSingularityAndSeedQueryLogic<>);
INSTANTIATE_GEOMETRY_TASK_NODE(TensorHyperstreamlineNetworkTransformLogic<>);

#undef INSTANTIATE_GEOMETRY_TASK_NODE

NodeID GeometryDOD::add_transform_node_by_id(
    TaskGraphDOD &r_graph,
    GeometryTransformLogicID p_id,
    const godot::Dictionary &p_props,
    const GeometryNodeExecutionConfig &p_cfg
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_geometry_task_node(r_graph, l, p_cfg);
    };
    switch (p_id) {
        case GeometryTransformLogicID::PolygonTriangulateAndOffset:  return build.template operator()<PolygonTriangulateAndOffsetTransformLogic<>>();
        case GeometryTransformLogicID::CSGClipAndSeamWeld:           return build.template operator()<CSGClipAndSeamWeldTransformLogic<>>();
        case GeometryTransformLogicID::VoronoiDelaunayDual:          return build.template operator()<VoronoiDelaunayDualTransformLogic<>>();
        case GeometryTransformLogicID::SDFFieldAndContour:           return build.template operator()<SDFFieldAndContourTransformLogic<>>();
        case GeometryTransformLogicID::TensorFieldEvaluateAndEigen:  return build.template operator()<TensorFieldEvaluateAndEigenTransformLogic<>>();
        case GeometryTransformLogicID::TensorHyperstreamlineNetwork: return build.template operator()<TensorHyperstreamlineNetworkTransformLogic<>>();
        default:                                                     return INVALID_ID;
    }
}

NodeID GeometryDOD::add_query_node_by_id(
    TaskGraphDOD &r_graph,
    GeometryQueryLogicID p_id,
    const godot::Dictionary &p_props,
    const GeometryNodeExecutionConfig &p_cfg
) const {
    if (p_id == GeometryQueryLogicID::PolygonBoundaryCull) {
        PolygonBoundaryCullQueryLogic<> l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_geometry_task_node(r_graph, l, p_cfg);
    }
    if (p_id == GeometryQueryLogicID::TensorSingularityAndSeed) {
        TensorSingularityAndSeedQueryLogic<> l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_geometry_task_node(r_graph, l, p_cfg);
    }
    return INVALID_ID;
}

NodeID GeometryDOD::add_metadata_node_by_id(
    TaskGraphDOD &r_graph,
    GeometryMetadataLogicID p_id,
    const godot::Dictionary &p_props,
    const GeometryNodeExecutionConfig &p_cfg
) const {
    if (p_id == GeometryMetadataLogicID::SpatialHashPartition) {
        SpatialHashPartitionMetadataLogic<> l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_geometry_task_node(r_graph, l, p_cfg);
    }
    return INVALID_ID;
}

GeometryPipelineTopology GeometryDOD::inject_pipeline_stages(
    TaskGraphDOD &r_graph,
    uint32_t p_stage_mask,
    bool p_auto_connect_stages
) {
    GeometryPipelineTopology topo{};
    bool prev_was_parallel = false;

    auto append_stage = [&](GeometryPipelineStageFlags p_flag, const std::vector<NodeID> &p_nodes, bool p_chain_internal) {
        if (p_nodes.empty()) return;
        GeometryStageAnchor anchor{p_flag, p_nodes.front(), p_nodes.back(), p_nodes};

        if (p_chain_internal) {
            for (size_t i = 0; i + 1 < p_nodes.size(); ++i) {
                r_graph.connect_nodes(p_nodes[i], 0, p_nodes[i + 1], 0);
            }
        }
        if (p_auto_connect_stages && !topo.stages.empty()) {
            const GeometryStageAnchor &prev        = topo.stages.back();
            const std::vector<NodeID> up_exits     = prev_was_parallel ? prev.nodes : std::vector<NodeID>{prev.exit_node};
            const std::vector<NodeID> down_entries = p_chain_internal ? std::vector<NodeID>{anchor.entry_node} : anchor.nodes;
            for (NodeID u : up_exits) {
                for (NodeID d : down_entries) {
                    r_graph.connect_nodes(u, 0, d, 0);
                }
            }
        }
        if (topo.first_node == INVALID_ID) topo.first_node = anchor.entry_node;
        topo.last_node    = anchor.exit_node;
        prev_was_parallel = !p_chain_internal;
        topo.stages.push_back(std::move(anchor));
    };

    if (p_stage_mask & GEOMETRY_STAGE_SPATIAL_HASH_METADATA) {
        append_stage(GEOMETRY_STAGE_SPATIAL_HASH_METADATA, {add_spatial_hash_partition_node(r_graph)}, true);
    }
    if (p_stage_mask & GEOMETRY_STAGE_BOUNDARY_CULL_CLASSIFY) {
        GeometryNodeExecutionConfig cull_cfg{};
        cull_cfg.query_op = QueryOp::ADD;
        append_stage(GEOMETRY_STAGE_BOUNDARY_CULL_CLASSIFY, {add_polygon_boundary_cull_node(r_graph, {}, cull_cfg)}, true);
    }
    if (p_stage_mask & GEOMETRY_STAGE_POLYGON_TRIANGULATE_CSG) {
        append_stage(
            GEOMETRY_STAGE_POLYGON_TRIANGULATE_CSG,
            {add_polygon_triangulate_node(r_graph), add_csg_clip_and_seam_weld_node(r_graph)},
            true
        );
    }
    if (p_stage_mask & GEOMETRY_STAGE_VORONOI_DELAUNAY_DUAL) {
        append_stage(GEOMETRY_STAGE_VORONOI_DELAUNAY_DUAL, {add_voronoi_delaunay_dual_node(r_graph)}, true);
    }
    if (p_stage_mask & GEOMETRY_STAGE_SDF_AND_CONTOURS) {
        append_stage(GEOMETRY_STAGE_SDF_AND_CONTOURS, {add_sdf_field_and_contour_node(r_graph)}, true);
    }
    if (p_stage_mask & GEOMETRY_STAGE_TENSOR_FIELD_AND_STREAMLINES) {
        GeometryNodeExecutionConfig seed_cfg{};
        seed_cfg.query_op = QueryOp::ADD;
        append_stage(
            GEOMETRY_STAGE_TENSOR_FIELD_AND_STREAMLINES,
            {
                add_tensor_field_evaluate_node(r_graph),
                add_tensor_singularity_and_seed_node(r_graph, {}, seed_cfg),
                add_tensor_hyperstreamline_network_node(r_graph)
            },
            true
        );
    }

    active_topology_ = topo;
    return topo;
}

bool GeometryDOD::schedule_node_with_stage(
    TaskGraphDOD &r_graph,
    const GeometryPipelineTopology &p_topology,
    GeometryPipelineStageFlags p_target_stage,
    NodeID p_custom_node,
    bool p_join_downstream
) {
    if (p_custom_node == INVALID_ID) return false;
    for (size_t i = 0; i < p_topology.stages.size(); ++i) {
        if (p_topology.stages[i].stage == p_target_stage) {
            if (i > 0 && p_topology.stages[i - 1].exit_node != INVALID_ID) {
                r_graph.connect_nodes(p_topology.stages[i - 1].exit_node, 0, p_custom_node, 0);
            }
            if (p_join_downstream && (i + 1) < p_topology.stages.size() && p_topology.stages[i + 1].entry_node != INVALID_ID) {
                r_graph.connect_nodes(p_custom_node, 0, p_topology.stages[i + 1].entry_node, 0);
            }
            return true;
        }
    }
    return false;
}

bool GeometryDOD::schedule_node_between_stages(
    TaskGraphDOD &r_graph,
    const GeometryPipelineTopology &p_topology,
    GeometryPipelineStageFlags p_upstream_stage,
    GeometryPipelineStageFlags p_downstream_stage,
    NodeID p_custom_node
) {
    if (p_custom_node == INVALID_ID) return false;
    const GeometryStageAnchor *up   = p_topology.get_stage_anchor(p_upstream_stage);
    const GeometryStageAnchor *down = p_topology.get_stage_anchor(p_downstream_stage);
    if (!up || !down || !up->is_valid() || !down->is_valid()) return false;
    r_graph.connect_nodes(up->exit_node, 0, p_custom_node, 0);
    r_graph.connect_nodes(p_custom_node, 0, down->entry_node, 0);
    return true;
}

void GeometryDOD::execute_step(double p_delta, TaskGraphDOD *p_override_graph) {
    sync_selection_ranges();
    if (TaskGraphDOD *target_graph = p_override_graph ? p_override_graph : default_task_graph_.get()) {
        if (target_graph->get_node_count() > 0) {
            MemoryGrantPOD topo_grant{};
            if (target_graph->bake_topology_grant(topo_grant) && memory_manager_) {
                memory_manager_->release_grant(topo_grant);
            }
        }
        target_graph->execute_graph_dod(p_delta);
    }
}

} // namespace ideam::core

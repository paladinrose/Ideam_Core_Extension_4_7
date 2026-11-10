#include "spline_dod.h"

#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

constexpr size_t DEFAULT_SPLINES_MASTER_BLOCK_BYTES    = 16ULL * 1024ULL * 1024ULL; // 16 MB
constexpr size_t DEFAULT_SPLINES_TRANSIENT_BLOCK_BYTES = 4ULL * 1024ULL * 1024ULL;  // 4 MB

struct SplineSlotAccessSpec {
    SplineBufferSlot slot   = SplineBufferSlot::COUNT;
    BufferAccessMode access = BufferAccessMode::READ;
};

template <typename T_Logic>
struct SplineTaskDescriptor;

#define DEFINE_SPLINE_TASK_DESCRIPTOR(LogicStruct, EnumVal, S0, A0, S1, A1, S2, A2, S3, A3) \
    template <typename T>                                                                   \
    struct SplineTaskDescriptor<LogicStruct<T>> {                                           \
        static constexpr auto logic_id = EnumVal;                                           \
        static constexpr std::array<SplineSlotAccessSpec, 4> default_grants = {{            \
            {SplineBufferSlot::S0, BufferAccessMode::A0},                                   \
            {SplineBufferSlot::S1, BufferAccessMode::A1},                                   \
            {SplineBufferSlot::S2, BufferAccessMode::A2},                                   \
            {SplineBufferSlot::S3, BufferAccessMode::A3}                                    \
        }};                                                                                 \
    }

DEFINE_SPLINE_TASK_DESCRIPTOR(
    SplineArcLengthLUTMetadataLogic,
    SplinesMetadataLogicID::SplineArcLengthLUT,
    SPLINE_HEADER_POD, READ,
    SPLINE_SEGMENT_COEFFS_4D_POD, READ,
    SPLINE_WEIGHT_LUT_POD, READ,
    SPLINE_DISTANCE_LUT_POD, READ
);

DEFINE_SPLINE_TASK_DESCRIPTOR(
    SplineThresholdAndCorridorQueryLogic,
    SplinesQueryLogicID::SplineThresholdAndCorridor,
    SPLINE_CLOSEST_DISTANCE_FLOAT32, READ,
    SPLINE_EVAL_OUTPUT_VEC4, READ,
    BRIDGED_GEOM_POLYLINE_SLICE_POD, READ,
    BRIDGED_GEOM_POLYLINE_VERTEX_2D, READ
);

DEFINE_SPLINE_TASK_DESCRIPTOR(
    SplineBatchEvaluateTransformLogic,
    SplinesTransformLogicID::SplineBatchEvaluate,
    SPLINE_EVAL_OUTPUT_VEC4, READ_WRITE,
    SPLINE_HEADER_POD, READ,
    SPLINE_SEGMENT_COEFFS_4D_POD, READ,
    SPLINE_SAMPLE_REQUEST_POD, READ
);

DEFINE_SPLINE_TASK_DESCRIPTOR(
    SplineFlattenToPolylineTransformLogic,
    SplinesTransformLogicID::SplineFlattenToPolyline,
    BRIDGED_GEOM_POLYLINE_SLICE_POD, READ_WRITE,
    SPLINE_HEADER_POD, READ,
    SPLINE_SEGMENT_COEFFS_4D_POD, READ,
    BRIDGED_GEOM_POLYLINE_VERTEX_2D, READ_WRITE
);

DEFINE_SPLINE_TASK_DESCRIPTOR(
    SplineBishopFrameSweepTransformLogic,
    SplinesTransformLogicID::SplineBishopFrameSweep,
    BRIDGED_GEOM_CURVE_FRAME_3D_POD, READ_WRITE,
    SPLINE_HEADER_POD, READ,
    SPLINE_SEGMENT_COEFFS_4D_POD, READ,
    BRIDGED_GEOM_POLYLINE_VERTEX_3D, READ
);

DEFINE_SPLINE_TASK_DESCRIPTOR(
    SplineClosestPointProjectTransformLogic,
    SplinesTransformLogicID::SplineClosestPointProject,
    SPLINE_EVAL_OUTPUT_VEC4, READ_WRITE,
    SPLINE_HEADER_POD, READ,
    SPLINE_SEGMENT_COEFFS_4D_POD, READ,
    SPLINE_QUERY_POINT_VEC4, READ
);

#undef DEFINE_SPLINE_TASK_DESCRIPTOR

} // anonymous namespace

SplineDOD::SplineDOD(
    std::shared_ptr<MemoryManagerDOD> p_memory_manager,
    const SplinesBufferCapacities &p_capacities
)
    : memory_manager_(std::move(p_memory_manager)),
      capacities_(p_capacities) {
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_SPLINES_MASTER_BLOCK_BYTES,
            DEFAULT_SPLINES_TRANSIENT_BLOCK_BYTES
        );
    }
    default_task_graph_ = std::make_shared<TaskGraphDOD>(memory_manager_.get());
    default_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
    initialize_buffers(capacities_);
}

void SplineDOD::initialize_buffers(const SplinesBufferCapacities &p_capacities) {
    capacities_ = p_capacities;
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_SPLINES_MASTER_BLOCK_BYTES,
            DEFAULT_SPLINES_TRANSIENT_BLOCK_BYTES
        );
    }

    spline_allocator_.initialize(capacities_.max_splines);

    for (size_t i = 0; i < SPLINE_BUFFER_SLOT_COUNT; ++i) {
        const SplineBufferSpec &spec = SPLINE_BUFFER_SPECS[i];
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

uint32_t SplineDOD::_active_count_for_group(SplineTableGroup p_group) const noexcept {
    switch (p_group) {
        case SplineTableGroup::SPLINE_HEADER_TABLE:          return spline_allocator_.get_high_water_mark();
        case SplineTableGroup::SPLINE_SEGMENT_SLICE:         return segment_cursor_;
        case SplineTableGroup::SPLINE_WEIGHT_LUT_SLICE:      return weight_lut_cursor_;
        case SplineTableGroup::SPLINE_DISTANCE_LUT_SLICE:    return distance_lut_cursor_;
        case SplineTableGroup::SAMPLE_REQUEST_TABLE:         return sample_request_cursor_;
        case SplineTableGroup::QUERY_PROJECTION_TABLE:       return query_point_cursor_;
        case SplineTableGroup::BRIDGED_GEOM_POLYLINE_SLICE:  return std::max(bridged_polyline_cursor_, spline_allocator_.get_high_water_mark());
        case SplineTableGroup::BRIDGED_GEOM_VERTEX_2D_SLICE: return bridged_vertex_2d_cursor_;
        case SplineTableGroup::BRIDGED_GEOM_VERTEX_3D_SLICE: return bridged_vertex_3d_cursor_;
        case SplineTableGroup::BRIDGED_GEOM_FRAME_3D_SLICE:  return bridged_frame_3d_cursor_;
        default:                                             return 0u;
    }
}

void SplineDOD::sync_selection_ranges() noexcept {
    if (!memory_manager_) return;
    for (size_t i = 0; i < SPLINE_BUFFER_SLOT_COUNT; ++i) {
        MemoryBufferPOD *buf = memory_manager_->get_buffer(manifest_.buffer_ids[i]);
        if (!buf) continue;
        const SplineBufferSpec &spec = SPLINE_BUFFER_SPECS[i];
        const int64_t desired = static_cast<int64_t>(_active_count_for_group(spec.table_group));
        if (has_external_geometry_buffers_ &&
            (spec.table_group == SplineTableGroup::BRIDGED_GEOM_POLYLINE_SLICE ||
             spec.table_group == SplineTableGroup::BRIDGED_GEOM_VERTEX_2D_SLICE ||
             spec.table_group == SplineTableGroup::BRIDGED_GEOM_VERTEX_3D_SLICE ||
             spec.table_group == SplineTableGroup::BRIDGED_GEOM_FRAME_3D_SLICE)) {
            buf->current_count = std::min<int64_t>(std::max<int64_t>(buf->current_count, desired), buf->max_elements);
        } else {
            buf->current_count = std::min<int64_t>(desired, buf->max_elements);
        }
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void SplineDOD::reset_all_slots() noexcept {
    spline_allocator_.clear();
    segment_cursor_           = 0u;
    weight_lut_cursor_        = 0u;
    distance_lut_cursor_      = 0u;
    sample_request_cursor_    = 0u;
    query_point_cursor_       = 0u;
    bridged_polyline_cursor_  = 0u;
    bridged_vertex_2d_cursor_ = 0u;
    bridged_vertex_3d_cursor_ = 0u;
    bridged_frame_3d_cursor_  = 0u;

    sync_selection_ranges();
}

void SplineDOD::populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing) const {
    auto set_key = [&](std::string_view p_key, uint32_t p_val) {
        if (p_key.empty() || p_val == INVALID_ID) return;
        const godot::String key_str(std::string(p_key).c_str());
        if (p_overwrite_existing || !r_props.has(key_str)) {
            r_props[key_str] = static_cast<int64_t>(p_val);
        }
    };
    for (size_t i = 0; i < SPLINE_BUFFER_SLOT_COUNT; ++i) {
        const uint32_t buf_id = manifest_.buffer_ids[i];
        set_key(SPLINE_BUFFER_SPECS[i].canonical_name, buf_id);
        for (std::string_view alias : SPLINE_BUFFER_SPECS[i].aliases) {
            set_key(alias, buf_id);
        }
    }
}

GrantPartPOD SplineDOD::make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept {
    if (memory_manager_ && p_buffer_id != INVALID_ID) {
        if (const MemoryBufferPOD *buf = memory_manager_->get_buffer(p_buffer_id)) {
            return make_grant_requirement(p_buffer_id, p_access, buf->element_stride, buf->max_elements);
        }
    }
    return make_grant_requirement(p_buffer_id, p_access, sizeof(uint32_t), 0);
}

void SplineDOD::bind_external_geometry_buffers(
    uint32_t p_polyline_slice_buf_id,
    uint32_t p_polyline_vertex_2d_buf_id,
    uint32_t p_polyline_vertex_3d_buf_id,
    uint32_t p_curve_frame_3d_buf_id
) noexcept {
    if (p_polyline_slice_buf_id != INVALID_ID) {
        manifest_.set(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD, p_polyline_slice_buf_id);
        has_external_geometry_buffers_ = true;
    }
    if (p_polyline_vertex_2d_buf_id != INVALID_ID) {
        manifest_.set(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_2D, p_polyline_vertex_2d_buf_id);
        has_external_geometry_buffers_ = true;
    }
    if (p_polyline_vertex_3d_buf_id != INVALID_ID) {
        manifest_.set(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_3D, p_polyline_vertex_3d_buf_id);
    }
    if (p_curve_frame_3d_buf_id != INVALID_ID) {
        manifest_.set(SplineBufferSlot::BRIDGED_GEOM_CURVE_FRAME_3D_POD, p_curve_frame_3d_buf_id);
    }
}

// ============================================================================
// Templated 1D-4D Spline Baking (`bake_spline`)
// ============================================================================

template <size_t N, FloatingPointScalar T>
uint32_t SplineDOD::bake_spline(
    const SplineBase<N, T> &p_spline,
    uint8_t p_subdivisions_per_section,
    uint8_t p_w_channel_mode,
    uint32_t p_lut_bins,
    GeometryDOD *r_optional_linked_geometry_dod
) {
    const size_t sec_count = p_spline.get_section_count();
    const uint32_t bins    = std::max(8u, p_lut_bins);

    if (segment_cursor_ + sec_count > capacities_.max_segments ||
        weight_lut_cursor_ + bins > capacities_.max_lut_entries ||
        distance_lut_cursor_ + bins > capacities_.max_lut_entries) {
        return INVALID_ID;
    }

    const uint32_t slot = spline_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    auto *headers = get_column_ptr<SplineHeaderPOD>(SplineBufferSlot::SPLINE_HEADER_POD);
    auto *coeffs  = get_column_ptr<SplineSegmentCoeffsPOD<4, float>>(SplineBufferSlot::SPLINE_SEGMENT_COEFFS_4D_POD);
    auto *w_lut   = get_column_ptr<SplineParametricLUTEntryPOD<float>>(SplineBufferSlot::SPLINE_WEIGHT_LUT_POD);
    auto *d_lut   = get_column_ptr<SplineParametricLUTEntryPOD<float>>(SplineBufferSlot::SPLINE_DISTANCE_LUT_POD);
    if (!headers || !coeffs || !w_lut || !d_lut) return INVALID_ID;

    // 1. Bake 4D Horner Polynomial Coefficients
    const auto raw_coeffs = p_spline.bake_segment_coefficients();
    const uint32_t seg_start = segment_cursor_;
    for (size_t i = 0; i < raw_coeffs.size(); ++i) {
        coeffs[seg_start + i] = _promote_coeffs_to_4d_float(raw_coeffs[i]);
    }
    segment_cursor_ += static_cast<uint32_t>(raw_coeffs.size());

    // 2. Bake Weighted & Arc-Length Distance LUTs
    const auto raw_w_lut = p_spline.bake_parametric_lut(bins, true);
    const auto raw_d_lut = p_spline.bake_parametric_lut(bins, false);
    const uint32_t w_start = weight_lut_cursor_;
    const uint32_t d_start = distance_lut_cursor_;

    for (size_t i = 0; i < bins; ++i) {
        if (i < raw_w_lut.size()) {
            w_lut[w_start + i].section_index = raw_w_lut[i].section_index;
            w_lut[w_start + i].local_t_start = static_cast<float>(raw_w_lut[i].local_t_start);
            w_lut[w_start + i].local_t_slope = static_cast<float>(raw_w_lut[i].local_t_slope);
        }
        if (i < raw_d_lut.size()) {
            d_lut[d_start + i].section_index = raw_d_lut[i].section_index;
            d_lut[d_start + i].local_t_start = static_cast<float>(raw_d_lut[i].local_t_start);
            d_lut[d_start + i].local_t_slope = static_cast<float>(raw_d_lut[i].local_t_slope);
        }
    }
    weight_lut_cursor_   += bins;
    distance_lut_cursor_ += bins;

    // 3. Reserve destination PolylineSlicePOD & CurveFrame3DPOD ranges for Wave 1 S_T2 / S_T3
    const uint32_t subdivs    = std::max<uint8_t>(1u, p_subdivisions_per_section);
    const bool closed         = p_spline.get_wrap();
    const uint32_t flat_verts = (sec_count > 0u)
        ? (closed ? static_cast<uint32_t>(sec_count * subdivs) : static_cast<uint32_t>(sec_count * subdivs + 1u))
        : 0u;

    uint32_t geom_poly_slot = slot;
    uint32_t frame_start    = 0u;
    uint32_t frame_count    = 0u;

    if constexpr (N == 2) {
        if (r_optional_linked_geometry_dod) {
            std::vector<glm::vec2> placeholder(flat_verts, glm::vec2(0.0f));
            const uint32_t alloc_slot = r_optional_linked_geometry_dod->allocate_polyline_2d(placeholder, closed);
            if (alloc_slot != INVALID_ID) {
                geom_poly_slot = alloc_slot;
                bridged_polyline_cursor_  = std::max(bridged_polyline_cursor_, alloc_slot + 1u);
                bridged_vertex_2d_cursor_ += flat_verts;
            }
        } else if (auto *slices = get_column_ptr<PolylineSlicePOD>(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD)) {
            if (slot < capacities_.max_bridged_polylines &&
                bridged_vertex_2d_cursor_ + flat_verts <= capacities_.max_bridged_vertices_2d) {
                slices[slot].vertex_offset = bridged_vertex_2d_cursor_;
                slices[slot].vertex_count  = flat_verts;
                slices[slot].dimension     = 2u;
                slices[slot].is_closed     = closed ? 1u : 0u;
                bridged_polyline_cursor_   = std::max(bridged_polyline_cursor_, slot + 1u);
                bridged_vertex_2d_cursor_ += flat_verts;
            }
        }
    } else if constexpr (N >= 3) {
        frame_count = flat_verts;
        if (r_optional_linked_geometry_dod) {
            std::vector<glm::vec3> placeholder(flat_verts, glm::vec3(0.0f));
            const uint32_t alloc_slot = r_optional_linked_geometry_dod->allocate_polyline_3d(placeholder, closed);
            if (alloc_slot != INVALID_ID) {
                geom_poly_slot = alloc_slot;
                bridged_polyline_cursor_  = std::max(bridged_polyline_cursor_, alloc_slot + 1u);
                bridged_vertex_3d_cursor_ += flat_verts;
            }
            std::vector<CurveFrame3DPOD<float>> placeholder_frames(frame_count);
            const uint32_t f_off = r_optional_linked_geometry_dod->append_curve_frames_3d(placeholder_frames);
            if (f_off != INVALID_ID) {
                frame_start              = f_off;
                bridged_frame_3d_cursor_ = std::max(bridged_frame_3d_cursor_, f_off + frame_count);
            }
        } else {
            if (auto *slices = get_column_ptr<PolylineSlicePOD>(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD)) {
                if (slot < capacities_.max_bridged_polylines &&
                    bridged_vertex_3d_cursor_ + flat_verts <= capacities_.max_bridged_vertices_3d) {
                    slices[slot].vertex_offset = bridged_vertex_3d_cursor_;
                    slices[slot].vertex_count  = flat_verts;
                    slices[slot].dimension     = 3u;
                    slices[slot].is_closed     = closed ? 1u : 0u;
                    bridged_polyline_cursor_   = std::max(bridged_polyline_cursor_, slot + 1u);
                    bridged_vertex_3d_cursor_ += flat_verts;
                }
            }
            if (bridged_frame_3d_cursor_ + frame_count <= capacities_.max_bridged_frames_3d) {
                frame_start               = bridged_frame_3d_cursor_;
                bridged_frame_3d_cursor_ += frame_count;
            }
        }
    }

    typename SplineBase<N, T>::VecType min_v{static_cast<T>(0)};
    typename SplineBase<N, T>::VecType max_v{static_cast<T>(0)};
    if (p_spline.get_point_count() > 0u) {
        p_spline.get_min_max(min_v, max_v, false);
    }

    SplineHeaderPOD hdr{};
    hdr.segment_offset           = seg_start;
    hdr.segment_count            = static_cast<uint32_t>(raw_coeffs.size());
    hdr.weight_lut_offset        = w_start;
    hdr.distance_lut_offset      = d_start;
    hdr.lut_bin_count            = bins;
    hdr.geom_polyline_slot       = geom_poly_slot;
    hdr.geom_frame_offset        = frame_start;
    hdr.geom_frame_count         = frame_count;
    hdr.total_weight             = static_cast<float>(p_spline.get_total_weight());
    hdr.total_length             = static_cast<float>(p_spline.get_total_spline_length());
    hdr.dimension                = static_cast<uint8_t>(N);
    hdr.is_wrap                  = closed ? 1u : 0u;
    hdr.w_channel_mode           = p_w_channel_mode;
    hdr.subdivisions_per_section = static_cast<uint8_t>(subdivs);
    for (size_t c = 0; c < N && c < 4; ++c) {
        hdr.bounds_min[static_cast<glm::length_t>(c)] = static_cast<float>(min_v[static_cast<glm::length_t>(c)]);
    }
    headers[slot] = hdr;

    sync_selection_ranges();
    return slot;
}

template uint32_t SplineDOD::bake_spline<1, float>(const SplineBase<1, float> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);
template uint32_t SplineDOD::bake_spline<2, float>(const SplineBase<2, float> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);
template uint32_t SplineDOD::bake_spline<3, float>(const SplineBase<3, float> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);
template uint32_t SplineDOD::bake_spline<4, float>(const SplineBase<4, float> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);
template uint32_t SplineDOD::bake_spline<1, double>(const SplineBase<1, double> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);
template uint32_t SplineDOD::bake_spline<2, double>(const SplineBase<2, double> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);
template uint32_t SplineDOD::bake_spline<3, double>(const SplineBase<3, double> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);
template uint32_t SplineDOD::bake_spline<4, double>(const SplineBase<4, double> &, uint8_t, uint8_t, uint32_t, GeometryDOD *);

SplineHeaderPOD SplineDOD::get_spline_header(uint32_t p_slot) const noexcept {
    if (p_slot >= capacities_.max_splines) return SplineHeaderPOD{};
    const auto *headers = get_column_ptr<SplineHeaderPOD>(SplineBufferSlot::SPLINE_HEADER_POD);
    return headers ? headers[p_slot] : SplineHeaderPOD{};
}

std::span<const SplineSegmentCoeffsPOD<4, float>> SplineDOD::get_spline_segments(uint32_t p_slot) const noexcept {
    const SplineHeaderPOD hdr = get_spline_header(p_slot);
    const auto *coeffs        = get_column_ptr<SplineSegmentCoeffsPOD<4, float>>(SplineBufferSlot::SPLINE_SEGMENT_COEFFS_4D_POD);
    if (!coeffs || hdr.segment_count == 0u || hdr.segment_offset + hdr.segment_count > capacities_.max_segments) {
        return {};
    }
    return std::span<const SplineSegmentCoeffsPOD<4, float>>(coeffs + hdr.segment_offset, hdr.segment_count);
}

// ============================================================================
// Batch Evaluation & Proximity Projection Staging / Readback
// ============================================================================

void SplineDOD::stage_sample_requests(std::span<const SplineSampleRequestPOD> p_requests) {
    auto *reqs = get_column_ptr<SplineSampleRequestPOD>(SplineBufferSlot::SPLINE_SAMPLE_REQUEST_POD);
    if (!reqs) return;
    const size_t copy_n = std::min<size_t>(p_requests.size(), capacities_.max_sample_requests);
    if (copy_n > 0u) {
        std::copy_n(p_requests.data(), copy_n, reqs);
    }
    sample_request_cursor_ = static_cast<uint32_t>(copy_n);
    sync_selection_ranges();
}

std::span<const glm::vec4> SplineDOD::get_evaluated_outputs() const noexcept {
    const auto *out = get_column_ptr<glm::vec4>(SplineBufferSlot::SPLINE_EVAL_OUTPUT_VEC4);
    return (out && sample_request_cursor_ > 0u)
        ? std::span<const glm::vec4>(out, sample_request_cursor_)
        : std::span<const glm::vec4>{};
}

std::span<const glm::vec4> SplineDOD::get_evaluated_tangents() const noexcept {
    const auto *tan = get_column_ptr<glm::vec4>(SplineBufferSlot::SPLINE_EVAL_TANGENT_VEC4);
    return (tan && sample_request_cursor_ > 0u)
        ? std::span<const glm::vec4>(tan, sample_request_cursor_)
        : std::span<const glm::vec4>{};
}

void SplineDOD::stage_proximity_query_points(std::span<const glm::vec4> p_query_points) {
    auto *qpts = get_column_ptr<glm::vec4>(SplineBufferSlot::SPLINE_QUERY_POINT_VEC4);
    if (!qpts) return;
    const size_t copy_n = std::min<size_t>(p_query_points.size(), capacities_.max_query_points);
    if (copy_n > 0u) {
        std::copy_n(p_query_points.data(), copy_n, qpts);
    }
    query_point_cursor_ = static_cast<uint32_t>(copy_n);
    sync_selection_ranges();
}

std::span<const float> SplineDOD::get_closest_percents() const noexcept {
    const auto *pct = get_column_ptr<float>(SplineBufferSlot::SPLINE_CLOSEST_PERCENT_FLOAT32);
    return (pct && query_point_cursor_ > 0u)
        ? std::span<const float>(pct, query_point_cursor_)
        : std::span<const float>{};
}

std::span<const float> SplineDOD::get_closest_distances() const noexcept {
    const auto *dist = get_column_ptr<float>(SplineBufferSlot::SPLINE_CLOSEST_DISTANCE_FLOAT32);
    return (dist && query_point_cursor_ > 0u)
        ? std::span<const float>(dist, query_point_cursor_)
        : std::span<const float>{};
}

std::span<const int32_t> SplineDOD::get_closest_anchors() const noexcept {
    const auto *anc = get_column_ptr<int32_t>(SplineBufferSlot::SPLINE_CLOSEST_ANCHOR_INT32);
    return (anc && query_point_cursor_ > 0u)
        ? std::span<const int32_t>(anc, query_point_cursor_)
        : std::span<const int32_t>{};
}

PolylineSlicePOD SplineDOD::get_flattened_polyline_slice(uint32_t p_spline_slot) const noexcept {
    const SplineHeaderPOD hdr = get_spline_header(p_spline_slot);
    const uint32_t p_slot     = (hdr.geom_polyline_slot != 0xFFFFFFFFu) ? hdr.geom_polyline_slot : p_spline_slot;
    const auto slices         = get_column_span<const PolylineSlicePOD>(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_SLICE_POD);
    return (p_slot < slices.size()) ? slices[p_slot] : PolylineSlicePOD{};
}

std::span<const glm::vec2> SplineDOD::get_flattened_polyline_vertices_2d(uint32_t p_spline_slot) const noexcept {
    const PolylineSlicePOD ps = get_flattened_polyline_slice(p_spline_slot);
    const auto verts          = get_column_span<const glm::vec2>(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_2D);
    if (ps.vertex_count == 0u || ps.vertex_offset + ps.vertex_count > verts.size()) return {};
    return verts.subspan(ps.vertex_offset, ps.vertex_count);
}

std::span<const glm::vec3> SplineDOD::get_flattened_polyline_vertices_3d(uint32_t p_spline_slot) const noexcept {
    const PolylineSlicePOD ps = get_flattened_polyline_slice(p_spline_slot);
    const auto verts          = get_column_span<const glm::vec3>(SplineBufferSlot::BRIDGED_GEOM_POLYLINE_VERTEX_3D);
    if (ps.vertex_count == 0u || ps.vertex_offset + ps.vertex_count > verts.size()) return {};
    return verts.subspan(ps.vertex_offset, ps.vertex_count);
}

std::span<const CurveFrame3DPOD<float>> SplineDOD::get_swept_bishop_frames_3d(uint32_t p_spline_slot) const noexcept {
    const SplineHeaderPOD hdr = get_spline_header(p_spline_slot);
    const auto frames         = get_column_span<const CurveFrame3DPOD<float>>(SplineBufferSlot::BRIDGED_GEOM_CURVE_FRAME_3D_POD);
    if (hdr.geom_frame_count == 0u || hdr.geom_frame_offset + hdr.geom_frame_count > frames.size()) return {};
    return frames.subspan(hdr.geom_frame_offset, hdr.geom_frame_count);
}

// ============================================================================
// Templated Task Node Builder & Pipeline Stage Injection
// ============================================================================

void SplineDOD::_bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept {
    auto *dst       = static_cast<uint32_t *>(r_logic_bytes);
    const auto *src = static_cast<const uint32_t *>(p_default_bytes);
    for (size_t i = 0, n = p_size / sizeof(uint32_t); i < n; ++i) {
        if (dst[i] == INVALID_ID && src[i] != INVALID_ID) {
            dst[i] = src[i];
        }
    }
}

template <typename T_Logic>
NodeID SplineDOD::add_spline_task_node(
    TaskGraphDOD &r_graph,
    const T_Logic &p_logic_override,
    const SplinesNodeExecutionConfig &p_config
) const {
    using Descriptor = SplineTaskDescriptor<T_Logic>;

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
            native_task = SplinesTransformTaskRegistry::create(
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = SplinesQueryTaskRegistry::create(
                static_cast<uint32_t>(p_config.query_op),
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = SplinesMetadataTaskRegistry::create(
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

#define INSTANTIATE_SPLINE_TASK_NODE(LogicType) \
    template NodeID SplineDOD::add_spline_task_node<LogicType>(TaskGraphDOD &, const LogicType &, const SplinesNodeExecutionConfig &) const

INSTANTIATE_SPLINE_TASK_NODE(SplineArcLengthLUTMetadataLogic<>);
INSTANTIATE_SPLINE_TASK_NODE(SplineThresholdAndCorridorQueryLogic<>);
INSTANTIATE_SPLINE_TASK_NODE(SplineBatchEvaluateTransformLogic<>);
INSTANTIATE_SPLINE_TASK_NODE(SplineFlattenToPolylineTransformLogic<>);
INSTANTIATE_SPLINE_TASK_NODE(SplineBishopFrameSweepTransformLogic<>);
INSTANTIATE_SPLINE_TASK_NODE(SplineClosestPointProjectTransformLogic<>);

#undef INSTANTIATE_SPLINE_TASK_NODE

NodeID SplineDOD::add_transform_node_by_id(
    TaskGraphDOD &r_graph,
    SplinesTransformLogicID p_id,
    const godot::Dictionary &p_props,
    const SplinesNodeExecutionConfig &p_cfg
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_spline_task_node(r_graph, l, p_cfg);
    };
    switch (p_id) {
        case SplinesTransformLogicID::SplineBatchEvaluate:       return build.template operator()<SplineBatchEvaluateTransformLogic<>>();
        case SplinesTransformLogicID::SplineFlattenToPolyline:   return build.template operator()<SplineFlattenToPolylineTransformLogic<>>();
        case SplinesTransformLogicID::SplineBishopFrameSweep:    return build.template operator()<SplineBishopFrameSweepTransformLogic<>>();
        case SplinesTransformLogicID::SplineClosestPointProject: return build.template operator()<SplineClosestPointProjectTransformLogic<>>();
        default:                                                 return INVALID_ID;
    }
}

NodeID SplineDOD::add_query_node_by_id(
    TaskGraphDOD &r_graph,
    SplinesQueryLogicID p_id,
    const godot::Dictionary &p_props,
    const SplinesNodeExecutionConfig &p_cfg
) const {
    if (p_id == SplinesQueryLogicID::SplineThresholdAndCorridor) {
        SplineThresholdAndCorridorQueryLogic<> l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_spline_task_node(r_graph, l, p_cfg);
    }
    return INVALID_ID;
}

NodeID SplineDOD::add_metadata_node_by_id(
    TaskGraphDOD &r_graph,
    SplinesMetadataLogicID p_id,
    const godot::Dictionary &p_props,
    const SplinesNodeExecutionConfig &p_cfg
) const {
    if (p_id == SplinesMetadataLogicID::SplineArcLengthLUT) {
        SplineArcLengthLUTMetadataLogic<> l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_spline_task_node(r_graph, l, p_cfg);
    }
    return INVALID_ID;
}

SplinesPipelineTopology SplineDOD::inject_pipeline_stages(
    TaskGraphDOD &r_graph,
    uint32_t p_stage_mask,
    bool p_auto_connect_stages
) {
    SplinesPipelineTopology topo{};
    bool prev_was_parallel = false;

    auto append_stage = [&](SplinesPipelineStageFlags p_flag, const std::vector<NodeID> &p_nodes, bool p_chain_internal) {
        if (p_nodes.empty()) return;
        SplinesStageAnchor anchor{p_flag, p_nodes.front(), p_nodes.back(), p_nodes};

        if (p_chain_internal) {
            for (size_t i = 0; i + 1 < p_nodes.size(); ++i) {
                r_graph.connect_nodes(p_nodes[i], 0, p_nodes[i + 1], 0);
            }
        }
        if (p_auto_connect_stages && !topo.stages.empty()) {
            const SplinesStageAnchor &prev         = topo.stages.back();
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

    if (p_stage_mask & SPLINES_STAGE_ARC_LENGTH_METADATA) {
        append_stage(SPLINES_STAGE_ARC_LENGTH_METADATA, {add_arc_length_metadata_node(r_graph)}, true);
    }
    if (p_stage_mask & SPLINES_STAGE_GEOMETRY_FLATTEN_AND_SWEEP) {
        append_stage(
            SPLINES_STAGE_GEOMETRY_FLATTEN_AND_SWEEP,
            {add_flatten_to_polyline_node(r_graph), add_bishop_frame_sweep_node(r_graph)},
            true
        );
    }
    if (p_stage_mask & SPLINES_STAGE_BATCH_EVAL_AND_PROJECT) {
        append_stage(
            SPLINES_STAGE_BATCH_EVAL_AND_PROJECT,
            {add_batch_evaluate_node(r_graph), add_closest_point_project_node(r_graph)},
            true
        );
    }
    if (p_stage_mask & SPLINES_STAGE_CORRIDOR_AND_THRESHOLD_QUERY) {
        append_stage(
            SPLINES_STAGE_CORRIDOR_AND_THRESHOLD_QUERY,
            {add_threshold_and_corridor_query_node(r_graph)},
            true
        );
    }

    active_topology_ = topo;
    return topo;
}

bool SplineDOD::schedule_node_with_stage(
    TaskGraphDOD &r_graph,
    const SplinesPipelineTopology &p_topology,
    SplinesPipelineStageFlags p_target_stage,
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

bool SplineDOD::schedule_node_between_stages(
    TaskGraphDOD &r_graph,
    const SplinesPipelineTopology &p_topology,
    SplinesPipelineStageFlags p_upstream_stage,
    SplinesPipelineStageFlags p_downstream_stage,
    NodeID p_custom_node
) {
    if (p_custom_node == INVALID_ID) return false;
    const SplinesStageAnchor *up   = p_topology.get_stage_anchor(p_upstream_stage);
    const SplinesStageAnchor *down = p_topology.get_stage_anchor(p_downstream_stage);
    if (!up || !down || !up->is_valid() || !down->is_valid()) return false;
    r_graph.connect_nodes(up->exit_node, 0, p_custom_node, 0);
    r_graph.connect_nodes(p_custom_node, 0, down->entry_node, 0);
    return true;
}

void SplineDOD::execute_step(double p_delta, TaskGraphDOD *p_override_graph) {
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

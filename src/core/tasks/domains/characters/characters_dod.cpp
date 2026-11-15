#include "characters_dod.h"
#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

template <typename T_Elem>
bool copy_span_to_buffer(MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::span<const T_Elem> p_src) {
    if (!p_mem || p_buf_id == INVALID_ID) return false;
    MemoryBufferPOD *buf = p_mem->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr) return false;
    const size_t max_elems = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
    const size_t count     = std::min<size_t>(p_src.size(), max_elems);
    if (count > 0) {
        std::memcpy(buf->master_block_ptr + buf->memory_offset, p_src.data(), count * sizeof(T_Elem));
    }
    buf->current_count = static_cast<int64_t>(count);
    return true;
}

template <typename T_Elem>
void read_buffer_to_vector(const MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::vector<T_Elem> &r_out) {
    r_out.clear();
    if (!p_mem || p_buf_id == INVALID_ID) return;
    const MemoryBufferPOD *buf = const_cast<MemoryManagerDOD *>(p_mem)->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr || buf->current_count <= 0) return;
    const size_t count = static_cast<size_t>(std::min<int64_t>(buf->current_count, buf->max_elements));
    r_out.resize(count);
    std::memcpy(r_out.data(), buf->master_block_ptr + buf->memory_offset, count * sizeof(T_Elem));
}

} // namespace

CharactersDOD::CharactersDOD() {
    character_mesh_.build_archetype_preset(CharacterBodyArchetype::BIPED);
    morph_weights_.assign(MORPH_DEFAULT_CHANNEL_COUNT, 0.0f);
    character_mesh_.generate_default_garment_bindings(garment_bindings_);
}

void CharactersDOD::set_morph_weights(std::span<const float> p_weights) {
    morph_weights_.assign(p_weights.begin(), p_weights.end());
    if (morph_weights_.size() < MORPH_DEFAULT_CHANNEL_COUNT) {
        morph_weights_.resize(MORPH_DEFAULT_CHANNEL_COUNT, 0.0f);
    }
}

void CharactersDOD::set_morph_channel_weight(uint32_t p_channel, float p_weight) {
    if (p_channel >= morph_weights_.size()) {
        morph_weights_.resize(static_cast<size_t>(p_channel) + 1u, 0.0f);
    }
    morph_weights_[p_channel] = p_weight;
}

void CharactersDOD::set_garment_bindings(std::span<const SurfaceGarmentBindPod> p_binds) {
    garment_bindings_.assign(p_binds.begin(), p_binds.end());
}

bool CharactersDOD::initialize_characters_memory(MemoryManagerDOD *p_mem) {
    if (!p_mem) return false;
    memory_manager_ = p_mem;

    auto alloc_buf = [&](const char *, size_t p_elem_count, size_t p_elem_bytes) -> uint32_t {
        const size_t total_bytes = std::max<size_t>(1u, p_elem_count) * p_elem_bytes;
        const uint32_t id = p_mem->create_buffer(BufferLayoutType::FLAT, total_bytes, 64u);
        if (MemoryBufferPOD *buf = p_mem->get_buffer(id)) {
            buf->max_elements  = static_cast<int64_t>(std::max<size_t>(1u, p_elem_count));
            buf->current_count = 0;
        }
        return id;
    };

    manifest_.set(
        CharactersBufferSlot::QUAD_LOOP_VERTEX_POD,
        alloc_buf("CHR_QuadLoopVertices", capacities_.max_vertices, sizeof(QuadLoopVertexPod))
    );
    manifest_.set(
        CharactersBufferSlot::QUAD_FACE_POD,
        alloc_buf("CHR_QuadFaces", capacities_.max_quads, sizeof(QuadFacePod))
    );
    manifest_.set(
        CharactersBufferSlot::MORPH_DELTA_POD,
        alloc_buf("CHR_MorphDeltas", capacities_.max_morph_deltas, sizeof(MorphDeltaPod))
    );
    manifest_.set(
        CharactersBufferSlot::MORPH_WEIGHT_FLOAT32,
        alloc_buf("CHR_MorphWeights", capacities_.max_morph_channels, sizeof(float))
    );
    manifest_.set(
        CharactersBufferSlot::DEFORMED_POSITION_VEC3,
        alloc_buf("CHR_DeformedPositions", capacities_.max_vertices, sizeof(glm::vec3))
    );
    manifest_.set(
        CharactersBufferSlot::DEFORMED_NORMAL_VEC3,
        alloc_buf("CHR_DeformedNormals", capacities_.max_vertices, sizeof(glm::vec3))
    );
    manifest_.set(
        CharactersBufferSlot::GARMENT_BIND_POD,
        alloc_buf("CHR_GarmentBinds", capacities_.max_garment_vertices, sizeof(SurfaceGarmentBindPod))
    );
    manifest_.set(
        CharactersBufferSlot::GARMENT_POSITION_VEC3,
        alloc_buf("CHR_GarmentPositions", capacities_.max_garment_vertices, sizeof(glm::vec3))
    );
    manifest_.set(
        CharactersBufferSlot::GARMENT_VELOCITY_VEC3,
        alloc_buf("CHR_GarmentVelocities", capacities_.max_garment_vertices, sizeof(glm::vec3))
    );
    manifest_.set(
        CharactersBufferSlot::VERTEX_ACTIVE_LOD_MASK,
        alloc_buf("CHR_VertexActiveLODMasks", capacities_.max_vertices, sizeof(uint32_t))
    );

    return manifest_.is_valid();
}

bool CharactersDOD::stage_character_to_dod(
    const ParametricCharacterMesh &p_mesh,
    std::span<const float> p_morph_weights,
    std::span<const SurfaceGarmentBindPod> p_garment_binds
) {
    if (!memory_manager_) return false;
    if (!manifest_.is_valid()) {
        if (!initialize_characters_memory(memory_manager_)) return false;
    }

    copy_span_to_buffer(memory_manager_, manifest_.get(CharactersBufferSlot::QUAD_LOOP_VERTEX_POD), std::span<const QuadLoopVertexPod>(p_mesh.base_vertices()));
    copy_span_to_buffer(memory_manager_, manifest_.get(CharactersBufferSlot::QUAD_FACE_POD), std::span<const QuadFacePod>(p_mesh.base_quads()));
    copy_span_to_buffer(memory_manager_, manifest_.get(CharactersBufferSlot::MORPH_DELTA_POD), std::span<const MorphDeltaPod>(p_mesh.sparse_morph_deltas()));
    copy_span_to_buffer(memory_manager_, manifest_.get(CharactersBufferSlot::MORPH_WEIGHT_FLOAT32), p_morph_weights);
    copy_span_to_buffer(memory_manager_, manifest_.get(CharactersBufferSlot::GARMENT_BIND_POD), p_garment_binds);

    const int64_t v_count = static_cast<int64_t>(std::min<size_t>(p_mesh.base_vertices().size(), capacities_.max_vertices));
    if (MemoryBufferPOD *pos_buf = memory_manager_->get_buffer(manifest_.get(CharactersBufferSlot::DEFORMED_POSITION_VEC3))) {
        pos_buf->current_count = v_count;
    }
    if (MemoryBufferPOD *nrm_buf = memory_manager_->get_buffer(manifest_.get(CharactersBufferSlot::DEFORMED_NORMAL_VEC3))) {
        nrm_buf->current_count = v_count;
    }
    if (MemoryBufferPOD *msk_buf = memory_manager_->get_buffer(manifest_.get(CharactersBufferSlot::VERTEX_ACTIVE_LOD_MASK))) {
        msk_buf->current_count = v_count;
    }

    const int64_t g_count = static_cast<int64_t>(std::min<size_t>(p_garment_binds.size(), capacities_.max_garment_vertices));
    if (MemoryBufferPOD *gpos_buf = memory_manager_->get_buffer(manifest_.get(CharactersBufferSlot::GARMENT_POSITION_VEC3))) {
        gpos_buf->current_count = g_count;
    }
    if (MemoryBufferPOD *gvel_buf = memory_manager_->get_buffer(manifest_.get(CharactersBufferSlot::GARMENT_VELOCITY_VEC3))) {
        gvel_buf->current_count = g_count;
        if (gvel_buf->master_block_ptr && g_count > 0) {
            std::memset(gvel_buf->master_block_ptr + gvel_buf->memory_offset, 0, static_cast<size_t>(g_count) * sizeof(glm::vec3));
        }
    }

    return true;
}

bool CharactersDOD::stage_character_to_dod() {
    if (garment_bindings_.empty() && character_mesh_.quad_count() > 0u) {
        character_mesh_.generate_default_garment_bindings(garment_bindings_, active_partition_mask_);
    }
    return stage_character_to_dod(
        character_mesh_,
        std::span<const float>(morph_weights_),
        std::span<const SurfaceGarmentBindPod>(garment_bindings_)
    );
}

bool CharactersDOD::execute_full_pipeline_direct(uint32_t p_stage_mask) {
    if (!memory_manager_ || !manifest_.is_valid()) return false;

    TaskContextPOD ctx{};
    ctx.manager = memory_manager_;
    ctx.delta   = 1.0 / 60.0;
    SingleElementView<uint32_t, FlatStrategy> dummy_view{};

    if ((p_stage_mask & CHARACTERS_STAGE_QUAD_LOOP_LOD) != 0u) {
        QuadLoopSubdivisionAndLODTransformLogic<> chr1{};
        chr1.target_buffer_id          = manifest_.get(CharactersBufferSlot::DEFORMED_POSITION_VEC3);
        chr1.quad_vertex_buffer_id     = manifest_.get(CharactersBufferSlot::QUAD_LOOP_VERTEX_POD);
        chr1.quad_face_buffer_id       = manifest_.get(CharactersBufferSlot::QUAD_FACE_POD);
        chr1.deformed_normal_buffer_id = manifest_.get(CharactersBufferSlot::DEFORMED_NORMAL_VEC3);
        chr1.active_lod_mask_buffer_id = manifest_.get(CharactersBufferSlot::VERTEX_ACTIVE_LOD_MASK);
        chr1.target_lod_level          = target_lod_level_;
        chr1.execute<decltype(dummy_view), FlatStrategy>(ctx, dummy_view);
    }

    if ((p_stage_mask & CHARACTERS_STAGE_SPARSE_MORPH_SLIDERS) != 0u) {
        SparseMorphSliderEvaluateTransformLogic<> chr2{};
        chr2.target_buffer_id          = manifest_.get(CharactersBufferSlot::DEFORMED_POSITION_VEC3);
        chr2.deformed_normal_buffer_id = manifest_.get(CharactersBufferSlot::DEFORMED_NORMAL_VEC3);
        chr2.quad_vertex_buffer_id     = manifest_.get(CharactersBufferSlot::QUAD_LOOP_VERTEX_POD);
        chr2.morph_delta_buffer_id     = manifest_.get(CharactersBufferSlot::MORPH_DELTA_POD);
        chr2.morph_weight_buffer_id    = manifest_.get(CharactersBufferSlot::MORPH_WEIGHT_FLOAT32);
        chr2.active_partition_mask     = active_partition_mask_;
        chr2.execute<decltype(dummy_view), FlatStrategy>(ctx, dummy_view);
    }

    if ((p_stage_mask & CHARACTERS_STAGE_GARMENT_SURFACE_REFIT) != 0u) {
        SurfaceGarmentRefitTransformLogic<> chr3{};
        chr3.target_buffer_id            = manifest_.get(CharactersBufferSlot::GARMENT_POSITION_VEC3);
        chr3.garment_bind_buffer_id      = manifest_.get(CharactersBufferSlot::GARMENT_BIND_POD);
        chr3.quad_face_buffer_id         = manifest_.get(CharactersBufferSlot::QUAD_FACE_POD);
        chr3.deformed_position_buffer_id = manifest_.get(CharactersBufferSlot::DEFORMED_POSITION_VEC3);
        chr3.deformed_normal_buffer_id   = manifest_.get(CharactersBufferSlot::DEFORMED_NORMAL_VEC3);
        chr3.garment_velocity_buffer_id  = manifest_.get(CharactersBufferSlot::GARMENT_VELOCITY_VEC3);
        chr3.extra_normal_offset         = extra_garment_offset_;
        chr3.xpbd_relax_iterations       = xpbd_relax_iterations_;
        chr3.xpbd_compliance             = xpbd_compliance_;
        chr3.execute<decltype(dummy_view), FlatStrategy>(ctx, dummy_view);
    }

    return true;
}

void CharactersDOD::read_deformed_positions(std::vector<glm::vec3> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(CharactersBufferSlot::DEFORMED_POSITION_VEC3), r_out);
}

void CharactersDOD::read_deformed_normals(std::vector<glm::vec3> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(CharactersBufferSlot::DEFORMED_NORMAL_VEC3), r_out);
}

void CharactersDOD::read_garment_positions(std::vector<glm::vec3> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(CharactersBufferSlot::GARMENT_POSITION_VEC3), r_out);
}

void CharactersDOD::read_active_lod_masks(std::vector<uint32_t> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(CharactersBufferSlot::VERTEX_ACTIVE_LOD_MASK), r_out);
}

} // namespace ideam::core

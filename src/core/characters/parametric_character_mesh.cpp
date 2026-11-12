#include "parametric_character_mesh.h"
#include <algorithm>

namespace ideam::core {

void ParametricCharacterMesh::clear() noexcept {
    base_vertices_.clear();
    base_quads_.clear();
    sparse_morph_deltas_.clear();
}

void ParametricCharacterMesh::build_archetype_preset(CharacterBodyArchetype p_archetype) {
    clear();
    archetype_ = p_archetype;

    // Seed an initial quad-loop ring prism so the blueprint is immediately valid
    static constexpr glm::vec3 RING_OFFSETS[8] = {
        {-0.25f, 0.0f, -0.15f}, { 0.25f, 0.0f, -0.15f},
        { 0.25f, 0.0f,  0.15f}, {-0.25f, 0.0f,  0.15f},
        {-0.25f, 1.0f, -0.15f}, { 0.25f, 1.0f, -0.15f},
        { 0.25f, 1.0f,  0.15f}, {-0.25f, 1.0f,  0.15f}
    };
    base_vertices_.resize(8);
    for (size_t i = 0; i < 8; ++i) {
        base_vertices_[i].rest_position  = RING_OFFSETS[i];
        base_vertices_[i].rest_normal    = glm::normalize(RING_OFFSETS[i] - glm::vec3(0.0f, 0.5f, 0.0f));
        base_vertices_[i].partition_id   = (i < 4) ? PARTITION_PELVIS : PARTITION_TORSO;
        base_vertices_[i].min_lod_level  = 0;
        base_vertices_[i].edge_ring_mask = (i < 4) ? 1ULL : 2ULL;
    }
    base_quads_.push_back({0, 1, 5, 4});
    base_quads_.push_back({1, 2, 6, 5});
    base_quads_.push_back({2, 3, 7, 6});
    base_quads_.push_back({3, 0, 4, 7});
}

glm::vec3 ParametricCharacterMesh::evaluate_garment_surface_point(
    const SurfaceGarmentBindPod &p_bind
) const noexcept {
    if (base_quads_.empty()) {
        return glm::vec3(0.0f);
    }
    const uint32_t qid = std::min<uint32_t>(p_bind.quad_id, static_cast<uint32_t>(base_quads_.size() - 1));
    const auto &q = base_quads_[qid];
    const glm::vec3 &p00 = base_vertices_[q[0]].rest_position;
    const glm::vec3 &p10 = base_vertices_[q[1]].rest_position;
    const glm::vec3 &p11 = base_vertices_[q[2]].rest_position;
    const glm::vec3 &p01 = base_vertices_[q[3]].rest_position;

    const glm::vec3 pos = glm::mix(glm::mix(p00, p10, p_bind.u), glm::mix(p01, p11, p_bind.u), p_bind.v);
    const glm::vec3 nrm = glm::normalize(glm::cross(p10 - p00, p01 - p00));
    return pos + nrm * p_bind.d_normal;
}

} // namespace ideam::core

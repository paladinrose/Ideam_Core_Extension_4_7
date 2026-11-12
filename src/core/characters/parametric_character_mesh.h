#pragma once

/**
 * parametric_character_mesh.h
 * Pure C++26 Quad-Loop Character Mesh & Surface-Coordinate Apparel Fitter
 * (`src/core/characters/`).
 */

#include "character_common.h"
#include <array>
#include <vector>

namespace ideam::core {

class ParametricCharacterMesh {
private:
    CharacterBodyArchetype archetype_ = CharacterBodyArchetype::BIPED;
    std::vector<QuadLoopVertexPod> base_vertices_;
    std::vector<std::array<uint32_t, 4>> base_quads_;
    std::vector<MorphDeltaPod> sparse_morph_deltas_;

public:
    ParametricCharacterMesh() = default;
    ~ParametricCharacterMesh() = default;

    void clear() noexcept;
    void build_archetype_preset(CharacterBodyArchetype p_archetype);

    [[nodiscard]] CharacterBodyArchetype archetype() const noexcept { return archetype_; }
    [[nodiscard]] size_t vertex_count() const noexcept { return base_vertices_.size(); }
    [[nodiscard]] size_t quad_count() const noexcept { return base_quads_.size(); }

    /**
     * Evaluates a garment vertex bound at `(quad_id, u, v, d_normal)` against the current
     * deformed body quad surface.
     */
    [[nodiscard]] glm::vec3 evaluate_garment_surface_point(
        const SurfaceGarmentBindPod &p_bind
    ) const noexcept;
};

} // namespace ideam::core

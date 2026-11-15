#pragma once

/**
 * parametric_character_mesh.h
 * Pure C++26 Quad-Loop Character Mesh, Anatomical Sparse Morph Slider Evaluator,
 * and Curvilinear Surface-Coordinate Apparel Fitter (`src/core/characters/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.2 — `IdeamCharacters`)
 * ============================================================================
 * 1. QUAD-LOOP PARAMETRIC TOPOLOGY (`CHR1`):
 *    - Generates clean quad-ring loops across 6 body archetypes (`BIPED`, `QUADRUPED`,
 *      `AVIAN`, `FLYING_EYE`, `SERPENTINE`, `CUSTOM`).
 *    - Tags every vertex with `AnatomicalPartitionId` (`r_selection.partition_ids`),
 *      `edge_ring_mask` (`r_selection.group_masks`), and `min_lod_level` (`0..3` in
 *      `r_selection.lod_levels`) so quad-ring loops can be deterministically compacted
 *      or subdivided without breaking surface UVs or clothing bindings.
 *
 * 2. ANATOMICAL SPARSE MORPH SLIDERS (`CHR2`):
 *    - Stores `MorphDeltaPod` displacements in a flat sparse array indexed by
 *      `(vertex_index, morph_channel)`.
 *    - Supports partition-masked evaluation (`active_partition_mask`), including
 *      individual finger/digit curl channels (`MORPH_DIGIT_CURL_0..4`) that can be
 *      driven directly by OpenXR controller trigger/grip floats.
 *
 * 3. CURVILINEAR SURFACE GARMENT REFITTING (`CHR3`):
 *    - Binds apparel vertices in body-relative curvilinear coordinates
 *      `(quad_id, u, v, d_normal)`.
 *    - Evaluates bilinear quad positions and smooth interpolated normals over deformed
 *      character quads so garments refit automatically to any body archetype or morph
 *      slider combination before feeding `XPBDVertexSoftBodyTransformLogic` (`M31`).
 */

#include "character_common.h"
#include <span>
#include <vector>

namespace ideam::core {

class ParametricCharacterMesh {
private:
    CharacterBodyArchetype archetype_ = CharacterBodyArchetype::BIPED;
    std::vector<QuadLoopVertexPod> base_vertices_;
    std::vector<QuadFacePod> base_quads_;
    std::vector<MorphDeltaPod> sparse_morph_deltas_;

    uint32_t _append_quad_ring(
        const glm::vec3 &p_center,
        const glm::vec3 &p_radii,
        AnatomicalPartitionId p_partition,
        uint8_t p_min_lod_level,
        uint64_t p_edge_ring_mask,
        uint32_t p_ring_index,
        const glm::vec3 &p_axis_u = glm::vec3(1.0f, 0.0f, 0.0f),
        const glm::vec3 &p_axis_v = glm::vec3(0.0f, 0.0f, 1.0f)
    );

    void _bridge_quad_rings(uint32_t p_ring_a_start, uint32_t p_ring_b_start);
    void _build_default_sparse_morph_deltas();

public:
    ParametricCharacterMesh() = default;
    ~ParametricCharacterMesh() = default;

    void clear() noexcept;
    void build_archetype_preset(CharacterBodyArchetype p_archetype);

    [[nodiscard]] CharacterBodyArchetype archetype() const noexcept { return archetype_; }
    [[nodiscard]] size_t vertex_count() const noexcept { return base_vertices_.size(); }
    [[nodiscard]] size_t quad_count() const noexcept { return base_quads_.size(); }
    [[nodiscard]] size_t morph_delta_count() const noexcept { return sparse_morph_deltas_.size(); }

    [[nodiscard]] const std::vector<QuadLoopVertexPod> &base_vertices() const noexcept { return base_vertices_; }
    [[nodiscard]] std::vector<QuadLoopVertexPod> &base_vertices_mut() noexcept { return base_vertices_; }

    [[nodiscard]] const std::vector<QuadFacePod> &base_quads() const noexcept { return base_quads_; }
    [[nodiscard]] std::vector<QuadFacePod> &base_quads_mut() noexcept { return base_quads_; }

    [[nodiscard]] const std::vector<MorphDeltaPod> &sparse_morph_deltas() const noexcept { return sparse_morph_deltas_; }
    [[nodiscard]] std::vector<MorphDeltaPod> &sparse_morph_deltas_mut() noexcept { return sparse_morph_deltas_; }

    uint32_t add_vertex(const QuadLoopVertexPod &p_vertex);
    uint32_t add_quad(const QuadFacePod &p_quad);
    uint32_t add_morph_delta(const MorphDeltaPod &p_delta);

    // --- CHR1: Quad-Loop Subdivision & LOD Evaluation ---
    static uint32_t evaluate_quad_loop_lod(
        std::span<const QuadLoopVertexPod> p_vertices,
        std::span<const QuadFacePod> p_quads,
        uint8_t p_target_lod_level,
        uint64_t p_active_ring_mask,
        glm::vec3 *r_out_positions,
        glm::vec3 *r_out_normals,
        uint32_t *r_out_active_masks,
        size_t p_out_capacity
    ) noexcept;

    // --- CHR2: Sparse Morph Slider Evaluation Across Anatomical Partitions ---
    static void evaluate_sparse_morph_sliders(
        std::span<const QuadLoopVertexPod> p_vertices,
        std::span<const MorphDeltaPod> p_deltas,
        std::span<const float> p_channel_weights,
        uint32_t p_active_partition_mask,
        glm::vec3 *r_positions,
        glm::vec3 *r_normals,
        size_t p_vertex_count
    ) noexcept;

    // --- CHR3: Curvilinear Surface Garment Point & Batch Refitting ---
    [[nodiscard]] static glm::vec3 evaluate_garment_surface_point_on_buffers(
        const SurfaceGarmentBindPod &p_bind,
        std::span<const QuadFacePod> p_quads,
        std::span<const glm::vec3> p_deformed_positions,
        std::span<const glm::vec3> p_deformed_normals,
        float p_extra_normal_offset = 0.0f,
        glm::vec3 *r_out_surface_normal = nullptr
    ) noexcept;

    [[nodiscard]] glm::vec3 evaluate_garment_surface_point(
        const SurfaceGarmentBindPod &p_bind
    ) const noexcept;

    static void evaluate_garment_surface_batch(
        std::span<const SurfaceGarmentBindPod> p_binds,
        std::span<const QuadFacePod> p_quads,
        std::span<const glm::vec3> p_deformed_positions,
        std::span<const glm::vec3> p_deformed_normals,
        float p_extra_normal_offset,
        glm::vec3 *r_out_garment_positions,
        size_t p_out_capacity
    ) noexcept;

    void generate_default_garment_bindings(
        std::vector<SurfaceGarmentBindPod> &r_out_binds,
        uint32_t p_partition_mask = (1u << PARTITION_TORSO) | (1u << PARTITION_PELVIS),
        float p_normal_offset = 0.015f,
        uint32_t p_substeps_per_quad_axis = 2u
    ) const;
};

} // namespace ideam::core

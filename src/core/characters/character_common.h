#pragma once

/**
 * character_common.h
 * Pure C++26 POD structures for `IdeamCharacters` (`src/core/characters/`).
 *
 * Implements the Section 8 Parametric Critters & Auto-Fitting Apparel architecture:
 *   - `CharacterBodyArchetype`: `BIPED`, `QUADRUPED`, `AVIAN`, `FLYING_EYE`, `SERPENTINE`, `CUSTOM`.
 *   - `AnatomicalPartitionId`: Maps to `r_selection.partition_ids` (`HEAD`, `TORSO`, `PELVIS`,
 *     `UPPER_LIMB`, `LOWER_LIMB`, `DIGIT_0`..`DIGIT_4`, `TAIL`, `WING`).
 *   - `QuadLoopVertexPod`: Base mesh vertex tagged with `partition_id`, `edge_ring_mask`
 *     (`r_selection.group_masks`), and `min_lod_level` (`r_selection.lod_levels` 0..3).
 *   - `MorphDeltaPod`: `SPARSE_SET` morph slider displacement delta.
 *   - `SurfaceGarmentBindPod`: Curvilinear body-relative clothing coordinate
 *     `(quad_id, u, v, d_normal)` so garments automatically refit across any morph state.
 */

#include <cstdint>
#include <glm/glm.hpp>

namespace ideam::core {

enum class CharacterBodyArchetype : uint8_t {
    BIPED      = 0,
    QUADRUPED  = 1,
    AVIAN      = 2,
    FLYING_EYE = 3,
    SERPENTINE = 4,
    CUSTOM     = 5
};

enum AnatomicalPartitionId : uint16_t {
    PARTITION_TORSO      = 0,
    PARTITION_HEAD       = 1,
    PARTITION_PELVIS     = 2,
    PARTITION_UPPER_LIMB = 3,
    PARTITION_LOWER_LIMB = 4,
    PARTITION_DIGIT_0    = 5,
    PARTITION_DIGIT_1    = 6,
    PARTITION_DIGIT_2    = 7,
    PARTITION_DIGIT_3    = 8,
    PARTITION_DIGIT_4    = 9,
    PARTITION_TAIL       = 10,
    PARTITION_WING       = 11
};

struct QuadLoopVertexPod {
    glm::vec3 rest_position   = glm::vec3(0.0f);
    glm::vec3 rest_normal     = glm::vec3(0.0f, 1.0f, 0.0f);
    uint16_t  partition_id    = PARTITION_TORSO;
    uint8_t   min_lod_level   = 0; ///< 0 = coarse cage, 1..3 = progressively subdivided quad rings
    uint8_t   reserved        = 0;
    uint64_t  edge_ring_mask  = 0ULL;
};

struct MorphDeltaPod {
    uint32_t  vertex_index    = 0;
    uint32_t  morph_channel   = 0;
    glm::vec3 position_delta  = glm::vec3(0.0f);
    glm::vec3 normal_delta    = glm::vec3(0.0f);
};

/**
 * Curvilinear body-relative coordinate `(quad_id, u, v, d_normal)` for auto-fitting apparel.
 */
struct SurfaceGarmentBindPod {
    uint32_t quad_id   = 0;
    float    u         = 0.5f;
    float    v         = 0.5f;
    float    d_normal  = 0.01f;
};

} // namespace ideam::core

#pragma once

/**
 * character_common.h
 * Pure C++26 POD structures for `IdeamCharacters` (`src/core/characters/`) and Section 2.2
 * Parametric Characters, Active Ragdoll & Spline-Rope Bridges.
 *
 * Implements the Section 8 / Section 2.2 Parametric Critters & Auto-Fitting Apparel architecture:
 *   - `CharacterBodyArchetype`: `BIPED`, `QUADRUPED`, `AVIAN`, `FLYING_EYE`, `SERPENTINE`, `CUSTOM`.
 *   - `AnatomicalPartitionId`: Maps to `r_selection.partition_ids` (`TORSO`, `HEAD`, `PELVIS`,
 *     `UPPER_LIMB`, `LOWER_LIMB`, `DIGIT_0`..`DIGIT_4`, `TAIL`, `WING`).
 *   - `CharacterMorphChannelId`: Canonical anatomical sparse morph slider channels (`0..11`),
 *     including `MORPH_DIGIT_CURL_0..4` for direct OpenXR controller trigger/grip binding.
 *   - `QuadLoopVertexPod` (32B): Base mesh vertex tagged with `partition_id`, `edge_ring_mask`
 *     (`r_selection.group_masks`), and `min_lod_level` (`r_selection.lod_levels` 0..3).
 *   - `QuadFacePod` (16B): Quad-ring face indices `(v00, v10, v11, v01)`.
 *   - `MorphDeltaPod` (32B): `SPARSE_SET` morph slider displacement & normal delta.
 *   - `SurfaceGarmentBindPod` (16B): Curvilinear body-relative clothing coordinate
 *     `(quad_id, u, v, d_normal)` so garments automatically refit across any morph state.
 *   - `OmniRotationConstraintPod` (32B): Per-bone Euler rotation constraint (`M45` / `OmniRotationConstraint.cs`).
 *   - `PhysicalAnimatorSyncPod` (64B): Per-bone PD active-ragdoll synchronizer (`P14` / `PhysicalAnimator.cs`).
 *   - `PhysicsRopeSegmentPod` (32B): Catmull-Rom spline <-> hinge chain segment (`SPL12` / `PhysicsRope.cs`).
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string_view>

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
    PARTITION_WING       = 11,
    PARTITION_COUNT      = 12
};

static constexpr uint32_t PARTITION_MASK_ALL = (1u << PARTITION_COUNT) - 1u;

enum CharacterMorphChannelId : uint32_t {
    MORPH_STATURE_HEIGHT     = 0,  ///< Vertical stature / spine & limb elongation
    MORPH_TORSO_BULK         = 1,  ///< Chest, shoulder & waist radial girth (`PARTITION_TORSO` + `PARTITION_PELVIS`)
    MORPH_HEAD_SCALE         = 2,  ///< Cranial / ocular proportion (`PARTITION_HEAD`)
    MORPH_UPPER_LIMB_REACH   = 3,  ///< Arm / forelimb length & thickness (`PARTITION_UPPER_LIMB`)
    MORPH_LOWER_LIMB_STRIDE  = 4,  ///< Leg / hindlimb length & thickness (`PARTITION_LOWER_LIMB`)
    MORPH_DIGIT_CURL_0       = 5,  ///< Thumb / Digit 0 flexion (`PARTITION_DIGIT_0`, XR thumb/grip bindable)
    MORPH_DIGIT_CURL_1       = 6,  ///< Index / Digit 1 flexion (`PARTITION_DIGIT_1`, XR trigger bindable)
    MORPH_DIGIT_CURL_2       = 7,  ///< Middle / Digit 2 flexion (`PARTITION_DIGIT_2`, XR grip bindable)
    MORPH_DIGIT_CURL_3       = 8,  ///< Ring / Digit 3 flexion (`PARTITION_DIGIT_3`, XR grip bindable)
    MORPH_DIGIT_CURL_4       = 9,  ///< Pinky / Digit 4 flexion (`PARTITION_DIGIT_4`, XR grip bindable)
    MORPH_TAIL_LENGTH        = 10, ///< Caudal extension & taper (`PARTITION_TAIL`)
    MORPH_WING_SPAN          = 11, ///< Wing chord & span extension (`PARTITION_WING`)
    MORPH_DEFAULT_CHANNEL_COUNT = 12
};

[[nodiscard]] constexpr std::string_view anatomical_partition_to_string(uint16_t p_partition) noexcept {
    switch (p_partition) {
        case PARTITION_TORSO:      return "Torso";
        case PARTITION_HEAD:       return "Head";
        case PARTITION_PELVIS:     return "Pelvis";
        case PARTITION_UPPER_LIMB: return "Upper Limb";
        case PARTITION_LOWER_LIMB: return "Lower Limb";
        case PARTITION_DIGIT_0:    return "Digit 0 (Thumb)";
        case PARTITION_DIGIT_1:    return "Digit 1 (Index)";
        case PARTITION_DIGIT_2:    return "Digit 2 (Middle)";
        case PARTITION_DIGIT_3:    return "Digit 3 (Ring)";
        case PARTITION_DIGIT_4:    return "Digit 4 (Pinky)";
        case PARTITION_TAIL:       return "Tail";
        case PARTITION_WING:       return "Wing";
        default:                   return "Custom Partition";
    }
}

[[nodiscard]] constexpr std::string_view morph_channel_to_string(uint32_t p_channel) noexcept {
    switch (p_channel) {
        case MORPH_STATURE_HEIGHT:    return "Stature Height";
        case MORPH_TORSO_BULK:        return "Torso Bulk";
        case MORPH_HEAD_SCALE:        return "Head Scale";
        case MORPH_UPPER_LIMB_REACH:  return "Upper Limb Reach";
        case MORPH_LOWER_LIMB_STRIDE: return "Lower Limb Stride";
        case MORPH_DIGIT_CURL_0:      return "Digit 0 Curl (Thumb)";
        case MORPH_DIGIT_CURL_1:      return "Digit 1 Curl (Index)";
        case MORPH_DIGIT_CURL_2:      return "Digit 2 Curl (Middle)";
        case MORPH_DIGIT_CURL_3:      return "Digit 3 Curl (Ring)";
        case MORPH_DIGIT_CURL_4:      return "Digit 4 Curl (Pinky)";
        case MORPH_TAIL_LENGTH:       return "Tail Length";
        case MORPH_WING_SPAN:         return "Wing Span";
        default:                      return "Custom Morph Slider";
    }
}

/**
 * `QuadLoopVertexPod` (32 Bytes)
 * Base quad-loop vertex tagged with `partition_id`, `min_lod_level` (`0..3`), and `edge_ring_mask`.
 */
struct QuadLoopVertexPod {
    glm::vec3 rest_position   = glm::vec3(0.0f);             ///< 12B (0..12)
    glm::vec3 rest_normal     = glm::vec3(0.0f, 1.0f, 0.0f); ///< 12B (12..24)
    uint16_t  partition_id    = PARTITION_TORSO;             ///< 2B  (24..26)
    uint8_t   min_lod_level   = 0;                           ///< 1B  (26): 0 = coarse cage, 1..3 = subdivided quad rings
    uint8_t   reserved        = 0;                           ///< 1B  (27)
    uint32_t  ring_index      = 0;                           ///< 4B  (28..32) -> 32B before mask or 40B with 64-bit mask
    uint64_t  edge_ring_mask  = 0ULL;                        ///< 8B: Bitmask of topological quad-loop rings
};

/**
 * `QuadFacePod` (16 Bytes, alignas(16))
 * Counter-clockwise quad face indices `(v00, v10, v11, v01)` connecting adjacent quad rings.
 */
struct alignas(16) QuadFacePod {
    std::array<uint32_t, 4> indices = {0u, 0u, 0u, 0u};

    constexpr QuadFacePod() = default;
    constexpr QuadFacePod(uint32_t p_i0, uint32_t p_i1, uint32_t p_i2, uint32_t p_i3) noexcept
        : indices{p_i0, p_i1, p_i2, p_i3} {}

    [[nodiscard]] constexpr uint32_t &operator[](size_t p_idx) noexcept { return indices[p_idx]; }
    [[nodiscard]] constexpr const uint32_t &operator[](size_t p_idx) const noexcept { return indices[p_idx]; }
};
static_assert(sizeof(QuadFacePod) == 16, "QuadFacePod must be 16 bytes.");

/**
 * `MorphDeltaPod` (32 Bytes, alignas(16))
 * Sparse-set morph slider displacement and normal delta record (`CHR2`).
 */
struct alignas(16) MorphDeltaPod {
    uint32_t  vertex_index    = 0;               ///< 4B (0..4)
    uint32_t  morph_channel   = 0;               ///< 4B (4..8)
    glm::vec3 position_delta  = glm::vec3(0.0f); ///< 12B (8..20)
    glm::vec3 normal_delta    = glm::vec3(0.0f); ///< 12B (20..32) -> 32B total
};
static_assert(sizeof(MorphDeltaPod) == 32, "MorphDeltaPod must be 32 bytes.");

/**
 * `SurfaceGarmentBindPod` (16 Bytes, alignas(16))
 * Curvilinear body-relative coordinate `(quad_id, u, v, d_normal)` for auto-fitting apparel (`CHR3`).
 */
struct alignas(16) SurfaceGarmentBindPod {
    uint32_t quad_id   = 0;     ///< 4B (0..4): Host body quad index in `base_quads_`
    float    u         = 0.5f;  ///< 4B (4..8): Bilinear u parameter in [0, 1]
    float    v         = 0.5f;  ///< 4B (8..12): Bilinear v parameter in [0, 1]
    float    d_normal  = 0.01f; ///< 4B (12..16): Normal offset distance in meters -> 16B total
};
static_assert(sizeof(SurfaceGarmentBindPod) == 16, "SurfaceGarmentBindPod must be 16 bytes.");

/**
 * `OmniRotationAxisFlags` (`M45` — `OmniRotationConstraint.cs` port)
 */
enum OmniRotationAxisFlags : uint8_t {
    OMNI_AXIS_NONE = 0u,
    OMNI_AXIS_X    = 1u << 0,
    OMNI_AXIS_Y    = 1u << 1,
    OMNI_AXIS_Z    = 1u << 2,
    OMNI_AXIS_ALL  = OMNI_AXIS_X | OMNI_AXIS_Y | OMNI_AXIS_Z
};

/**
 * `OmniRotationConstraintPod` (32 Bytes, alignas(16))
 * Per-bone Euler rotation constraint descriptor for `OmniRotationConstraintTransformLogic` (`M45`).
 */
struct alignas(16) OmniRotationConstraintPod {
    uint32_t  source_bone_index = 0;               ///< 4B (0..4)
    uint32_t  target_bone_index = 0;               ///< 4B (4..8)
    glm::vec3 angle_offset_deg  = glm::vec3(0.0f); ///< 12B (8..20): Euler offset in degrees
    float     blend_weight      = 1.0f;            ///< 4B (20..24): Constraint blend weight in [0, 1]
    uint8_t   axis_bind_mask    = OMNI_AXIS_ALL;   ///< 1B (24): `bindX | bindY | bindZ`
    uint8_t   axis_invert_mask  = OMNI_AXIS_NONE;  ///< 1B (25): `invertX | invertY | invertZ`
    uint8_t   is_local          = 1u;              ///< 1B (26): `1` = local pose Euler, `0` = global pose Euler
    uint8_t   reserved[5]       = {0};             ///< 5B (27..32) -> 32B total
};
static_assert(sizeof(OmniRotationConstraintPod) == 32, "OmniRotationConstraintPod must be 32 bytes.");

/**
 * `PhysicalAnimatorSyncPod` (64 Bytes, alignas(64))
 * Per-bone active-ragdoll PD controller settings ported from `PhysicalAnimator.cs` (`SynchronizerSettings`).
 */
struct alignas(64) PhysicalAnimatorSyncPod {
    uint32_t bone_index          = 0;       ///< 4B (0..4)
    uint32_t body_rid_slot       = 0;       ///< 4B (4..8)
    float    position_spring     = 800.0f;  ///< 4B (8..12)
    float    position_damper     = 45.0f;   ///< 4B (12..16)
    float    position_min_force  = 0.0f;    ///< 4B (16..20)
    float    position_max_force  = 2500.0f; ///< 4B (20..24)
    float    rotation_spring     = 400.0f;  ///< 4B (24..28)
    float    rotation_damper     = 25.0f;   ///< 4B (28..32)
    float    rotation_min_torque = 0.0f;    ///< 4B (32..36)
    float    rotation_max_torque = 1200.0f; ///< 4B (36..40)
    float    blend_weight        = 1.0f;    ///< 4B (40..44)
    uint8_t  is_active           = 1u;      ///< 1B (44)
    uint8_t  reserved[19]        = {0};     ///< 19B (45..64) -> 64B total
};
static_assert(sizeof(PhysicalAnimatorSyncPod) == 64, "PhysicalAnimatorSyncPod must be 64 bytes.");

/**
 * `PhysicsRopeSegmentPod` (32 Bytes, alignas(16))
 * Per-segment rigid-body hinge solver record ported from `PhysicsRope.cs` (`SPL12`).
 */
struct alignas(16) PhysicsRopeSegmentPod {
    uint32_t  solver_index    = 0;               ///< 4B (0..4): Physics hinge solver index (`0 .. numberOfPhysicsSolvers - 1`)
    uint32_t  segment_index   = 0;               ///< 4B (4..8): Sub-segment index (`0 .. numberOfSegmentsPerSolver - 1`)
    float     rest_length     = 0.25f;           ///< 4B (8..12)
    float     hinge_stiffness = 120.0f;          ///< 4B (12..16)
    glm::vec3 position        = glm::vec3(0.0f); ///< 12B (16..28)
    float     hinge_damping   = 10.0f;           ///< 4B (28..32) -> 32B total
};
static_assert(sizeof(PhysicsRopeSegmentPod) == 32, "PhysicsRopeSegmentPod must be 32 bytes.");

} // namespace ideam::core

#pragma once

/**
 * characters_dod_common.h
 * Buffer slots, capacities, manifests, and pipeline stage masks for `CharactersDOD`
 * (`src/core/tasks/domains/characters/`).
 */

#include "../../../characters/character_common.h"
#include "../../../characters/parametric_character_mesh.h"
#include "../../../memory/memory_manager_dod.h"
namespace ideam::core { class TaskGraphDOD; }
#include <array>
#include <cstdint>

namespace ideam::core {

enum class CharactersBufferSlot : uint32_t {
    QUAD_LOOP_VERTEX_POD    = 0, ///< `QuadLoopVertexPod` (40B)
    QUAD_FACE_POD           = 1, ///< `QuadFacePod` (16B)
    MORPH_DELTA_POD         = 2, ///< `MorphDeltaPod` (32B)
    MORPH_WEIGHT_FLOAT32    = 3, ///< `float` (4B) per morph slider channel
    DEFORMED_POSITION_VEC3  = 4, ///< `glm::vec3` / `godot::Vector3` (12B) deformed body positions
    DEFORMED_NORMAL_VEC3    = 5, ///< `glm::vec3` / `godot::Vector3` (12B) deformed body normals
    GARMENT_BIND_POD        = 6, ///< `SurfaceGarmentBindPod` (16B) curvilinear `(quad_id, u, v, d_normal)`
    GARMENT_POSITION_VEC3   = 7, ///< `glm::vec3` / `godot::Vector3` (12B) refitted garment positions (feeds XPBD!)
    GARMENT_VELOCITY_VEC3   = 8, ///< `glm::vec3` / `godot::Vector3` (12B) garment XPBD velocities
    VERTEX_ACTIVE_LOD_MASK  = 9, ///< `uint32_t` (4B) active LOD & partition mask per vertex
    COUNT                   = 10
};

enum CharactersStageFlags : uint32_t {
    CHARACTERS_STAGE_NONE                  = 0u,
    CHARACTERS_STAGE_QUAD_LOOP_LOD         = 1u << 0, ///< `CHR1`: `QuadLoopSubdivisionAndLODTransformLogic`
    CHARACTERS_STAGE_SPARSE_MORPH_SLIDERS  = 1u << 1, ///< `CHR2`: `SparseMorphSliderEvaluateTransformLogic`
    CHARACTERS_STAGE_GARMENT_SURFACE_REFIT = 1u << 2, ///< `CHR3`: `SurfaceGarmentRefitTransformLogic`
    CHARACTERS_STAGE_DEFAULT_PIPELINE_MASK =
        CHARACTERS_STAGE_QUAD_LOOP_LOD |
        CHARACTERS_STAGE_SPARSE_MORPH_SLIDERS |
        CHARACTERS_STAGE_GARMENT_SURFACE_REFIT
};

struct CharactersBufferCapacities {
    size_t max_vertices        = 1024;
    size_t max_quads           = 1024;
    size_t max_morph_deltas    = 4096;
    size_t max_morph_channels  = 64;
    size_t max_garment_vertices = 2048;
};

struct CharactersBufferManifest {
    std::array<uint32_t, static_cast<size_t>(CharactersBufferSlot::COUNT)> buffer_ids = {
        INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID,
        INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID
    };

    [[nodiscard]] constexpr uint32_t get(CharactersBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    constexpr void set(CharactersBufferSlot p_slot, uint32_t p_id) noexcept {
        buffer_ids[static_cast<size_t>(p_slot)] = p_id;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return get(CharactersBufferSlot::QUAD_LOOP_VERTEX_POD) != INVALID_ID &&
               get(CharactersBufferSlot::QUAD_FACE_POD) != INVALID_ID &&
               get(CharactersBufferSlot::MORPH_DELTA_POD) != INVALID_ID &&
               get(CharactersBufferSlot::MORPH_WEIGHT_FLOAT32) != INVALID_ID &&
               get(CharactersBufferSlot::DEFORMED_POSITION_VEC3) != INVALID_ID &&
               get(CharactersBufferSlot::DEFORMED_NORMAL_VEC3) != INVALID_ID &&
               get(CharactersBufferSlot::GARMENT_BIND_POD) != INVALID_ID &&
               get(CharactersBufferSlot::GARMENT_POSITION_VEC3) != INVALID_ID;
    }
};

} // namespace ideam::core

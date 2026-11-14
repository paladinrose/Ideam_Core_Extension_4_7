#pragma once

/**
 * procgen_common.h
 * Shared pure C++26 Procedural Generation primitives (`src/core/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (THREE-PILLAR PROCEDURAL ARCHITECTURE)
 * ============================================================================
 * 1. PILLAR 1 — CSS-STYLE CASCADING ANALYSIS & GENERATION RULES:
 *    - Parallels `GameplayRule` and `GameplayStyle` in `IdeamGames`.
 *    - Both `AnalysisRule` and `GenerationRule` operate on a cascading, tag-and-type-matched
 *      specificity system (`CascadeLayer`, `RuleScope`, `priority`, `is_important`, `specificity`).
 *
 * 2. PILLAR 2 — UNIFIED GODOT-REFLECTION-BOUNDED ENVIRONMENT:
 *    - All procedural algorithms (`IdeamProceduralGeneration`, `IdeamWaveFunctionCollapse`,
 *      `IdeamQuantumDarwinism`, and future pattern solvers) define and mutate the same
 *      `ProceduralEnvironment`, bounded by Godot's reflection capabilities (`ClassDB`,
 *      `SceneState`, node hierarchies, names, `RID`s, properties, value ranges, methods, signals).
 *
 * 3. PILLAR 3 — TASKGRAPH-FIRST SYNERGY (`IdeamTasks` / `TaskGraphDOD`):
 *    - Pattern analysis, rule cascade compilation, forward WFC solving, and Quantum Darwinism
 *      einselection conflict repair are decomposed into 64-byte cache-aligned TaskGraph nodes
 *      that share contiguous DOD buffers.
 */

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

enum class SolverStatus : uint8_t {
    UNSOLVED      = 0,
    IN_PROGRESS   = 1,
    SOLVED        = 2,
    CONTRADICTION = 3
};

using ProcGenSolverStatus = SolverStatus;

/**
 * Compile-time / runtime FNV-1a 32-bit string hash for Godot reflection names
 * (class names, node names, prefixes, suffixes, property/method/signal names).
 * Empty strings hash to `0u` (wildcard / unconstrained selector).
 */
[[nodiscard]] constexpr uint32_t hash_reflection_symbol(std::string_view p_str) noexcept {
    if (p_str.empty()) return 0u;
    uint32_t h = 2166136261u;
    for (char c : p_str) {
        h ^= static_cast<uint8_t>(c);
        h *= 16777619u;
    }
    return (h == 0u) ? 1u : h;
}

/**
 * Deterministic SplitMix64 seed stream for reproducible procedural generation,
 * `ProceduralPattern` sampling, WFC tie-breaking, and Quantum Darwinism relaxation.
 */
struct SeedStreamPOD {
    uint64_t state = 0x9E3779B97F4A7C15ULL;

    constexpr explicit SeedStreamPOD(uint64_t p_seed = 0x9E3779B97F4A7C15ULL) noexcept
        : state(p_seed ? p_seed : 0x9E3779B97F4A7C15ULL) {}

    [[nodiscard]] constexpr uint64_t next_u64() noexcept {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    [[nodiscard]] constexpr float next_unit_float() noexcept {
        return static_cast<float>((next_u64() >> 40) * (1.0 / 16777216.0));
    }

    [[nodiscard]] constexpr uint32_t next_range_u32(uint32_t p_bound) noexcept {
        if (p_bound <= 1u) return 0u;
        return static_cast<uint32_t>(next_u64() % static_cast<uint64_t>(p_bound));
    }
};

using ProcGenSeedStreamPOD = SeedStreamPOD;

/**
 * 256-bit (`4 x uint64_t`) superposition domain mask supporting up to 256 distinct states/tokens
 * in a `ProceduralEnvironment` or paged WFC/Quantum Darwinism solver.
 */
struct BitmaskDomain256POD {
    std::array<uint64_t, 4> words{0ULL, 0ULL, 0ULL, 0ULL};

    constexpr void clear() noexcept {
        words = {0ULL, 0ULL, 0ULL, 0ULL};
    }

    constexpr void fill_first_n(uint32_t p_state_count) noexcept {
        clear();
        for (uint32_t s = 0; s < p_state_count && s < 256u; ++s) {
            set_bit(s);
        }
    }

    constexpr void set_bit(uint32_t p_state) noexcept {
        if (p_state < 256u) {
            words[p_state >> 6] |= (1ULL << (p_state & 63u));
        }
    }

    constexpr void clear_bit(uint32_t p_state) noexcept {
        if (p_state < 256u) {
            words[p_state >> 6] &= ~(1ULL << (p_state & 63u));
        }
    }

    [[nodiscard]] constexpr bool test_bit(uint32_t p_state) const noexcept {
        return (p_state < 256u) && ((words[p_state >> 6] & (1ULL << (p_state & 63u))) != 0ULL);
    }

    [[nodiscard]] constexpr uint32_t popcount() const noexcept {
        return static_cast<uint32_t>(
            std::popcount(words[0]) +
            std::popcount(words[1]) +
            std::popcount(words[2]) +
            std::popcount(words[3])
        );
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return (words[0] | words[1] | words[2] | words[3]) == 0ULL;
    }

    [[nodiscard]] constexpr int32_t first_set_state() const noexcept {
        for (uint32_t w = 0; w < 4u; ++w) {
            if (words[w] != 0ULL) {
                return static_cast<int32_t>((w << 6) + static_cast<uint32_t>(std::countr_zero(words[w])));
            }
        }
        return -1;
    }

    constexpr bool intersect_with(const BitmaskDomain256POD &p_other) noexcept {
        bool changed = false;
        for (size_t i = 0; i < 4u; ++i) {
            const uint64_t next = words[i] & p_other.words[i];
            if (next != words[i]) {
                words[i] = next;
                changed  = true;
            }
        }
        return changed;
    }

    constexpr void union_with(const BitmaskDomain256POD &p_other) noexcept {
        for (size_t i = 0; i < 4u; ++i) {
            words[i] |= p_other.words[i];
        }
    }

    constexpr bool subtract(const BitmaskDomain256POD &p_other) noexcept {
        bool changed = false;
        for (size_t i = 0; i < 4u; ++i) {
            const uint64_t next = words[i] & ~p_other.words[i];
            if (next != words[i]) {
                words[i] = next;
                changed  = true;
            }
        }
        return changed;
    }

    [[nodiscard]] constexpr bool overlaps(const BitmaskDomain256POD &p_other) const noexcept {
        return ((words[0] & p_other.words[0]) |
                (words[1] & p_other.words[1]) |
                (words[2] & p_other.words[2]) |
                (words[3] & p_other.words[3])) != 0ULL;
    }

    [[nodiscard]] constexpr bool operator==(const BitmaskDomain256POD &p_other) const noexcept {
        return words[0] == p_other.words[0] &&
               words[1] == p_other.words[1] &&
               words[2] == p_other.words[2] &&
               words[3] == p_other.words[3];
    }

    [[nodiscard]] inline float shannon_entropy() const noexcept {
        const uint32_t c = popcount();
        if (c <= 1u) return 0.0f;
        return std::log2(static_cast<float>(c));
    }
};

struct PinnedSlotConstraintPOD {
    uint32_t slot_id      = 0;
    uint32_t pinned_state = 0;
};

/**
 * ReflectionSignaturePOD (32 Bytes, alignas(16))
 * Compact DOD representation of a Godot Object/Node/Resource reflection signature
 * harvested from `ClassDB`, `SceneState`, `Object`, or `Resource`.
 *
 * Used by both `AnalysisRulePOD` (to filter which source nodes/objects to inspect)
 * and `GenerationRulePOD` (to match candidate tokens and slots in `ProceduralEnvironment`).
 */
struct alignas(16) ReflectionSignaturePOD {
    uint32_t class_name_hash     = 0; ///< Hash of Godot class name (e.g., "Node3D", "Resource")
    uint32_t object_name_hash    = 0; ///< Hash of exact Node/Resource name (e.g., "CastleTower")
    uint32_t name_prefix_hash    = 0; ///< Optional prefix match (e.g., "Castle")
    uint32_t name_suffix_hash    = 0; ///< Optional suffix match (e.g., "_cell")
    uint32_t group_or_tag_mask   = 0; ///< Bitmask of assigned Godot groups / Sememe tags
    uint32_t property_trait_mask = 0; ///< Bitmask of reflected property presence / range matches
    uint32_t method_trait_mask   = 0; ///< Bitmask of reflected method presence (`has_method`)
    uint32_t signal_trait_mask   = 0; ///< Bitmask of reflected signal presence (`has_signal`)

    /**
     * Evaluates whether this selector signature matches a candidate token/node signature.
     * Any `0` field in `*this` acts as a wildcard (`*` in CSS).
     */
    [[nodiscard]] constexpr bool matches_candidate(const ReflectionSignaturePOD &p_cand) const noexcept {
        if (class_name_hash != 0u && p_cand.class_name_hash != class_name_hash) return false;
        if (object_name_hash != 0u && p_cand.object_name_hash != object_name_hash) return false;
        if (name_prefix_hash != 0u && p_cand.name_prefix_hash != name_prefix_hash) return false;
        if (name_suffix_hash != 0u && p_cand.name_suffix_hash != name_suffix_hash) return false;
        if ((group_or_tag_mask & p_cand.group_or_tag_mask) != group_or_tag_mask) return false;
        if ((property_trait_mask & p_cand.property_trait_mask) != property_trait_mask) return false;
        if ((method_trait_mask & p_cand.method_trait_mask) != method_trait_mask) return false;
        if ((signal_trait_mask & p_cand.signal_trait_mask) != signal_trait_mask) return false;
        return true;
    }

    /**
     * Computes CSS-style selector specificity `(ID/Name > Prefix/Suffix > Props/Methods/Signals > Group/Sememe > Class)`.
     */
    [[nodiscard]] constexpr uint16_t compute_selector_specificity() const noexcept {
        uint32_t score = 0u;
        if (object_name_hash != 0u) score += 1000u;
        if (name_prefix_hash != 0u) score += 250u;
        if (name_suffix_hash != 0u) score += 250u;
        score += static_cast<uint32_t>(std::popcount(property_trait_mask)) * 50u;
        score += static_cast<uint32_t>(std::popcount(method_trait_mask)) * 50u;
        score += static_cast<uint32_t>(std::popcount(signal_trait_mask)) * 50u;
        score += static_cast<uint32_t>(std::popcount(group_or_tag_mask)) * 20u;
        if (class_name_hash != 0u) score += 10u;
        return static_cast<uint16_t>(std::min<uint32_t>(65535u, score));
    }
};
static_assert(sizeof(ReflectionSignaturePOD) == 32, "ReflectionSignaturePOD must be 32 bytes.");

} // namespace ideam::core

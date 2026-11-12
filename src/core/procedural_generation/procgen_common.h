#pragma once

/**
 * procgen_common.h
 * Shared pure C++26 Procedural Generation primitives (`src/core/procedural_generation/`).
 *
 * Serves as the foundational substrate that `IdeamWaveFunctionCollapse` (`src/core/wave_function_collapse/`)
 * leans on — exactly as `IdeamSplines` and `IdeamTiles` lean on `IdeamGeometry`:
 *   - `ProcGenSeedStreamPOD`: Deterministic SplitMix64 PRNG stream with channel sub-seeding.
 *   - `BitmaskDomain256POD`: 256-bit (`4 x uint64_t`) superposition domain bitmask used by
 *     `ConstraintGraphSolver` and `WFCSolverCore` (`PAGED` Bitmask Domains).
 *   - `PinnedSlotConstraintPOD`: Pre-collapsed landmark constraint (`domains[slot] = mask`).
 */

#include <array>
#include <bit>
#include <cstdint>

namespace ideam::core {

enum class ProcGenSolverStatus : uint8_t {
    UNSOLVED      = 0,
    IN_PROGRESS   = 1,
    SOLVED        = 2,
    CONTRADICTION = 3
};

/**
 * Deterministic SplitMix64 seed stream for reproducible procedural generation & WFC tie-breaking.
 */
struct ProcGenSeedStreamPOD {
    uint64_t state = 0x9E3779B97F4A7C15ULL;

    constexpr explicit ProcGenSeedStreamPOD(uint64_t p_seed = 0x9E3779B97F4A7C15ULL) noexcept
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
};

/**
 * 256-bit (`4 x uint64_t`) superposition domain mask supporting up to 256 distinct states/tiles.
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
};

struct PinnedSlotConstraintPOD {
    uint32_t slot_id       = 0;
    uint32_t pinned_state  = 0;
};

} // namespace ideam::core

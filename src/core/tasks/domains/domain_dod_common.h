#pragma once

/**
 * domain_dod_common.h
 * Shared pure C++26 Data-Oriented Design (DOD) slot allocators and memory grant
 * requirement helpers used across IDEAM task domains (IdeamGames, IdeamNarratives, etc.).
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"

namespace ideam::core {

/**
 * SlotIndexAllocator
 * O(1) free-list + dense bitset slot allocator for managing rows inside
 * domain DOD buffer tables without shifting arrays or invalidating indices.
 */
class SlotIndexAllocator {
private:
    uint32_t capacity_        = 0;
    uint32_t high_water_mark_ = 0;
    uint32_t active_count_    = 0;
    std::vector<uint32_t> free_list_;
    std::vector<uint64_t> active_bitset_;
    std::vector<uint32_t> generations_;

public:
    SlotIndexAllocator() = default;
    explicit SlotIndexAllocator(uint32_t p_capacity) {
        initialize(p_capacity);
    }

    void initialize(uint32_t p_capacity) {
        capacity_ = p_capacity;
        high_water_mark_ = 0;
        active_count_ = 0;
        free_list_.clear();
        free_list_.reserve(std::min<uint32_t>(p_capacity, 1024u));
        active_bitset_.assign((p_capacity + 63u) / 64u, 0ULL);
        generations_.assign(p_capacity, 1u);
    }

    void clear() noexcept {
        high_water_mark_ = 0;
        active_count_ = 0;
        free_list_.clear();
        std::fill(active_bitset_.begin(), active_bitset_.end(), 0ULL);
    }

    [[nodiscard]] uint32_t allocate() noexcept {
        uint32_t idx = INVALID_ID;
        if (!free_list_.empty()) {
            idx = free_list_.back();
            free_list_.pop_back();
        } else if (high_water_mark_ < capacity_) {
            idx = high_water_mark_++;
        } else {
            return INVALID_ID;
        }

        active_bitset_[idx >> 6u] |= (1ULL << (idx & 63u));
        ++active_count_;
        return idx;
    }

    bool release(uint32_t p_index) noexcept {
        if (!is_valid(p_index)) {
            return false;
        }
        active_bitset_[p_index >> 6u] &= ~(1ULL << (p_index & 63u));
        ++generations_[p_index];
        free_list_.push_back(p_index);
        --active_count_;

        while (high_water_mark_ > 0 && !is_valid(high_water_mark_ - 1u)) {
            // Trim trailing high-water mark if top slots are free, keeping free_list consistent
            break;
        }
        return true;
    }

    [[nodiscard]] inline bool is_valid(uint32_t p_index) const noexcept {
        if (p_index >= high_water_mark_) return false;
        return (active_bitset_[p_index >> 6u] & (1ULL << (p_index & 63u))) != 0ULL;
    }

    [[nodiscard]] inline uint32_t get_generation(uint32_t p_index) const noexcept {
        return (p_index < generations_.size()) ? generations_[p_index] : 0u;
    }

    [[nodiscard]] constexpr uint32_t get_capacity() const noexcept { return capacity_; }
    [[nodiscard]] constexpr uint32_t get_high_water_mark() const noexcept { return high_water_mark_; }
    [[nodiscard]] constexpr uint32_t get_active_count() const noexcept { return active_count_; }
    [[nodiscard]] std::span<const uint64_t> get_active_bitset() const noexcept { return active_bitset_; }
};

/**
 * make_grant_requirement
 * Inline helper to construct a 128-byte GrantPartPOD requirement for TaskGraphDOD::set_node_requirements().
 */
[[nodiscard]] constexpr GrantPartPOD make_grant_requirement(
    uint32_t p_buffer_id,
    BufferAccessMode p_access,
    uint32_t p_element_stride,
    int64_t p_element_count = 0,
    SelectionMode p_mode = SelectionMode::RANGE
) noexcept {
    GrantPartPOD part{};
    part.buffer_id = p_buffer_id;
    part.access_mode = p_access;
    part.element_stride = p_element_stride;
    part.column_id = 0;
    part.is_contiguous = (p_mode == SelectionMode::RANGE);
    part.selection.target_buffer_id = p_buffer_id;
    part.selection.mode = p_mode;
    part.selection.start_index = 0;
    part.selection.element_count = p_element_count;
    part.selection.capacity = p_element_count;
    return part;
}

} // namespace ideam::core
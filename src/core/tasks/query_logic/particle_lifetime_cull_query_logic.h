#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * V9: ParticleLifetimeCullQueryLogic
 * Deactivates expired particles (Age >= Lifetime) in r_selection and releases their bit slots
 * back into r_selection.unclaimed_mask so V1 ParticleEmitterClaimTransformLogic can recycle them.
 */
struct ParticleLifetimeCullQueryLogic {
    static constexpr std::string_view ui_name = "Particle Lifetime Expiration Cull & Anti-Grant Release";
    static constexpr std::string_view ui_category = "Particles/Query";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR2,
        DataType::FLOAT32
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::SLEEP,
        QueryOp::WAKE,
        QueryOp::INVERT
    };

    float fallback_lifetime = 2.0f;
    bool release_to_unclaimed_mask = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("fallback_lifetime", godot::Variant::FLOAT, fallback_lifetime);
        add_prop("release_to_unclaimed_mask", godot::Variant::BOOL, release_to_unclaimed_mask);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("fallback_lifetime")) fallback_lifetime = std::max(1e-4f, static_cast<float>(p_props["fallback_lifetime"]));
        if (p_props.has("release_to_unclaimed_mask")) release_to_unclaimed_mask = static_cast<bool>(p_props["release_to_unclaimed_mask"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& life_part = p_ctx.grant->parts[0];
        if (!life_part.raw_base_ptr) return;

        const GrantPartPOD* max_life_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        MemoryBufferSelectionPOD& sel = life_part.selection;
        const bool is_packed_v2 = (life_part.element_stride >= sizeof(godot::Vector2));
        const uint32_t l_stride = std::max<uint32_t>(is_packed_v2 ? sizeof(godot::Vector2) : sizeof(float), life_part.element_stride);
        const int64_t n_particles = std::min<int64_t>(
            (sel.capacity > 0) ? sel.capacity : static_cast<int64_t>(life_part.capacity_bytes / l_stride),
            static_cast<int64_t>(life_part.capacity_bytes / l_stride)
        );

        const float* max_lives = max_life_part ? reinterpret_cast<const float*>(max_life_part->raw_base_ptr) : nullptr;
        const int64_t max_l_count = max_life_part ? static_cast<int64_t>(max_life_part->capacity_bytes / sizeof(float)) : 0;

        for (int64_t i = 0; i < n_particles; ++i) {
            float age = 0.0f;
            float lifetime = fallback_lifetime;

            if (is_packed_v2) {
                const godot::Vector2& al = *reinterpret_cast<const godot::Vector2*>(life_part.raw_base_ptr + static_cast<size_t>(i) * l_stride);
                age = al.x;
                lifetime = al.y;
            } else {
                age = *reinterpret_cast<const float*>(life_part.raw_base_ptr + static_cast<size_t>(i) * l_stride);
                if (max_lives && i < max_l_count) {
                    lifetime = max_lives[i];
                }
            }

            if (age >= lifetime) {
                const uint64_t word = static_cast<uint64_t>(i) >> 6;
                const uint64_t bit = 1ULL << (i & 63);

                if (release_to_unclaimed_mask && sel.unclaimed_mask) {
                    sel.unclaimed_mask[word] |= bit;
                }

                if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                    const bool was_set = (sel.data.bitset[word] & bit) != 0;
                    if (p_op == QueryOp::SLEEP && was_set) {
                        sel.data.bitset[word] &= ~bit;
                        sel.element_count = std::max<int64_t>(0, sel.element_count - 1);
                    } else if (p_op == QueryOp::WAKE && !was_set) {
                        sel.data.bitset[word] |= bit;
                        sel.element_count++;
                    } else if (p_op == QueryOp::INVERT) {
                        sel.data.bitset[word] ^= bit;
                        sel.element_count += was_set ? -1 : 1;
                    }
                }
            }
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::SLEEP);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, QueryOp p_op, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, p_op);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::SLEEP);
    }
};

} // namespace ideam::core
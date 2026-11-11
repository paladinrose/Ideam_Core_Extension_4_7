#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * V1: ParticleEmitterClaimTransformLogic
 * Claims inactive particle slots from r_selection.unclaimed_mask via hardware countr_zero bit-scanning,
 * initializes Position (Part 0), Velocity (Part 1), and Age/Lifetime (Part 2), and wakes their active
 * bits in r_selection.
 */
struct ParticleEmitterClaimTransformLogic {
    static constexpr std::string_view ui_name = "Particle Emitter Anti-Grant Slot Claim & Spawn";
    static constexpr std::string_view ui_category = "Particles/Lifecycle";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR3,
        DataType::VECTOR2,
        DataType::FLOAT32
    };

    int32_t emitter_shape = 0; // 0 = Point/Sphere, 1 = Box Extents, 2 = Directional Cone
    godot::Vector3 emitter_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 emitter_extents = godot::Vector3(0.5f, 0.5f, 0.5f);
    godot::Vector3 initial_velocity = godot::Vector3(0.0f, 5.0f, 0.0f);
    float velocity_spread = 1.0f;
    float base_lifetime = 2.0f;
    float lifetime_randomness = 0.25f;
    int32_t burst_spawn_count = 16;
    int32_t random_seed = 9001;

    mutable uint32_t spawn_counter = 0;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("emitter_shape", godot::Variant::INT, emitter_shape, 2, "Sphere Volume,Box Extents,Directional Cone");
        add_prop("emitter_origin", godot::Variant::VECTOR3, emitter_origin);
        add_prop("emitter_extents", godot::Variant::VECTOR3, emitter_extents);
        add_prop("initial_velocity", godot::Variant::VECTOR3, initial_velocity);
        add_prop("velocity_spread", godot::Variant::FLOAT, velocity_spread);
        add_prop("base_lifetime", godot::Variant::FLOAT, base_lifetime);
        add_prop("lifetime_randomness", godot::Variant::FLOAT, lifetime_randomness);
        add_prop("burst_spawn_count", godot::Variant::INT, burst_spawn_count);
        add_prop("random_seed", godot::Variant::INT, random_seed);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("emitter_shape")) emitter_shape = std::clamp<int32_t>(static_cast<int32_t>(p_props["emitter_shape"]), 0, 2);
        if (p_props.has("emitter_origin")) emitter_origin = p_props["emitter_origin"];
        if (p_props.has("emitter_extents")) emitter_extents = p_props["emitter_extents"];
        if (p_props.has("initial_velocity")) initial_velocity = p_props["initial_velocity"];
        if (p_props.has("velocity_spread")) velocity_spread = std::max(0.0f, static_cast<float>(p_props["velocity_spread"]));
        if (p_props.has("base_lifetime")) base_lifetime = std::max(1e-3f, static_cast<float>(p_props["base_lifetime"]));
        if (p_props.has("lifetime_randomness")) lifetime_randomness = std::clamp(static_cast<float>(p_props["lifetime_randomness"]), 0.0f, 0.99f);
        if (p_props.has("burst_spawn_count")) burst_spawn_count = std::max<int32_t>(0, static_cast<int32_t>(p_props["burst_spawn_count"]));
        if (p_props.has("random_seed")) random_seed = static_cast<int32_t>(p_props["random_seed"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline float hash_signed(uint32_t x) noexcept {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return (static_cast<float>(x & 0x00FFFFFFu) / static_cast<float>(0x00800000u)) - 1.0f;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || burst_spawn_count <= 0) return;

        GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        if (!pos_part.raw_base_ptr) return;

        GrantPartPOD* vel_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;
        GrantPartPOD* life_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        MemoryBufferSelectionPOD& sel = pos_part.selection;
        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const int64_t max_particles = std::min<int64_t>(
            (sel.capacity > 0) ? sel.capacity : static_cast<int64_t>(pos_part.capacity_bytes / p_stride),
            static_cast<int64_t>(pos_part.capacity_bytes / p_stride)
        );
        if (max_particles <= 0) return;

        const size_t num_words = static_cast<size_t>((max_particles + 63) / 64);
        int32_t spawned = 0;

        for (size_t w = 0; w < num_words && spawned < burst_spawn_count; ++w) {
            uint64_t avail_mask = sel.unclaimed_mask
                ? sel.unclaimed_mask[w]
                : ((sel.mode == SelectionMode::DENSE && sel.data.bitset) ? ~sel.data.bitset[w] : ~0ULL);

            while (avail_mask != 0ULL && spawned < burst_spawn_count) {
                const int32_t bit = std::countr_zero(avail_mask);
                const int64_t slot = static_cast<int64_t>(w * 64 + bit);
                if (slot >= max_particles) break;

                const uint64_t bit_flag = (1ULL << bit);
                avail_mask &= ~bit_flag;

                if (sel.unclaimed_mask) {
                    sel.unclaimed_mask[w] &= ~bit_flag;
                }
                if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                    if (!(sel.data.bitset[w] & bit_flag)) {
                        sel.data.bitset[w] |= bit_flag;
                        sel.element_count++;
                    }
                } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices && sel.element_count < sel.capacity) {
                    sel.data.indices[sel.element_count++] = slot;
                } else {
                    sel.element_count = std::max<int64_t>(sel.element_count, slot + 1);
                }

                const uint32_t s_id = static_cast<uint32_t>(random_seed) + (spawn_counter++ * 747796405u);
                const float rx = hash_signed(s_id + 1u);
                const float ry = hash_signed(s_id + 2u);
                const float rz = hash_signed(s_id + 3u);

                godot::Vector3 spawn_pos = emitter_origin;
                if (emitter_shape == 1) {
                    spawn_pos += godot::Vector3(rx * emitter_extents.x, ry * emitter_extents.y, rz * emitter_extents.z);
                } else {
                    godot::Vector3 dir(rx, ry, rz);
                    const float len = dir.length();
                    if (len > 1e-5f) dir /= len;
                    const float r_scale = std::abs(hash_signed(s_id + 4u));
                    spawn_pos += dir * (emitter_extents.x * r_scale);
                }
                *reinterpret_cast<godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(slot) * p_stride) = spawn_pos;

                if (vel_part) {
                    const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vel_part->element_stride);
                    if (static_cast<size_t>(slot + 1) * v_stride <= vel_part->capacity_bytes) {
                        const godot::Vector3 jitter(
                            hash_signed(s_id + 5u) * velocity_spread,
                            hash_signed(s_id + 6u) * velocity_spread,
                            hash_signed(s_id + 7u) * velocity_spread
                        );
                        *reinterpret_cast<godot::Vector3*>(vel_part->raw_base_ptr + static_cast<size_t>(slot) * v_stride) =
                            initial_velocity + jitter;
                    }
                }

                if (life_part) {
                    const float life = std::max(1e-3f, base_lifetime * (1.0f + hash_signed(s_id + 8u) * lifetime_randomness));
                    if (life_part->element_stride >= sizeof(godot::Vector2)) {
                        if (static_cast<size_t>(slot + 1) * life_part->element_stride <= life_part->capacity_bytes) {
                            // Vector2(age = 0.0f, lifetime)
                            *reinterpret_cast<godot::Vector2*>(life_part->raw_base_ptr + static_cast<size_t>(slot) * life_part->element_stride) =
                                godot::Vector2(0.0f, life);
                        }
                    } else if (static_cast<size_t>(slot + 1) * sizeof(float) <= life_part->capacity_bytes) {
                        reinterpret_cast<float*>(life_part->raw_base_ptr)[slot] = 0.0f;
                    }
                }

                ++spawned;
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core
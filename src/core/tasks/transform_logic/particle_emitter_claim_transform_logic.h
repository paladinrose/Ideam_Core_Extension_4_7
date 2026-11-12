#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class ParticleEmitterShape : uint8_t {
    SPHERE_VOLUME    = 0,
    BOX_EXTENTS      = 1,
    DIRECTIONAL_CONE = 2
};

/**
 * ParticleEmitterClaimTransformLogic<T> (V1)
 * Claims inactive particle slots from r_selection.unclaimed_mask via hardware countr_zero bit-scanning,
 * initializes Position (position_buffer_id), Velocity (velocity_buffer_id), and Age/Lifetime
 * (lifetime_buffer_id), and wakes their active bits in r_selection.
 */
template <typename T = godot::Vector3>
struct alignas(64) ParticleEmitterClaimTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR2 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Particle Emitter Anti-Grant Slot Claim & Spawn";
    static constexpr std::string_view ui_category  = "Particles/Lifecycle";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t position_buffer_id = INVALID_ID; // 4B: Primary target particle position buffer (Vector3)
    uint32_t velocity_buffer_id = INVALID_ID; // 4B: Optional particle velocity buffer (Vector3)
    uint32_t lifetime_buffer_id = INVALID_ID; // 4B: Optional Vector2(age, lifetime) or float(age) buffer -> 12B

    godot::Vector3 emitter_origin{0.0f, 0.0f, 0.0f};   // 12B -> 24B
    godot::Vector3 emitter_extents{0.5f, 0.5f, 0.5f};  // 12B -> 36B
    godot::Vector3 initial_velocity{0.0f, 5.0f, 0.0f}; // 12B -> 48B

    float velocity_spread               = 1.0f;  // 4B -> 52B
    float base_lifetime                 = 2.0f;  // 4B -> 56B
    float lifetime_randomness           = 0.25f; // 4B -> 60B
    uint16_t burst_spawn_count          = 16;    // 2B -> 62B
    ParticleEmitterShape emitter_shape  = ParticleEmitterShape::SPHERE_VOLUME; // 1B
    mutable uint8_t spawn_counter       = 0;     // 1B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pbuf_prop;
        pbuf_prop["name"] = "position_buffer_id";
        pbuf_prop["type"] = godot::Variant::INT;
        pbuf_prop["hint_string"] = "buffer_option";
        props.push_back(pbuf_prop);

        godot::Dictionary vbuf_prop;
        vbuf_prop["name"] = "velocity_buffer_id";
        vbuf_prop["type"] = godot::Variant::INT;
        vbuf_prop["hint_string"] = "buffer_option";
        props.push_back(vbuf_prop);

        godot::Dictionary lbuf_prop;
        lbuf_prop["name"] = "lifetime_buffer_id";
        lbuf_prop["type"] = godot::Variant::INT;
        lbuf_prop["hint_string"] = "buffer_option";
        props.push_back(lbuf_prop);

        godot::Dictionary shp_prop;
        shp_prop["name"] = "emitter_shape";
        shp_prop["type"] = godot::Variant::INT;
        shp_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        shp_prop["hint_string"] = "Sphere Volume:0,Box Extents:1,Directional Cone:2";
        props.push_back(shp_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "emitter_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary ext_prop;
        ext_prop["name"] = "emitter_extents";
        ext_prop["type"] = godot::Variant::VECTOR3;
        ext_prop["hint_string"] = "suffix:m";
        props.push_back(ext_prop);

        godot::Dictionary vel_prop;
        vel_prop["name"] = "initial_velocity";
        vel_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(vel_prop);

        godot::Dictionary spr_prop;
        spr_prop["name"] = "velocity_spread";
        spr_prop["type"] = godot::Variant::FLOAT;
        props.push_back(spr_prop);

        godot::Dictionary life_prop;
        life_prop["name"] = "base_lifetime";
        life_prop["type"] = godot::Variant::FLOAT;
        life_prop["hint_string"] = "suffix:s";
        props.push_back(life_prop);

        godot::Dictionary lrand_prop;
        lrand_prop["name"] = "lifetime_randomness";
        lrand_prop["type"] = godot::Variant::FLOAT;
        lrand_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        lrand_prop["hint_string"] = "0.0,0.99,0.01";
        props.push_back(lrand_prop);

        godot::Dictionary cnt_prop;
        cnt_prop["name"] = "burst_spawn_count";
        cnt_prop["type"] = godot::Variant::INT;
        cnt_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        cnt_prop["hint_string"] = "0,65535,1";
        props.push_back(cnt_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("velocity_buffer_id")) {
            velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        }
        if (p_props.has("lifetime_buffer_id")) {
            lifetime_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["lifetime_buffer_id"]));
        }
        if (p_props.has("emitter_shape")) {
            emitter_shape = static_cast<ParticleEmitterShape>(std::clamp<int64_t>(static_cast<int64_t>(p_props["emitter_shape"]), 0, 2));
        }
        if (p_props.has("emitter_origin")) {
            emitter_origin = static_cast<godot::Vector3>(p_props["emitter_origin"]);
        }
        if (p_props.has("emitter_extents")) {
            emitter_extents = static_cast<godot::Vector3>(p_props["emitter_extents"]);
        }
        if (p_props.has("initial_velocity")) {
            initial_velocity = static_cast<godot::Vector3>(p_props["initial_velocity"]);
        }
        if (p_props.has("velocity_spread")) {
            velocity_spread = std::max(0.0f, static_cast<float>(p_props["velocity_spread"]));
        }
        if (p_props.has("base_lifetime")) {
            base_lifetime = std::max(1e-3f, static_cast<float>(p_props["base_lifetime"]));
        }
        if (p_props.has("lifetime_randomness")) {
            lifetime_randomness = std::clamp(static_cast<float>(p_props["lifetime_randomness"]), 0.0f, 0.99f);
        }
        if (p_props.has("burst_spawn_count")) {
            burst_spawn_count = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["burst_spawn_count"]), 0, 65535));
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, godot::Vector2> || std::is_same_v<T, float>) {
            if (burst_spawn_count == 0) return;

            GrantPartPOD* pos_part = const_cast<GrantPartPOD*>(p_context.get_grant_part(position_buffer_id));
            if (!pos_part || !pos_part->raw_base_ptr) return;

            const GrantPartPOD* vel_part  = (velocity_buffer_id != INVALID_ID) ? p_context.get_grant_part(velocity_buffer_id) : nullptr;
            const GrantPartPOD* life_part = (lifetime_buffer_id != INVALID_ID) ? p_context.get_grant_part(lifetime_buffer_id) : nullptr;

            MemoryBufferSelectionPOD& sel = pos_part->selection;
            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const int64_t max_particles = std::min<int64_t>(
                (sel.capacity > 0) ? sel.capacity : static_cast<int64_t>(pos_part->capacity_bytes / p_stride),
                static_cast<int64_t>(pos_part->capacity_bytes / p_stride)
            );
            if (max_particles <= 0) return;

            const size_t num_words = static_cast<size_t>((max_particles + 63) / 64);
            uint32_t spawned = 0;

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

                    const uint32_t s_id = static_cast<uint32_t>(slot * 1664525u) + (static_cast<uint32_t>(spawn_counter++) * 747796405u);
                    const float rx = _hash_signed(s_id + 1u);
                    const float ry = _hash_signed(s_id + 2u);
                    const float rz = _hash_signed(s_id + 3u);

                    godot::Vector3 spawn_pos = emitter_origin;
                    if (emitter_shape == ParticleEmitterShape::BOX_EXTENTS) {
                        spawn_pos += godot::Vector3(rx * emitter_extents.x, ry * emitter_extents.y, rz * emitter_extents.z);
                    } else {
                        godot::Vector3 dir(rx, ry, rz);
                        const float len = dir.length();
                        if (len > 1e-5f) dir /= len;
                        const float r_scale = std::abs(_hash_signed(s_id + 4u));
                        spawn_pos += dir * (emitter_extents.x * r_scale);
                    }
                    *reinterpret_cast<godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(slot) * p_stride) = spawn_pos;

                    if (vel_part && vel_part->raw_base_ptr) {
                        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vel_part->element_stride);
                        if (static_cast<size_t>(slot + 1) * v_stride <= vel_part->capacity_bytes) {
                            const godot::Vector3 jitter(
                                _hash_signed(s_id + 5u) * velocity_spread,
                                _hash_signed(s_id + 6u) * velocity_spread,
                                _hash_signed(s_id + 7u) * velocity_spread
                            );
                            *reinterpret_cast<godot::Vector3*>(vel_part->raw_base_ptr + static_cast<size_t>(slot) * v_stride) =
                                initial_velocity + jitter;
                        }
                    }

                    if (life_part && life_part->raw_base_ptr) {
                        const float life = std::max(1e-3f, base_lifetime * (1.0f + _hash_signed(s_id + 8u) * lifetime_randomness));
                        if (life_part->element_stride >= sizeof(godot::Vector2)) {
                            if (static_cast<size_t>(slot + 1) * life_part->element_stride <= life_part->capacity_bytes) {
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
    }

private:
    static inline float _hash_signed(uint32_t p_x) noexcept {
        p_x ^= p_x >> 16;
        p_x *= 0x7feb352du;
        p_x ^= p_x >> 15;
        p_x *= 0x846ca68bu;
        p_x ^= p_x >> 16;
        return (static_cast<float>(p_x & 0x00FFFFFFu) / static_cast<float>(0x00800000u)) - 1.0f;
    }
};

static_assert(sizeof(ParticleEmitterClaimTransformLogic<>) == 64, "ParticleEmitterClaimTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ParticleEmitterClaimTransformLogic<>>, "ParticleEmitterClaimTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
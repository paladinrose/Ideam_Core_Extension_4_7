#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ParticleLifetimeCullQueryLogic<T, T_Strategy, T_View> (V9)
 * Deactivates expired particles (Age >= Lifetime) in r_selection and releases their bit slots
 * back into r_selection.unclaimed_mask so V1 ParticleEmitterClaimTransformLogic can recycle them.
 */
template <
    typename T = godot::Vector2,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ParticleLifetimeCullQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR2 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Particle Lifetime Expiration Cull & Anti-Grant Release";
    static constexpr std::string_view ui_category  = "Particles/Query";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id       = INVALID_ID; // 4B: Packed Vector2(age, lifetime) or float age buffer
    uint32_t max_lifetime_buffer_id = INVALID_ID; // 4B: Optional float max lifetime buffer
    float fallback_lifetime         = 2.0f;       // 4B
    bool release_to_unclaimed_mask  = true;       // 1B
    uint8_t reserved_padding[51]    = {0};        // 51B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("max_lifetime_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("fallback_lifetime", godot::Variant::FLOAT, 2.0f);
        add_prop("release_to_unclaimed_mask", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("max_lifetime_buffer_id")) max_lifetime_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_lifetime_buffer_id"]));
        if (p_props.has("fallback_lifetime")) fallback_lifetime = std::max(1e-4f, static_cast<float>(p_props["fallback_lifetime"]));
        if (p_props.has("release_to_unclaimed_mask")) release_to_unclaimed_mask = static_cast<bool>(p_props["release_to_unclaimed_mask"]);
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        const T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* life_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!life_part || !life_part->raw_base_ptr) return;

            const GrantPartPOD* max_life_part = (max_lifetime_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(max_lifetime_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);

            const bool is_packed_v2 = (life_part->element_stride >= sizeof(godot::Vector2));
            const uint32_t l_stride = std::max<uint32_t>(is_packed_v2 ? sizeof(godot::Vector2) : sizeof(float), life_part->element_stride);
            const int64_t n_particles = std::min<int64_t>(
                (r_selection.capacity > 0) ? r_selection.capacity : static_cast<int64_t>(life_part->capacity_bytes / l_stride),
                static_cast<int64_t>(life_part->capacity_bytes / l_stride)
            );

            const float* max_lives = (max_life_part && max_life_part->raw_base_ptr)
                ? reinterpret_cast<const float*>(max_life_part->raw_base_ptr)
                : nullptr;
            const int64_t max_l_count = max_lives ? static_cast<int64_t>(max_life_part->capacity_bytes / sizeof(float)) : 0;

            for (int64_t i = 0; i < n_particles; ++i) {
                float age = 0.0f;
                float lifetime = fallback_lifetime;

                if (is_packed_v2) {
                    const godot::Vector2& al = *reinterpret_cast<const godot::Vector2*>(life_part->raw_base_ptr + static_cast<size_t>(i) * l_stride);
                    age = al.x;
                    lifetime = al.y;
                } else {
                    age = *reinterpret_cast<const float*>(life_part->raw_base_ptr + static_cast<size_t>(i) * l_stride);
                    if (max_lives && i < max_l_count) {
                        lifetime = max_lives[i];
                    }
                }

                const bool expired = (age >= lifetime);
                const uint64_t word = static_cast<uint64_t>(i) >> 6;
                const uint64_t bit = 1ULL << (i & 63);

                if constexpr (Op == QueryOp::CULL) {
                    if (expired) {
                        if (release_to_unclaimed_mask && r_selection.unclaimed_mask) {
                            r_selection.unclaimed_mask[word] |= bit;
                        }
                        if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                            if (r_selection.data.bitset[word] & bit) {
                                r_selection.data.bitset[word] &= ~bit;
                                r_selection.element_count = std::max<int64_t>(0, r_selection.element_count - 1);
                            }
                        }
                    }
                } else if constexpr (Op == QueryOp::ADD) {
                    if (!expired) {
                        p_ctx.queue_selection_command(target_buffer_id, i);
                        if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                            if (!(r_selection.data.bitset[word] & bit)) {
                                r_selection.data.bitset[word] |= bit;
                                r_selection.element_count++;
                            }
                        }
                    }
                }
            }
        }
    }
};

static_assert(sizeof(ParticleLifetimeCullQueryLogic<>) == 64, "ParticleLifetimeCullQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<ParticleLifetimeCullQueryLogic<>>, "ParticleLifetimeCullQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
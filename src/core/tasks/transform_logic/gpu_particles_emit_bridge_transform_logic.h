#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * GPUParticlesEmitBridgeTransformLogic<T, T_Strategy, T_View> (V8)
 * Triggers GPU particle sub-emissions directly on a RenderingServer particles RID
 * via RenderingServer::particles_emit(particles_rid, xform, velocity, color, custom, emit_flags)
 * from active/selected DOD particle or impact buffers.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) GPUParticlesEmitBridgeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::COLOR | DataType::TRANSFORM3D;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RenderingServer GPUParticles Emit Bridge";
    static constexpr std::string_view ui_category  = "Particles/Rendering";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::RID particles_rid;                                                   // 8B
    uint32_t target_buffer_id        = INVALID_ID;                              // 4B: Position/Transform3D buffer
    uint32_t velocity_buffer_id      = INVALID_ID;                              // 4B -> 16B
    uint32_t color_buffer_id         = INVALID_ID;                              // 4B
    uint32_t custom_buffer_id        = INVALID_ID;                              // 4B
    uint32_t emit_flags              = 1u | 4u;                                 // 4B: POSITION (1) | VELOCITY (4)
    int32_t max_emissions_per_frame  = 256;                                     // 4B -> 32B
    godot::Color default_color       = godot::Color(1.0f, 1.0f, 1.0f, 1.0f);    // 16B -> 48B
    godot::Color default_custom      = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);    // 16B -> Total = 64B

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
        add_buf("velocity_buffer_id");
        add_buf("color_buffer_id");
        add_buf("custom_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("particles_rid", godot::Variant::RID, godot::RID());
        add_prop("emit_flags", godot::Variant::INT, static_cast<int64_t>(1 | 4));
        add_prop("default_color", godot::Variant::COLOR, godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
        add_prop("default_custom", godot::Variant::COLOR, godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
        add_prop("max_emissions_per_frame", godot::Variant::INT, 256);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("position_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        }
        if (p_props.has("velocity_buffer_id")) velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        if (p_props.has("color_buffer_id")) color_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["color_buffer_id"]));
        if (p_props.has("custom_buffer_id")) custom_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["custom_buffer_id"]));
        if (p_props.has("particles_rid")) {
            const godot::Variant v = p_props["particles_rid"];
            if (v.get_type() == godot::Variant::RID) {
                particles_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&particles_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("emit_flags")) emit_flags = static_cast<uint32_t>(static_cast<int64_t>(p_props["emit_flags"]));
        if (p_props.has("default_color")) default_color = p_props["default_color"];
        if (p_props.has("default_custom")) default_custom = p_props["default_custom"];
        if (p_props.has("max_emissions_per_frame")) max_emissions_per_frame = std::max<int32_t>(1, static_cast<int32_t>(p_props["max_emissions_per_frame"]));
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!particles_rid.is_valid()) return;

            const GrantPartPOD* pos_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!pos_part || !pos_part->raw_base_ptr) return;

            godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
            if (!rs) return;

            const GrantPartPOD* vel_part = (velocity_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(velocity_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);
            const GrantPartPOD* col_part = (color_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(color_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);
            const GrantPartPOD* cust_part = (custom_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(custom_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr) ? &p_ctx.grant->parts[3] : nullptr);

            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const int64_t max_pos = static_cast<int64_t>(pos_part->capacity_bytes / p_stride);
            const MemoryBufferSelectionPOD& sel = pos_part->selection;

            int32_t emitted = 0;

            auto emit_at = [&](int64_t i) {
                if (i < 0 || i >= max_pos || emitted >= max_emissions_per_frame) return;
                godot::Transform3D xform;
                if (p_stride >= sizeof(godot::Transform3D)) {
                    xform = *reinterpret_cast<const godot::Transform3D*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride);
                } else {
                    xform.origin = *reinterpret_cast<const godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride);
                }

                godot::Vector3 vel(0.0f, 0.0f, 0.0f);
                if (vel_part && vel_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= vel_part->capacity_bytes) {
                    vel = reinterpret_cast<const godot::Vector3*>(vel_part->raw_base_ptr)[i];
                }

                godot::Color col = default_color;
                if (col_part && col_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(godot::Color) <= col_part->capacity_bytes) {
                    col = reinterpret_cast<const godot::Color*>(col_part->raw_base_ptr)[i];
                }

                godot::Color cust = default_custom;
                if (cust_part && cust_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(godot::Color) <= cust_part->capacity_bytes) {
                    cust = reinterpret_cast<const godot::Color*>(cust_part->raw_base_ptr)[i];
                }

                rs->particles_emit(particles_rid, xform, vel, col, cust, emit_flags);
                ++emitted;
            };

            if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t k = 0; k < sel.element_count && emitted < max_emissions_per_frame; ++k) {
                    emit_at(sel.data.indices[k]);
                }
            } else if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const int64_t limit = std::min<int64_t>(max_pos, sel.capacity);
                for (int64_t i = 0; i < limit && emitted < max_emissions_per_frame; ++i) {
                    if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) {
                        emit_at(i);
                    }
                }
            } else {
                const int64_t count = (sel.element_count > 0) ? std::min<int64_t>(sel.element_count, max_pos) : max_pos;
                for (int64_t i = 0; i < count && emitted < max_emissions_per_frame; ++i) {
                    emit_at(i);
                }
            }
        }
    }
};

static_assert(sizeof(GPUParticlesEmitBridgeTransformLogic<>) == 64, "GPUParticlesEmitBridgeTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<GPUParticlesEmitBridgeTransformLogic<>>, "GPUParticlesEmitBridgeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
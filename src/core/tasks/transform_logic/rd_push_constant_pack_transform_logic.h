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
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * RDPushConstantPackTransformLogic<T> (G_RD3)
 * Packs per-frame execution constants (delta, element_count, time, user_flags, origin, and custom_vec4)
 * into a 16-byte aligned (32B or 64B) GLSL push-constant struct inside push_constant_buffer_id.
 */
template <typename T = float>
struct alignas(64) RDPushConstantPackTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    struct alignas(16) PushConstantBlock64Pod {
        float delta;
        float time_seconds;
        uint32_t element_count;
        uint32_t user_flags;
        float origin_x;
        float origin_y;
        float origin_z;
        float scalar_param;
        float custom_vec4[4];
        float extra_vec4[4];
    };

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::VECTOR4 | DataType::INT32 | DataType::BYTE;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD Compute Push-Constant Block Packer";
    static constexpr std::string_view ui_category  = "GPU/Packing";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t push_constant_buffer_id = INVALID_ID; // 4B: Destination 16..64B push constant buffer
    uint32_t count_source_buffer_id  = INVALID_ID; // 4B: Optional buffer whose selection.element_count populates element_count
    float time_seconds               = 0.0f;       // 4B: Base elapsed time in seconds
    int32_t user_flags               = 0;          // 4B: Bitfield user flags -> 16B

    godot::Vector3 origin{0.0f, 0.0f, 0.0f};       // 12B: World/local origin
    float scalar_param               = 1.0f;       // 4B: Scalar uniform -> 32B
    godot::Vector4 custom_vec4{0.0f, 0.0f, 0.0f, 0.0f}; // 16B -> 48B
    godot::Vector4 extra_vec4{0.0f, 0.0f, 0.0f, 0.0f};  // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pcb_prop;
        pcb_prop["name"] = "push_constant_buffer_id";
        pcb_prop["type"] = godot::Variant::INT;
        pcb_prop["hint_string"] = "buffer_option";
        props.push_back(pcb_prop);

        godot::Dictionary csb_prop;
        csb_prop["name"] = "count_source_buffer_id";
        csb_prop["type"] = godot::Variant::INT;
        csb_prop["hint_string"] = "buffer_option";
        props.push_back(csb_prop);

        godot::Dictionary tm_prop;
        tm_prop["name"] = "time_seconds";
        tm_prop["type"] = godot::Variant::FLOAT;
        props.push_back(tm_prop);

        godot::Dictionary flg_prop;
        flg_prop["name"] = "user_flags";
        flg_prop["type"] = godot::Variant::INT;
        props.push_back(flg_prop);

        godot::Dictionary org_prop;
        org_prop["name"] = "origin";
        org_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(org_prop);

        godot::Dictionary scl_prop;
        scl_prop["name"] = "scalar_param";
        scl_prop["type"] = godot::Variant::FLOAT;
        props.push_back(scl_prop);

        godot::Dictionary cv4_prop;
        cv4_prop["name"] = "custom_vec4";
        cv4_prop["type"] = godot::Variant::VECTOR4;
        props.push_back(cv4_prop);

        godot::Dictionary ev4_prop;
        ev4_prop["name"] = "extra_vec4";
        ev4_prop["type"] = godot::Variant::VECTOR4;
        props.push_back(ev4_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (push_constant_buffer_id != INVALID_ID) ? push_constant_buffer_id : count_source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("push_constant_buffer_id")) {
            push_constant_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["push_constant_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && push_constant_buffer_id == INVALID_ID) {
            push_constant_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && push_constant_buffer_id == INVALID_ID) {
            push_constant_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("count_source_buffer_id")) {
            count_source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["count_source_buffer_id"]));
        }
        if (p_props.has("time_seconds")) {
            time_seconds = static_cast<float>(p_props["time_seconds"]);
        }
        if (p_props.has("user_flags")) {
            user_flags = static_cast<int32_t>(p_props["user_flags"]);
        }
        if (p_props.has("origin")) {
            origin = static_cast<godot::Vector3>(p_props["origin"]);
        }
        if (p_props.has("scalar_param")) {
            scalar_param = static_cast<float>(p_props["scalar_param"]);
        }
        if (p_props.has("custom_vec4")) {
            custom_vec4 = static_cast<godot::Vector4>(p_props["custom_vec4"]);
        }
        if (p_props.has("extra_vec4")) {
            extra_vec4 = static_cast<godot::Vector4>(p_props["extra_vec4"]);
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
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* pc_part = p_context.get_grant_part(push_constant_buffer_id);
            if (!pc_part || !pc_part->raw_base_ptr || pc_part->capacity_bytes < 16) return;

            uint32_t elem_count = static_cast<uint32_t>(std::max<int64_t>(0, pc_part->selection.element_count));
            if (count_source_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* cs_part = p_context.get_grant_part(count_source_buffer_id)) {
                    elem_count = static_cast<uint32_t>(std::max<int64_t>(0, cs_part->selection.element_count));
                }
            }

            PushConstantBlock64Pod block{};
            block.delta         = static_cast<float>(p_context.delta);
            block.time_seconds  = time_seconds + static_cast<float>(p_context.delta);
            block.element_count = elem_count;
            block.user_flags    = static_cast<uint32_t>(user_flags);
            block.origin_x      = origin.x;
            block.origin_y      = origin.y;
            block.origin_z      = origin.z;
            block.scalar_param  = scalar_param;
            block.custom_vec4[0] = custom_vec4.x;
            block.custom_vec4[1] = custom_vec4.y;
            block.custom_vec4[2] = custom_vec4.z;
            block.custom_vec4[3] = custom_vec4.w;
            block.extra_vec4[0]  = extra_vec4.x;
            block.extra_vec4[1]  = extra_vec4.y;
            block.extra_vec4[2]  = extra_vec4.z;
            block.extra_vec4[3]  = extra_vec4.w;

            const size_t copy_bytes = std::min<size_t>(sizeof(PushConstantBlock64Pod), (pc_part->capacity_bytes / 16) * 16);
            std::memcpy(pc_part->raw_base_ptr, &block, copy_bytes);
        }
    }
};

static_assert(sizeof(RDPushConstantPackTransformLogic<>) == 64, "RDPushConstantPackTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDPushConstantPackTransformLogic<>>, "RDPushConstantPackTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
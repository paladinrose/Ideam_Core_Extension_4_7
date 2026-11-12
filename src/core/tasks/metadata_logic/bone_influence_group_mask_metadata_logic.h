#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/vector4i.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * BoneInfluenceGroupMaskMetadataLogic<T, T_Strategy, T_View> (M44)
 * Reads 4-bone skinning indices and weights per vertex, stamping bone influence bitflags
 * (for bones with weight >= weight_threshold) into r_selection.group_masks and the dominant
 * bone ID into r_selection.partition_ids.
 */
template <
    typename T = godot::Vector4,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) BoneInfluenceGroupMaskMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR4 | DataType::INT32 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Bone Influence Group Mask Metadata";
    static constexpr std::string_view ui_category  = "Model/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t bone_index_buffer_id         = INVALID_ID; // 4B: int32x4 bone index buffer
    uint32_t bone_weight_buffer_id        = INVALID_ID; // 4B: float4 bone weight buffer
    float weight_threshold                = 0.20f;      // 4B -> 16B
    int32_t bone_id_shift_offset          = 0;          // 4B
    bool write_dominant_bone_to_partition = true;       // 1B
    uint8_t reserved_padding[43]          = {0};        // 43B -> Total = 64B

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
        add_buf("bone_index_buffer_id");
        add_buf("bone_weight_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("weight_threshold", godot::Variant::FLOAT, 0.20f);
        add_prop("bone_id_shift_offset", godot::Variant::INT, 0);
        add_prop("write_dominant_bone_to_partition", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (bone_index_buffer_id != INVALID_ID) ? bone_index_buffer_id : bone_weight_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("bone_index_buffer_id")) bone_index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bone_index_buffer_id"]));
        if (p_props.has("bone_weight_buffer_id")) bone_weight_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bone_weight_buffer_id"]));
        if (p_props.has("weight_threshold")) weight_threshold = std::clamp(static_cast<float>(p_props["weight_threshold"]), 0.0f, 1.0f);
        if (p_props.has("bone_id_shift_offset")) bone_id_shift_offset = std::max<int32_t>(0, static_cast<int32_t>(p_props["bone_id_shift_offset"]));
        if (p_props.has("write_dominant_bone_to_partition")) write_dominant_bone_to_partition = static_cast<bool>(p_props["write_dominant_bone_to_partition"]);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.group_masks || r_selection.capacity <= 0) return;

            const GrantPartPOD* bone_idx_part = (bone_index_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(bone_index_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            const GrantPartPOD* bone_wgt_part = (bone_weight_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(bone_weight_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 2) ? &p_ctx.grant->parts[2] : nullptr);
            if (!bone_idx_part || !bone_idx_part->raw_base_ptr || !bone_wgt_part || !bone_wgt_part->raw_base_ptr) return;

            const int64_t n_verts = std::min<int64_t>({
                (r_selection.element_count > 0) ? r_selection.element_count : r_selection.capacity,
                static_cast<int64_t>(bone_idx_part->capacity_bytes / (sizeof(int32_t) * 4)),
                static_cast<int64_t>(bone_wgt_part->capacity_bytes / (sizeof(float) * 4))
            });

            const int32_t* bone_indices = reinterpret_cast<const int32_t*>(bone_idx_part->raw_base_ptr);
            const float* bone_weights = reinterpret_cast<const float*>(bone_wgt_part->raw_base_ptr);

            for (int64_t i = 0; i < n_verts; ++i) {
                const int32_t* bi = bone_indices + i * 4;
                const float* bw = bone_weights + i * 4;

                uint32_t mask = 0;
                int32_t dominant_bone = bi[0];
                float max_w = -1.0f;

                for (int k = 0; k < 4; ++k) {
                    const float w = bw[k];
                    const int32_t b = bi[k];
                    if (w > max_w) {
                        max_w = w;
                        dominant_bone = b;
                    }
                    if (w >= weight_threshold) {
                        const int32_t bit_idx = (b - bone_id_shift_offset) & 31;
                        if (bit_idx >= 0 && bit_idx < 32) {
                            mask |= (1u << bit_idx);
                        }
                    }
                }

                r_selection.group_masks[i] = mask;
                if (write_dominant_bone_to_partition && r_selection.partition_ids) {
                    r_selection.partition_ids[i] = dominant_bone;
                }
            }
        }
    }
};

static_assert(sizeof(BoneInfluenceGroupMaskMetadataLogic<>) == 64, "BoneInfluenceGroupMaskMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<BoneInfluenceGroupMaskMetadataLogic<>>, "BoneInfluenceGroupMaskMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
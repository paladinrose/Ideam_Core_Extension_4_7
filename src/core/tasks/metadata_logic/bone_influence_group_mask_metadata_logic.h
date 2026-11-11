#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/vector4i.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * M44: BoneInfluenceGroupMaskMetadataLogic
 * Reads 4-bone skinning indices and weights per vertex, stamping bone influence bitflags
 * (for bones with weight >= weight_threshold) into r_selection.group_masks and the dominant
 * bone ID into r_selection.partition_ids.
 */
struct BoneInfluenceGroupMaskMetadataLogic {
    static constexpr std::string_view ui_name = "Bone Influence Group Mask Metadata";
    static constexpr std::string_view ui_category = "Model/Metadata";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR4,
        DataType::INT32,
        DataType::FLOAT32
    };

    float weight_threshold = 0.20f;
    int32_t bone_id_shift_offset = 0;
    bool write_dominant_bone_to_partition = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("weight_threshold", godot::Variant::FLOAT, weight_threshold);
        add_prop("bone_id_shift_offset", godot::Variant::INT, bone_id_shift_offset);
        add_prop("write_dominant_bone_to_partition", godot::Variant::BOOL, write_dominant_bone_to_partition);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("weight_threshold")) weight_threshold = std::clamp(static_cast<float>(p_props["weight_threshold"]), 0.0f, 1.0f);
        if (p_props.has("bone_id_shift_offset")) bone_id_shift_offset = std::max<int32_t>(0, static_cast<int32_t>(p_props["bone_id_shift_offset"]));
        if (p_props.has("write_dominant_bone_to_partition")) write_dominant_bone_to_partition = static_cast<bool>(p_props["write_dominant_bone_to_partition"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 3) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& bone_idx_part = p_ctx.grant->parts[1];
        const GrantPartPOD& bone_wgt_part = p_ctx.grant->parts[2];
        MemoryBufferSelectionPOD& sel = vtx_part.selection;
        if (!sel.group_masks || sel.capacity <= 0 || !bone_idx_part.raw_base_ptr || !bone_wgt_part.raw_base_ptr) return;

        const int64_t n_verts = std::min<int64_t>({
            (sel.element_count > 0) ? sel.element_count : sel.capacity,
            static_cast<int64_t>(bone_idx_part.capacity_bytes / (sizeof(int32_t) * 4)),
            static_cast<int64_t>(bone_wgt_part.capacity_bytes / (sizeof(float) * 4))
        });

        const int32_t* bone_indices = reinterpret_cast<const int32_t*>(bone_idx_part.raw_base_ptr);
        const float* bone_weights = reinterpret_cast<const float*>(bone_wgt_part.raw_base_ptr);

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

            sel.group_masks[i] = mask;
            if (write_dominant_bone_to_partition && sel.partition_ids) {
                sel.partition_ids[i] = dominant_bone;
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core
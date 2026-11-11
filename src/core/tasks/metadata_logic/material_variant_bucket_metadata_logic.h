#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * S8: MaterialVariantBucketMetadataLogic
 * Hashes material state ID, shader permutation flags, and LOD level into a 64-bit sort key in
 * r_selection.partition_ids for contiguous MultiMesh and draw-call batching.
 */
struct MaterialVariantBucketMetadataLogic {
    static constexpr std::string_view ui_name = "Material Variant & Shader LOD Bucket Metadata";
    static constexpr std::string_view ui_category = "Shader/Metadata";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::INT32,
        DataType::UINT32
    };

    int32_t num_buckets = 64;
    bool fold_lod_into_bucket = true;
    bool fold_group_mask_into_bucket = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("num_buckets", godot::Variant::INT, num_buckets);
        add_prop("fold_lod_into_bucket", godot::Variant::BOOL, fold_lod_into_bucket);
        add_prop("fold_group_mask_into_bucket", godot::Variant::BOOL, fold_group_mask_into_bucket);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("num_buckets")) num_buckets = std::max<int32_t>(0, static_cast<int32_t>(p_props["num_buckets"]));
        if (p_props.has("fold_lod_into_bucket")) fold_lod_into_bucket = static_cast<bool>(p_props["fold_lod_into_bucket"]);
        if (p_props.has("fold_group_mask_into_bucket")) fold_group_mask_into_bucket = static_cast<bool>(p_props["fold_group_mask_into_bucket"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline uint64_t splitmix64(uint64_t x) noexcept {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& mat_part = p_ctx.grant->parts[0];
        MemoryBufferSelectionPOD& sel = mat_part.selection;
        if (!sel.partition_ids || sel.capacity <= 0 || !mat_part.raw_base_ptr) return;

        const uint32_t stride = std::max<uint32_t>(sizeof(int32_t), mat_part.element_stride);
        const int64_t n_elems = std::min<int64_t>(
            (sel.element_count > 0) ? sel.element_count : sel.capacity,
            static_cast<int64_t>(mat_part.capacity_bytes / stride)
        );

        for (int64_t i = 0; i < n_elems; ++i) {
            const uint32_t mat_id = *reinterpret_cast<const uint32_t*>(mat_part.raw_base_ptr + static_cast<size_t>(i) * stride);
            const uint32_t g_mask = (fold_group_mask_into_bucket && sel.group_masks) ? sel.group_masks[i] : 0u;
            const uint8_t lod = (fold_lod_into_bucket && sel.lod_levels) ? sel.lod_levels[i] : 0u;

            uint64_t key = (static_cast<uint64_t>(mat_id) << 32) |
                           (static_cast<uint64_t>(g_mask & 0x00FFFFFFu) << 8) |
                           static_cast<uint64_t>(lod);

            if (num_buckets > 0) {
                key = splitmix64(key) % static_cast<uint64_t>(num_buckets);
            }
            sel.partition_ids[i] = static_cast<int64_t>(key);
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
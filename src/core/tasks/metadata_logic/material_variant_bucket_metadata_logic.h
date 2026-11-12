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

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * MaterialVariantBucketMetadataLogic<T, T_Strategy, T_View> (S8)
 * Hashes material state ID, shader permutation flags, and LOD level into a 64-bit sort key in
 * r_selection.partition_ids for contiguous MultiMesh and draw-call batching.
 */
template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) MaterialVariantBucketMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::UINT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Material Variant & Shader LOD Bucket Metadata";
    static constexpr std::string_view ui_category  = "Shader/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id        = INVALID_ID; // 4B: Material ID buffer
    int32_t num_buckets              = 64;         // 4B
    bool fold_lod_into_bucket        = true;       // 1B
    bool fold_group_mask_into_bucket = true;       // 1B
    uint8_t reserved_padding[54]     = {0};        // 54B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("num_buckets", godot::Variant::INT, 64);
        add_prop("fold_lod_into_bucket", godot::Variant::BOOL, true);
        add_prop("fold_group_mask_into_bucket", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("num_buckets")) num_buckets = std::max<int32_t>(0, static_cast<int32_t>(p_props["num_buckets"]));
        if (p_props.has("fold_lod_into_bucket")) fold_lod_into_bucket = static_cast<bool>(p_props["fold_lod_into_bucket"]);
        if (p_props.has("fold_group_mask_into_bucket")) fold_group_mask_into_bucket = static_cast<bool>(p_props["fold_group_mask_into_bucket"]);
    }

    static inline uint64_t splitmix64(uint64_t x) noexcept {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.partition_ids || r_selection.capacity <= 0) return;

            const GrantPartPOD* mat_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!mat_part || !mat_part->raw_base_ptr) return;

            const uint32_t stride = std::max<uint32_t>(sizeof(int32_t), mat_part->element_stride);
            const int64_t n_elems = std::min<int64_t>(
                (r_selection.element_count > 0) ? r_selection.element_count : r_selection.capacity,
                static_cast<int64_t>(mat_part->capacity_bytes / stride)
            );

            for (int64_t i = 0; i < n_elems; ++i) {
                const uint32_t mat_id = *reinterpret_cast<const uint32_t*>(mat_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                const uint32_t g_mask = (fold_group_mask_into_bucket && r_selection.group_masks) ? r_selection.group_masks[i] : 0u;
                const uint8_t lod = (fold_lod_into_bucket && r_selection.lod_levels) ? r_selection.lod_levels[i] : 0u;

                uint64_t key = (static_cast<uint64_t>(mat_id) << 32) |
                               (static_cast<uint64_t>(g_mask & 0x00FFFFFFu) << 8) |
                               static_cast<uint64_t>(lod);

                if (num_buckets > 0) {
                    key = splitmix64(key) % static_cast<uint64_t>(num_buckets);
                }
                r_selection.partition_ids[i] = static_cast<int64_t>(key);
            }
        }
    }
};

static_assert(sizeof(MaterialVariantBucketMetadataLogic<>) == 64, "MaterialVariantBucketMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<MaterialVariantBucketMetadataLogic<>>, "MaterialVariantBucketMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
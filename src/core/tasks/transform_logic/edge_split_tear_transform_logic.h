#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * M28: EdgeSplitTearTransformLogic
 * Evaluates edge stretch across indexed 3D triangle meshes against rest-pose lengths;
 * when relative stretch exceeds tear_threshold, claims a reserve vertex slot from
 * r_selection.unclaimed_mask (or spare tail capacity) and splits the shared vertex to tear the mesh.
 */
struct EdgeSplitTearTransformLogic {
    static constexpr std::string_view ui_name = "Edge Split & Cloth/Metal Tear";
    static constexpr std::string_view ui_category = "Model/Destruction";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::INT32
    };

    float tear_threshold = 0.35f;
    float fallback_rest_edge_length = 0.25f;
    float tear_recoil_offset = 0.01f;
    int32_t max_splits_per_frame = 64;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("tear_threshold", godot::Variant::FLOAT, tear_threshold);
        add_prop("fallback_rest_edge_length", godot::Variant::FLOAT, fallback_rest_edge_length);
        add_prop("tear_recoil_offset", godot::Variant::FLOAT, tear_recoil_offset);
        add_prop("max_splits_per_frame", godot::Variant::INT, max_splits_per_frame);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("tear_threshold")) tear_threshold = std::max(1e-4f, static_cast<float>(p_props["tear_threshold"]));
        if (p_props.has("fallback_rest_edge_length")) fallback_rest_edge_length = std::max(1e-5f, static_cast<float>(p_props["fallback_rest_edge_length"]));
        if (p_props.has("tear_recoil_offset")) tear_recoil_offset = std::max(0.0f, static_cast<float>(p_props["tear_recoil_offset"]));
        if (p_props.has("max_splits_per_frame")) max_splits_per_frame = std::max<int32_t>(1, static_cast<int32_t>(p_props["max_splits_per_frame"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static int32_t claim_unclaimed_slot(MemoryBufferSelectionPOD& r_sel, int32_t& io_cur_verts, int32_t max_verts) noexcept {
        if (r_sel.unclaimed_mask && r_sel.capacity > 0) {
            const int32_t words = static_cast<int32_t>((r_sel.capacity + 63) / 64);
            const int32_t start_word = io_cur_verts / 64;
            for (int32_t w = start_word; w < words; ++w) {
                const uint64_t mask = r_sel.unclaimed_mask[w];
                if (mask == 0ULL) continue;
                const int32_t bit = std::countr_zero(mask);
                const int32_t slot = w * 64 + bit;
                if (slot >= io_cur_verts && slot < max_verts) {
                    r_sel.unclaimed_mask[w] &= ~(1ULL << bit);
                    io_cur_verts = std::max(io_cur_verts, slot + 1);
                    return slot;
                }
            }
        }
        if (io_cur_verts < max_verts) {
            return io_cur_verts++;
        }
        return -1;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        GrantPartPOD& idx_part = p_ctx.grant->parts[1];
        if (!vtx_part.raw_base_ptr || !idx_part.raw_base_ptr) return;

        const GrantPartPOD* rest_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        godot::Vector3* verts = reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr);
        const int32_t max_verts = static_cast<int32_t>(vtx_part.capacity_bytes / sizeof(godot::Vector3));
        int32_t cur_verts = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
            : max_verts;

        const godot::Vector3* rest_verts = (rest_part && rest_part->capacity_bytes >= sizeof(godot::Vector3))
            ? reinterpret_cast<const godot::Vector3*>(rest_part->raw_base_ptr)
            : nullptr;
        const int32_t max_rest = rest_verts ? static_cast<int32_t>(rest_part->capacity_bytes / sizeof(godot::Vector3)) : 0;

        int32_t* indices = reinterpret_cast<int32_t*>(idx_part.raw_base_ptr);
        const int32_t n_indices = (idx_part.selection.element_count > 0)
            ? static_cast<int32_t>(idx_part.selection.element_count)
            : static_cast<int32_t>(idx_part.capacity_bytes / sizeof(int32_t));
        const int32_t n_tris = n_indices / 3;

        int32_t splits_done = 0;

        for (int32_t t = 0; t < n_tris && splits_done < max_splits_per_frame; ++t) {
            int32_t* tri = indices + t * 3;
            const int32_t i0 = tri[0], i1 = tri[1], i2 = tri[2];
            if (i0 < 0 || i0 >= cur_verts || i1 < 0 || i1 >= cur_verts || i2 < 0 || i2 >= cur_verts) continue;

            const godot::Vector3 centroid = (verts[i0] + verts[i1] + verts[i2]) / 3.0f;

            for (int e = 0; e < 3 && splits_done < max_splits_per_frame; ++e) {
                const int32_t a = tri[e];
                const int32_t b = tri[(e + 1) % 3];
                const float cur_len = (verts[b] - verts[a]).length();

                float rest_len = fallback_rest_edge_length;
                if (rest_verts && a < max_rest && b < max_rest) {
                    rest_len = std::max(1e-5f, (rest_verts[b] - rest_verts[a]).length());
                }

                const float strain = (cur_len - rest_len) / rest_len;
                if (strain > tear_threshold) {
                    const int32_t new_slot = claim_unclaimed_slot(vtx_part.selection, cur_verts, max_verts);
                    if (new_slot < 0) break;

                    const godot::Vector3 recoil_dir = (centroid - verts[b]).normalized();
                    verts[new_slot] = verts[b] + recoil_dir * tear_recoil_offset;
                    tri[(e + 1) % 3] = new_slot;
                    ++splits_done;
                }
            }
        }

        vtx_part.selection.element_count = cur_verts;
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core
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
 * M43: GeodesicRingMetadataLogic
 * Propagates K-step topological hop distance outward from seed vertices (or currently active
 * selection bits) along indexed triangle edges, stamping hop rings into r_selection.lod_levels.
 */
struct GeodesicRingMetadataLogic {
    static constexpr std::string_view ui_name = "Geodesic Topological Hop Ring Metadata";
    static constexpr std::string_view ui_category = "Model/Metadata";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::STENCIL,
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

    static constexpr int32_t MAX_RING_VERTS = 32768;

    int32_t seed_vertex_index = 0;
    int32_t max_hop_rings = 16;
    bool seed_from_active_selection = false;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("seed_vertex_index", godot::Variant::INT, seed_vertex_index);
        add_prop("max_hop_rings", godot::Variant::INT, max_hop_rings);
        add_prop("seed_from_active_selection", godot::Variant::BOOL, seed_from_active_selection);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("seed_vertex_index")) seed_vertex_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["seed_vertex_index"]));
        if (p_props.has("max_hop_rings")) max_hop_rings = std::clamp<int32_t>(static_cast<int32_t>(p_props["max_hop_rings"]), 1, 254);
        if (p_props.has("seed_from_active_selection")) seed_from_active_selection = static_cast<bool>(p_props["seed_from_active_selection"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_RING_VERTS * sizeof(uint8_t) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_RING_VERTS * sizeof(uint8_t) + 256;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& idx_part = p_ctx.grant->parts[1];
        MemoryBufferSelectionPOD& sel = vtx_part.selection;
        if (!sel.lod_levels || sel.capacity <= 0 || !idx_part.raw_base_ptr) return;

        const int32_t raw_v = (sel.element_count > 0)
            ? static_cast<int32_t>(sel.element_count)
            : static_cast<int32_t>(sel.capacity);
        const int32_t n_verts = std::min<int32_t>({raw_v, static_cast<int32_t>(sel.capacity), MAX_RING_VERTS});
        if (n_verts <= 0) return;

        for (int32_t i = 0; i < n_verts; ++i) {
            sel.lod_levels[i] = 255;
        }

        if (seed_from_active_selection && sel.mode == SelectionMode::DENSE && sel.data.bitset) {
            for (int32_t i = 0; i < n_verts; ++i) {
                if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) {
                    sel.lod_levels[i] = 0;
                }
            }
        } else if (seed_vertex_index >= 0 && seed_vertex_index < n_verts) {
            sel.lod_levels[seed_vertex_index] = 0;
        }

        const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part.raw_base_ptr);
        const int32_t n_tris = static_cast<int32_t>(((idx_part.selection.element_count > 0)
            ? idx_part.selection.element_count
            : static_cast<int64_t>(idx_part.capacity_bytes / sizeof(int32_t))) / 3);

        // Iterative Bellman-Ford wavefront relaxation over triangle edges for ring = 0 .. max_hop_rings - 1
        for (int32_t ring = 0; ring < max_hop_rings; ++ring) {
            const uint8_t cur_ring = static_cast<uint8_t>(ring);
            const uint8_t next_ring = static_cast<uint8_t>(ring + 1);
            bool any_advanced = false;

            for (int32_t t = 0; t < n_tris; ++t) {
                const int32_t i0 = indices[t * 3 + 0];
                const int32_t i1 = indices[t * 3 + 1];
                const int32_t i2 = indices[t * 3 + 2];
                if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

                const uint8_t r0 = sel.lod_levels[i0];
                const uint8_t r1 = sel.lod_levels[i1];
                const uint8_t r2 = sel.lod_levels[i2];

                if (r0 == cur_ring || r1 == cur_ring || r2 == cur_ring) {
                    if (r0 > next_ring) { sel.lod_levels[i0] = next_ring; any_advanced = true; }
                    if (r1 > next_ring) { sel.lod_levels[i1] = next_ring; any_advanced = true; }
                    if (r2 > next_ring) { sel.lod_levels[i2] = next_ring; any_advanced = true; }
                }
            }

            if (!any_advanced) break;
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
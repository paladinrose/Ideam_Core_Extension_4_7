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
 * M42: MeshConnectedIslandsMetadataLogic
 * Runs a path-compressed Disjoint-Set Union (DSU) over active 3D triangle indices in transient
 * workspace to label severed or disconnected mesh islands with contiguous component IDs in
 * r_selection.partition_ids.
 */
struct MeshConnectedIslandsMetadataLogic {
    static constexpr std::string_view ui_name = "Mesh Connected Islands (Topological DSU)";
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

    static constexpr int32_t MAX_DSU_VERTS = 32768;

    bool compact_island_ids = true;

    godot::Array get_property_list() const {
        godot::Array list;
        godot::Dictionary d;
        d["name"] = "compact_island_ids";
        d["type"] = static_cast<int>(godot::Variant::BOOL);
        d["default"] = compact_island_ids;
        list.push_back(d);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("compact_island_ids")) compact_island_ids = static_cast<bool>(p_props["compact_island_ids"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_DSU_VERTS * sizeof(int32_t) * 2 + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_DSU_VERTS * sizeof(int32_t) * 2 + 256;
    }

    static inline int32_t dsu_find(int32_t* parent, int32_t x) noexcept {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }

    static inline void dsu_union(int32_t* parent, int32_t a, int32_t b) noexcept {
        const int32_t ra = dsu_find(parent, a);
        const int32_t rb = dsu_find(parent, b);
        if (ra != rb) {
            if (ra < rb) parent[rb] = ra;
            else parent[ra] = rb;
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2 || !p_ctx.local_workspace) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& idx_part = p_ctx.grant->parts[1];
        MemoryBufferSelectionPOD& sel = vtx_part.selection;
        if (!sel.partition_ids || sel.capacity <= 0 || !idx_part.raw_base_ptr) return;

        const int32_t raw_v = (sel.element_count > 0)
            ? static_cast<int32_t>(sel.element_count)
            : static_cast<int32_t>(sel.capacity);
        const int32_t n_verts = std::min<int32_t>({raw_v, static_cast<int32_t>(sel.capacity), MAX_DSU_VERTS});
        if (n_verts <= 0) return;

        int32_t* parent = static_cast<int32_t*>(p_ctx.local_workspace);
        int32_t* root_to_compact = parent + MAX_DSU_VERTS;

        for (int32_t i = 0; i < n_verts; ++i) {
            parent[i] = i;
            root_to_compact[i] = -1;
        }

        const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part.raw_base_ptr);
        const int32_t n_tris = static_cast<int32_t>(((idx_part.selection.element_count > 0)
            ? idx_part.selection.element_count
            : static_cast<int64_t>(idx_part.capacity_bytes / sizeof(int32_t))) / 3);

        for (int32_t t = 0; t < n_tris; ++t) {
            const int32_t i0 = indices[t * 3 + 0];
            const int32_t i1 = indices[t * 3 + 1];
            const int32_t i2 = indices[t * 3 + 2];
            if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;
            if (i0 == i1 && i1 == i2) continue;

            dsu_union(parent, i0, i1);
            dsu_union(parent, i1, i2);
        }

        int32_t next_island_id = 0;
        for (int32_t i = 0; i < n_verts; ++i) {
            const int32_t r = dsu_find(parent, i);
            if (compact_island_ids) {
                if (root_to_compact[r] < 0) {
                    root_to_compact[r] = next_island_id++;
                }
                sel.partition_ids[i] = root_to_compact[r];
            } else {
                sel.partition_ids[i] = r;
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
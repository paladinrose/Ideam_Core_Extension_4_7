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
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * MeshConnectedIslandsMetadataLogic<T, T_Strategy, T_View> (M42)
 * Runs a path-compressed Disjoint-Set Union (DSU) over active 3D triangle indices in transient
 * workspace to label severed or disconnected mesh islands with contiguous component IDs in
 * r_selection.partition_ids.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) MeshConnectedIslandsMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr int32_t MAX_DSU_VERTS = 32768;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_DSU_VERTS * sizeof(int32_t) * 2 + 256;

    static constexpr std::string_view display_name = "Mesh Connected Islands (Topological DSU)";
    static constexpr std::string_view ui_category  = "Model/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B: Vertex buffer
    uint32_t index_buffer_id     = INVALID_ID; // 4B: Triangle index buffer
    bool compact_island_ids      = true;       // 1B
    uint8_t reserved_padding[55] = {0};        // 55B -> Total = 64B

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
        add_buf("index_buffer_id");

        godot::Dictionary d;
        d["name"] = "compact_island_ids";
        d["type"] = static_cast<int>(godot::Variant::BOOL);
        d["default"] = true;
        props.push_back(d);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : index_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("index_buffer_id")) index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        if (p_props.has("compact_island_ids")) compact_island_ids = static_cast<bool>(p_props["compact_island_ids"]);
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

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.partition_ids || r_selection.capacity <= 0 || !p_ctx.local_workspace) return;

            const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(index_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!idx_part || !idx_part->raw_base_ptr) return;

            const int32_t raw_v = (r_selection.element_count > 0)
                ? static_cast<int32_t>(r_selection.element_count)
                : static_cast<int32_t>(r_selection.capacity);
            const int32_t n_verts = std::min<int32_t>({raw_v, static_cast<int32_t>(r_selection.capacity), MAX_DSU_VERTS});
            if (n_verts <= 0) return;

            int32_t* parent = static_cast<int32_t*>(p_ctx.local_workspace);
            int32_t* root_to_compact = parent + MAX_DSU_VERTS;

            for (int32_t i = 0; i < n_verts; ++i) {
                parent[i] = i;
                root_to_compact[i] = -1;
            }

            const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_tris = static_cast<int32_t>(((idx_part->selection.element_count > 0)
                ? idx_part->selection.element_count
                : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3);

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
                    r_selection.partition_ids[i] = root_to_compact[r];
                } else {
                    r_selection.partition_ids[i] = r;
                }
            }
        }
    }
};

static_assert(sizeof(MeshConnectedIslandsMetadataLogic<>) == 64, "MeshConnectedIslandsMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<MeshConnectedIslandsMetadataLogic<>>, "MeshConnectedIslandsMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
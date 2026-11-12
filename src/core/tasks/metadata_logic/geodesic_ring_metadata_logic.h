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
 * GeodesicRingMetadataLogic<T, T_Strategy, T_View> (M43)
 * Propagates K-step topological hop distance outward from seed vertices (or currently active
 * selection bits) along indexed triangle edges, stamping hop rings into r_selection.lod_levels.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) GeodesicRingMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr int32_t MAX_RING_VERTS = 32768;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_RING_VERTS * sizeof(uint8_t) + 256;

    static constexpr std::string_view display_name = "Geodesic Topological Hop Ring Metadata";
    static constexpr std::string_view ui_category  = "Model/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id         = INVALID_ID; // 4B: Vertex buffer
    uint32_t index_buffer_id          = INVALID_ID; // 4B: Triangle index buffer
    int32_t seed_vertex_index         = 0;          // 4B
    int32_t max_hop_rings             = 16;         // 4B -> 16B
    bool seed_from_active_selection   = false;      // 1B
    uint8_t reserved_padding[47]      = {0};        // 47B -> Total = 64B

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

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("seed_vertex_index", godot::Variant::INT, 0);
        add_prop("max_hop_rings", godot::Variant::INT, 16);
        add_prop("seed_from_active_selection", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : index_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("index_buffer_id")) index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        if (p_props.has("seed_vertex_index")) seed_vertex_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["seed_vertex_index"]));
        if (p_props.has("max_hop_rings")) max_hop_rings = std::clamp<int32_t>(static_cast<int32_t>(p_props["max_hop_rings"]), 1, 254);
        if (p_props.has("seed_from_active_selection")) seed_from_active_selection = static_cast<bool>(p_props["seed_from_active_selection"]);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.lod_levels || r_selection.capacity <= 0) return;

            const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(index_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!idx_part || !idx_part->raw_base_ptr) return;

            const int32_t raw_v = (r_selection.element_count > 0)
                ? static_cast<int32_t>(r_selection.element_count)
                : static_cast<int32_t>(r_selection.capacity);
            const int32_t n_verts = std::min<int32_t>({raw_v, static_cast<int32_t>(r_selection.capacity), MAX_RING_VERTS});
            if (n_verts <= 0) return;

            for (int32_t i = 0; i < n_verts; ++i) {
                r_selection.lod_levels[i] = 255;
            }

            if (seed_from_active_selection && r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                for (int32_t i = 0; i < n_verts; ++i) {
                    if (r_selection.data.bitset[i >> 6] & (1ULL << (i & 63))) {
                        r_selection.lod_levels[i] = 0;
                    }
                }
            } else if (seed_vertex_index >= 0 && seed_vertex_index < n_verts) {
                r_selection.lod_levels[seed_vertex_index] = 0;
            }

            const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_tris = static_cast<int32_t>(((idx_part->selection.element_count > 0)
                ? idx_part->selection.element_count
                : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3);

            for (int32_t ring = 0; ring < max_hop_rings; ++ring) {
                const uint8_t cur_ring = static_cast<uint8_t>(ring);
                const uint8_t next_ring = static_cast<uint8_t>(ring + 1);
                bool any_advanced = false;

                for (int32_t t = 0; t < n_tris; ++t) {
                    const int32_t i0 = indices[t * 3 + 0];
                    const int32_t i1 = indices[t * 3 + 1];
                    const int32_t i2 = indices[t * 3 + 2];
                    if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

                    const uint8_t r0 = r_selection.lod_levels[i0];
                    const uint8_t r1 = r_selection.lod_levels[i1];
                    const uint8_t r2 = r_selection.lod_levels[i2];

                    if (r0 == cur_ring || r1 == cur_ring || r2 == cur_ring) {
                        if (r0 > next_ring) { r_selection.lod_levels[i0] = next_ring; any_advanced = true; }
                        if (r1 > next_ring) { r_selection.lod_levels[i1] = next_ring; any_advanced = true; }
                        if (r2 > next_ring) { r_selection.lod_levels[i2] = next_ring; any_advanced = true; }
                    }
                }

                if (!any_advanced) break;
            }
        }
    }
};

static_assert(sizeof(GeodesicRingMetadataLogic<>) == 64, "GeodesicRingMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<GeodesicRingMetadataLogic<>>, "GeodesicRingMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
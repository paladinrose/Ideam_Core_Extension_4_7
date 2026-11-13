#pragma once

/**
 * tensor_singularity_and_seed_query_logic.h
 * G_Q2: Scans a 2D Symmetric Tensor Field grid (`GEOM_TENSOR_CELL_POD`) for topological
 * Poincaré index singularities (`Wedge +1/2`, `Trisector -1/2`, `Center +1`, `Saddle -1`),
 * populates `GEOM_TENSOR_SINGULARITY_POD`, and filters/wakes valid hyperstreamline seeds
 * in `GEOM_TENSOR_STREAMLINE_SEED_POD`.
 *
 * Target View / Strategy: `SingleElementView` | `SparseSetView` / `FlatStrategy`
 * UI Category: `"Geometry/TensorField"`
 */

#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../../../../geometry/tensor_field_2d.h"
#include "../../../../memory/memory_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <typename T = float>
struct alignas(64) TensorSingularityAndSeedQueryLogic {
    using ValueType = T;

    // --- Compile-Time DOD Traits ---
    static constexpr std::string_view display_name        = "Tensor Field: Singularity & Streamline Seed Filter";
    static constexpr std::string_view ui_category         = "Geometry/TensorField";
    static constexpr ViewCapability required_capabilities = ViewCapability::READ_ONLY;
    static constexpr ViewLayout compatible_layouts        = ViewLayout::SINGLE_ELEMENT | ViewLayout::SPARSE_SET;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32 | DataType::BYTE;
    static constexpr size_t required_dimensions           = 0;
    static constexpr bool supports_cull                   = true;
    static constexpr bool supports_addition               = true;

    // --- 64-Byte Cache-Line Layout (16 x 4B = 64 Bytes) ---
    uint32_t target_buffer_id                = INVALID_ID; // 4B [0..3]   (GEOM_TENSOR_STREAMLINE_SEED_POD)
    uint32_t tensor_grid_header_buffer_id    = INVALID_ID; // 4B [4..7]   (GEOM_TENSOR_GRID_HEADER_POD)
    uint32_t tensor_cell_buffer_id           = INVALID_ID; // 4B [8..11]  (GEOM_TENSOR_CELL_POD)
    uint32_t tensor_singularity_buffer_id    = INVALID_ID; // 4B [12..15] (GEOM_TENSOR_SINGULARITY_POD)
    uint32_t tensor_singularity_count_buf_id = INVALID_ID; // 4B [16..19] (GEOM_TENSOR_SINGULARITY_COUNT)
    uint32_t tensor_seed_buffer_id           = INVALID_ID; // 4B [20..23] (GEOM_TENSOR_STREAMLINE_SEED_POD)
    uint32_t active_grid_slot                = 0u;         // 4B [24..27]
    float min_anisotropy_for_seed            = 1e-4f;      // 4B [28..31]
    float min_seed_priority                  = 0.0f;       // 4B [32..35]
    uint32_t auto_seed_singularities         = 1u;         // 4B [36..39]
    uint32_t reserved_pad_0                  = 0u;         // 4B [40..43]
    uint32_t reserved_pad_1                  = 0u;         // 4B [44..47]
    uint32_t reserved_pad_2                  = 0u;         // 4B [48..51]
    uint32_t reserved_pad_3                  = 0u;         // 4B [52..55]
    uint32_t reserved_pad_4                  = 0u;         // 4B [56..59]
    uint32_t reserved_pad_5                  = 0u;         // 4B [60..63]

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : tensor_seed_buffer_id;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, const godot::Variant &p_default) {
            godot::Dictionary d;
            d["name"]    = godot::String(p_name);
            d["type"]    = static_cast<int64_t>(p_type);
            d["default"] = p_default;
            d["hint"]    = godot::PROPERTY_HINT_NONE;
            d["usage"]   = godot::PROPERTY_USAGE_DEFAULT;
            props.push_back(d);
        };
        add_prop("target_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_grid_header_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_cell_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_singularity_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_singularity_count_buf_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_seed_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("active_grid_slot", godot::Variant::INT, int64_t(0));
        add_prop("min_anisotropy_for_seed", godot::Variant::FLOAT, 0.0001);
        add_prop("min_seed_priority", godot::Variant::FLOAT, 0.0);
        add_prop("auto_seed_singularities", godot::Variant::INT, int64_t(1));
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        auto read_u32 = [&](const char *p_key, uint32_t &r_dst) {
            const godot::String k(p_key);
            if (p_props.has(k)) {
                r_dst = static_cast<uint32_t>(static_cast<int64_t>(p_props[k]));
            }
        };
        read_u32("target_buffer_id", target_buffer_id);
        read_u32("tensor_grid_header_buffer_id", tensor_grid_header_buffer_id);
        read_u32("tensor_cell_buffer_id", tensor_cell_buffer_id);
        read_u32("tensor_singularity_buffer_id", tensor_singularity_buffer_id);
        read_u32("tensor_singularity_count_buf_id", tensor_singularity_count_buf_id);
        read_u32("tensor_seed_buffer_id", tensor_seed_buffer_id);
        read_u32("active_grid_slot", active_grid_slot);
        read_u32("auto_seed_singularities", auto_seed_singularities);
        if (p_props.has("min_anisotropy_for_seed")) {
            min_anisotropy_for_seed = static_cast<float>(static_cast<double>(p_props["min_anisotropy_for_seed"]));
        }
        if (p_props.has("min_seed_priority")) {
            min_seed_priority = static_cast<float>(static_cast<double>(p_props["min_seed_priority"]));
        }
        if (target_buffer_id == INVALID_ID) {
            target_buffer_id = tensor_seed_buffer_id;
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context,
        const T_View &p_view
    ) const {
        (void)p_view;
        MemoryManagerDOD *mgr = p_context.manager;
        if (!mgr) return;

        auto get_span = [&]<typename Elem>(uint32_t p_buf_id) noexcept -> std::span<Elem> {
            if (p_buf_id == INVALID_ID) return {};
            MemoryBufferPOD *buf = mgr->get_buffer(p_buf_id);
            if (!buf || !buf->master_block_ptr || buf->max_elements <= 0) return {};
            return std::span<Elem>(
                reinterpret_cast<Elem *>(buf->master_block_ptr + buf->memory_offset),
                static_cast<size_t>(buf->max_elements)
            );
        };

        auto headers   = get_span.template operator()<const TensorGridHeaderPod>(tensor_grid_header_buffer_id);
        auto cells     = get_span.template operator()<const SymmetricTensor2DPod>(tensor_cell_buffer_id);
        auto sings     = get_span.template operator()<TensorSingularityPod>(tensor_singularity_buffer_id);
        auto sing_cnt  = get_span.template operator()<uint32_t>(tensor_singularity_count_buf_id);
        auto seeds     = get_span.template operator()<TensorStreamlineSeedPod>(get_target_buffer_id());

        if (!headers.empty() && !cells.empty() && !sings.empty()) {
            const size_t g = (active_grid_slot < headers.size()) ? active_grid_slot : 0u;
            const uint32_t detected = TensorFieldMath2D::detect_grid_singularities(headers[g], cells, sings);
            if (!sing_cnt.empty()) {
                sing_cnt[0] = detected;
            }
        }

        if (seeds.empty()) return;
        MemoryBufferPOD *seed_buf = mgr->get_buffer(get_target_buffer_id());
        const size_t active_seeds = (seed_buf && seed_buf->current_count > 0)
            ? std::min<size_t>(static_cast<size_t>(seed_buf->current_count), seeds.size())
            : seeds.size();

        for (size_t i = 0; i < active_seeds; ++i) {
            const TensorStreamlineSeedPod &s = seeds[i];
            const bool valid = (s.active != 0u) && (s.priority >= min_seed_priority);
            if constexpr (Op == QueryOp::ADD) {
                if (valid) {
                    r_selection.add_index(static_cast<int64_t>(i));
                }
            } else {
                if (!valid) {
                    r_selection.remove_index(static_cast<int64_t>(i));
                }
            }
        }
    }
};

static_assert(sizeof(TensorSingularityAndSeedQueryLogic<float>) == 64, "TensorSingularityAndSeedQueryLogic must be 64 bytes!");

} // namespace ideam::core

#pragma once

/**
 * heatmap_grid_buffer_sync_transform_logic.h
 * Ecosystem Companion Bridge (UI1): Zero-Variant 2D Spatial DOD Buffer Sync into `HeatmapGrid::set_grid_data`.
 *
 * Performs a strided 2D spatial copy/quantization from a `Spatial2DStrategy` DOD buffer directly
 * into a staging byte buffer (`output_byte_buffer_id`) and/or a live `ideam::godot_ext::HeatmapGrid`
 * control via `HeatmapGrid::set_grid_data(const uint8_t*, int, int)` with zero per-cell Variant boxing.
 */

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../../godot/controls/heatmap_grid.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ideam::core {

#ifndef IDEAM_HEATMAP_GRID_SYNC_MODE_ENUM_DEFINED
#define IDEAM_HEATMAP_GRID_SYNC_MODE_ENUM_DEFINED

enum HeatmapGridSyncMode : uint8_t {
    HEATMAP_SYNC_RAW_BYTES        = 0, // Direct memcpy / byte cast into [0..255]
    HEATMAP_SYNC_NORMALIZE_RANGE  = 1, // Linearly maps [min_value, max_value] -> [0..255]
    HEATMAP_SYNC_SELECTION_STATES = 2  // Encodes Selection bitmask states (STATE_UNSELECTED=0, STATE_SELECTED=1, STATE_PENDING=3)
};

#endif // IDEAM_HEATMAP_GRID_SYNC_MODE_ENUM_DEFINED

template <typename T = uint8_t, typename T_Strategy = Spatial2DStrategy>
struct alignas(64) HeatmapGridBufferSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::SPATIAL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 2;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Heatmap Grid Buffer Sync";
    static constexpr std::string_view ui_category  = "UI/Controls";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint64_t heatmap_grid_object_id    = 0ULL;                    // 8B: Target HeatmapGrid ObjectID
    int64_t buffer_stride_y            = 0;                       // 8B: Row pitch in bytes (0 = buffer_width * stride) -> 16B

    uint32_t source_buffer_id          = INVALID_ID;              // 4B: Source 2D Spatial DOD buffer
    uint32_t output_byte_buffer_id     = INVALID_ID;              // 4B: Optional contiguous uint8_t staging buffer
    uint32_t selection_source_buffer_id= INVALID_ID;              // 4B: Optional buffer whose selection mask is visualized
    int32_t buffer_width               = 64;                      // 4B: 2D grid width (columns)
    int32_t buffer_height              = 64;                      // 4B: 2D grid height (rows)
    float min_value                    = 0.0f;                    // 4B: Normalization minimum for HEATMAP_SYNC_NORMALIZE_RANGE
    float max_value                    = 1.0f;                    // 4B: Normalization maximum for HEATMAP_SYNC_NORMALIZE_RANGE -> 44B

    uint8_t sync_mode                  = HEATMAP_SYNC_RAW_BYTES;  // 1B: HeatmapGridSyncMode (0..2)
    bool clamp_values                  = true;                    // 1B: Clamp normalized values to [0, 255]
    bool sync_columns_to_width         = true;                    // 1B: Update HeatmapGrid::columns to buffer_width
    bool push_to_heatmap_control       = true;                    // 1B: Invoke HeatmapGrid::set_grid_data when ObjectID is valid -> 48B
    uint8_t reserved_padding[16]       = {0};                     // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("source_buffer_id");
        add_buf("output_byte_buffer_id");
        add_buf("selection_source_buffer_id");

        godot::Dictionary mode_prop;
        mode_prop["name"]        = "sync_mode";
        mode_prop["type"]        = godot::Variant::INT;
        mode_prop["hint"]        = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Raw Bytes:0,Normalize Scalar Range:1,Selection State Overlay:2";
        props.push_back(mode_prop);

        godot::Dictionary oid_prop;
        oid_prop["name"] = "heatmap_grid_object_id";
        oid_prop["type"] = godot::Variant::INT;
        oid_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(oid_prop);

        godot::Dictionary w_prop;
        w_prop["name"]        = "buffer_width";
        w_prop["type"]        = godot::Variant::INT;
        w_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        w_prop["hint_string"] = "1,1024,1";
        props.push_back(w_prop);

        godot::Dictionary h_prop;
        h_prop["name"]        = "buffer_height";
        h_prop["type"]        = godot::Variant::INT;
        h_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        h_prop["hint_string"] = "1,1024,1";
        props.push_back(h_prop);

        godot::Dictionary sy_prop;
        sy_prop["name"] = "buffer_stride_y";
        sy_prop["type"] = godot::Variant::INT;
        sy_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(sy_prop);

        godot::Dictionary min_prop;
        min_prop["name"] = "min_value";
        min_prop["type"] = godot::Variant::FLOAT;
        min_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(min_prop);

        godot::Dictionary max_prop;
        max_prop["name"] = "max_value";
        max_prop["type"] = godot::Variant::FLOAT;
        max_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(max_prop);

        godot::Dictionary clp_prop;
        clp_prop["name"] = "clamp_values";
        clp_prop["type"] = godot::Variant::BOOL;
        clp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clp_prop);

        godot::Dictionary col_prop;
        col_prop["name"] = "sync_columns_to_width";
        col_prop["type"] = godot::Variant::BOOL;
        col_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_prop);

        godot::Dictionary push_prop;
        push_prop["name"] = "push_to_heatmap_control";
        push_prop["type"] = godot::Variant::BOOL;
        push_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(push_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (source_buffer_id != INVALID_ID)      return source_buffer_id;
        if (output_byte_buffer_id != INVALID_ID) return output_byte_buffer_id;
        return selection_source_buffer_id;
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        (void)p_context;
        if constexpr (requires { r_view.strategy.stride_y; }) {
            const size_t stride = (p_part && p_part->element_stride > 0u) ? p_part->element_stride : sizeof(T);
            if (buffer_stride_y > 0) {
                r_view.strategy.stride_y = buffer_stride_y;
            } else if (buffer_width > 0) {
                r_view.strategy.stride_y = static_cast<int64_t>(static_cast<size_t>(buffer_width) * stride);
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("output_byte_buffer_id")) {
            output_byte_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_byte_buffer_id"]));
        }
        if (p_props.has("selection_source_buffer_id")) {
            selection_source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["selection_source_buffer_id"]));
        }
        if (p_props.has("heatmap_grid_object_id")) {
            heatmap_grid_object_id = static_cast<uint64_t>(static_cast<int64_t>(p_props["heatmap_grid_object_id"]));
        }
        if (p_props.has("buffer_width")) {
            buffer_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["buffer_width"]));
        }
        if (p_props.has("buffer_height")) {
            buffer_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["buffer_height"]));
        }
        if (p_props.has("buffer_stride_y")) {
            buffer_stride_y = static_cast<int64_t>(p_props["buffer_stride_y"]);
        }
        if (p_props.has("min_value")) {
            min_value = static_cast<float>(p_props["min_value"]);
        }
        if (p_props.has("max_value")) {
            max_value = static_cast<float>(p_props["max_value"]);
        }
        if (p_props.has("sync_mode")) {
            sync_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["sync_mode"]), 0, 2));
        }
        if (p_props.has("clamp_values")) {
            clamp_values = static_cast<bool>(p_props["clamp_values"]);
        }
        if (p_props.has("sync_columns_to_width")) {
            sync_columns_to_width = static_cast<bool>(p_props["sync_columns_to_width"]);
        }
        if (p_props.has("push_to_heatmap_control")) {
            push_to_heatmap_control = static_cast<bool>(p_props["push_to_heatmap_control"]);
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        const uint32_t src_id = get_target_buffer_id();
        const GrantPartPOD* src_part = (src_id != INVALID_ID)
            ? p_context.get_grant_part(src_id)
            : ((p_context.grant && p_context.grant->part_count > 0) ? &p_context.grant->parts[0] : nullptr);
        if (!src_part || !src_part->raw_base_ptr) return;

        const GrantPartPOD* out_part = (output_byte_buffer_id != INVALID_ID)
            ? p_context.get_grant_part(output_byte_buffer_id)
            : nullptr;
        const GrantPartPOD* sel_part = (selection_source_buffer_id != INVALID_ID)
            ? p_context.get_grant_part(selection_source_buffer_id)
            : src_part;

        const uint32_t elem_stride = (src_part->element_stride > 0u) ? src_part->element_stride : static_cast<uint32_t>(sizeof(T));
        const int64_t max_src_elems = static_cast<int64_t>(src_part->capacity_bytes / elem_stride);
        if (max_src_elems <= 0) return;

        const int32_t grid_w = std::max<int32_t>(1, buffer_width);
        const int32_t grid_h = (buffer_height > 0)
            ? std::min<int32_t>(buffer_height, static_cast<int32_t>(max_src_elems / grid_w))
            : static_cast<int32_t>(max_src_elems / grid_w);
        if (grid_h <= 0) return;

        const int32_t total_cells = grid_w * grid_h;
        const int64_t row_pitch_bytes = (buffer_stride_y > 0)
            ? buffer_stride_y
            : static_cast<int64_t>(grid_w) * static_cast<int64_t>(elem_stride);

        uint8_t* dst_bytes = (out_part && out_part->raw_base_ptr && static_cast<int64_t>(out_part->capacity_bytes) >= total_cells)
            ? out_part->raw_base_ptr
            : nullptr;

        std::vector<uint8_t> stack_staging;
        if (!dst_bytes) {
            stack_staging.resize(static_cast<size_t>(total_cells), 0u);
            dst_bytes = stack_staging.data();
        }

        const uint8_t* src_raw = src_part->raw_base_ptr;
        const MemoryBufferSelectionPOD& sel = sel_part ? sel_part->selection : src_part->selection;

        // Fast-path contiguous 1-byte raw copy when unstrided
        if (sync_mode == HEATMAP_SYNC_RAW_BYTES && elem_stride == 1u && row_pitch_bytes == grid_w) {
            std::memcpy(dst_bytes, src_raw, static_cast<size_t>(total_cells));
        } else {
            const float range_span = ( std::abs(max_value - min_value) > 1e-8f ) ? (max_value - min_value) : 1.0f;
            const float inv_span   = 255.0f / range_span;

            for (int32_t r = 0; r < grid_h; ++r) {
                const uint8_t* src_row = src_raw + static_cast<int64_t>(r) * row_pitch_bytes;
                uint8_t* dst_row       = dst_bytes + static_cast<size_t>(r * grid_w);

                for (int32_t c = 0; c < grid_w; ++c) {
                    const int64_t flat_idx = static_cast<int64_t>(r) * grid_w + c;
                    if (sync_mode == HEATMAP_SYNC_SELECTION_STATES) {
                        uint8_t state_val = static_cast<uint8_t>(godot_ext::HeatmapGrid::STATE_UNSELECTED);
                        if (sel.is_selected(flat_idx)) {
                            state_val = static_cast<uint8_t>(godot_ext::HeatmapGrid::STATE_SELECTED);
                        } else if (sel.unclaimed_mask && flat_idx < sel.capacity &&
                                   ((sel.unclaimed_mask[flat_idx >> 6] & (1ULL << (flat_idx & 63))) == 0ULL)) {
                            state_val = static_cast<uint8_t>(godot_ext::HeatmapGrid::STATE_PENDING);
                        }
                        dst_row[c] = state_val;
                        continue;
                    }

                    const uint8_t* cell_ptr = src_row + static_cast<size_t>(c) * elem_stride;
                    if (sync_mode == HEATMAP_SYNC_RAW_BYTES) {
                        if constexpr (std::is_arithmetic_v<T>) {
                            const T val = *reinterpret_cast<const T*>(cell_ptr);
                            const int64_t iv = static_cast<int64_t>(val);
                            dst_row[c] = clamp_values
                                ? static_cast<uint8_t>(std::clamp<int64_t>(iv, 0, 255))
                                : static_cast<uint8_t>(iv & 0xFF);
                        } else {
                            dst_row[c] = *cell_ptr;
                        }
                    } else {
                        // HEATMAP_SYNC_NORMALIZE_RANGE
                        float fval = 0.0f;
                        if constexpr (std::is_arithmetic_v<T>) {
                            fval = static_cast<float>(*reinterpret_cast<const T*>(cell_ptr));
                        } else if (elem_stride >= sizeof(float)) {
                            fval = *reinterpret_cast<const float*>(cell_ptr);
                        } else {
                            fval = static_cast<float>(*cell_ptr);
                        }
                        float scaled = (fval - min_value) * inv_span;
                        if (clamp_values) {
                            scaled = std::clamp(scaled, 0.0f, 255.0f);
                        }
                        dst_row[c] = static_cast<uint8_t>(std::lround(scaled));
                    }
                }
            }
        }

        if (push_to_heatmap_control && heatmap_grid_object_id != 0ULL) {
            if (godot::Object* obj = godot::ObjectDB::get_instance(godot::ObjectID(heatmap_grid_object_id))) {
                if (auto* heatmap = godot::Object::cast_to<godot_ext::HeatmapGrid>(obj)) {
                    heatmap->set_grid_data(dst_bytes, total_cells, sync_columns_to_width ? grid_w : -1);
                }
            }
        }
    }
};

static_assert(sizeof(HeatmapGridBufferSyncTransformLogic<>) == 64, "HeatmapGridBufferSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<HeatmapGridBufferSyncTransformLogic<>>, "HeatmapGridBufferSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core

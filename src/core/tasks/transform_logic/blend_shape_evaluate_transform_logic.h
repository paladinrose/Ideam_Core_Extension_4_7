#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * BlendShapeEvaluateTransformLogic<T, MaxShapes>
 * Evaluates up to MaxShapes morph-target delta buffers in SIMD on the CPU:
 *   v_out[i] = v_base[i] + sum_{k=0..MaxShapes-1} (w_k * delta_k[i])
 * Supports multi-variant registration (e.g., MaxShapes = 1, 2, 4, 8) as well as
 * contiguous packed delta buffers when shapes > 6.
 */
template <typename T = godot::Vector3, size_t MaxShapes = 4>
struct alignas(64) BlendShapeEvaluateTransformLogic {
    static_assert(MaxShapes >= 1 && MaxShapes <= 8, "MaxShapes must be between 1 and 8.");

    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "BlendShape Evaluate";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t output_buffer_id       = INVALID_ID; // 4B: Primary target buffer (skinned/morphed vertices)
    uint32_t base_buffer_id         = INVALID_ID; // 4B: Base rest-pose vertex buffer (if INVALID_ID, accumulates in-place)
    uint32_t weights_buffer_id      = INVALID_ID; // 4B: Optional float buffer of dynamic morph weights
    uint32_t delta_buffer_ids[6]    = {           // 24B: Individual morph delta buffer IDs (for shapes 6..7, contiguous after slot 0)
        INVALID_ID, INVALID_ID, INVALID_ID,
        INVALID_ID, INVALID_ID, INVALID_ID
    };
    int16_t weights_fixed[8]        = {0};        // 16B: Normalized fixed-point weights (16384 = 1.0f, range [-2.0, +2.0])
    uint8_t active_shape_count      = static_cast<uint8_t>(MaxShapes); // 1B
    bool normalize_normals          = false;      // 1B: Normalize output vectors (when evaluating morph normals)
    uint8_t reserved_padding[10]    = {0};        // 10B -> Total = 64B

    static constexpr float FIXED_SCALE     = 16384.0f;
    static constexpr float INV_FIXED_SCALE = 1.0f / 16384.0f;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary count_prop;
        count_prop["name"] = "shape_count";
        count_prop["type"] = godot::Variant::INT;
        count_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        count_prop["hint_string"] = "logic_options";
        props.push_back(count_prop);

        godot::Dictionary out_prop;
        out_prop["name"] = "output_buffer_id";
        out_prop["type"] = godot::Variant::INT;
        out_prop["hint_string"] = "buffer_option";
        props.push_back(out_prop);

        godot::Dictionary base_prop;
        base_prop["name"] = "base_buffer_id";
        base_prop["type"] = godot::Variant::INT;
        base_prop["hint_string"] = "buffer_option";
        props.push_back(base_prop);

        godot::Dictionary wbuf_prop;
        wbuf_prop["name"] = "weights_buffer_id";
        wbuf_prop["type"] = godot::Variant::INT;
        wbuf_prop["hint_string"] = "buffer_option";
        props.push_back(wbuf_prop);

        constexpr size_t ExposedBufferSlots = (MaxShapes < 6) ? MaxShapes : 6;
        for (size_t k = 0; k < ExposedBufferSlots; ++k) {
            godot::Dictionary d_prop;
            d_prop["name"] = godot::String("delta_") + godot::String::num_int64(static_cast<int64_t>(k)) + godot::String("_buffer_id");
            d_prop["type"] = godot::Variant::INT;
            d_prop["hint_string"] = "buffer_option";
            props.push_back(d_prop);
        }

        for (size_t k = 0; k < MaxShapes; ++k) {
            godot::Dictionary w_prop;
            w_prop["name"] = godot::String("weight_") + godot::String::num_int64(static_cast<int64_t>(k));
            w_prop["type"] = godot::Variant::FLOAT;
            w_prop["hint"] = godot::PROPERTY_HINT_RANGE;
            w_prop["hint_string"] = "-2.0,2.0,0.01";
            props.push_back(w_prop);
        }

        godot::Dictionary norm_prop;
        norm_prop["name"] = "normalize_normals";
        norm_prop["type"] = godot::Variant::BOOL;
        norm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(norm_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return output_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("output_buffer_id")) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("base_buffer_id")) {
            base_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["base_buffer_id"]));
        }
        if (p_props.has("weights_buffer_id")) {
            weights_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["weights_buffer_id"]));
        }
        if (p_props.has("normalize_normals")) {
            normalize_normals = static_cast<bool>(p_props["normalize_normals"]);
        }

        for (size_t k = 0; k < 6; ++k) {
            const godot::String d_key = godot::String("delta_") + godot::String::num_int64(static_cast<int64_t>(k)) + godot::String("_buffer_id");
            if (p_props.has(d_key)) {
                delta_buffer_ids[k] = static_cast<uint32_t>(static_cast<int64_t>(p_props[d_key]));
            }
        }

        for (size_t k = 0; k < 8; ++k) {
            const godot::String w_key = godot::String("weight_") + godot::String::num_int64(static_cast<int64_t>(k));
            if (p_props.has(w_key)) {
                const float wf = std::clamp(static_cast<float>(p_props[w_key]), -1.99f, 1.99f);
                weights_fixed[k] = static_cast<int16_t>( std::lround(wf * FIXED_SCALE));
            }
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* out_part = p_context.get_grant_part(output_buffer_id);
            if (!out_part || !out_part->raw_base_ptr) return;

            const int64_t vert_count = out_part->selection.capacity;
            if (vert_count <= 0) return;

            const GrantPartPOD* base_part = (base_buffer_id != INVALID_ID && base_buffer_id != output_buffer_id)
                ? p_context.get_grant_part(base_buffer_id)
                : nullptr;
            const godot::Vector3* __restrict base_verts = (base_part && base_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3*>(base_part->raw_base_ptr)
                : nullptr;

            // Resolve weights (either from dynamic weights_buffer_id or unpacked from weights_fixed)
            float weights[MaxShapes] = {0.0f};
            const float* dyn_weights = nullptr;
            size_t dyn_weight_count  = 0;
            if (weights_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* w_part = p_context.get_grant_part(weights_buffer_id)) {
                    if (w_part->raw_base_ptr) {
                        dyn_weights      = reinterpret_cast<const float*>(w_part->raw_base_ptr);
                        dyn_weight_count = w_part->capacity_bytes / sizeof(float);
                    }
                }
            }
            for (size_t k = 0; k < MaxShapes; ++k) {
                weights[k] = (dyn_weights && k < dyn_weight_count)
                    ? dyn_weights[k]
                    : (static_cast<float>(weights_fixed[k]) * INV_FIXED_SCALE);
            }

            // Resolve morph delta pointers
            const godot::Vector3* delta_ptrs[MaxShapes] = {nullptr};
            const GrantPartPOD* first_delta_part = (delta_buffer_ids[0] != INVALID_ID)
                ? p_context.get_grant_part(delta_buffer_ids[0])
                : nullptr;

            for (size_t k = 0; k < MaxShapes; ++k) {
                if (k < 6 && delta_buffer_ids[k] != INVALID_ID) {
                    if (const GrantPartPOD* d_part = p_context.get_grant_part(delta_buffer_ids[k])) {
                        if (d_part->raw_base_ptr && d_part->capacity_bytes >= static_cast<size_t>(vert_count) * sizeof(godot::Vector3)) {
                            delta_ptrs[k] = reinterpret_cast<const godot::Vector3*>(d_part->raw_base_ptr);
                            continue;
                        }
                    }
                }
                // Fallback: packed contiguous morph target slices inside delta_buffer_ids[0]
                if (first_delta_part && first_delta_part->raw_base_ptr) {
                    const size_t slice_bytes = static_cast<size_t>(vert_count) * sizeof(godot::Vector3);
                    if (first_delta_part->capacity_bytes >= (k + 1) * slice_bytes) {
                        delta_ptrs[k] = reinterpret_cast<const godot::Vector3*>(first_delta_part->raw_base_ptr + k * slice_bytes);
                    }
                }
            }

            auto eval_vertex = [&](int64_t idx) noexcept {
                godot::Vector3 acc = base_verts ? base_verts[idx] : _read_view(p_view, idx);
                for (size_t k = 0; k < MaxShapes; ++k) {
                    const float wk = weights[k];
                    if (delta_ptrs[k] && std::abs(wk) > 1e-6f) {
                        acc += delta_ptrs[k][idx] * wk;
                    }
                }
                if (normalize_normals) {
                    acc = acc.normalized();
                }
                _write_view(p_view, idx, acc);
            };

            const MemoryBufferSelectionPOD& sel = out_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) eval_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    eval_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    eval_vertex(i);
                }
            }
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T _read_view(const T_View& p_view, int64_t idx) noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else {
            return static_cast<T>(p_view[idx]);
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _write_view(const T_View& p_view, int64_t idx, const T& value) noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<T*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

static_assert(sizeof(BlendShapeEvaluateTransformLogic<>) == 64, "BlendShapeEvaluateTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core
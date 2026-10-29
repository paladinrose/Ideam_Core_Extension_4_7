#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/aosoa_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class TransformApplyMode : uint8_t {
    POINT_XFORM              = 0, // Basis + Origin translation (positions)
    BASIS_XFORM              = 1, // Rotation + Scale only (velocities/offsets)
    NORMAL_INVERSE_TRANSPOSE = 2  // Inverse-transpose basis + normalize (normals/tangents under non-uniform scale)
};

/**
 * Transform3DBatchTransformLogic<T>
 * Applies a godot::Transform3D (or an indexed palette of Transform3D matrices bound via
 * transform_palette_buffer_id and selection.partition_ids) across Vector3 or Vector4 elements.
 * Supports both SIMD lane iteration (AOSOAView) and scalar/selection iteration (SingleElementView/SwapView).
 */
template <typename T = godot::Vector3>
struct alignas(64) Transform3DBatchTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR4 | DataType::VECTOR4D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Transform3D Batch";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id            = INVALID_ID; // Primary Vector3/Vector4 buffer to transform
    uint32_t source_buffer_id            = INVALID_ID; // Optional source buffer (if INVALID_ID, transforms in-place)
    uint32_t transform_palette_buffer_id = INVALID_ID; // Optional palette of Transform3D matrices
    TransformApplyMode mode              = TransformApplyMode::POINT_XFORM;
    bool use_partition_ids               = false;      // Index into palette using selection.partition_ids[idx]
    uint8_t reserved_flags[2]            = {0};

    // 48-Byte Transform3D (3x Vector3 basis rows + 1x Vector3 origin = 48B)
    godot::Transform3D transform{};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary src_prop;
        src_prop["name"] = "source_buffer_id";
        src_prop["type"] = godot::Variant::INT;
        src_prop["hint_string"] = "buffer_option";
        props.push_back(src_prop);

        godot::Dictionary pal_prop;
        pal_prop["name"] = "transform_palette_buffer_id";
        pal_prop["type"] = godot::Variant::INT;
        pal_prop["hint_string"] = "buffer_option";
        props.push_back(pal_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Point Xform (Basis + Origin):0,Basis Xform (Rotate + Scale):1,Normal Inverse-Transpose:2";
        props.push_back(mode_prop);

        godot::Dictionary part_prop;
        part_prop["name"] = "use_partition_ids";
        part_prop["type"] = godot::Variant::BOOL;
        part_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(part_prop);

        godot::Dictionary xform_prop;
        xform_prop["name"] = "transform";
        xform_prop["type"] = godot::Variant::TRANSFORM3D;
        xform_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(xform_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }
        if (p_props.has("transform_palette_buffer_id")) {
            transform_palette_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transform_palette_buffer_id"]));
        }
        if (p_props.has("mode")) {
            mode = static_cast<TransformApplyMode>(static_cast<uint8_t>(static_cast<int64_t>(p_props["mode"])));
        }
        if (p_props.has("use_partition_ids")) {
            use_partition_ids = static_cast<bool>(p_props["use_partition_ids"]);
        }
        if (p_props.has("transform")) {
            transform = static_cast<godot::Transform3D>(p_props["transform"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, godot::Vector4>) {
            const GrantPartPOD* dst_part = p_context.get_grant_part(target_buffer_id);
            if (!dst_part || !dst_part->raw_base_ptr) return;

            const GrantPartPOD* src_part = (source_buffer_id != INVALID_ID && source_buffer_id != target_buffer_id)
                ? p_context.get_grant_part(source_buffer_id)
                : nullptr;
            const T* __restrict src_raw = (src_part && src_part->raw_base_ptr)
                ? reinterpret_cast<const T*>(src_part->raw_base_ptr)
                : nullptr;

            const godot::Transform3D* __restrict palette = nullptr;
            size_t palette_count = 0;
            if (transform_palette_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* pal_part = p_context.get_grant_part(transform_palette_buffer_id)) {
                    if (pal_part->raw_base_ptr) {
                        palette = reinterpret_cast<const godot::Transform3D*>(pal_part->raw_base_ptr);
                        palette_count = pal_part->capacity_bytes / sizeof(godot::Transform3D);
                    }
                }
            }

            // Precompute default basis (or inverse-transpose basis for normals) outside the hot loop
            godot::Basis active_basis = transform.basis;
            if (mode == TransformApplyMode::NORMAL_INVERSE_TRANSPOSE) {
                active_basis = transform.basis.inverse().transposed();
            }
            const godot::Vector3 active_origin = transform.origin;

            const MemoryBufferSelectionPOD& sel = dst_part->selection;
            const int64_t* __restrict part_ids  = (use_partition_ids && sel.partition_ids) ? sel.partition_ids : nullptr;

            auto eval_element = [&](const T& in_val, int64_t idx) noexcept -> T {
                godot::Basis b   = active_basis;
                godot::Vector3 o = active_origin;

                if (palette && palette_count > 0) {
                    size_t pal_idx = 0;
                    if (part_ids) {
                        const int64_t pid = part_ids[idx];
                        if (pid >= 0 && static_cast<size_t>(pid) < palette_count) {
                            pal_idx = static_cast<size_t>(pid);
                        }
                    } else if (static_cast<size_t>(idx) < palette_count) {
                        pal_idx = static_cast<size_t>(idx);
                    }
                    const godot::Transform3D& xf = palette[pal_idx];
                    b = (mode == TransformApplyMode::NORMAL_INVERSE_TRANSPOSE)
                        ? xf.basis.inverse().transposed()
                        : xf.basis;
                    o = xf.origin;
                }

                if constexpr (std::is_same_v<T, godot::Vector3>) {
                    if (mode == TransformApplyMode::POINT_XFORM) {
                        return b.xform(in_val) + o;
                    } else if (mode == TransformApplyMode::BASIS_XFORM) {
                        return b.xform(in_val);
                    } else {
                        return b.xform(in_val).normalized();
                    }
                } else {
                    // Vector4: transform xyz, preserve w (e.g., tangent binormal sign or homogeneous weight)
                    const godot::Vector3 v3(in_val.x, in_val.y, in_val.z);
                    godot::Vector3 out3;
                    if (mode == TransformApplyMode::POINT_XFORM) {
                        out3 = b.xform(v3) + o;
                    } else if (mode == TransformApplyMode::BASIS_XFORM) {
                        out3 = b.xform(v3);
                    } else {
                        out3 = b.xform(v3).normalized();
                    }
                    return godot::Vector4(out3.x, out3.y, out3.z, in_val.w);
                }
            };

            // Fast-path SIMD lane iteration when bound to an AOSOAView on a contiguous RANGE selection
            if constexpr (requires { p_view.lane_count(); p_view.get_lane(0); T_View::lane_width; }) {
                if (sel.mode == SelectionMode::RANGE && sel.start_index == 0 && !src_raw) {
                    constexpr size_t L_WIDTH = T_View::lane_width;
                    const size_t lanes = p_view.lane_count();
                    for (size_t l = 0; l < lanes; ++l) {
                        T* __restrict lane_ptr = p_view.get_lane(l);
                        const int64_t base_idx = static_cast<int64_t>(l * L_WIDTH);
                        for (size_t k = 0; k < L_WIDTH; ++k) {
                            lane_ptr[k] = eval_element(lane_ptr[k], base_idx + static_cast<int64_t>(k));
                        }
                    }
                    const int64_t tail_start = static_cast<int64_t>(lanes * L_WIDTH);
                    for (int64_t i = tail_start; i < sel.element_count; ++i) {
                        const T in_v = _read_view(p_view, i);
                        _write_view(p_view, i, eval_element(in_v, i));
                    }
                    return;
                }
            }

            auto process_idx = [&](int64_t idx) noexcept {
                const T in_v = src_raw ? src_raw[idx] : _read_view(p_view, idx);
                _write_view(p_view, idx, eval_element(in_v, idx));
            };

            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) process_idx(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    process_idx(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    process_idx(i);
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

static_assert(sizeof(Transform3DBatchTransformLogic<>) == 64, "Transform3DBatchTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core
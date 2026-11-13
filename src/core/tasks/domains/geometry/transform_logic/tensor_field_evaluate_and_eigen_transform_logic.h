#pragma once

/**
 * tensor_field_evaluate_and_eigen_transform_logic.h
 * G_T5: Evaluates a 2D Symmetric Tensor Field (`SymmetricTensor2DPod`) and its orthogonal
 * Major/Minor Eigenvector Fields (`glm::vec2`) across a contiguous grid from a flat buffer of
 * `TensorBasisElementPod` generators, bound `PolylineSlicePOD` contours, and `SDFGridHeaderPOD`
 * heightmap/distance gradients.
 *
 * Target View / Strategy: `SingleElementView` | `AOSOAView` / `FlatStrategy` | `Spatial2DStrategy`
 * UI Category: `"Geometry/TensorField"`
 */

#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../geometry/tensor_field_2d.h"
#include "../../../../memory/memory_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <typename T = float>
struct alignas(64) TensorFieldEvaluateAndEigenTransformLogic {
    using ValueType = T;

    // --- Compile-Time DOD Traits ---
    static constexpr std::string_view display_name        = "Tensor Field: Evaluate Basis & Eigenvectors (2D)";
    static constexpr std::string_view ui_category         = "Geometry/TensorField";
    static constexpr ViewCapability required_capabilities = ViewCapability::READ_WRITE;
    static constexpr ViewLayout compatible_layouts        = ViewLayout::SINGLE_ELEMENT | ViewLayout::AOSOA;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32 | DataType::BYTE;
    static constexpr size_t required_dimensions           = 0;

    // --- 64-Byte Cache-Line Layout (16 x 4B = 64 Bytes) ---
    uint32_t target_buffer_id                = INVALID_ID; // 4B [0..3]   (GEOM_TENSOR_CELL_POD)
    uint32_t tensor_grid_header_buffer_id    = INVALID_ID; // 4B [4..7]   (GEOM_TENSOR_GRID_HEADER_POD)
    uint32_t tensor_basis_buffer_id          = INVALID_ID; // 4B [8..11]  (GEOM_TENSOR_BASIS_POD)
    uint32_t tensor_cell_buffer_id           = INVALID_ID; // 4B [12..15] (GEOM_TENSOR_CELL_POD)
    uint32_t tensor_eigen_major_buffer_id    = INVALID_ID; // 4B [16..19] (GEOM_TENSOR_EIGEN_MAJOR_2D)
    uint32_t tensor_eigen_minor_buffer_id    = INVALID_ID; // 4B [20..23] (GEOM_TENSOR_EIGEN_MINOR_2D)
    uint32_t polyline_slice_buffer_id        = INVALID_ID; // 4B [24..27] (GEOM_POLYLINE_SLICE_POD)
    uint32_t polyline_vertex_2d_buffer_id    = INVALID_ID; // 4B [28..31] (GEOM_POLYLINE_VERTEX_2D)
    uint32_t sdf_grid_header_buffer_id       = INVALID_ID; // 4B [32..35] (GEOM_SDF_GRID_HEADER_POD)
    uint32_t sdf_grid_float32_buffer_id      = INVALID_ID; // 4B [36..39] (GEOM_SDF_GRID_FLOAT32)
    uint32_t active_grid_slot                = 0u;         // 4B [40..43]
    float global_anisotropy_scale            = 1.0f;       // 4B [44..47]
    uint32_t normalize_tensors               = 0u;         // 4B [48..51]
    uint32_t reserved_pad_0                  = 0u;         // 4B [52..55]
    uint32_t reserved_pad_1                  = 0u;         // 4B [56..59]
    uint32_t reserved_pad_2                  = 0u;         // 4B [60..63]

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : tensor_cell_buffer_id;
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
        add_prop("tensor_basis_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_cell_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_eigen_major_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_eigen_minor_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("polyline_slice_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("polyline_vertex_2d_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("sdf_grid_header_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("sdf_grid_float32_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("active_grid_slot", godot::Variant::INT, int64_t(0));
        add_prop("global_anisotropy_scale", godot::Variant::FLOAT, 1.0);
        add_prop("normalize_tensors", godot::Variant::INT, int64_t(0));
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
        read_u32("tensor_basis_buffer_id", tensor_basis_buffer_id);
        read_u32("tensor_cell_buffer_id", tensor_cell_buffer_id);
        read_u32("tensor_eigen_major_buffer_id", tensor_eigen_major_buffer_id);
        read_u32("tensor_eigen_minor_buffer_id", tensor_eigen_minor_buffer_id);
        read_u32("polyline_slice_buffer_id", polyline_slice_buffer_id);
        read_u32("polyline_vertex_2d_buffer_id", polyline_vertex_2d_buffer_id);
        read_u32("sdf_grid_header_buffer_id", sdf_grid_header_buffer_id);
        read_u32("sdf_grid_float32_buffer_id", sdf_grid_float32_buffer_id);
        read_u32("active_grid_slot", active_grid_slot);
        read_u32("normalize_tensors", normalize_tensors);
        if (p_props.has("global_anisotropy_scale")) {
            global_anisotropy_scale = static_cast<float>(static_cast<double>(p_props["global_anisotropy_scale"]));
        }
        if (target_buffer_id == INVALID_ID) {
            target_buffer_id = tensor_cell_buffer_id;
        }
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        MemoryManagerDOD *mgr = p_context.manager;
        if (!mgr) return;

        auto get_span = [&]<typename Elem>(uint32_t p_buf_id) noexcept -> std::span<Elem> {
            if (p_buf_id == INVALID_ID) return {};
            MemoryBufferPOD *buf = mgr->get_buffer(p_buf_id);
            if (!buf || !buf->master_block_ptr || buf->max_elements <= 0) return {};
            const size_t cnt = (buf->current_count > 0)
                ? static_cast<size_t>(std::min(buf->current_count, buf->max_elements))
                : static_cast<size_t>(buf->max_elements);
            return std::span<Elem>(reinterpret_cast<Elem *>(buf->master_block_ptr + buf->memory_offset), cnt);
        };

        auto headers     = get_span.template operator()<const TensorGridHeaderPod>(tensor_grid_header_buffer_id);
        auto bases       = get_span.template operator()<const TensorBasisElementPod>(tensor_basis_buffer_id);
        auto cells       = get_span.template operator()<SymmetricTensor2DPod>(get_target_buffer_id());
        auto major_eigen = get_span.template operator()<glm::vec2>(tensor_eigen_major_buffer_id);
        auto minor_eigen = get_span.template operator()<glm::vec2>(tensor_eigen_minor_buffer_id);
        auto polylines   = get_span.template operator()<const PolylineSlicePOD>(polyline_slice_buffer_id);
        auto poly_verts  = get_span.template operator()<const glm::vec2>(polyline_vertex_2d_buffer_id);
        auto sdf_headers = get_span.template operator()<const SDFGridHeaderPOD>(sdf_grid_header_buffer_id);
        auto sdf_cells   = get_span.template operator()<const float>(sdf_grid_float32_buffer_id);

        if (headers.empty() || cells.empty()) return;

        const size_t start_grid = (active_grid_slot < headers.size()) ? active_grid_slot : 0u;
        const size_t end_grid   = (active_grid_slot < headers.size()) ? (start_grid + 1u) : headers.size();

        for (size_t g = start_grid; g < end_grid; ++g) {
            const TensorGridHeaderPod &hdr = headers[g];
            const SDFGridHeaderPOD *sdf_hdr = (hdr.sdf_grid_slot < sdf_headers.size())
                ? &sdf_headers[hdr.sdf_grid_slot]
                : nullptr;

            TensorFieldMath2D::evaluate_tensor_grid(
                hdr,
                bases,
                cells,
                major_eigen,
                minor_eigen,
                polylines,
                poly_verts,
                sdf_hdr,
                sdf_cells
            );

            if (normalize_tensors != 0u || std::abs(global_anisotropy_scale - 1.0f) > 1e-5f) {
                const size_t total = static_cast<size_t>(hdr.width) * static_cast<size_t>(hdr.height);
                for (size_t i = 0; i < total && (hdr.cell_offset + i) < cells.size(); ++i) {
                    SymmetricTensor2DPod &t = cells[hdr.cell_offset + i];
                    if (normalize_tensors != 0u) {
                        t = t.normalized();
                    }
                    t.a *= global_anisotropy_scale;
                    t.b *= global_anisotropy_scale;
                }
            }
        }
    }
};

static_assert(sizeof(TensorFieldEvaluateAndEigenTransformLogic<float>) == 64, "TensorFieldEvaluateAndEigenTransformLogic must be 64 bytes!");

} // namespace ideam::core

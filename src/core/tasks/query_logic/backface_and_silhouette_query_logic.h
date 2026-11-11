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
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * M41: BackfaceAndSilhouetteQueryLogic
 * Evaluates view direction V = normalize(camera_origin - P) against vertex normals N to
 * wake/sleep front-facing vertices, back-facing vertices, or silhouette rim contour vertices.
 */
struct BackfaceAndSilhouetteQueryLogic {
    static constexpr std::string_view ui_name = "Backface & Silhouette Rim Query";
    static constexpr std::string_view ui_category = "Model/Query";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 1> supported_types = {
        DataType::VECTOR3
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    int32_t filter_mode = 2; // 0 = Front-Facing (N.V > 0), 1 = Back-Facing (N.V < 0), 2 = Silhouette Rim (|N.V| <= band)
    godot::Vector3 camera_origin = godot::Vector3(0.0f, 2.0f, 8.0f);
    float silhouette_band = 0.18f;
    bool orthographic_view = false;
    godot::Vector3 ortho_view_dir = godot::Vector3(0.0f, 0.0f, 1.0f);

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("filter_mode", godot::Variant::INT, filter_mode, 2, "Front-Facing,Back-Facing,Silhouette Rim");
        add_prop("camera_origin", godot::Variant::VECTOR3, camera_origin);
        add_prop("silhouette_band", godot::Variant::FLOAT, silhouette_band);
        add_prop("orthographic_view", godot::Variant::BOOL, orthographic_view);
        add_prop("ortho_view_dir", godot::Variant::VECTOR3, ortho_view_dir);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("filter_mode")) filter_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["filter_mode"]), 0, 2);
        if (p_props.has("camera_origin")) camera_origin = p_props["camera_origin"];
        if (p_props.has("silhouette_band")) silhouette_band = std::clamp(static_cast<float>(p_props["silhouette_band"]), 0.0f, 1.0f);
        if (p_props.has("orthographic_view")) orthographic_view = static_cast<bool>(p_props["orthographic_view"]);
        if (p_props.has("ortho_view_dir")) {
            godot::Vector3 d = p_props["ortho_view_dir"];
            ortho_view_dir = (d.length_squared() > 1e-8f) ? d.normalized() : godot::Vector3(0.0f, 0.0f, 1.0f);
        }
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline void apply_selection_op(MemoryBufferSelectionPOD& r_sel, int64_t idx, QueryOp p_op) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
            const uint64_t word = static_cast<uint64_t>(idx) >> 6;
            const uint64_t bit = 1ULL << (idx & 63);
            const bool was_set = (r_sel.data.bitset[word] & bit) != 0;
            if (p_op == QueryOp::WAKE && !was_set) {
                r_sel.data.bitset[word] |= bit;
                r_sel.element_count++;
            } else if (p_op == QueryOp::SLEEP && was_set) {
                r_sel.data.bitset[word] &= ~bit;
                r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
            } else if (p_op == QueryOp::INVERT) {
                r_sel.data.bitset[word] ^= bit;
                r_sel.element_count += was_set ? -1 : 1;
            }
        } else if (r_sel.mode == SelectionMode::SPARSE && r_sel.data.indices) {
            if (p_op == QueryOp::WAKE && r_sel.element_count < r_sel.capacity) {
                r_sel.data.indices[r_sel.element_count++] = idx;
            }
        }
    }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& nrm_part = p_ctx.grant->parts[1];
        if (!vtx_part.raw_base_ptr || !nrm_part.raw_base_ptr) return;

        MemoryBufferSelectionPOD& sel = vtx_part.selection;
        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part.element_stride);
        const uint32_t n_stride = std::max<uint32_t>(sizeof(godot::Vector3), nrm_part.element_stride);
        const int64_t n_verts = std::min<int64_t>(
            (sel.element_count > 0) ? sel.element_count : static_cast<int64_t>(vtx_part.capacity_bytes / v_stride),
            static_cast<int64_t>(nrm_part.capacity_bytes / n_stride)
        );

        const godot::Vector3 ortho_dir = (ortho_view_dir.length_squared() > 1e-8f)
            ? ortho_view_dir.normalized()
            : godot::Vector3(0.0f, 0.0f, 1.0f);

        for (int64_t i = 0; i < n_verts; ++i) {
            const godot::Vector3& pos = *reinterpret_cast<const godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i) * v_stride);
            const godot::Vector3& nrm = *reinterpret_cast<const godot::Vector3*>(nrm_part.raw_base_ptr + static_cast<size_t>(i) * n_stride);

            const godot::Vector3 view_vec = orthographic_view
                ? ortho_dir
                : (camera_origin - pos).normalized();
            const float ndotv = nrm.dot(view_vec);

            bool match = false;
            if (filter_mode == 0) {
                match = (ndotv > silhouette_band);
            } else if (filter_mode == 1) {
                match = (ndotv < -silhouette_band);
            } else {
                match = (std::abs(ndotv) <= silhouette_band);
            }

            if (match) {
                apply_selection_op(sel, i, p_op);
            }
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, QueryOp p_op, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, p_op);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }
};

} // namespace ideam::core
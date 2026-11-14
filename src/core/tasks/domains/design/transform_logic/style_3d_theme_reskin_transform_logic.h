#pragma once

/**
 * style_3d_theme_reskin_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`DSG2`) for `IdeamDesign`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.1 — `DSG2`)
 * ============================================================================
 * Maps hierarchical `SemanticRoleID` / `SememeID` bitpaths (`Surface.Support.Seat.Chair`)
 * to `Style3DRulePod` mesh AABBs and `DesignScaleMode` (`UNIFORM_FIT`, `STRETCH_MIDDLE`,
 * `REPEAT_TILES`, `SPLINE_LOFT_DEFORM`) via the CSS cascade (`!important > layer > priority > specificity`).
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../design/structure_layout_solver.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) Style3DThemeReskinTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Design 3D CSS Theme Cascade Reskin";
    static constexpr std::string_view ui_category  = "Design";

    uint32_t target_buffer_id     = INVALID_ID; ///< `STRUCTURE_NODE_POD` (`StructureNodePod`)
    uint32_t style_rule_buffer_id = INVALID_ID; ///< `STYLE_3D_RULE_POD` (`Style3DRulePod`)
    uint32_t enforce_cascade_sort = 1u;
    float    global_scale_factor  = 1.0f;
    uint8_t  reserved_padding[48] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("style_rule_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("enforce_cascade_sort", godot::Variant::INT,   static_cast<int64_t>(1));
        add_prop("global_scale_factor",  godot::Variant::FLOAT, 1.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))     target_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("style_rule_buffer_id")) style_rule_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["style_rule_buffer_id"]));
        if (p_props.has("enforce_cascade_sort")) enforce_cascade_sort = static_cast<uint32_t>(static_cast<int64_t>(p_props["enforce_cascade_sort"]));
        if (p_props.has("global_scale_factor"))  global_scale_factor  = static_cast<float>(static_cast<double>(p_props["global_scale_factor"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_Elem>
    [[nodiscard]] static T_Elem *_resolve_ptr(const TaskContextPOD &p_ctx, uint32_t p_buf_id, size_t &r_cap) noexcept {
        r_cap = 0u;
        if (p_buf_id == INVALID_ID) return nullptr;
        if (const GrantPartPOD *part = p_ctx.get_grant_part(p_buf_id)) {
            if (part->raw_base_ptr) {
                const size_t cap = part->capacity_bytes / sizeof(T_Elem);
                r_cap = (part->selection.element_count > 0)
                    ? std::min<size_t>(static_cast<size_t>(part->selection.element_count), cap)
                    : cap;
                return reinterpret_cast<T_Elem *>(part->raw_base_ptr);
            }
        }
        if (p_ctx.manager) {
            if (MemoryBufferPOD *buf = p_ctx.manager->get_buffer(p_buf_id)) {
                if (buf->master_block_ptr) {
                    uint8_t *raw = buf->master_block_ptr + buf->memory_offset;
                    r_cap = static_cast<size_t>(std::max<int64_t>(0, buf->current_count > 0 ? buf->current_count : buf->max_elements));
                    return reinterpret_cast<T_Elem *>(raw);
                }
            }
        }
        return nullptr;
    }

    static void reskin_in_place(
        StructureNodePod *r_nodes,
        uint32_t p_n_nodes,
        Style3DRulePod *r_rules,
        uint32_t p_n_rules,
        bool p_sort_cascade,
        float p_scale_factor
    ) noexcept {
        if (!r_nodes || p_n_nodes == 0u || !r_rules || p_n_rules == 0u) return;

        StructureLayoutSolver solver;
        for (uint32_t i = 0; i < p_n_nodes; ++i) {
            solver.add_structure_node(r_nodes[i]);
        }
        for (uint32_t i = 0; i < p_n_rules; ++i) {
            solver.add_style_rule(r_rules[i]);
        }

        if (p_sort_cascade) {
            StructureLayoutSolver::sort_style_rules_by_cascade(solver.style_rules_mut());
        }
        solver.apply_theme_cascade_to_nodes();

        const auto &solved_nodes = solver.nodes();
        for (uint32_t i = 0; i < p_n_nodes && i < solved_nodes.size(); ++i) {
            r_nodes[i] = solved_nodes[i];
            if (p_scale_factor != 1.0f && p_scale_factor > 0.0f) {
                r_nodes[i].rest_obb_extents = r_nodes[i].rest_obb_extents * p_scale_factor;
            }
        }
        const auto &sorted_rules = solver.style_rules();
        for (uint32_t i = 0; i < p_n_rules && i < sorted_rules.size(); ++i) {
            r_rules[i] = sorted_rules[i];
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t node_cap = 0, rule_cap = 0;
        auto *nodes = _resolve_ptr<StructureNodePod>(p_ctx, target_buffer_id, node_cap);
        auto *rules = _resolve_ptr<Style3DRulePod>(p_ctx, style_rule_buffer_id, rule_cap);

        reskin_in_place(
            nodes, static_cast<uint32_t>(node_cap),
            rules, static_cast<uint32_t>(rule_cap),
            enforce_cascade_sort != 0u,
            global_scale_factor
        );
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(Style3DThemeReskinTransformLogic<>) == 64, "Style3DThemeReskinTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<Style3DThemeReskinTransformLogic<>>, "Style3DThemeReskinTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core

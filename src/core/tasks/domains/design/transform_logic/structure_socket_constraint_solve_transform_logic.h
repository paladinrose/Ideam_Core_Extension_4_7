#pragma once

/**
 * structure_socket_constraint_solve_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`DSG1`) for `IdeamDesign`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.1 — `DSG1`)
 * ============================================================================
 * Batch DOD execution of `StructureLayoutSolver::solve_layout_constraints()` and
 * `solve_socket_contacts()` over `StructureNodePod` and `SocketPod` buffers.
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
struct alignas(64) StructureSocketConstraintSolveTransformLogic {
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

    static constexpr std::string_view display_name = "Design 3D Box-Model & Socket Constraint Solve";
    static constexpr std::string_view ui_category  = "Design";

    uint32_t target_buffer_id      = INVALID_ID; ///< `STRUCTURE_NODE_POD` (`StructureNodePod`)
    uint32_t socket_buffer_id      = INVALID_ID; ///< `SOCKET_POD` (`SocketPod`)
    uint32_t status_mask_buffer_id = INVALID_ID; ///< `NODE_STATUS_MASK` (`uint32_t`)
    uint32_t solve_sockets_first   = 1u;
    float    relaxation_damping    = 1.0f;
    uint8_t  reserved_padding[44]  = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("socket_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("status_mask_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("solve_sockets_first",   godot::Variant::INT,   static_cast<int64_t>(1));
        add_prop("relaxation_damping",    godot::Variant::FLOAT, 1.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))      target_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("socket_buffer_id"))      socket_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["socket_buffer_id"]));
        if (p_props.has("status_mask_buffer_id")) status_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["status_mask_buffer_id"]));
        if (p_props.has("solve_sockets_first"))   solve_sockets_first   = static_cast<uint32_t>(static_cast<int64_t>(p_props["solve_sockets_first"]));
        if (p_props.has("relaxation_damping"))    relaxation_damping    = static_cast<float>(static_cast<double>(p_props["relaxation_damping"]));
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

    static void solve_in_place(
        StructureNodePod *r_nodes,
        uint32_t p_n_nodes,
        SocketPod *r_sockets,
        uint32_t p_n_sockets,
        uint32_t *r_status_masks,
        uint32_t p_n_status_masks,
        bool p_solve_sockets
    ) noexcept {
        if (!r_nodes || p_n_nodes == 0u) return;

        StructureLayoutSolver solver;
        for (uint32_t i = 0; i < p_n_nodes; ++i) {
            solver.add_structure_node(r_nodes[i]);
        }
        if (r_sockets) {
            for (uint32_t i = 0; i < p_n_sockets; ++i) {
                solver.add_socket(r_sockets[i]);
            }
        }

        solver.solve_layout_constraints();
        if (p_solve_sockets) {
            solver.solve_socket_contacts();
        }

        const auto &solved_nodes = solver.nodes();
        for (uint32_t i = 0; i < p_n_nodes && i < solved_nodes.size(); ++i) {
            r_nodes[i] = solved_nodes[i];
            if (r_status_masks && i < p_n_status_masks) {
                r_status_masks[i] = (r_nodes[i].semantic_tag_id >> 24) & 0xFFu;
            }
        }
        if (r_sockets) {
            const auto &solved_sockets = solver.sockets();
            for (uint32_t i = 0; i < p_n_sockets && i < solved_sockets.size(); ++i) {
                r_sockets[i] = solved_sockets[i];
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t node_cap = 0, sock_cap = 0, mask_cap = 0;
        auto *nodes        = _resolve_ptr<StructureNodePod>(p_ctx, target_buffer_id, node_cap);
        auto *sockets      = _resolve_ptr<SocketPod>(p_ctx, socket_buffer_id, sock_cap);
        auto *status_masks = _resolve_ptr<uint32_t>(p_ctx, status_mask_buffer_id, mask_cap);

        solve_in_place(
            nodes, static_cast<uint32_t>(node_cap),
            sockets, static_cast<uint32_t>(sock_cap),
            status_masks, static_cast<uint32_t>(mask_cap),
            solve_sockets_first != 0u
        );
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(StructureSocketConstraintSolveTransformLogic<>) == 64, "StructureSocketConstraintSolveTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<StructureSocketConstraintSolveTransformLogic<>>, "StructureSocketConstraintSolveTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core

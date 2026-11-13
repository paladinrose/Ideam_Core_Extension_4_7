#pragma once

/**
 * reflection_selector_match_query_logic.h
 * 64-byte cache-aligned QueryLogic task struct for `IdeamProceduralGeneration`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Bridges `AnalysisRule`, `GenerationRule`, `ProceduralPattern`, and `ProceduralPatterner`
 *   into the shared `ProceduralEnvironment` DOD tables.
 * - Evaluates Godot reflection signatures (`ClassDB` inheritance, node names, suffixes like
 *   `"_cell"`, properties, methods, signals) and compiles cascading CSS-like `GenerationRule`s
 *   into flat bitmask domains and parent/child/sibling cardinality constraints.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ReflectionSelectorMatchQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::UINT32 | DataType::FLOAT32;

    static constexpr bool supports_cull            = true;
    static constexpr bool supports_addition        = true;
    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Reflection Selector Match (CSS Rule Filter)";
    static constexpr std::string_view ui_category  = "ProceduralGeneration";

    uint32_t target_buffer_id    = INVALID_ID;
    uint32_t rule_buffer_id      = INVALID_ID;
    float    threshold           = 0.75f;
    uint8_t  reserved_padding[52]= {0};

    static godot::Array get_ui_properties() { return godot::Array(); }
    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }
    void apply_properties(const godot::Dictionary &p_props) noexcept { (void)p_props; }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view; (void)p_ctx;
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_ctx; (void)p_view;
        // Architectural stub: Full DOD query evaluation over ProceduralEnvironment buffers.
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_query(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<Op, T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(ReflectionSelectorMatchQueryLogic<>) == 64, "ReflectionSelectorMatchQueryLogic must be 64 bytes!");
static_assert(IsQueryLogic<ReflectionSelectorMatchQueryLogic<>>, "ReflectionSelectorMatchQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core

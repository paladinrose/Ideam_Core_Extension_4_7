#pragma once

/**
 * pattern_domain_initialize_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct for `IdeamProceduralGeneration`.
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
#include "../../../transform_logic/transform_logic_traits.h"
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
struct alignas(64) PatternDomainInitializeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::UINT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "ProceduralPattern Domain & Hierarchy Initialize";
    static constexpr std::string_view ui_category  = "ProceduralGeneration";

    uint32_t target_buffer_id    = INVALID_ID;
    uint32_t rule_buffer_id      = INVALID_ID;
    uint32_t pointer_buffer_id   = INVALID_ID;
    uint8_t  reserved_padding[52]= {0};

    static godot::Array get_ui_properties() { return godot::Array(); }
    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }
    void apply_properties(const godot::Dictionary &p_props) noexcept { (void)p_props; }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view; (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_ctx; (void)p_view;
        // Architectural stub: Full DOD transform execution over ProceduralEnvironment buffers.
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(PatternDomainInitializeTransformLogic<>) == 64, "PatternDomainInitializeTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<PatternDomainInitializeTransformLogic<>>, "PatternDomainInitializeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core

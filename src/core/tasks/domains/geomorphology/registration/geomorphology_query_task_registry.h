#pragma once

#include "../../../i_native_task.h"
#include "../../../../memory/memory_common.h"
#include "../../../registration/ideam_task_registry.h"
#include "../../../query_task.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <array>
#include <cstdint>

namespace ideam::core {

enum class GeomorphologyQueryLogicID : uint32_t {
    GeomorphicHazardAndResourceVein = 0,
    Count
};

using GeomorphologyQueryTaskFactoryFn = INativeTask *(*)();

class GeomorphologyQueryTaskRegistry {
public:
    static constexpr size_t O_COUNT = 2; // 0 = CULL, 1 = ADD
    static constexpr size_t L_COUNT = static_cast<size_t>(GeomorphologyQueryLogicID::Count);
    static constexpr size_t STORAGE_COUNT = (L_COUNT > 0) ? L_COUNT : 1;
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);

    static const std::array<LogicVariantGroup, STORAGE_COUNT> logic_variants;
    static constexpr size_t SUB_MATRIX_SIZE = O_COUNT * V_COUNT * S_COUNT * T_COUNT;
    using SubMatrix = std::array<GeomorphologyQueryTaskFactoryFn, SUB_MATRIX_SIZE>;
    static std::array<const SubMatrix *, STORAGE_COUNT> logic_matrices;

    static void init_execution_routing();
    static void cleanup_execution_routing();
    static void generate_ui_matrices(godot::Dictionary &p_matrix);

    static std::unique_ptr<INativeTask> create(
        uint32_t p_op_id,
        uint32_t p_logic_id,
        uint32_t p_view_id,
        uint32_t p_strategy_id,
        uint32_t p_type_id
    );
};

} // namespace ideam::core

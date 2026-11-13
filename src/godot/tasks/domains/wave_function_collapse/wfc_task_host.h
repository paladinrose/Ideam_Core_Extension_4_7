#pragma once

/**
 * wfc_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedWaveFunctionCollapse` (`src/godot/tasks/domains/wave_function_collapse/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Operates on the shared `ProceduralEnvironment` populated by `IdeamProceduralGeneration`.
 * - Runs minimum-entropy slot collapse and propagates both spatial port adjacency and
 *   cascading `GenerationRule` hierarchy constraints (`CastleGrounds -> CastleTower`,
 *   child cardinality `[min..max]`, and sibling dependencies like `TowerLanding requires TowerStairs`).
 * - Flags any contradiction slots in `POINTER_OR_CONFLICT_MASK` so `IdeamQuantumDarwinism`
 *   TaskGraph nodes can stabilize the local environment horizon box without full-grid backtracking.
 */

#include "data_oriented_wfc.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class WaveFunctionCollapseTaskHost : public TaskGraphHost {
    GDCLASS(WaveFunctionCollapseTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedWaveFunctionCollapse> domain_resource_;

protected:
    static void _bind_methods();

public:
    WaveFunctionCollapseTaskHost() = default;
    ~WaveFunctionCollapseTaskHost() override = default;

    void set_domain_resource(const godot::Ref<DataOrientedWaveFunctionCollapse> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedWaveFunctionCollapse> get_domain_resource() const { return domain_resource_; }
};

} // namespace ideam::godot_ext

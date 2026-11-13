#pragma once

/**
 * data_oriented_wfc.h
 * Godot `Resource` bridge for `WaveFunctionCollapseDOD` (`src/godot/tasks/domains/wave_function_collapse/`).
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

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../../../core/tasks/domains/wave_function_collapse/wfc_dod.h"
#include <memory>

namespace ideam::godot_ext {

class DataOrientedWaveFunctionCollapse : public godot::Resource {
    GDCLASS(DataOrientedWaveFunctionCollapse, godot::Resource)

private:
    std::unique_ptr<core::WaveFunctionCollapseDOD> dod_engine_;

protected:
    static void _bind_methods();

public:
    DataOrientedWaveFunctionCollapse();
    ~DataOrientedWaveFunctionCollapse() override = default;

    [[nodiscard]] core::WaveFunctionCollapseDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }
};

} // namespace ideam::godot_ext

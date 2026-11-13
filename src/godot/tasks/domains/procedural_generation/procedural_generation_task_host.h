#pragma once

/**
 * procedural_generation_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedProceduralGeneration` (`src/godot/tasks/domains/procedural_generation/`).
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

#include "data_oriented_procedural_generation.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class ProceduralGenerationTaskHost : public TaskGraphHost {
    GDCLASS(ProceduralGenerationTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedProceduralGeneration> domain_resource_;

protected:
    static void _bind_methods();

public:
    ProceduralGenerationTaskHost() = default;
    ~ProceduralGenerationTaskHost() override = default;

    void set_domain_resource(const godot::Ref<DataOrientedProceduralGeneration> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedProceduralGeneration> get_domain_resource() const { return domain_resource_; }
};

} // namespace ideam::godot_ext

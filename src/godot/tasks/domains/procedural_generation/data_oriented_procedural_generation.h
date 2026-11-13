#pragma once

/**
 * data_oriented_procedural_generation.h
 * Godot `Resource` bridge for `ProceduralGenerationDOD` (`src/godot/tasks/domains/procedural_generation/`).
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

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../../../core/tasks/domains/procedural_generation/procedural_generation_dod.h"
#include <memory>

namespace ideam::godot_ext {

class DataOrientedProceduralGeneration : public godot::Resource {
    GDCLASS(DataOrientedProceduralGeneration, godot::Resource)

private:
    std::unique_ptr<core::ProceduralGenerationDOD> dod_engine_;

protected:
    static void _bind_methods();

public:
    DataOrientedProceduralGeneration();
    ~DataOrientedProceduralGeneration() override = default;

    [[nodiscard]] core::ProceduralGenerationDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }
};

} // namespace ideam::godot_ext

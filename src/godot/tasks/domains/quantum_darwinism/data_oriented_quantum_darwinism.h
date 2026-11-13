#pragma once

/**
 * data_oriented_quantum_darwinism.h
 * Godot `Resource` bridge for `QuantumDarwinismDOD` (`src/godot/tasks/domains/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Implements Quantum Darwinism conflict stabilization on `TaskGraphDOD` over the shared
 *   `ProceduralEnvironment`:
 *     1. `ConflictHorizonBoxPartitionMetadataLogic`: Draws the bounded environment box (E_box)
 *        around a WFC contradiction or cardinality conflict slot.
 *     2. `PointerStateEinselectionQueryLogic`: Evaluates `GenerationRule` scores and neighbor
 *        mutual-information redundancy (R_delta) to select settled Pointer States.
 *     3. `CoherenceCascadeRelaxAndPropagateTransformLogic`: Evaluates which lower-precedence
 *        CSS cascade properties/siblings on intermediate cells between each Pointer State and
 *        the problem cell can be altered without losing coherence, then propagates pointer
 *        properties inward to resolve the conflict.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../../../core/tasks/domains/quantum_darwinism/quantum_darwinism_dod.h"
#include <memory>

namespace ideam::godot_ext {

class DataOrientedQuantumDarwinism : public godot::Resource {
    GDCLASS(DataOrientedQuantumDarwinism, godot::Resource)

private:
    std::unique_ptr<core::QuantumDarwinismDOD> dod_engine_;

protected:
    static void _bind_methods();

public:
    DataOrientedQuantumDarwinism();
    ~DataOrientedQuantumDarwinism() override = default;

    [[nodiscard]] core::QuantumDarwinismDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }
};

} // namespace ideam::godot_ext

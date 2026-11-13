#pragma once

/**
 * quantum_darwinism_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedQuantumDarwinism` (`src/godot/tasks/domains/quantum_darwinism/`).
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

#include "data_oriented_quantum_darwinism.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class QuantumDarwinismTaskHost : public TaskGraphHost {
    GDCLASS(QuantumDarwinismTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedQuantumDarwinism> domain_resource_;

protected:
    static void _bind_methods();

public:
    QuantumDarwinismTaskHost() = default;
    ~QuantumDarwinismTaskHost() override = default;

    void set_domain_resource(const godot::Ref<DataOrientedQuantumDarwinism> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedQuantumDarwinism> get_domain_resource() const { return domain_resource_; }
};

} // namespace ideam::godot_ext

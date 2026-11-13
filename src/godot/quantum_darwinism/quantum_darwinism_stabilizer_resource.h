#pragma once

/**
 * quantum_darwinism_stabilizer_resource.h
 * `QuantumDarwinismStabilizerResource` and `QuantumDarwinismStabilizerNode`
 * (`src/godot/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Leans on `ProceduralGeneratorResource` and `ProceduralGenerationHost` from
 * `src/godot/procedural_generation/`, operating on the same `ProceduralPattern`
 * and `ProceduralEnvironment` as `IdeamWaveFunctionCollapse`.
 */

#include "../procedural_generation/procedural_generator_resource.h"
#include "../procedural_generation/procedural_pattern.h"
#include "../../core/quantum_darwinism/quantum_darwinism_solver.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class QuantumDarwinismStabilizerResource : public ProceduralGeneratorResource {
    GDCLASS(QuantumDarwinismStabilizerResource, ProceduralGeneratorResource)

private:
    int max_horizon_radius_          = 4;
    float pointer_redundancy_thresh_ = 0.75f;
    float fragile_redundancy_thresh_ = 0.30f;
    godot::Ref<ProceduralPattern> pattern_;

protected:
    static void _bind_methods();

public:
    QuantumDarwinismStabilizerResource() = default;
    ~QuantumDarwinismStabilizerResource() override = default;

    void set_max_horizon_radius(int p_radius);
    [[nodiscard]] int get_max_horizon_radius() const noexcept { return max_horizon_radius_; }

    void set_pointer_redundancy_threshold(float p_thresh);
    [[nodiscard]] float get_pointer_redundancy_threshold() const noexcept { return pointer_redundancy_thresh_; }

    void set_fragile_redundancy_threshold(float p_thresh);
    [[nodiscard]] float get_fragile_redundancy_threshold() const noexcept { return fragile_redundancy_thresh_; }

    void set_pattern(const godot::Ref<ProceduralPattern> &p_pattern) { pattern_ = p_pattern; emit_changed(); }
    [[nodiscard]] godot::Ref<ProceduralPattern> get_pattern() const { return pattern_; }
};

class QuantumDarwinismStabilizerNode : public ProceduralGenerationHost {
    GDCLASS(QuantumDarwinismStabilizerNode, ProceduralGenerationHost)

private:
    core::QuantumDarwinismSolver solver_;

protected:
    static void _bind_methods();

public:
    QuantumDarwinismStabilizerNode() = default;
    ~QuantumDarwinismStabilizerNode() override = default;
};

} // namespace ideam::godot_ext

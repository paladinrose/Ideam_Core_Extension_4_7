#pragma once

/**
 * procedural_generator_resource.h
 * Abstract base `Resource` (`ProceduralGeneratorResource`) and SceneTree host (`ProceduralGenerationHost`)
 * for `IdeamProceduralGeneration` (`src/godot/procedural_generation/`).
 *
 * Just as `GeometryResource` in `IdeamGeometry` is the shared base resource for `SplineResource`
 * and `TilingResource`, `ProceduralGeneratorResource` provides the shared seed, Undo/Redo snapshot
 * protocol, and constraint-validation contract that `WFCRuleTensorResource` in
 * `IdeamWaveFunctionCollapse` inherits from.
 */

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include "../utilities/ideam_undo_redo.h"
#include "../../core/procedural_generation/procgen_common.h"

namespace ideam::godot_ext {

class ProceduralGeneratorResource : public godot::Resource {
    GDCLASS(ProceduralGeneratorResource, godot::Resource)

private:
    int64_t master_seed_           = 1337;
    bool deterministic_mode_       = true;
    godot::Ref<IdeamUndoRedo> undo_redo_;

protected:
    static void _bind_methods();

public:
    ProceduralGeneratorResource() = default;
    ~ProceduralGeneratorResource() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_master_seed(int64_t p_seed);
    [[nodiscard]] int64_t get_master_seed() const noexcept { return master_seed_; }

    void set_deterministic_mode(bool p_deterministic);
    [[nodiscard]] bool get_deterministic_mode() const noexcept { return deterministic_mode_; }

    [[nodiscard]] virtual godot::Dictionary get_snapshot_state() const;
    virtual void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
};

class ProceduralGenerationHost : public godot::Node {
    GDCLASS(ProceduralGenerationHost, godot::Node)

private:
    godot::Ref<ProceduralGeneratorResource> generator_resource_;
    bool auto_generate_on_ready_ = false;

protected:
    static void _bind_methods();

public:
    ProceduralGenerationHost() = default;
    ~ProceduralGenerationHost() override = default;

    void set_generator_resource(const godot::Ref<ProceduralGeneratorResource> &p_res);
    [[nodiscard]] godot::Ref<ProceduralGeneratorResource> get_generator_resource() const { return generator_resource_; }

    void set_auto_generate_on_ready(bool p_auto);
    [[nodiscard]] bool get_auto_generate_on_ready() const { return auto_generate_on_ready_; }
};

} // namespace ideam::godot_ext

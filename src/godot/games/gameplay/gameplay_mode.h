#pragma once

#include "gameplay_resource.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameplayPerspective;
class GameplayDifficulty;
class GameplayControlMap;
class GameplayAccessibilityOption;
class GameplayAudioOption;
class GameplayGraphicsOption;
class GameplayRule;

/**
 * DOD NOTE: GameplayMode acts as a collection of gameplay-altering components
 * representing the Mode Layer (RuleCascadeLayer::LAYER_MODE) in the GameplayStyle cascade.
 * Storing these in separate TypedArrays of Objects (AoS) creates a fragmented memory
 * profile. In a high-performance C++26 environment, these should be refactored
 * into a single contiguous Block of Memory (SoA) where components are stored as
 * raw POD data, minimizing cache misses during mode initialization and updates.
 */
class GameplayMode : public GameplayResource {
    GDCLASS(GameplayMode, GameplayResource)

protected:
    static void _bind_methods();

private:
    godot::TypedArray<GameplayPerspective> perspectives;
    godot::TypedArray<GameplayDifficulty> difficulties;
    godot::TypedArray<GameplayControlMap> control_maps;
    godot::TypedArray<GameplayAccessibilityOption> accessibility_options;
    godot::TypedArray<GameplayAudioOption> audio_options;
    godot::TypedArray<GameplayGraphicsOption> graphics_options;

public:
    GameplayMode();
    ~GameplayMode();

    // Setters / Getters
    void set_perspectives(const godot::TypedArray<GameplayPerspective> &p_perspectives);
    godot::TypedArray<GameplayPerspective> get_perspectives() const;

    void set_difficulties(const godot::TypedArray<GameplayDifficulty> &p_difficulties);
    godot::TypedArray<GameplayDifficulty> get_difficulties() const;

    void set_control_maps(const godot::TypedArray<GameplayControlMap> &p_control_maps);
    godot::TypedArray<GameplayControlMap> get_control_maps() const;

    void set_accessibility_options(const godot::TypedArray<GameplayAccessibilityOption> &p_options);
    godot::TypedArray<GameplayAccessibilityOption> get_accessibility_options() const;

    void set_audio_options(const godot::TypedArray<GameplayAudioOption> &p_options);
    godot::TypedArray<GameplayAudioOption> get_audio_options() const;

    void set_graphics_options(const godot::TypedArray<GameplayGraphicsOption> &p_options);
    godot::TypedArray<GameplayGraphicsOption> get_graphics_options() const;

    // --- Cascade & Mode Functions ---

    /**
     * Merges `p_other`'s domain options into this mode by matching `matches_identity()`
     * (and appending any new option resources not yet present).
     */
    void apply_mode(const godot::Ref<GameplayMode> &p_other);

    /**
     * Gathers all active GameplayRules contributed by this GameplayMode:
     * 1. Associated rules on the GameplayMode itself (stamped with RuleCascadeLayer::LAYER_MODE if unset)
     * 2. Synthesized and associated rules from all enabled perspectives, difficulties,
     *    control_maps, accessibility_options, audio_options, and graphics_options.
     */
    virtual godot::TypedArray<GameplayRule> collect_rules() const override;

    // --- Persistence & GameplayResource Overrides ---
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

} // namespace ideam::godot_ext
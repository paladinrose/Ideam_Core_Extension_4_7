#pragma once

/**
 * procedural_pattern.h
 * `ProceduralPattern` (`src/godot/procedural_generation/`).
 */

#include "../../core/procedural_generation/procedural_environment.h"
#include "generation_rule.h"
#include "procedural_generator_resource.h"
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

class ProceduralPattern : public ProceduralGeneratorResource {
    GDCLASS(ProceduralPattern, ProceduralGeneratorResource)

private:
    godot::TypedArray<GenerationRule> generation_rules_;
    godot::PackedStringArray discovered_tokens_;

protected:
    static void _bind_methods();

public:
    ProceduralPattern() = default;
    ~ProceduralPattern() override = default;

    void set_generation_rules(const godot::TypedArray<GenerationRule> &p_rules);
    [[nodiscard]] godot::TypedArray<GenerationRule> get_generation_rules() const { return generation_rules_; }

    void set_discovered_tokens(const godot::PackedStringArray &p_tokens) { discovered_tokens_ = p_tokens; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_discovered_tokens() const { return discovered_tokens_; }

    void add_generation_rule(const godot::Ref<GenerationRule> &p_rule);
    void clear_generation_rules();
    [[nodiscard]] int get_generation_rule_count() const { return generation_rules_.size(); }

    void add_discovered_token(const godot::String &p_token);
    [[nodiscard]] int find_token_index(const godot::String &p_token) const;

    /**
     * Compiles this `ProceduralPattern` (`discovered_tokens_` + cascading `generation_rules_`)
     * into flat `TokenDescriptorPOD`, `GenerationRulePOD`, and `BitmaskDomain256POD` buffers
     * inside `r_env`.
     */
    void compile_to_environment(core::ProceduralEnvironment &r_env) const;

    /**
     * Populates `discovered_tokens_` and `generation_rules_` from a synthesized
     * `core::ProceduralEnvironment`.
     */
    void import_from_environment(const core::ProceduralEnvironment &p_env);

    // --- IdeamUndoRedo Commit Wrappers ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;
    void commit_add_generation_rule(
        const godot::Ref<GenerationRule> &p_rule,
        const godot::String &p_action_name = "Add Generation Rule to ProceduralPattern"
    );
    void commit_clear_generation_rules(
        const godot::String &p_action_name = "Clear ProceduralPattern Generation Rules"
    );
};

} // namespace ideam::godot_ext

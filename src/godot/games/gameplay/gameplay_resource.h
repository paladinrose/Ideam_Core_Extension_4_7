#pragma once

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameplayRule;
class GameEntity;

/**
 * DOD NOTE: GameplayResource serves as the foundational utility base for all
 * gameplay configuration resources (GameplayStyle, GameplayRule, GameplayMode,
 * GameplayPerspective, GameplayDifficulty, GameplayControlMap,
 * GameplayAccessibilityOption, and future Graphics/Audio/Network option resources).
 *
 * While inheriting from godot::Resource provides native Inspector authoring and
 * .tres/.res serialization (AoS), GameplayResource bridges the gap toward our
 * Data-Oriented Design (DOD) pipeline in three ways:
 * 1. O(1) Interned Identity & Tag Matching: Uses godot::StringName (`resource_id`,
 *    `category`, `option_tags`, `required_tags`, `excluded_tags`) so compatibility
 *    and intersection queries compare interned pointer/hash values instead of
 *    heap-allocated String characters.
 * 2. Declarative Rule Synthesis (`collect_rules()`): Allows any option resource to
 *    emit `GameplayRule` instances that compile into `GameplayStyle` cascades
 *    targeting the `GameEntity` and `GameProperty` hierarchy.
 * 3. Unified ConfigFile & Dictionary I/O: Replaces ad-hoc GDScript settings
 *    boilerplate (like the TPS demo's `settings.gd` and `menu.gd`) with polymorphic
 *    `save_resource()` / `load_resource()` and `ConfigFile` serialization.
 */
class GameplayResource : public godot::Resource {
    GDCLASS(GameplayResource, godot::Resource)

protected:
    static void _bind_methods();

private:
    // --- Identity & Metadata ---
    godot::String title;
    godot::StringName resource_id;
    godot::String description;
    godot::StringName category;
    bool enabled = true;

    // --- Tags & Intersection Compatibility ---
    // Semantic tags emitted by this resource when active (e.g., &"OverTheShoulder", &"HighContrast").
    godot::TypedArray<godot::StringName> option_tags;

    // All tags in `required_tags` must be present in the active context for this resource to be compatible.
    godot::TypedArray<godot::StringName> required_tags;

    // If any tag in `excluded_tags` is present in the active context, this resource is incompatible.
    godot::TypedArray<godot::StringName> excluded_tags;

    // --- Cascading Rules Integration ---
    // Explicit GameplayRules authored on this resource in the Inspector.
    godot::TypedArray<GameplayRule> associated_rules;

    // --- Persistence Metadata ---
    godot::String config_section = "gameplay";

public:
    GameplayResource();
    virtual ~GameplayResource();

    // --- Identity & Metadata Setters / Getters ---
    void set_title(const godot::String &p_title);
    godot::String get_title() const;

    void set_resource_id(const godot::StringName &p_id);
    godot::StringName get_resource_id() const;

    /**
     * Returns `resource_id` if non-empty; otherwise falls back to `StringName(title)`.
     * Guarantees O(1) interned comparisons even when only `title` was authored in the Inspector.
     */
    godot::StringName get_effective_id() const;

    void set_description(const godot::String &p_description);
    godot::String get_description() const;

    void set_category(const godot::StringName &p_category);
    godot::StringName get_category() const;

    void set_enabled(bool p_enabled);
    bool get_enabled() const;

    // --- Tags & Intersection Compatibility ---
    void set_option_tags(const godot::TypedArray<godot::StringName> &p_tags);
    godot::TypedArray<godot::StringName> get_option_tags() const;

    void set_required_tags(const godot::TypedArray<godot::StringName> &p_tags);
    godot::TypedArray<godot::StringName> get_required_tags() const;

    void set_excluded_tags(const godot::TypedArray<godot::StringName> &p_tags);
    godot::TypedArray<godot::StringName> get_excluded_tags() const;

    bool has_tag(const godot::StringName &p_tag) const;
    void add_tag(const godot::StringName &p_tag);
    void remove_tag(const godot::StringName &p_tag);

    /**
     * Evaluates whether this resource is compatible with an active set of context tags:
     * - Returns false if any tag in `required_tags` is missing from `p_active_tags`.
     * - Returns false if any tag in `excluded_tags` is present in `p_active_tags`.
     */
    bool is_compatible_with_tags(const godot::TypedArray<godot::StringName> &p_active_tags) const;

    /**
     * Evaluates mutual compatibility between this resource and `p_other` by checking
     * each resource's `option_tags` against the other's `required_tags` and `excluded_tags`.
     */
    bool is_compatible_with_resource(const godot::Ref<GameplayResource> &p_other) const;

    /**
     * Fast identity check comparing `get_effective_id()` (or `title` if both are unnamed).
     */
    bool matches_identity(const godot::Ref<GameplayResource> &p_other) const;

    // --- Cascading Rules Integration ---
    void set_associated_rules(const godot::TypedArray<GameplayRule> &p_rules);
    godot::TypedArray<GameplayRule> get_associated_rules() const;

    void add_associated_rule(const godot::Ref<GameplayRule> &p_rule);
    void remove_associated_rule(const godot::Ref<GameplayRule> &p_rule);
    void clear_associated_rules();

    /**
     * Gathers all active GameplayRules contributed by this resource.
     * Base implementation returns `associated_rules` when `enabled == true` (or empty if disabled).
     * Subclasses (such as GameplayAccessibilityOption or GameplayDifficulty) override this to
     * append dynamically synthesized GameplayRules derived from their scalar/enum properties.
     */
    virtual godot::TypedArray<GameplayRule> collect_rules() const;

    /**
     * Convenience helper that evaluates all rules from `collect_rules()` against `p_entity`.
     * Returns the number of rules that matched and were applied.
     */
    int apply_rules_to_entity(GameEntity *p_entity, bool p_is_self = true) const;

    // --- Persistence & ConfigFile Serialization ---
    void set_config_section(const godot::String &p_section);
    godot::String get_config_section() const;

    /**
     * Copies base GameplayResource properties from `p_other` into this instance.
     */
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other);

    /**
     * Polymorphic Dictionary serialization. Subclasses should call
     * `GameplayResource::save_resource()` / `GameplayResource::load_resource(p_data)`
     * and merge their domain-specific keys.
     */
    virtual godot::Dictionary save_resource() const;
    virtual void load_resource(const godot::Dictionary &p_data);

    /**
     * Reads/writes this resource's serialized keys directly to a `godot::ConfigFile`
     * under `p_section` (or `config_section` if `p_section` is empty).
     */
    bool save_to_config(const godot::Ref<godot::ConfigFile> &p_config, const godot::String &p_section = "") const;
    bool load_from_config(const godot::Ref<godot::ConfigFile> &p_config, const godot::String &p_section = "");

    godot::Error save_to_config_file(const godot::String &p_path, const godot::String &p_section = "") const;
    godot::Error load_from_config_file(const godot::String &p_path, const godot::String &p_section = "");
};

} // namespace ideam::godot_ext
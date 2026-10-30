#pragma once

#include "gameplay_resource.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

/**
 * DOD NOTE: Extracting difficulty enums at namespace scope with explicit int32_t
 * backing allows them to be stored in flat Structure of Arrays (SoA) configuration
 * columns or evaluated via bitwise/integer operations without pulling in the full
 * GameplayDifficulty class definition.
 */
enum DifficultyLevel : int32_t {
    DIFFICULTY_STORY = 0,     // Narrative / Effortless / Beginner
    DIFFICULTY_EASY = 1,      // Forgiving / Casual
    DIFFICULTY_NORMAL = 2,    // Balanced baseline (1.0x)
    DIFFICULTY_HARD = 3,      // Challenging / Veteran
    DIFFICULTY_NIGHTMARE = 4, // Extreme / Survivalist
    DIFFICULTY_CUSTOM = 5     // Fine-tuned custom multipliers or mixed scales
};

enum DifficultyScaleMode : int32_t {
    SCALE_MODE_UNIFIED = 0,      // One unified difficulty level drives all gameplay aspects
    SCALE_MODE_SPLIT_ASPECTS = 1 // Independent scales per gameplay domain (Silent Hill style Action vs. Riddle, Stealth, Survival, etc.)
};

/**
 * GameplayDifficulty manages how challenging the game is, supporting both a single
 * unified scale for the entire game (`SCALE_MODE_UNIFIED`) and independent per-aspect
 * scales (`SCALE_MODE_SPLIT_ASPECTS`) inspired by the Silent Hill series (e.g., separate
 * Action/Combat Level, Riddle/Puzzle Level, Stealth/Perception Level, and Survival/Resource
 * Level, plus arbitrary project-specific aspect scales).
 *
 * Beyond storing preset tiers and fine-grained multipliers, GameplayDifficulty integrates
 * directly with the `GameplayStyle` cascade by overriding `collect_rules()`. When enabled,
 * it synthesizes baseline `GameplayRule` instances (priority 30) targeting enemy agents,
 * weapons, player health, and puzzle/interaction entities—which can in turn be overridden
 * by higher-priority or `!important` rules from `GameplayAccessibilityOption`.
 */
class GameplayDifficulty : public GameplayResource {
    GDCLASS(GameplayDifficulty, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- 0. Mode, Preset Tiers & Tags ---
    DifficultyScaleMode scale_mode = DifficultyScaleMode::SCALE_MODE_UNIFIED;
    DifficultyLevel overall_level = DifficultyLevel::DIFFICULTY_NORMAL;

    // Independent domain scales (active independently in SCALE_MODE_SPLIT_ASPECTS, or synced in SCALE_MODE_UNIFIED)
    DifficultyLevel combat_level = DifficultyLevel::DIFFICULTY_NORMAL;   // Action / Combat scale
    DifficultyLevel puzzle_level = DifficultyLevel::DIFFICULTY_NORMAL;   // Riddle / Puzzle & Exploration scale
    DifficultyLevel stealth_level = DifficultyLevel::DIFFICULTY_NORMAL;  // AI Perception / Stealth scale
    DifficultyLevel survival_level = DifficultyLevel::DIFFICULTY_NORMAL; // Resource Scarcity / Hazard scale

    // Arbitrary named aspect scales (StringName/String -> int DifficultyLevel or float multiplier)
    godot::Dictionary custom_aspect_levels;

    // Legacy string tags array preserved for compatibility alongside GameplayResource::option_tags
    godot::TypedArray<godot::String> difficulty_tags;

    bool allow_mid_game_change = true;
    bool dynamic_difficulty_adjustment = false;
    godot::Vector2 dynamic_difficulty_bounds = godot::Vector2(0.75f, 1.25f);

    // --- 1. Combat & Action Scale (Silent Hill "Action Level") ---
    float enemy_health_multiplier = 1.0f;
    float enemy_damage_multiplier = 1.0f;
    float enemy_speed_multiplier = 1.0f;
    float enemy_aggression_multiplier = 1.0f;
    float enemy_attack_charge_multiplier = 1.0f;
    float player_damage_dealt_multiplier = 1.0f;
    float player_health_multiplier = 1.0f;
    bool friendly_fire_enabled = false;

    // --- 2. Puzzle, Riddle & Exploration Scale (Silent Hill "Riddle Level") ---
    int puzzle_complexity_tier = 2; // 0 = Story, 1 = Easy, 2 = Normal, 3 = Hard, 4 = Nightmare/Shakespearean
    float puzzle_hint_availability = 1.0f;
    float puzzle_time_limit_multiplier = 1.0f;
    bool allow_puzzle_skip = false;
    bool waypoint_guidance_enabled = true;
    bool map_auto_marking_enabled = true;

    // --- 3. Stealth & AI Perception Scale ---
    float ai_vision_range_multiplier = 1.0f;
    float ai_hearing_range_multiplier = 1.0f;
    float ai_detection_speed_multiplier = 1.0f;
    float alert_decay_rate_multiplier = 1.0f;

    // --- 4. Survival, Economy & Resource Scale ---
    float loot_abundance_multiplier = 1.0f;
    float healing_efficacy_multiplier = 1.0f;
    float stamina_drain_multiplier = 1.0f;
    float hazard_damage_multiplier = 1.0f;
    bool save_limit_enabled = false;
    bool permadeath_enabled = false;

    // --- 5. Rule Synthesis Target Selectors ---
    godot::StringName enemy_group_tag = godot::StringName("RedRobot");
    godot::StringName enemy_weapon_group_tag = godot::StringName("HeavyCannon");
    godot::StringName player_group_tag = godot::StringName("Player");
    godot::StringName puzzle_group_tag = godot::StringName("Puzzle");

    // Internal helper to synchronize semantic option_tags and difficulty_tags with active tiers
    void _sync_difficulty_tags();

public:
    GameplayDifficulty();
    ~GameplayDifficulty() override;

    // --- Static Conversion Utility ---
    static float level_to_scalar(DifficultyLevel p_level);
    static godot::String level_to_name(DifficultyLevel p_level);

    // --- 0. Mode, Preset Tiers & Tags Setters / Getters ---
    void set_scale_mode(DifficultyScaleMode p_mode);
    DifficultyScaleMode get_scale_mode() const;

    void set_overall_level(DifficultyLevel p_level);
    DifficultyLevel get_overall_level() const;
    void apply_overall_preset(DifficultyLevel p_level);

    void set_combat_level(DifficultyLevel p_level);
    DifficultyLevel get_combat_level() const;
    void apply_combat_preset(DifficultyLevel p_level);

    void set_puzzle_level(DifficultyLevel p_level);
    DifficultyLevel get_puzzle_level() const;
    void apply_puzzle_preset(DifficultyLevel p_level);

    void set_stealth_level(DifficultyLevel p_level);
    DifficultyLevel get_stealth_level() const;
    void apply_stealth_preset(DifficultyLevel p_level);

    void set_survival_level(DifficultyLevel p_level);
    DifficultyLevel get_survival_level() const;
    void apply_survival_preset(DifficultyLevel p_level);

    void set_custom_aspect_levels(const godot::Dictionary &p_aspects);
    godot::Dictionary get_custom_aspect_levels() const;
    void set_aspect_level(const godot::StringName &p_aspect, int p_level);
    int get_aspect_level(const godot::StringName &p_aspect, int p_default = static_cast<int>(DifficultyLevel::DIFFICULTY_NORMAL)) const;
    void set_aspect_multiplier(const godot::StringName &p_aspect, float p_multiplier);
    float get_aspect_multiplier(const godot::StringName &p_aspect, float p_default = 1.0f) const;

    void set_difficulty_tags(const godot::TypedArray<godot::String> &p_tags);
    godot::TypedArray<godot::String> get_difficulty_tags() const;

    void set_allow_mid_game_change(bool p_allow);
    bool get_allow_mid_game_change() const;

    void set_dynamic_difficulty_adjustment(bool p_enabled);
    bool get_dynamic_difficulty_adjustment() const;

    void set_dynamic_difficulty_bounds(const godot::Vector2 &p_bounds);
    godot::Vector2 get_dynamic_difficulty_bounds() const;

    // --- 1. Combat & Action Setters / Getters ---
    void set_enemy_health_multiplier(float p_mult);
    float get_enemy_health_multiplier() const;

    void set_enemy_damage_multiplier(float p_mult);
    float get_enemy_damage_multiplier() const;

    void set_enemy_speed_multiplier(float p_mult);
    float get_enemy_speed_multiplier() const;

    void set_enemy_aggression_multiplier(float p_mult);
    float get_enemy_aggression_multiplier() const;

    void set_enemy_attack_charge_multiplier(float p_mult);
    float get_enemy_attack_charge_multiplier() const;

    void set_player_damage_dealt_multiplier(float p_mult);
    float get_player_damage_dealt_multiplier() const;

    void set_player_health_multiplier(float p_mult);
    float get_player_health_multiplier() const;

    void set_friendly_fire_enabled(bool p_enabled);
    bool get_friendly_fire_enabled() const;

    // --- 2. Puzzle, Riddle & Exploration Setters / Getters ---
    void set_puzzle_complexity_tier(int p_tier);
    int get_puzzle_complexity_tier() const;

    void set_puzzle_hint_availability(float p_availability);
    float get_puzzle_hint_availability() const;

    void set_puzzle_time_limit_multiplier(float p_mult);
    float get_puzzle_time_limit_multiplier() const;

    void set_allow_puzzle_skip(bool p_allow);
    bool get_allow_puzzle_skip() const;

    void set_waypoint_guidance_enabled(bool p_enabled);
    bool get_waypoint_guidance_enabled() const;

    void set_map_auto_marking_enabled(bool p_enabled);
    bool get_map_auto_marking_enabled() const;

    // --- 3. Stealth & AI Perception Setters / Getters ---
    void set_ai_vision_range_multiplier(float p_mult);
    float get_ai_vision_range_multiplier() const;

    void set_ai_hearing_range_multiplier(float p_mult);
    float get_ai_hearing_range_multiplier() const;

    void set_ai_detection_speed_multiplier(float p_mult);
    float get_ai_detection_speed_multiplier() const;

    void set_alert_decay_rate_multiplier(float p_mult);
    float get_alert_decay_rate_multiplier() const;

    // --- 4. Survival, Economy & Resource Setters / Getters ---
    void set_loot_abundance_multiplier(float p_mult);
    float get_loot_abundance_multiplier() const;

    void set_healing_efficacy_multiplier(float p_mult);
    float get_healing_efficacy_multiplier() const;

    void set_stamina_drain_multiplier(float p_mult);
    float get_stamina_drain_multiplier() const;

    void set_hazard_damage_multiplier(float p_mult);
    float get_hazard_damage_multiplier() const;

    void set_save_limit_enabled(bool p_enabled);
    bool get_save_limit_enabled() const;

    void set_permadeath_enabled(bool p_enabled);
    bool get_permadeath_enabled() const;

    // --- 5. Rule Synthesis Target Selectors Setters / Getters ---
    void set_enemy_group_tag(const godot::StringName &p_tag);
    godot::StringName get_enemy_group_tag() const;

    void set_enemy_weapon_group_tag(const godot::StringName &p_tag);
    godot::StringName get_enemy_weapon_group_tag() const;

    void set_player_group_tag(const godot::StringName &p_tag);
    godot::StringName get_player_group_tag() const;

    void set_puzzle_group_tag(const godot::StringName &p_tag);
    godot::StringName get_puzzle_group_tag() const;

    // --- Application, Rule Synthesis & Persistence ---
    void apply_difficulty(const godot::Ref<GameplayDifficulty> &p_other);

    virtual godot::TypedArray<GameplayRule> collect_rules() const override;
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::DifficultyLevel);
VARIANT_ENUM_CAST(ideam::godot_ext::DifficultyScaleMode);
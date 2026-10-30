#include "gameplay_difficulty.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>

#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayDifficulty::_bind_methods() {
    // --- Enums ---
    BIND_ENUM_CONSTANT(DifficultyLevel::DIFFICULTY_STORY);
    BIND_ENUM_CONSTANT(DifficultyLevel::DIFFICULTY_EASY);
    BIND_ENUM_CONSTANT(DifficultyLevel::DIFFICULTY_NORMAL);
    BIND_ENUM_CONSTANT(DifficultyLevel::DIFFICULTY_HARD);
    BIND_ENUM_CONSTANT(DifficultyLevel::DIFFICULTY_NIGHTMARE);
    BIND_ENUM_CONSTANT(DifficultyLevel::DIFFICULTY_CUSTOM);

    BIND_ENUM_CONSTANT(DifficultyScaleMode::SCALE_MODE_UNIFIED);
    BIND_ENUM_CONSTANT(DifficultyScaleMode::SCALE_MODE_SPLIT_ASPECTS);

    // --- Static Helpers ---
    godot::ClassDB::bind_static_method(
        "GameplayDifficulty",
        godot::D_METHOD("level_to_scalar", "level"),
        &GameplayDifficulty::level_to_scalar
    );
    godot::ClassDB::bind_static_method(
        "GameplayDifficulty",
        godot::D_METHOD("level_to_name", "level"),
        &GameplayDifficulty::level_to_name
    );

    // --- 0. Mode, Preset Tiers & Tags ---
    ADD_GROUP("Difficulty Mode & Presets", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_scale_mode", "mode"), &GameplayDifficulty::set_scale_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_scale_mode"), &GameplayDifficulty::get_scale_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "scale_mode", godot::PROPERTY_HINT_ENUM, "Unified Scale,Split Aspects (Silent Hill Style)"),
        "set_scale_mode",
        "get_scale_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_overall_level", "level"), &GameplayDifficulty::set_overall_level);
    godot::ClassDB::bind_method(godot::D_METHOD("get_overall_level"), &GameplayDifficulty::get_overall_level);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_overall_preset", "level"), &GameplayDifficulty::apply_overall_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "overall_level", godot::PROPERTY_HINT_ENUM, "Story,Easy,Normal,Hard,Nightmare,Custom"),
        "set_overall_level",
        "get_overall_level"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_combat_level", "level"), &GameplayDifficulty::set_combat_level);
    godot::ClassDB::bind_method(godot::D_METHOD("get_combat_level"), &GameplayDifficulty::get_combat_level);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_combat_preset", "level"), &GameplayDifficulty::apply_combat_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "combat_level", godot::PROPERTY_HINT_ENUM, "Story,Easy,Normal,Hard,Nightmare,Custom"),
        "set_combat_level",
        "get_combat_level"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_puzzle_level", "level"), &GameplayDifficulty::set_puzzle_level);
    godot::ClassDB::bind_method(godot::D_METHOD("get_puzzle_level"), &GameplayDifficulty::get_puzzle_level);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_puzzle_preset", "level"), &GameplayDifficulty::apply_puzzle_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "puzzle_level", godot::PROPERTY_HINT_ENUM, "Story,Easy,Normal,Hard,Nightmare,Custom"),
        "set_puzzle_level",
        "get_puzzle_level"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_stealth_level", "level"), &GameplayDifficulty::set_stealth_level);
    godot::ClassDB::bind_method(godot::D_METHOD("get_stealth_level"), &GameplayDifficulty::get_stealth_level);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_stealth_preset", "level"), &GameplayDifficulty::apply_stealth_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "stealth_level", godot::PROPERTY_HINT_ENUM, "Story,Easy,Normal,Hard,Nightmare,Custom"),
        "set_stealth_level",
        "get_stealth_level"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_survival_level", "level"), &GameplayDifficulty::set_survival_level);
    godot::ClassDB::bind_method(godot::D_METHOD("get_survival_level"), &GameplayDifficulty::get_survival_level);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_survival_preset", "level"), &GameplayDifficulty::apply_survival_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "survival_level", godot::PROPERTY_HINT_ENUM, "Story,Easy,Normal,Hard,Nightmare,Custom"),
        "set_survival_level",
        "get_survival_level"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_aspect_levels", "aspects"), &GameplayDifficulty::set_custom_aspect_levels);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_aspect_levels"), &GameplayDifficulty::get_custom_aspect_levels);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::DICTIONARY, "custom_aspect_levels"),
        "set_custom_aspect_levels",
        "get_custom_aspect_levels"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aspect_level", "aspect", "level"), &GameplayDifficulty::set_aspect_level);
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_aspect_level", "aspect", "default_level"),
        &GameplayDifficulty::get_aspect_level,
        DEFVAL(static_cast<int>(DifficultyLevel::DIFFICULTY_NORMAL))
    );
    godot::ClassDB::bind_method(godot::D_METHOD("set_aspect_multiplier", "aspect", "multiplier"), &GameplayDifficulty::set_aspect_multiplier);
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_aspect_multiplier", "aspect", "default_multiplier"),
        &GameplayDifficulty::get_aspect_multiplier,
        DEFVAL(1.0f)
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulty_tags", "tags"), &GameplayDifficulty::set_difficulty_tags);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulty_tags"), &GameplayDifficulty::get_difficulty_tags);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "difficulty_tags", godot::PROPERTY_HINT_TYPE_STRING, "String"),
        "set_difficulty_tags",
        "get_difficulty_tags"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_mid_game_change", "allow"), &GameplayDifficulty::set_allow_mid_game_change);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_mid_game_change"), &GameplayDifficulty::get_allow_mid_game_change);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_mid_game_change"), "set_allow_mid_game_change", "get_allow_mid_game_change");

    godot::ClassDB::bind_method(godot::D_METHOD("set_dynamic_difficulty_adjustment", "enabled"), &GameplayDifficulty::set_dynamic_difficulty_adjustment);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dynamic_difficulty_adjustment"), &GameplayDifficulty::get_dynamic_difficulty_adjustment);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "dynamic_difficulty_adjustment"),
        "set_dynamic_difficulty_adjustment",
        "get_dynamic_difficulty_adjustment"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_dynamic_difficulty_bounds", "bounds"), &GameplayDifficulty::set_dynamic_difficulty_bounds);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dynamic_difficulty_bounds"), &GameplayDifficulty::get_dynamic_difficulty_bounds);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::VECTOR2, "dynamic_difficulty_bounds"),
        "set_dynamic_difficulty_bounds",
        "get_dynamic_difficulty_bounds"
    );

    // --- 1. Combat & Action Scale ---
    ADD_GROUP("Combat & Action", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_health_multiplier", "multiplier"), &GameplayDifficulty::set_enemy_health_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_health_multiplier"), &GameplayDifficulty::get_enemy_health_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "enemy_health_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,5.0,0.05"),
        "set_enemy_health_multiplier",
        "get_enemy_health_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_damage_multiplier", "multiplier"), &GameplayDifficulty::set_enemy_damage_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_damage_multiplier"), &GameplayDifficulty::get_enemy_damage_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "enemy_damage_multiplier", godot::PROPERTY_HINT_RANGE, "0.0,5.0,0.05"),
        "set_enemy_damage_multiplier",
        "get_enemy_damage_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_speed_multiplier", "multiplier"), &GameplayDifficulty::set_enemy_speed_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_speed_multiplier"), &GameplayDifficulty::get_enemy_speed_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "enemy_speed_multiplier", godot::PROPERTY_HINT_RANGE, "0.25,3.0,0.05"),
        "set_enemy_speed_multiplier",
        "get_enemy_speed_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_aggression_multiplier", "multiplier"), &GameplayDifficulty::set_enemy_aggression_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_aggression_multiplier"), &GameplayDifficulty::get_enemy_aggression_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "enemy_aggression_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,3.0,0.05"),
        "set_enemy_aggression_multiplier",
        "get_enemy_aggression_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_attack_charge_multiplier", "multiplier"), &GameplayDifficulty::set_enemy_attack_charge_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_attack_charge_multiplier"), &GameplayDifficulty::get_enemy_attack_charge_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "enemy_attack_charge_multiplier", godot::PROPERTY_HINT_RANGE, "0.25,3.0,0.05"),
        "set_enemy_attack_charge_multiplier",
        "get_enemy_attack_charge_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_damage_dealt_multiplier", "multiplier"), &GameplayDifficulty::set_player_damage_dealt_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_damage_dealt_multiplier"), &GameplayDifficulty::get_player_damage_dealt_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "player_damage_dealt_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,5.0,0.05"),
        "set_player_damage_dealt_multiplier",
        "get_player_damage_dealt_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_health_multiplier", "multiplier"), &GameplayDifficulty::set_player_health_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_health_multiplier"), &GameplayDifficulty::get_player_health_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "player_health_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,5.0,0.05"),
        "set_player_health_multiplier",
        "get_player_health_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_friendly_fire_enabled", "enabled"), &GameplayDifficulty::set_friendly_fire_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_friendly_fire_enabled"), &GameplayDifficulty::get_friendly_fire_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "friendly_fire_enabled"), "set_friendly_fire_enabled", "get_friendly_fire_enabled");

    // --- 2. Puzzle, Riddle & Exploration Scale ---
    ADD_GROUP("Puzzle & Riddle", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_puzzle_complexity_tier", "tier"), &GameplayDifficulty::set_puzzle_complexity_tier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_puzzle_complexity_tier"), &GameplayDifficulty::get_puzzle_complexity_tier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "puzzle_complexity_tier", godot::PROPERTY_HINT_RANGE, "0,4,1"),
        "set_puzzle_complexity_tier",
        "get_puzzle_complexity_tier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_puzzle_hint_availability", "availability"), &GameplayDifficulty::set_puzzle_hint_availability);
    godot::ClassDB::bind_method(godot::D_METHOD("get_puzzle_hint_availability"), &GameplayDifficulty::get_puzzle_hint_availability);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "puzzle_hint_availability", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.1"),
        "set_puzzle_hint_availability",
        "get_puzzle_hint_availability"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_puzzle_time_limit_multiplier", "multiplier"), &GameplayDifficulty::set_puzzle_time_limit_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_puzzle_time_limit_multiplier"), &GameplayDifficulty::get_puzzle_time_limit_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "puzzle_time_limit_multiplier", godot::PROPERTY_HINT_RANGE, "0.25,5.0,0.05"),
        "set_puzzle_time_limit_multiplier",
        "get_puzzle_time_limit_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_puzzle_skip", "allow"), &GameplayDifficulty::set_allow_puzzle_skip);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_puzzle_skip"), &GameplayDifficulty::get_allow_puzzle_skip);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_puzzle_skip"), "set_allow_puzzle_skip", "get_allow_puzzle_skip");

    godot::ClassDB::bind_method(godot::D_METHOD("set_waypoint_guidance_enabled", "enabled"), &GameplayDifficulty::set_waypoint_guidance_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_waypoint_guidance_enabled"), &GameplayDifficulty::get_waypoint_guidance_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "waypoint_guidance_enabled"), "set_waypoint_guidance_enabled", "get_waypoint_guidance_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_map_auto_marking_enabled", "enabled"), &GameplayDifficulty::set_map_auto_marking_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_auto_marking_enabled"), &GameplayDifficulty::get_map_auto_marking_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "map_auto_marking_enabled"), "set_map_auto_marking_enabled", "get_map_auto_marking_enabled");

    // --- 3. Stealth & AI Perception Scale ---
    ADD_GROUP("Stealth & Perception", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_ai_vision_range_multiplier", "multiplier"), &GameplayDifficulty::set_ai_vision_range_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ai_vision_range_multiplier"), &GameplayDifficulty::get_ai_vision_range_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ai_vision_range_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,3.0,0.05"),
        "set_ai_vision_range_multiplier",
        "get_ai_vision_range_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ai_hearing_range_multiplier", "multiplier"), &GameplayDifficulty::set_ai_hearing_range_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ai_hearing_range_multiplier"), &GameplayDifficulty::get_ai_hearing_range_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ai_hearing_range_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,3.0,0.05"),
        "set_ai_hearing_range_multiplier",
        "get_ai_hearing_range_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ai_detection_speed_multiplier", "multiplier"), &GameplayDifficulty::set_ai_detection_speed_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ai_detection_speed_multiplier"), &GameplayDifficulty::get_ai_detection_speed_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ai_detection_speed_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,3.0,0.05"),
        "set_ai_detection_speed_multiplier",
        "get_ai_detection_speed_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_alert_decay_rate_multiplier", "multiplier"), &GameplayDifficulty::set_alert_decay_rate_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_alert_decay_rate_multiplier"), &GameplayDifficulty::get_alert_decay_rate_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "alert_decay_rate_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,3.0,0.05"),
        "set_alert_decay_rate_multiplier",
        "get_alert_decay_rate_multiplier"
    );

    // --- 4. Survival, Economy & Resource Scale ---
    ADD_GROUP("Survival & Resources", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_loot_abundance_multiplier", "multiplier"), &GameplayDifficulty::set_loot_abundance_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_loot_abundance_multiplier"), &GameplayDifficulty::get_loot_abundance_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "loot_abundance_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,4.0,0.05"),
        "set_loot_abundance_multiplier",
        "get_loot_abundance_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_healing_efficacy_multiplier", "multiplier"), &GameplayDifficulty::set_healing_efficacy_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_healing_efficacy_multiplier"), &GameplayDifficulty::get_healing_efficacy_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "healing_efficacy_multiplier", godot::PROPERTY_HINT_RANGE, "0.1,3.0,0.05"),
        "set_healing_efficacy_multiplier",
        "get_healing_efficacy_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_stamina_drain_multiplier", "multiplier"), &GameplayDifficulty::set_stamina_drain_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_stamina_drain_multiplier"), &GameplayDifficulty::get_stamina_drain_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "stamina_drain_multiplier", godot::PROPERTY_HINT_RANGE, "0.0,3.0,0.05"),
        "set_stamina_drain_multiplier",
        "get_stamina_drain_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_hazard_damage_multiplier", "multiplier"), &GameplayDifficulty::set_hazard_damage_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hazard_damage_multiplier"), &GameplayDifficulty::get_hazard_damage_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "hazard_damage_multiplier", godot::PROPERTY_HINT_RANGE, "0.0,5.0,0.05"),
        "set_hazard_damage_multiplier",
        "get_hazard_damage_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_save_limit_enabled", "enabled"), &GameplayDifficulty::set_save_limit_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_save_limit_enabled"), &GameplayDifficulty::get_save_limit_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "save_limit_enabled"), "set_save_limit_enabled", "get_save_limit_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_permadeath_enabled", "enabled"), &GameplayDifficulty::set_permadeath_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_permadeath_enabled"), &GameplayDifficulty::get_permadeath_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "permadeath_enabled"), "set_permadeath_enabled", "get_permadeath_enabled");

    // --- 5. Rule Synthesis Target Selectors ---
    ADD_GROUP("Rule Target Selectors", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_group_tag", "tag"), &GameplayDifficulty::set_enemy_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_group_tag"), &GameplayDifficulty::get_enemy_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "enemy_group_tag"), "set_enemy_group_tag", "get_enemy_group_tag");

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_weapon_group_tag", "tag"), &GameplayDifficulty::set_enemy_weapon_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_weapon_group_tag"), &GameplayDifficulty::get_enemy_weapon_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "enemy_weapon_group_tag"), "set_enemy_weapon_group_tag", "get_enemy_weapon_group_tag");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_group_tag", "tag"), &GameplayDifficulty::set_player_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_group_tag"), &GameplayDifficulty::get_player_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "player_group_tag"), "set_player_group_tag", "get_player_group_tag");

    godot::ClassDB::bind_method(godot::D_METHOD("set_puzzle_group_tag", "tag"), &GameplayDifficulty::set_puzzle_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_puzzle_group_tag"), &GameplayDifficulty::get_puzzle_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "puzzle_group_tag"), "set_puzzle_group_tag", "get_puzzle_group_tag");

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("apply_difficulty", "other"), &GameplayDifficulty::apply_difficulty);
}

GameplayDifficulty::GameplayDifficulty() {
    set_category(godot::StringName("Difficulty"));
    set_config_section("difficulty");
    _sync_difficulty_tags();
}

GameplayDifficulty::~GameplayDifficulty() {
}

// --- Static Conversion Utilities ---

float GameplayDifficulty::level_to_scalar(DifficultyLevel p_level) {
    switch (p_level) {
        case DifficultyLevel::DIFFICULTY_STORY:
            return 0.5f;
        case DifficultyLevel::DIFFICULTY_EASY:
            return 0.75f;
        case DifficultyLevel::DIFFICULTY_NORMAL:
            return 1.0f;
        case DifficultyLevel::DIFFICULTY_HARD:
            return 1.35f;
        case DifficultyLevel::DIFFICULTY_NIGHTMARE:
            return 1.75f;
        case DifficultyLevel::DIFFICULTY_CUSTOM:
        default:
            return 1.0f;
    }
}

godot::String GameplayDifficulty::level_to_name(DifficultyLevel p_level) {
    switch (p_level) {
        case DifficultyLevel::DIFFICULTY_STORY:
            return "Story";
        case DifficultyLevel::DIFFICULTY_EASY:
            return "Easy";
        case DifficultyLevel::DIFFICULTY_NORMAL:
            return "Normal";
        case DifficultyLevel::DIFFICULTY_HARD:
            return "Hard";
        case DifficultyLevel::DIFFICULTY_NIGHTMARE:
            return "Nightmare";
        case DifficultyLevel::DIFFICULTY_CUSTOM:
        default:
            return "Custom";
    }
}

void GameplayDifficulty::_sync_difficulty_tags() {
    const char *levels[] = {"Story", "Easy", "Normal", "Hard", "Nightmare", "Custom"};
    const char *prefixes[] = {"Difficulty_", "Action_", "Riddle_", "Stealth_", "Survival_"};

    for (const char *prefix : prefixes) {
        for (const char *lvl : levels) {
            godot::String tag_str = godot::String(prefix) + godot::String(lvl);
            remove_tag(godot::StringName(tag_str));
            int idx = difficulty_tags.find(tag_str);
            if (idx >= 0) {
                difficulty_tags.remove_at(idx);
            }
        }
    }

    auto append_tier_tag = [this](const godot::String &p_prefix, DifficultyLevel p_lvl) {
        godot::String tag_str = p_prefix + level_to_name(p_lvl);
        add_tag(godot::StringName(tag_str));
        if (!difficulty_tags.has(tag_str)) {
            difficulty_tags.append(tag_str);
        }
    };

    append_tier_tag("Difficulty_", overall_level);
    append_tier_tag("Action_", combat_level);
    append_tier_tag("Riddle_", puzzle_level);
    append_tier_tag("Stealth_", stealth_level);
    append_tier_tag("Survival_", survival_level);
}

// --- 0. Mode, Preset Tiers & Tags Setters / Getters ---

void GameplayDifficulty::set_scale_mode(DifficultyScaleMode p_mode) {
    if (scale_mode == p_mode) return;
    scale_mode = p_mode;
    if (scale_mode == DifficultyScaleMode::SCALE_MODE_UNIFIED && overall_level != DifficultyLevel::DIFFICULTY_CUSTOM) {
        apply_overall_preset(overall_level);
    } else {
        _sync_difficulty_tags();
        emit_changed();
    }
}
DifficultyScaleMode GameplayDifficulty::get_scale_mode() const { return scale_mode; }

void GameplayDifficulty::set_overall_level(DifficultyLevel p_level) {
    if (overall_level == p_level) return;
    overall_level = p_level;
    if (p_level != DifficultyLevel::DIFFICULTY_CUSTOM) {
        apply_overall_preset(p_level);
    } else {
        _sync_difficulty_tags();
        emit_changed();
    }
}
DifficultyLevel GameplayDifficulty::get_overall_level() const { return overall_level; }

void GameplayDifficulty::apply_overall_preset(DifficultyLevel p_level) {
    overall_level = p_level;
    if (p_level != DifficultyLevel::DIFFICULTY_CUSTOM) {
        apply_combat_preset(p_level);
        apply_puzzle_preset(p_level);
        apply_stealth_preset(p_level);
        apply_survival_preset(p_level);
    }
    _sync_difficulty_tags();
    emit_changed();
}

void GameplayDifficulty::set_combat_level(DifficultyLevel p_level) {
    if (combat_level == p_level) return;
    combat_level = p_level;
    if (p_level != DifficultyLevel::DIFFICULTY_CUSTOM) {
        apply_combat_preset(p_level);
    } else {
        _sync_difficulty_tags();
        emit_changed();
    }
}
DifficultyLevel GameplayDifficulty::get_combat_level() const { return combat_level; }

void GameplayDifficulty::apply_combat_preset(DifficultyLevel p_level) {
    combat_level = p_level;
    switch (p_level) {
        case DifficultyLevel::DIFFICULTY_STORY:
            enemy_health_multiplier = 0.5f;
            enemy_damage_multiplier = 0.35f;
            enemy_speed_multiplier = 0.8f;
            enemy_aggression_multiplier = 0.5f;
            enemy_attack_charge_multiplier = 1.6f; // Slower telegraph wind-up
            player_damage_dealt_multiplier = 1.5f;
            player_health_multiplier = 1.75f;
            friendly_fire_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_EASY:
            enemy_health_multiplier = 0.75f;
            enemy_damage_multiplier = 0.65f;
            enemy_speed_multiplier = 0.9f;
            enemy_aggression_multiplier = 0.75f;
            enemy_attack_charge_multiplier = 1.25f;
            player_damage_dealt_multiplier = 1.25f;
            player_health_multiplier = 1.25f;
            friendly_fire_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_NORMAL:
            enemy_health_multiplier = 1.0f;
            enemy_damage_multiplier = 1.0f;
            enemy_speed_multiplier = 1.0f;
            enemy_aggression_multiplier = 1.0f;
            enemy_attack_charge_multiplier = 1.0f;
            player_damage_dealt_multiplier = 1.0f;
            player_health_multiplier = 1.0f;
            friendly_fire_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_HARD:
            enemy_health_multiplier = 1.35f;
            enemy_damage_multiplier = 1.4f;
            enemy_speed_multiplier = 1.15f;
            enemy_aggression_multiplier = 1.3f;
            enemy_attack_charge_multiplier = 0.85f;
            player_damage_dealt_multiplier = 0.9f;
            player_health_multiplier = 0.85f;
            friendly_fire_enabled = true;
            break;
        case DifficultyLevel::DIFFICULTY_NIGHTMARE:
            enemy_health_multiplier = 1.75f;
            enemy_damage_multiplier = 2.0f;
            enemy_speed_multiplier = 1.3f;
            enemy_aggression_multiplier = 1.6f;
            enemy_attack_charge_multiplier = 0.7f;
            player_damage_dealt_multiplier = 0.75f;
            player_health_multiplier = 0.65f;
            friendly_fire_enabled = true;
            break;
        case DifficultyLevel::DIFFICULTY_CUSTOM:
        default:
            break;
    }
    _sync_difficulty_tags();
    emit_changed();
}

void GameplayDifficulty::set_puzzle_level(DifficultyLevel p_level) {
    if (puzzle_level == p_level) return;
    puzzle_level = p_level;
    if (p_level != DifficultyLevel::DIFFICULTY_CUSTOM) {
        apply_puzzle_preset(p_level);
    } else {
        _sync_difficulty_tags();
        emit_changed();
    }
}
DifficultyLevel GameplayDifficulty::get_puzzle_level() const { return puzzle_level; }

void GameplayDifficulty::apply_puzzle_preset(DifficultyLevel p_level) {
    puzzle_level = p_level;
    switch (p_level) {
        case DifficultyLevel::DIFFICULTY_STORY:
            puzzle_complexity_tier = 0;
            puzzle_hint_availability = 2.0f;
            puzzle_time_limit_multiplier = 3.0f;
            allow_puzzle_skip = true;
            waypoint_guidance_enabled = true;
            map_auto_marking_enabled = true;
            break;
        case DifficultyLevel::DIFFICULTY_EASY:
            puzzle_complexity_tier = 1;
            puzzle_hint_availability = 1.5f;
            puzzle_time_limit_multiplier = 1.5f;
            allow_puzzle_skip = true;
            waypoint_guidance_enabled = true;
            map_auto_marking_enabled = true;
            break;
        case DifficultyLevel::DIFFICULTY_NORMAL:
            puzzle_complexity_tier = 2;
            puzzle_hint_availability = 1.0f;
            puzzle_time_limit_multiplier = 1.0f;
            allow_puzzle_skip = false;
            waypoint_guidance_enabled = true;
            map_auto_marking_enabled = true;
            break;
        case DifficultyLevel::DIFFICULTY_HARD:
            puzzle_complexity_tier = 3;
            puzzle_hint_availability = 0.25f;
            puzzle_time_limit_multiplier = 0.75f;
            allow_puzzle_skip = false;
            waypoint_guidance_enabled = false;
            map_auto_marking_enabled = true;
            break;
        case DifficultyLevel::DIFFICULTY_NIGHTMARE:
            puzzle_complexity_tier = 4; // Classic Silent Hill "Shakespearean / Extra Riddle" complexity
            puzzle_hint_availability = 0.0f;
            puzzle_time_limit_multiplier = 0.5f;
            allow_puzzle_skip = false;
            waypoint_guidance_enabled = false;
            map_auto_marking_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_CUSTOM:
        default:
            break;
    }
    _sync_difficulty_tags();
    emit_changed();
}

void GameplayDifficulty::set_stealth_level(DifficultyLevel p_level) {
    if (stealth_level == p_level) return;
    stealth_level = p_level;
    if (p_level != DifficultyLevel::DIFFICULTY_CUSTOM) {
        apply_stealth_preset(p_level);
    } else {
        _sync_difficulty_tags();
        emit_changed();
    }
}
DifficultyLevel GameplayDifficulty::get_stealth_level() const { return stealth_level; }

void GameplayDifficulty::apply_stealth_preset(DifficultyLevel p_level) {
    stealth_level = p_level;
    switch (p_level) {
        case DifficultyLevel::DIFFICULTY_STORY:
            ai_vision_range_multiplier = 0.55f;
            ai_hearing_range_multiplier = 0.55f;
            ai_detection_speed_multiplier = 0.5f;
            alert_decay_rate_multiplier = 1.75f;
            break;
        case DifficultyLevel::DIFFICULTY_EASY:
            ai_vision_range_multiplier = 0.75f;
            ai_hearing_range_multiplier = 0.75f;
            ai_detection_speed_multiplier = 0.75f;
            alert_decay_rate_multiplier = 1.35f;
            break;
        case DifficultyLevel::DIFFICULTY_NORMAL:
            ai_vision_range_multiplier = 1.0f;
            ai_hearing_range_multiplier = 1.0f;
            ai_detection_speed_multiplier = 1.0f;
            alert_decay_rate_multiplier = 1.0f;
            break;
        case DifficultyLevel::DIFFICULTY_HARD:
            ai_vision_range_multiplier = 1.25f;
            ai_hearing_range_multiplier = 1.25f;
            ai_detection_speed_multiplier = 1.35f;
            alert_decay_rate_multiplier = 0.7f;
            break;
        case DifficultyLevel::DIFFICULTY_NIGHTMARE:
            ai_vision_range_multiplier = 1.5f;
            ai_hearing_range_multiplier = 1.5f;
            ai_detection_speed_multiplier = 1.75f;
            alert_decay_rate_multiplier = 0.45f;
            break;
        case DifficultyLevel::DIFFICULTY_CUSTOM:
        default:
            break;
    }
    _sync_difficulty_tags();
    emit_changed();
}

void GameplayDifficulty::set_survival_level(DifficultyLevel p_level) {
    if (survival_level == p_level) return;
    survival_level = p_level;
    if (p_level != DifficultyLevel::DIFFICULTY_CUSTOM) {
        apply_survival_preset(p_level);
    } else {
        _sync_difficulty_tags();
        emit_changed();
    }
}
DifficultyLevel GameplayDifficulty::get_survival_level() const { return survival_level; }

void GameplayDifficulty::apply_survival_preset(DifficultyLevel p_level) {
    survival_level = p_level;
    switch (p_level) {
        case DifficultyLevel::DIFFICULTY_STORY:
            loot_abundance_multiplier = 2.0f;
            healing_efficacy_multiplier = 1.75f;
            stamina_drain_multiplier = 0.4f;
            hazard_damage_multiplier = 0.25f;
            save_limit_enabled = false;
            permadeath_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_EASY:
            loot_abundance_multiplier = 1.4f;
            healing_efficacy_multiplier = 1.35f;
            stamina_drain_multiplier = 0.7f;
            hazard_damage_multiplier = 0.6f;
            save_limit_enabled = false;
            permadeath_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_NORMAL:
            loot_abundance_multiplier = 1.0f;
            healing_efficacy_multiplier = 1.0f;
            stamina_drain_multiplier = 1.0f;
            hazard_damage_multiplier = 1.0f;
            save_limit_enabled = false;
            permadeath_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_HARD:
            loot_abundance_multiplier = 0.7f;
            healing_efficacy_multiplier = 0.8f;
            stamina_drain_multiplier = 1.25f;
            hazard_damage_multiplier = 1.35f;
            save_limit_enabled = false;
            permadeath_enabled = false;
            break;
        case DifficultyLevel::DIFFICULTY_NIGHTMARE:
            loot_abundance_multiplier = 0.45f;
            healing_efficacy_multiplier = 0.6f;
            stamina_drain_multiplier = 1.6f;
            hazard_damage_multiplier = 2.0f;
            save_limit_enabled = true;
            break;
        case DifficultyLevel::DIFFICULTY_CUSTOM:
        default:
            break;
    }
    _sync_difficulty_tags();
    emit_changed();
}

void GameplayDifficulty::set_custom_aspect_levels(const godot::Dictionary &p_aspects) {
    if (custom_aspect_levels == p_aspects) return;
    custom_aspect_levels = p_aspects.duplicate();
    emit_changed();
}
godot::Dictionary GameplayDifficulty::get_custom_aspect_levels() const { return custom_aspect_levels; }

void GameplayDifficulty::set_aspect_level(const godot::StringName &p_aspect, int p_level) {
    if (p_aspect.is_empty()) return;
    DifficultyLevel clamped_lvl = static_cast<DifficultyLevel>(
        godot::Math::clamp(p_level, static_cast<int>(DifficultyLevel::DIFFICULTY_STORY), static_cast<int>(DifficultyLevel::DIFFICULTY_CUSTOM))
    );

    if (p_aspect == godot::StringName("Combat") || p_aspect == godot::StringName("Action")) {
        set_combat_level(clamped_lvl);
        return;
    }
    if (p_aspect == godot::StringName("Puzzle") || p_aspect == godot::StringName("Riddle")) {
        set_puzzle_level(clamped_lvl);
        return;
    }
    if (p_aspect == godot::StringName("Stealth") || p_aspect == godot::StringName("Perception")) {
        set_stealth_level(clamped_lvl);
        return;
    }
    if (p_aspect == godot::StringName("Survival") || p_aspect == godot::StringName("Resources")) {
        set_survival_level(clamped_lvl);
        return;
    }

    godot::String key(p_aspect);
    custom_aspect_levels[key] = static_cast<int>(clamped_lvl);
    emit_changed();
}

int GameplayDifficulty::get_aspect_level(const godot::StringName &p_aspect, int p_default) const {
    if (p_aspect == godot::StringName("Combat") || p_aspect == godot::StringName("Action")) {
        return static_cast<int>(combat_level);
    }
    if (p_aspect == godot::StringName("Puzzle") || p_aspect == godot::StringName("Riddle")) {
        return static_cast<int>(puzzle_level);
    }
    if (p_aspect == godot::StringName("Stealth") || p_aspect == godot::StringName("Perception")) {
        return static_cast<int>(stealth_level);
    }
    if (p_aspect == godot::StringName("Survival") || p_aspect == godot::StringName("Resources")) {
        return static_cast<int>(survival_level);
    }

    godot::String key(p_aspect);
    if (custom_aspect_levels.has(key)) {
        return static_cast<int>(custom_aspect_levels[key]);
    }
    return p_default;
}

void GameplayDifficulty::set_aspect_multiplier(const godot::StringName &p_aspect, float p_multiplier) {
    if (p_aspect.is_empty()) return;
    godot::String key(p_aspect);
    custom_aspect_levels[key] = godot::Math::clamp(p_multiplier, 0.0f, 10.0f);
    emit_changed();
}

float GameplayDifficulty::get_aspect_multiplier(const godot::StringName &p_aspect, float p_default) const {
    if (p_aspect == godot::StringName("Combat") || p_aspect == godot::StringName("Action")) {
        return level_to_scalar(combat_level);
    }
    if (p_aspect == godot::StringName("Puzzle") || p_aspect == godot::StringName("Riddle")) {
        return level_to_scalar(puzzle_level);
    }
    if (p_aspect == godot::StringName("Stealth") || p_aspect == godot::StringName("Perception")) {
        return level_to_scalar(stealth_level);
    }
    if (p_aspect == godot::StringName("Survival") || p_aspect == godot::StringName("Resources")) {
        return level_to_scalar(survival_level);
    }

    godot::String key(p_aspect);
    if (custom_aspect_levels.has(key)) {
        godot::Variant v = custom_aspect_levels[key];
        if (v.get_type() == godot::Variant::INT) {
            return level_to_scalar(static_cast<DifficultyLevel>(static_cast<int>(v)));
        }
        return static_cast<float>(v);
    }
    return p_default;
}

void GameplayDifficulty::set_difficulty_tags(const godot::TypedArray<godot::String> &p_tags) {
    if (p_tags == difficulty_tags) return;
    difficulty_tags = p_tags;
    for (int i = 0; i < difficulty_tags.size(); ++i) {
        godot::String s = difficulty_tags[i];
        if (!s.is_empty()) {
            add_tag(godot::StringName(s));
        }
    }
    emit_changed();
}
godot::TypedArray<godot::String> GameplayDifficulty::get_difficulty_tags() const { return difficulty_tags; }

void GameplayDifficulty::set_allow_mid_game_change(bool p_allow) {
    if (allow_mid_game_change == p_allow) return;
    allow_mid_game_change = p_allow;
    emit_changed();
}
bool GameplayDifficulty::get_allow_mid_game_change() const { return allow_mid_game_change; }

void GameplayDifficulty::set_dynamic_difficulty_adjustment(bool p_enabled) {
    if (dynamic_difficulty_adjustment == p_enabled) return;
    dynamic_difficulty_adjustment = p_enabled;
    emit_changed();
}
bool GameplayDifficulty::get_dynamic_difficulty_adjustment() const { return dynamic_difficulty_adjustment; }

void GameplayDifficulty::set_dynamic_difficulty_bounds(const godot::Vector2 &p_bounds) {
    godot::Vector2 clamped(
        godot::Math::clamp(p_bounds.x, 0.1f, 5.0f),
        godot::Math::clamp(p_bounds.y, 0.1f, 5.0f)
    );
    if (dynamic_difficulty_bounds.is_equal_approx(clamped)) return;
    dynamic_difficulty_bounds = clamped;
    emit_changed();
}
godot::Vector2 GameplayDifficulty::get_dynamic_difficulty_bounds() const { return dynamic_difficulty_bounds; }

// --- 1. Combat & Action Setters / Getters ---

void GameplayDifficulty::set_enemy_health_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 10.0f);
    if (godot::Math::is_equal_approx(enemy_health_multiplier, clamped)) return;
    enemy_health_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_enemy_health_multiplier() const { return enemy_health_multiplier; }

void GameplayDifficulty::set_enemy_damage_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.0f, 10.0f);
    if (godot::Math::is_equal_approx(enemy_damage_multiplier, clamped)) return;
    enemy_damage_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_enemy_damage_multiplier() const { return enemy_damage_multiplier; }

void GameplayDifficulty::set_enemy_speed_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.1f, 5.0f);
    if (godot::Math::is_equal_approx(enemy_speed_multiplier, clamped)) return;
    enemy_speed_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_enemy_speed_multiplier() const { return enemy_speed_multiplier; }

void GameplayDifficulty::set_enemy_aggression_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 5.0f);
    if (godot::Math::is_equal_approx(enemy_aggression_multiplier, clamped)) return;
    enemy_aggression_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_enemy_aggression_multiplier() const { return enemy_aggression_multiplier; }

void GameplayDifficulty::set_enemy_attack_charge_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.1f, 5.0f);
    if (godot::Math::is_equal_approx(enemy_attack_charge_multiplier, clamped)) return;
    enemy_attack_charge_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_enemy_attack_charge_multiplier() const { return enemy_attack_charge_multiplier; }

void GameplayDifficulty::set_player_damage_dealt_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 10.0f);
    if (godot::Math::is_equal_approx(player_damage_dealt_multiplier, clamped)) return;
    player_damage_dealt_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_player_damage_dealt_multiplier() const { return player_damage_dealt_multiplier; }

void GameplayDifficulty::set_player_health_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 10.0f);
    if (godot::Math::is_equal_approx(player_health_multiplier, clamped)) return;
    player_health_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_player_health_multiplier() const { return player_health_multiplier; }

void GameplayDifficulty::set_friendly_fire_enabled(bool p_enabled) {
    if (friendly_fire_enabled == p_enabled) return;
    friendly_fire_enabled = p_enabled;
    emit_changed();
}
bool GameplayDifficulty::get_friendly_fire_enabled() const { return friendly_fire_enabled; }

// --- 2. Puzzle, Riddle & Exploration Setters / Getters ---

void GameplayDifficulty::set_puzzle_complexity_tier(int p_tier) {
    int clamped = godot::Math::clamp(p_tier, 0, 10);
    if (puzzle_complexity_tier == clamped) return;
    puzzle_complexity_tier = clamped;
    emit_changed();
}
int GameplayDifficulty::get_puzzle_complexity_tier() const { return puzzle_complexity_tier; }

void GameplayDifficulty::set_puzzle_hint_availability(float p_availability) {
    float clamped = godot::Math::clamp(p_availability, 0.0f, 5.0f);
    if (godot::Math::is_equal_approx(puzzle_hint_availability, clamped)) return;
    puzzle_hint_availability = clamped;
    emit_changed();
}
float GameplayDifficulty::get_puzzle_hint_availability() const { return puzzle_hint_availability; }

void GameplayDifficulty::set_puzzle_time_limit_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.1f, 10.0f);
    if (godot::Math::is_equal_approx(puzzle_time_limit_multiplier, clamped)) return;
    puzzle_time_limit_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_puzzle_time_limit_multiplier() const { return puzzle_time_limit_multiplier; }

void GameplayDifficulty::set_allow_puzzle_skip(bool p_allow) {
    if (allow_puzzle_skip == p_allow) return;
    allow_puzzle_skip = p_allow;
    emit_changed();
}
bool GameplayDifficulty::get_allow_puzzle_skip() const { return allow_puzzle_skip; }

void GameplayDifficulty::set_waypoint_guidance_enabled(bool p_enabled) {
    if (waypoint_guidance_enabled == p_enabled) return;
    waypoint_guidance_enabled = p_enabled;
    emit_changed();
}
bool GameplayDifficulty::get_waypoint_guidance_enabled() const { return waypoint_guidance_enabled; }

void GameplayDifficulty::set_map_auto_marking_enabled(bool p_enabled) {
    if (map_auto_marking_enabled == p_enabled) return;
    map_auto_marking_enabled = p_enabled;
    emit_changed();
}
bool GameplayDifficulty::get_map_auto_marking_enabled() const { return map_auto_marking_enabled; }

// --- 3. Stealth & AI Perception Setters / Getters ---

void GameplayDifficulty::set_ai_vision_range_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 5.0f);
    if (godot::Math::is_equal_approx(ai_vision_range_multiplier, clamped)) return;
    ai_vision_range_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_ai_vision_range_multiplier() const { return ai_vision_range_multiplier; }

void GameplayDifficulty::set_ai_hearing_range_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 5.0f);
    if (godot::Math::is_equal_approx(ai_hearing_range_multiplier, clamped)) return;
    ai_hearing_range_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_ai_hearing_range_multiplier() const { return ai_hearing_range_multiplier; }

void GameplayDifficulty::set_ai_detection_speed_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 5.0f);
    if (godot::Math::is_equal_approx(ai_detection_speed_multiplier, clamped)) return;
    ai_detection_speed_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_ai_detection_speed_multiplier() const { return ai_detection_speed_multiplier; }

void GameplayDifficulty::set_alert_decay_rate_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 5.0f);
    if (godot::Math::is_equal_approx(alert_decay_rate_multiplier, clamped)) return;
    alert_decay_rate_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_alert_decay_rate_multiplier() const { return alert_decay_rate_multiplier; }

// --- 4. Survival, Economy & Resource Setters / Getters ---

void GameplayDifficulty::set_loot_abundance_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.0f, 10.0f);
    if (godot::Math::is_equal_approx(loot_abundance_multiplier, clamped)) return;
    loot_abundance_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_loot_abundance_multiplier() const { return loot_abundance_multiplier; }

void GameplayDifficulty::set_healing_efficacy_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.05f, 10.0f);
    if (godot::Math::is_equal_approx(healing_efficacy_multiplier, clamped)) return;
    healing_efficacy_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_healing_efficacy_multiplier() const { return healing_efficacy_multiplier; }

void GameplayDifficulty::set_stamina_drain_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.0f, 10.0f);
    if (godot::Math::is_equal_approx(stamina_drain_multiplier, clamped)) return;
    stamina_drain_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_stamina_drain_multiplier() const { return stamina_drain_multiplier; }

void GameplayDifficulty::set_hazard_damage_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.0f, 10.0f);
    if (godot::Math::is_equal_approx(hazard_damage_multiplier, clamped)) return;
    hazard_damage_multiplier = clamped;
    emit_changed();
}
float GameplayDifficulty::get_hazard_damage_multiplier() const { return hazard_damage_multiplier; }

void GameplayDifficulty::set_save_limit_enabled(bool p_enabled) {
    if (save_limit_enabled == p_enabled) return;
    save_limit_enabled = p_enabled;
    emit_changed();
}
bool GameplayDifficulty::get_save_limit_enabled() const { return save_limit_enabled; }

void GameplayDifficulty::set_permadeath_enabled(bool p_enabled) {
    if (permadeath_enabled == p_enabled) return;
    permadeath_enabled = p_enabled;
    emit_changed();
}
bool GameplayDifficulty::get_permadeath_enabled() const { return permadeath_enabled; }

// --- 5. Rule Synthesis Target Selectors Setters / Getters ---

void GameplayDifficulty::set_enemy_group_tag(const godot::StringName &p_tag) {
    if (enemy_group_tag == p_tag) return;
    enemy_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayDifficulty::get_enemy_group_tag() const { return enemy_group_tag; }

void GameplayDifficulty::set_enemy_weapon_group_tag(const godot::StringName &p_tag) {
    if (enemy_weapon_group_tag == p_tag) return;
    enemy_weapon_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayDifficulty::get_enemy_weapon_group_tag() const { return enemy_weapon_group_tag; }

void GameplayDifficulty::set_player_group_tag(const godot::StringName &p_tag) {
    if (player_group_tag == p_tag) return;
    player_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayDifficulty::get_player_group_tag() const { return player_group_tag; }

void GameplayDifficulty::set_puzzle_group_tag(const godot::StringName &p_tag) {
    if (puzzle_group_tag == p_tag) return;
    puzzle_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayDifficulty::get_puzzle_group_tag() const { return puzzle_group_tag; }

// --- Application, Rule Synthesis & Persistence ---

void GameplayDifficulty::apply_difficulty(const godot::Ref<GameplayDifficulty> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }

    GameplayResource::apply_resource(p_other);

    scale_mode = p_other->get_scale_mode();
    overall_level = p_other->get_overall_level();
    combat_level = p_other->get_combat_level();
    puzzle_level = p_other->get_puzzle_level();
    stealth_level = p_other->get_stealth_level();
    survival_level = p_other->get_survival_level();
    custom_aspect_levels = p_other->get_custom_aspect_levels().duplicate();
    difficulty_tags = p_other->get_difficulty_tags().duplicate();
    allow_mid_game_change = p_other->get_allow_mid_game_change();
    dynamic_difficulty_adjustment = p_other->get_dynamic_difficulty_adjustment();
    dynamic_difficulty_bounds = p_other->get_dynamic_difficulty_bounds();

    enemy_health_multiplier = p_other->get_enemy_health_multiplier();
    enemy_damage_multiplier = p_other->get_enemy_damage_multiplier();
    enemy_speed_multiplier = p_other->get_enemy_speed_multiplier();
    enemy_aggression_multiplier = p_other->get_enemy_aggression_multiplier();
    enemy_attack_charge_multiplier = p_other->get_enemy_attack_charge_multiplier();
    player_damage_dealt_multiplier = p_other->get_player_damage_dealt_multiplier();
    player_health_multiplier = p_other->get_player_health_multiplier();
    friendly_fire_enabled = p_other->get_friendly_fire_enabled();

    puzzle_complexity_tier = p_other->get_puzzle_complexity_tier();
    puzzle_hint_availability = p_other->get_puzzle_hint_availability();
    puzzle_time_limit_multiplier = p_other->get_puzzle_time_limit_multiplier();
    allow_puzzle_skip = p_other->get_allow_puzzle_skip();
    waypoint_guidance_enabled = p_other->get_waypoint_guidance_enabled();
    map_auto_marking_enabled = p_other->get_map_auto_marking_enabled();

    ai_vision_range_multiplier = p_other->get_ai_vision_range_multiplier();
    ai_hearing_range_multiplier = p_other->get_ai_hearing_range_multiplier();
    ai_detection_speed_multiplier = p_other->get_ai_detection_speed_multiplier();
    alert_decay_rate_multiplier = p_other->get_alert_decay_rate_multiplier();

    loot_abundance_multiplier = p_other->get_loot_abundance_multiplier();
    healing_efficacy_multiplier = p_other->get_healing_efficacy_multiplier();
    stamina_drain_multiplier = p_other->get_stamina_drain_multiplier();
    hazard_damage_multiplier = p_other->get_hazard_damage_multiplier();
    save_limit_enabled = p_other->get_save_limit_enabled();
    permadeath_enabled = p_other->get_permadeath_enabled();

    enemy_group_tag = p_other->get_enemy_group_tag();
    enemy_weapon_group_tag = p_other->get_enemy_weapon_group_tag();
    player_group_tag = p_other->get_player_group_tag();
    puzzle_group_tag = p_other->get_puzzle_group_tag();

    _sync_difficulty_tags();
    emit_changed();
}

void GameplayDifficulty::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    godot::Ref<GameplayDifficulty> diff_other = p_other;
    if (diff_other.is_valid()) {
        apply_difficulty(diff_other);
    } else {
        GameplayResource::apply_resource(p_other);
    }
}

godot::TypedArray<GameplayRule> GameplayDifficulty::collect_rules() const {
    godot::TypedArray<GameplayRule> collected = GameplayResource::collect_rules();
    if (!get_enabled()) {
        return collected;
    }

    // 1. Enemy Health Scaling
    if (!godot::Math::is_equal_approx(enemy_health_multiplier, 1.0f) && !enemy_group_tag.is_empty()) {
        godot::Ref<GameplayRule> rule;
        rule.instantiate();
        rule->set_title("Difficulty_EnemyHealth");
        rule->set_scope(RuleScope::DESCENDANTS);
        rule->set_target_class(godot::StringName("GameAgent"));
        rule->set_target_title_or_group(enemy_group_tag);
        rule->set_target_property(godot::StringName("Health"));
        rule->set_operation(RuleOp::MULTIPLY);
        rule->set_value(enemy_health_multiplier);
        rule->set_priority(30);
        collected.append(rule);
    }

    // 2. Enemy Damage Scaling
    if (!godot::Math::is_equal_approx(enemy_damage_multiplier, 1.0f) && !enemy_group_tag.is_empty()) {
        godot::Ref<GameplayRule> rule;
        rule.instantiate();
        rule->set_title("Difficulty_EnemyDamage");
        rule->set_scope(RuleScope::DESCENDANTS);
        rule->set_target_class(godot::StringName("GameEntity"));
        rule->set_target_title_or_group(enemy_group_tag);
        rule->set_target_property(godot::StringName("Damage"));
        rule->set_operation(RuleOp::MULTIPLY);
        rule->set_value(enemy_damage_multiplier);
        rule->set_priority(30);
        collected.append(rule);
    }

    // 3. Enemy Movement / Action Speed (`time_scale`)
    if (!godot::Math::is_equal_approx(enemy_speed_multiplier, 1.0f) && !enemy_group_tag.is_empty()) {
        godot::Ref<GameplayRule> rule;
        rule.instantiate();
        rule->set_title("Difficulty_EnemyTimeScale");
        rule->set_scope(RuleScope::DESCENDANTS);
        rule->set_target_class(godot::StringName("GameAgent"));
        rule->set_target_title_or_group(enemy_group_tag);
        rule->set_target_property(godot::StringName("time_scale"));
        rule->set_operation(RuleOp::MULTIPLY);
        rule->set_value(enemy_speed_multiplier);
        rule->set_priority(30);
        collected.append(rule);
    }

    // 4. Enemy Weapon Charge / Telegraph Multiplier
    if (!godot::Math::is_equal_approx(enemy_attack_charge_multiplier, 1.0f) && !enemy_weapon_group_tag.is_empty()) {
        godot::Ref<GameplayRule> rule;
        rule.instantiate();
        rule->set_title("Difficulty_EnemyChargeTime");
        rule->set_scope(RuleScope::DESCENDANTS);
        rule->set_target_class(godot::StringName("GamePiece"));
        rule->set_target_title_or_group(enemy_weapon_group_tag);
        rule->set_target_property(godot::StringName("ChargeTime"));
        rule->set_operation(RuleOp::MULTIPLY);
        rule->set_value(enemy_attack_charge_multiplier);
        rule->set_priority(30);
        collected.append(rule);
    }

    // 5. Player Health Scaling
    if (!godot::Math::is_equal_approx(player_health_multiplier, 1.0f) && !player_group_tag.is_empty()) {
        godot::Ref<GameplayRule> rule;
        rule.instantiate();
        rule->set_title("Difficulty_PlayerHealth");
        rule->set_scope(RuleScope::DESCENDANTS);
        rule->set_target_class(godot::StringName("GameEntity"));
        rule->set_target_title_or_group(player_group_tag);
        rule->set_target_property(godot::StringName("Health"));
        rule->set_operation(RuleOp::MULTIPLY);
        rule->set_value(player_health_multiplier);
        rule->set_priority(30);
        collected.append(rule);
    }

    // 6. Stealth Perception: VisionRange & HearingRange on Enemy GameAgents
    if (!godot::Math::is_equal_approx(ai_vision_range_multiplier, 1.0f) && !enemy_group_tag.is_empty()) {
        godot::Ref<GameplayRule> rule;
        rule.instantiate();
        rule->set_title("Difficulty_AIVisionRange");
        rule->set_scope(RuleScope::DESCENDANTS);
        rule->set_target_class(godot::StringName("GameAgent"));
        rule->set_target_title_or_group(enemy_group_tag);
        rule->set_target_property(godot::StringName("VisionRange"));
        rule->set_operation(RuleOp::MULTIPLY);
        rule->set_value(ai_vision_range_multiplier);
        rule->set_priority(30);
        collected.append(rule);
    }

    if (!godot::Math::is_equal_approx(ai_hearing_range_multiplier, 1.0f) && !enemy_group_tag.is_empty()) {
        godot::Ref<GameplayRule> rule;
        rule.instantiate();
        rule->set_title("Difficulty_AIHearingRange");
        rule->set_scope(RuleScope::DESCENDANTS);
        rule->set_target_class(godot::StringName("GameAgent"));
        rule->set_target_title_or_group(enemy_group_tag);
        rule->set_target_property(godot::StringName("HearingRange"));
        rule->set_operation(RuleOp::MULTIPLY);
        rule->set_value(ai_hearing_range_multiplier);
        rule->set_priority(30);
        collected.append(rule);
    }

    // 7. Puzzle Complexity Tier & Time Limit Scaling on Puzzle GameEntities / GameInteractions
    if (!puzzle_group_tag.is_empty()) {
        godot::Ref<GameplayRule> tier_rule;
        tier_rule.instantiate();
        tier_rule->set_title("Difficulty_PuzzleComplexityTier");
        tier_rule->set_scope(RuleScope::DESCENDANTS);
        tier_rule->set_target_class(godot::StringName("GameEntity"));
        tier_rule->set_target_title_or_group(puzzle_group_tag);
        tier_rule->set_target_property(godot::StringName("ComplexityTier"));
        tier_rule->set_operation(RuleOp::SET);
        tier_rule->set_value(puzzle_complexity_tier);
        tier_rule->set_priority(30);
        collected.append(tier_rule);

        if (!godot::Math::is_equal_approx(puzzle_time_limit_multiplier, 1.0f)) {
            godot::Ref<GameplayRule> time_rule;
            time_rule.instantiate();
            time_rule->set_title("Difficulty_PuzzleTimeLimit");
            time_rule->set_scope(RuleScope::DESCENDANTS);
            time_rule->set_target_class(godot::StringName("GameEntity"));
            time_rule->set_target_title_or_group(puzzle_group_tag);
            time_rule->set_target_property(godot::StringName("TimeLimit"));
            time_rule->set_operation(RuleOp::MULTIPLY);
            time_rule->set_value(puzzle_time_limit_multiplier);
            time_rule->set_priority(30);
            collected.append(time_rule);
        }
    }

    return collected;
}

godot::Dictionary GameplayDifficulty::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();

    data["scale_mode"] = static_cast<int>(scale_mode);
    data["overall_level"] = static_cast<int>(overall_level);
    data["combat_level"] = static_cast<int>(combat_level);
    data["puzzle_level"] = static_cast<int>(puzzle_level);
    data["stealth_level"] = static_cast<int>(stealth_level);
    data["survival_level"] = static_cast<int>(survival_level);
    data["custom_aspect_levels"] = custom_aspect_levels.duplicate();

    godot::Array tags_arr;
    for (int i = 0; i < difficulty_tags.size(); ++i) {
        tags_arr.append(difficulty_tags[i]);
    }
    data["difficulty_tags"] = tags_arr;

    data["allow_mid_game_change"] = allow_mid_game_change;
    data["dynamic_difficulty_adjustment"] = dynamic_difficulty_adjustment;
    data["dynamic_difficulty_bounds"] = dynamic_difficulty_bounds;

    data["enemy_health_multiplier"] = enemy_health_multiplier;
    data["enemy_damage_multiplier"] = enemy_damage_multiplier;
    data["enemy_speed_multiplier"] = enemy_speed_multiplier;
    data["enemy_aggression_multiplier"] = enemy_aggression_multiplier;
    data["enemy_attack_charge_multiplier"] = enemy_attack_charge_multiplier;
    data["player_damage_dealt_multiplier"] = player_damage_dealt_multiplier;
    data["player_health_multiplier"] = player_health_multiplier;
    data["friendly_fire_enabled"] = friendly_fire_enabled;

    data["puzzle_complexity_tier"] = puzzle_complexity_tier;
    data["puzzle_hint_availability"] = puzzle_hint_availability;
    data["puzzle_time_limit_multiplier"] = puzzle_time_limit_multiplier;
    data["allow_puzzle_skip"] = allow_puzzle_skip;
    data["waypoint_guidance_enabled"] = waypoint_guidance_enabled;
    data["map_auto_marking_enabled"] = map_auto_marking_enabled;

    data["ai_vision_range_multiplier"] = ai_vision_range_multiplier;
    data["ai_hearing_range_multiplier"] = ai_hearing_range_multiplier;
    data["ai_detection_speed_multiplier"] = ai_detection_speed_multiplier;
    data["alert_decay_rate_multiplier"] = alert_decay_rate_multiplier;

    data["loot_abundance_multiplier"] = loot_abundance_multiplier;
    data["healing_efficacy_multiplier"] = healing_efficacy_multiplier;
    data["stamina_drain_multiplier"] = stamina_drain_multiplier;
    data["hazard_damage_multiplier"] = hazard_damage_multiplier;
    data["save_limit_enabled"] = save_limit_enabled;
    data["permadeath_enabled"] = permadeath_enabled;

    data["enemy_group_tag"] = godot::String(enemy_group_tag);
    data["enemy_weapon_group_tag"] = godot::String(enemy_weapon_group_tag);
    data["player_group_tag"] = godot::String(player_group_tag);
    data["puzzle_group_tag"] = godot::String(puzzle_group_tag);

    return data;
}

void GameplayDifficulty::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("scale_mode")) {
        scale_mode = static_cast<DifficultyScaleMode>(static_cast<int>(p_data["scale_mode"]));
    }
    if (p_data.has("overall_level")) {
        overall_level = static_cast<DifficultyLevel>(static_cast<int>(p_data["overall_level"]));
    }
    if (p_data.has("combat_level")) {
        combat_level = static_cast<DifficultyLevel>(static_cast<int>(p_data["combat_level"]));
    }
    if (p_data.has("puzzle_level")) {
        puzzle_level = static_cast<DifficultyLevel>(static_cast<int>(p_data["puzzle_level"]));
    }
    if (p_data.has("stealth_level")) {
        stealth_level = static_cast<DifficultyLevel>(static_cast<int>(p_data["stealth_level"]));
    }
    if (p_data.has("survival_level")) {
        survival_level = static_cast<DifficultyLevel>(static_cast<int>(p_data["survival_level"]));
    }
    if (p_data.has("custom_aspect_levels") && p_data["custom_aspect_levels"].get_type() == godot::Variant::DICTIONARY) {
        custom_aspect_levels = godot::Dictionary(p_data["custom_aspect_levels"]).duplicate();
    }

    if (p_data.has("difficulty_tags") && p_data["difficulty_tags"].get_type() == godot::Variant::ARRAY) {
        difficulty_tags.clear();
        godot::Array arr = p_data["difficulty_tags"];
        for (int i = 0; i < arr.size(); ++i) {
            difficulty_tags.append(godot::String(arr[i]));
        }
    }

    if (p_data.has("allow_mid_game_change")) allow_mid_game_change = p_data["allow_mid_game_change"];
    if (p_data.has("dynamic_difficulty_adjustment")) dynamic_difficulty_adjustment = p_data["dynamic_difficulty_adjustment"];
    if (p_data.has("dynamic_difficulty_bounds")) set_dynamic_difficulty_bounds(p_data["dynamic_difficulty_bounds"]);

    if (p_data.has("enemy_health_multiplier")) set_enemy_health_multiplier(p_data["enemy_health_multiplier"]);
    if (p_data.has("enemy_damage_multiplier")) set_enemy_damage_multiplier(p_data["enemy_damage_multiplier"]);
    if (p_data.has("enemy_speed_multiplier")) set_enemy_speed_multiplier(p_data["enemy_speed_multiplier"]);
    if (p_data.has("enemy_aggression_multiplier")) set_enemy_aggression_multiplier(p_data["enemy_aggression_multiplier"]);
    if (p_data.has("enemy_attack_charge_multiplier")) set_enemy_attack_charge_multiplier(p_data["enemy_attack_charge_multiplier"]);
    if (p_data.has("player_damage_dealt_multiplier")) set_player_damage_dealt_multiplier(p_data["player_damage_dealt_multiplier"]);
    if (p_data.has("player_health_multiplier")) set_player_health_multiplier(p_data["player_health_multiplier"]);
    if (p_data.has("friendly_fire_enabled")) friendly_fire_enabled = p_data["friendly_fire_enabled"];

    if (p_data.has("puzzle_complexity_tier")) set_puzzle_complexity_tier(p_data["puzzle_complexity_tier"]);
    if (p_data.has("puzzle_hint_availability")) set_puzzle_hint_availability(p_data["puzzle_hint_availability"]);
    if (p_data.has("puzzle_time_limit_multiplier")) set_puzzle_time_limit_multiplier(p_data["puzzle_time_limit_multiplier"]);
    if (p_data.has("allow_puzzle_skip")) allow_puzzle_skip = p_data["allow_puzzle_skip"];
    if (p_data.has("waypoint_guidance_enabled")) waypoint_guidance_enabled = p_data["waypoint_guidance_enabled"];
    if (p_data.has("map_auto_marking_enabled")) map_auto_marking_enabled = p_data["map_auto_marking_enabled"];

    if (p_data.has("ai_vision_range_multiplier")) set_ai_vision_range_multiplier(p_data["ai_vision_range_multiplier"]);
    if (p_data.has("ai_hearing_range_multiplier")) set_ai_hearing_range_multiplier(p_data["ai_hearing_range_multiplier"]);
    if (p_data.has("ai_detection_speed_multiplier")) set_ai_detection_speed_multiplier(p_data["ai_detection_speed_multiplier"]);
    if (p_data.has("alert_decay_rate_multiplier")) set_alert_decay_rate_multiplier(p_data["alert_decay_rate_multiplier"]);

    if (p_data.has("loot_abundance_multiplier")) set_loot_abundance_multiplier(p_data["loot_abundance_multiplier"]);
    if (p_data.has("healing_efficacy_multiplier")) set_healing_efficacy_multiplier(p_data["healing_efficacy_multiplier"]);
    if (p_data.has("stamina_drain_multiplier")) set_stamina_drain_multiplier(p_data["stamina_drain_multiplier"]);
    if (p_data.has("hazard_damage_multiplier")) set_hazard_damage_multiplier(p_data["hazard_damage_multiplier"]);
    if (p_data.has("save_limit_enabled")) save_limit_enabled = p_data["save_limit_enabled"];
    if (p_data.has("permadeath_enabled")) permadeath_enabled = p_data["permadeath_enabled"];

    if (p_data.has("enemy_group_tag")) enemy_group_tag = godot::StringName(godot::String(p_data["enemy_group_tag"]));
    if (p_data.has("enemy_weapon_group_tag")) enemy_weapon_group_tag = godot::StringName(godot::String(p_data["enemy_weapon_group_tag"]));
    if (p_data.has("player_group_tag")) player_group_tag = godot::StringName(godot::String(p_data["player_group_tag"]));
    if (p_data.has("puzzle_group_tag")) puzzle_group_tag = godot::StringName(godot::String(p_data["puzzle_group_tag"]));

    _sync_difficulty_tags();
    emit_changed();
}

} // namespace ideam::godot_ext
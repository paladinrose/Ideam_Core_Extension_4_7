#pragma once

/**
 * smart_tool_profile.h
 * Tier 3 (`SmartTool`) serializable preference, template, and custom asset bank profile
 * (`src/godot/tools/smart/`).
 *
 * Stores:
 *   - User preferences (`preferences` Dictionary)
 *   - Custom template file paths (`custom_template_paths`)
 *   - Named custom tool asset banks (`custom_asset_banks`, e.g. canvas brushes, garment packs)
 *   - Semantic tags (`semantic_tags`) and `help_topic_id`
 *
 * Designed to be embedded inside a Tier 4 `WorkflowProfile` so a workflow can override a
 * SmartTool's normal user preferences strictly within that workflow's `ToolContext`.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

class SmartToolProfile : public godot::Resource {
    GDCLASS(SmartToolProfile, godot::Resource)

private:
    godot::StringName tool_id_;
    godot::String profile_name_ = "Default";
    godot::Dictionary preferences_;
    godot::PackedStringArray custom_template_paths_;
    godot::Dictionary custom_asset_banks_; // bank_name (String) -> PackedStringArray of res:// paths
    godot::PackedStringArray semantic_tags_;
    godot::StringName help_topic_id_;

protected:
    static void _bind_methods();

public:
    SmartToolProfile() = default;
    ~SmartToolProfile() override = default;

    void set_tool_id(const godot::StringName &p_id) { tool_id_ = p_id; }
    [[nodiscard]] godot::StringName get_tool_id() const { return tool_id_; }

    void set_profile_name(const godot::String &p_name) { profile_name_ = p_name; }
    [[nodiscard]] godot::String get_profile_name() const { return profile_name_; }

    void set_preferences(const godot::Dictionary &p_prefs) { preferences_ = p_prefs; }
    [[nodiscard]] godot::Dictionary get_preferences() const { return preferences_; }

    void set_preference(const godot::String &p_key, const godot::Variant &p_value);
    [[nodiscard]] godot::Variant get_preference(
        const godot::String &p_key,
        const godot::Variant &p_default = godot::Variant()
    ) const;

    void set_custom_template_paths(const godot::PackedStringArray &p_paths) { custom_template_paths_ = p_paths; }
    [[nodiscard]] godot::PackedStringArray get_custom_template_paths() const { return custom_template_paths_; }

    void set_custom_asset_banks(const godot::Dictionary &p_banks) { custom_asset_banks_ = p_banks; }
    [[nodiscard]] godot::Dictionary get_custom_asset_banks() const { return custom_asset_banks_; }
    [[nodiscard]] godot::PackedStringArray get_asset_bank(const godot::String &p_bank_name) const;
    void add_asset_to_bank(const godot::String &p_bank_name, const godot::String &p_asset_path);

    void set_semantic_tags(const godot::PackedStringArray &p_tags) { semantic_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const { return help_topic_id_; }

    // Produces a non-destructive merged copy where `p_override` keys win over this profile.
    [[nodiscard]] godot::Ref<SmartToolProfile> merged_with(const godot::Ref<SmartToolProfile> &p_override) const;
};

} // namespace ideam::godot_ext

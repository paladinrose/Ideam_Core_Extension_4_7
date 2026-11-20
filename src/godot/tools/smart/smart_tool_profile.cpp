#include "smart_tool_profile.h"

namespace ideam::godot_ext {

void SmartToolProfile::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tool_id", "id"), &SmartToolProfile::set_tool_id);
    ClassDB::bind_method(D_METHOD("get_tool_id"), &SmartToolProfile::get_tool_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tool_id"), "set_tool_id", "get_tool_id");

    ClassDB::bind_method(D_METHOD("set_profile_name", "name"), &SmartToolProfile::set_profile_name);
    ClassDB::bind_method(D_METHOD("get_profile_name"), &SmartToolProfile::get_profile_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "profile_name"), "set_profile_name", "get_profile_name");

    ClassDB::bind_method(D_METHOD("set_preferences", "preferences"), &SmartToolProfile::set_preferences);
    ClassDB::bind_method(D_METHOD("get_preferences"), &SmartToolProfile::get_preferences);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "preferences"), "set_preferences", "get_preferences");

    ClassDB::bind_method(D_METHOD("set_preference", "key", "value"), &SmartToolProfile::set_preference);
    ClassDB::bind_method(
        D_METHOD("get_preference", "key", "default_value"),
        &SmartToolProfile::get_preference,
        DEFVAL(Variant())
    );

    ClassDB::bind_method(
        D_METHOD("set_custom_template_paths", "paths"),
        &SmartToolProfile::set_custom_template_paths
    );
    ClassDB::bind_method(D_METHOD("get_custom_template_paths"), &SmartToolProfile::get_custom_template_paths);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_STRING_ARRAY, "custom_template_paths"),
        "set_custom_template_paths",
        "get_custom_template_paths"
    );

    ClassDB::bind_method(D_METHOD("set_custom_asset_banks", "banks"), &SmartToolProfile::set_custom_asset_banks);
    ClassDB::bind_method(D_METHOD("get_custom_asset_banks"), &SmartToolProfile::get_custom_asset_banks);
    ADD_PROPERTY(
        PropertyInfo(Variant::DICTIONARY, "custom_asset_banks"),
        "set_custom_asset_banks",
        "get_custom_asset_banks"
    );
    ClassDB::bind_method(D_METHOD("get_asset_bank", "bank_name"), &SmartToolProfile::get_asset_bank);
    ClassDB::bind_method(
        D_METHOD("add_asset_to_bank", "bank_name", "asset_path"),
        &SmartToolProfile::add_asset_to_bank
    );

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &SmartToolProfile::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &SmartToolProfile::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &SmartToolProfile::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &SmartToolProfile::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("merged_with", "override_profile"), &SmartToolProfile::merged_with);
}

void SmartToolProfile::set_preference(const godot::String &p_key, const godot::Variant &p_value) {
    preferences_[p_key] = p_value;
}

godot::Variant SmartToolProfile::get_preference(
    const godot::String &p_key,
    const godot::Variant &p_default
) const {
    if (preferences_.has(p_key)) {
        return preferences_[p_key];
    }
    return p_default;
}

godot::PackedStringArray SmartToolProfile::get_asset_bank(const godot::String &p_bank_name) const {
    if (custom_asset_banks_.has(p_bank_name)) {
        return custom_asset_banks_[p_bank_name];
    }
    return godot::PackedStringArray();
}

void SmartToolProfile::add_asset_to_bank(const godot::String &p_bank_name, const godot::String &p_asset_path) {
    if (p_bank_name.is_empty() || p_asset_path.is_empty()) {
        return;
    }
    godot::PackedStringArray paths = get_asset_bank(p_bank_name);
    if (!paths.has(p_asset_path)) {
        paths.push_back(p_asset_path);
        custom_asset_banks_[p_bank_name] = paths;
    }
}

godot::Ref<SmartToolProfile> SmartToolProfile::merged_with(
    const godot::Ref<SmartToolProfile> &p_override
) const {
    godot::Ref<SmartToolProfile> result;
    result.instantiate();
    result->set_tool_id(tool_id_);
    result->set_profile_name(profile_name_);
    result->set_preferences(preferences_.duplicate(true));
    result->set_custom_template_paths(custom_template_paths_);
    result->set_custom_asset_banks(custom_asset_banks_.duplicate(true));
    result->set_semantic_tags(semantic_tags_);
    result->set_help_topic_id(help_topic_id_);

    if (p_override.is_null()) {
        return result;
    }

    if (!p_override->get_profile_name().is_empty()) {
        result->set_profile_name(p_override->get_profile_name());
    }

    godot::Dictionary merged_prefs = result->get_preferences();
    const godot::Dictionary ov_prefs = p_override->get_preferences();
    const godot::Array pref_keys = ov_prefs.keys();
    for (int i = 0; i < pref_keys.size(); ++i) {
        merged_prefs[pref_keys[i]] = ov_prefs[pref_keys[i]];
    }
    result->set_preferences(merged_prefs);

    if (!p_override->get_custom_template_paths().is_empty()) {
        result->set_custom_template_paths(p_override->get_custom_template_paths());
    }

    godot::Dictionary merged_banks = result->get_custom_asset_banks();
    const godot::Dictionary ov_banks = p_override->get_custom_asset_banks();
    const godot::Array bank_keys = ov_banks.keys();
    for (int i = 0; i < bank_keys.size(); ++i) {
        merged_banks[bank_keys[i]] = ov_banks[bank_keys[i]];
    }
    result->set_custom_asset_banks(merged_banks);

    if (!p_override->get_semantic_tags().is_empty()) {
        result->set_semantic_tags(p_override->get_semantic_tags());
    }
    if (p_override->get_help_topic_id() != godot::StringName()) {
        result->set_help_topic_id(p_override->get_help_topic_id());
    }

    return result;
}

} // namespace ideam::godot_ext

#include "gameplay_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "../game_entity.h"
#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayResource::_bind_methods() {
    // --- Identity & Metadata ---
    ADD_GROUP("Identity", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &GameplayResource::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &GameplayResource::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");

    godot::ClassDB::bind_method(godot::D_METHOD("set_resource_id", "resource_id"), &GameplayResource::set_resource_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resource_id"), &GameplayResource::get_resource_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_id"), &GameplayResource::get_effective_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "resource_id"), "set_resource_id", "get_resource_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_description", "description"), &GameplayResource::set_description);
    godot::ClassDB::bind_method(godot::D_METHOD("get_description"), &GameplayResource::get_description);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING, "description", godot::PROPERTY_HINT_MULTILINE_TEXT),
        "set_description",
        "get_description"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_category", "category"), &GameplayResource::set_category);
    godot::ClassDB::bind_method(godot::D_METHOD("get_category"), &GameplayResource::get_category);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "category"), "set_category", "get_category");

    godot::ClassDB::bind_method(godot::D_METHOD("set_enabled", "enabled"), &GameplayResource::set_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enabled"), &GameplayResource::get_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "enabled"), "set_enabled", "get_enabled");

    // --- Tags & Intersection Compatibility ---
    ADD_GROUP("Tags & Compatibility", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_option_tags", "tags"), &GameplayResource::set_option_tags);
    godot::ClassDB::bind_method(godot::D_METHOD("get_option_tags"), &GameplayResource::get_option_tags);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "option_tags", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_option_tags",
        "get_option_tags"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_required_tags", "tags"), &GameplayResource::set_required_tags);
    godot::ClassDB::bind_method(godot::D_METHOD("get_required_tags"), &GameplayResource::get_required_tags);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "required_tags", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_required_tags",
        "get_required_tags"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_excluded_tags", "tags"), &GameplayResource::set_excluded_tags);
    godot::ClassDB::bind_method(godot::D_METHOD("get_excluded_tags"), &GameplayResource::get_excluded_tags);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "excluded_tags", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_excluded_tags",
        "get_excluded_tags"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("has_tag", "tag"), &GameplayResource::has_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("add_tag", "tag"), &GameplayResource::add_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_tag", "tag"), &GameplayResource::remove_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("is_compatible_with_tags", "active_tags"), &GameplayResource::is_compatible_with_tags);
    godot::ClassDB::bind_method(godot::D_METHOD("is_compatible_with_resource", "other"), &GameplayResource::is_compatible_with_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("matches_identity", "other"), &GameplayResource::matches_identity);

    // --- Cascading Rules Integration ---
    ADD_GROUP("Rules", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_associated_rules", "rules"), &GameplayResource::set_associated_rules);
    godot::ClassDB::bind_method(godot::D_METHOD("get_associated_rules"), &GameplayResource::get_associated_rules);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "associated_rules", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayRule"),
        "set_associated_rules",
        "get_associated_rules"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("add_associated_rule", "rule"), &GameplayResource::add_associated_rule);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_associated_rule", "rule"), &GameplayResource::remove_associated_rule);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_associated_rules"), &GameplayResource::clear_associated_rules);
    godot::ClassDB::bind_method(godot::D_METHOD("collect_rules"), &GameplayResource::collect_rules);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_rules_to_entity", "entity", "is_self"), &GameplayResource::apply_rules_to_entity, DEFVAL(true));

    // --- Persistence & ConfigFile Serialization ---
    ADD_GROUP("Persistence", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_config_section", "section"), &GameplayResource::set_config_section);
    godot::ClassDB::bind_method(godot::D_METHOD("get_config_section"), &GameplayResource::get_config_section);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "config_section"), "set_config_section", "get_config_section");

    godot::ClassDB::bind_method(godot::D_METHOD("apply_resource", "other"), &GameplayResource::apply_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("save_resource"), &GameplayResource::save_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("load_resource", "data"), &GameplayResource::load_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("save_to_config", "config", "section"), &GameplayResource::save_to_config, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("load_from_config", "config", "section"), &GameplayResource::load_from_config, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("save_to_config_file", "path", "section"), &GameplayResource::save_to_config_file, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("load_from_config_file", "path", "section"), &GameplayResource::load_from_config_file, DEFVAL(""));
}

GameplayResource::GameplayResource() {
    // DOD NOTE: Keeping constructor overhead minimal to support fast
    // instantiation during scene/resource loading.
}

GameplayResource::~GameplayResource() {
}

// --- Identity & Metadata Setters / Getters ---

void GameplayResource::set_title(const godot::String &p_title) {
    if (p_title == title) return;
    title = p_title;
    emit_changed();
}

godot::String GameplayResource::get_title() const {
    return title;
}

void GameplayResource::set_resource_id(const godot::StringName &p_id) {
    if (p_id == resource_id) return;
    resource_id = p_id;
    emit_changed();
}

godot::StringName GameplayResource::get_resource_id() const {
    return resource_id;
}

godot::StringName GameplayResource::get_effective_id() const {
    if (!resource_id.is_empty()) {
        return resource_id;
    }
    if (!title.is_empty()) {
        return godot::StringName(title);
    }
    return godot::StringName();
}

void GameplayResource::set_description(const godot::String &p_description) {
    if (p_description == description) return;
    description = p_description;
    emit_changed();
}

godot::String GameplayResource::get_description() const {
    return description;
}

void GameplayResource::set_category(const godot::StringName &p_category) {
    if (p_category == category) return;
    category = p_category;
    emit_changed();
}

godot::StringName GameplayResource::get_category() const {
    return category;
}

void GameplayResource::set_enabled(bool p_enabled) {
    if (p_enabled == enabled) return;
    enabled = p_enabled;
    emit_changed();
}

bool GameplayResource::get_enabled() const {
    return enabled;
}

// --- Tags & Intersection Compatibility ---

void GameplayResource::set_option_tags(const godot::TypedArray<godot::StringName> &p_tags) {
    if (p_tags == option_tags) return;
    option_tags = p_tags;
    emit_changed();
}

godot::TypedArray<godot::StringName> GameplayResource::get_option_tags() const {
    return option_tags;
}

void GameplayResource::set_required_tags(const godot::TypedArray<godot::StringName> &p_tags) {
    if (p_tags == required_tags) return;
    required_tags = p_tags;
    emit_changed();
}

godot::TypedArray<godot::StringName> GameplayResource::get_required_tags() const {
    return required_tags;
}

void GameplayResource::set_excluded_tags(const godot::TypedArray<godot::StringName> &p_tags) {
    if (p_tags == excluded_tags) return;
    excluded_tags = p_tags;
    emit_changed();
}

godot::TypedArray<godot::StringName> GameplayResource::get_excluded_tags() const {
    return excluded_tags;
}

bool GameplayResource::has_tag(const godot::StringName &p_tag) const {
    if (p_tag.is_empty()) return false;
    return option_tags.has(p_tag);
}

void GameplayResource::add_tag(const godot::StringName &p_tag) {
    if (p_tag.is_empty() || option_tags.has(p_tag)) return;
    option_tags.append(p_tag);
    emit_changed();
}

void GameplayResource::remove_tag(const godot::StringName &p_tag) {
    int idx = option_tags.find(p_tag);
    if (idx >= 0) {
        option_tags.remove_at(idx);
        emit_changed();
    }
}

bool GameplayResource::is_compatible_with_tags(const godot::TypedArray<godot::StringName> &p_active_tags) const {
    // 1. Every required_tag must be present in p_active_tags
    for (int i = 0; i < required_tags.size(); ++i) {
        godot::StringName req = required_tags[i];
        if (!req.is_empty() && !p_active_tags.has(req)) {
            return false;
        }
    }

    // 2. No excluded_tag may be present in p_active_tags
    for (int i = 0; i < excluded_tags.size(); ++i) {
        godot::StringName excl = excluded_tags[i];
        if (!excl.is_empty() && p_active_tags.has(excl)) {
            return false;
        }
    }

    return true;
}

bool GameplayResource::is_compatible_with_resource(const godot::Ref<GameplayResource> &p_other) const {
    if (!p_other.is_valid()) {
        return false;
    }

    // Check if either resource explicitly excludes a tag emitted by the other
    godot::TypedArray<godot::StringName> other_tags = p_other->get_option_tags();
    for (int i = 0; i < excluded_tags.size(); ++i) {
        godot::StringName excl = excluded_tags[i];
        if (!excl.is_empty() && other_tags.has(excl)) {
            return false;
        }
    }

    godot::TypedArray<godot::StringName> other_excluded = p_other->get_excluded_tags();
    for (int i = 0; i < option_tags.size(); ++i) {
        godot::StringName tag = option_tags[i];
        if (!tag.is_empty() && other_excluded.has(tag)) {
            return false;
        }
    }

    return true;
}

bool GameplayResource::matches_identity(const godot::Ref<GameplayResource> &p_other) const {
    if (!p_other.is_valid()) {
        return false;
    }
    godot::StringName my_id = get_effective_id();
    godot::StringName other_id = p_other->get_effective_id();
    if (!my_id.is_empty() && !other_id.is_empty()) {
        return my_id == other_id;
    }
    return title == p_other->get_title();
}

// --- Cascading Rules Integration ---

void GameplayResource::set_associated_rules(const godot::TypedArray<GameplayRule> &p_rules) {
    if (p_rules == associated_rules) return;
    associated_rules = p_rules;
    emit_changed();
}

godot::TypedArray<GameplayRule> GameplayResource::get_associated_rules() const {
    return associated_rules;
}

void GameplayResource::add_associated_rule(const godot::Ref<GameplayRule> &p_rule) {
    if (!p_rule.is_valid()) return;
    associated_rules.append(p_rule);
    emit_changed();
}

void GameplayResource::remove_associated_rule(const godot::Ref<GameplayRule> &p_rule) {
    if (!p_rule.is_valid()) return;
    int idx = associated_rules.find(p_rule);
    if (idx >= 0) {
        associated_rules.remove_at(idx);
        emit_changed();
    }
}

void GameplayResource::clear_associated_rules() {
    if (associated_rules.is_empty()) return;
    associated_rules.clear();
    emit_changed();
}

godot::TypedArray<GameplayRule> GameplayResource::collect_rules() const {
    godot::TypedArray<GameplayRule> collected;
    if (!enabled) {
        return collected;
    }

    for (int i = 0; i < associated_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(associated_rules[i]));
        if (rule.is_valid() && rule->get_enabled()) {
            collected.append(rule);
        }
    }
    return collected;
}

int GameplayResource::apply_rules_to_entity(GameEntity *p_entity, bool p_is_self) const {
    if (!p_entity || !enabled) {
        return 0;
    }

    godot::TypedArray<GameplayRule> active_rules = collect_rules();
    int applied_count = 0;
    for (int i = 0; i < active_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(active_rules[i]));
        if (rule.is_valid() && rule->apply_to_entity(p_entity, p_is_self)) {
            ++applied_count;
        }
    }
    return applied_count;
}

// --- Persistence & ConfigFile Serialization ---

void GameplayResource::set_config_section(const godot::String &p_section) {
    if (p_section == config_section) return;
    config_section = p_section;
    emit_changed();
}

godot::String GameplayResource::get_config_section() const {
    return config_section;
}

void GameplayResource::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }

    if (!p_other->get_title().is_empty()) {
        title = p_other->get_title();
    }
    if (!p_other->get_resource_id().is_empty()) {
        resource_id = p_other->get_resource_id();
    }
    if (!p_other->get_description().is_empty()) {
        description = p_other->get_description();
    }
    if (!p_other->get_category().is_empty()) {
        category = p_other->get_category();
    }
    enabled = p_other->get_enabled();

    if (!p_other->get_option_tags().is_empty()) {
        option_tags = p_other->get_option_tags().duplicate();
    }
    if (!p_other->get_required_tags().is_empty()) {
        required_tags = p_other->get_required_tags().duplicate();
    }
    if (!p_other->get_excluded_tags().is_empty()) {
        excluded_tags = p_other->get_excluded_tags().duplicate();
    }
    if (!p_other->get_associated_rules().is_empty()) {
        associated_rules = p_other->get_associated_rules().duplicate();
    }
    if (!p_other->get_config_section().is_empty()) {
        config_section = p_other->get_config_section();
    }

    emit_changed();
}

godot::Dictionary GameplayResource::save_resource() const {
    godot::Dictionary data;
    data["title"] = title;
    data["resource_id"] = godot::String(resource_id);
    data["description"] = description;
    data["category"] = godot::String(category);
    data["enabled"] = enabled;
    data["config_section"] = config_section;

    godot::Array tags_arr;
    for (int i = 0; i < option_tags.size(); ++i) {
        tags_arr.append(godot::String(godot::StringName(option_tags[i])));
    }
    data["option_tags"] = tags_arr;

    godot::Array req_arr;
    for (int i = 0; i < required_tags.size(); ++i) {
        req_arr.append(godot::String(godot::StringName(required_tags[i])));
    }
    data["required_tags"] = req_arr;

    godot::Array excl_arr;
    for (int i = 0; i < excluded_tags.size(); ++i) {
        excl_arr.append(godot::String(godot::StringName(excluded_tags[i])));
    }
    data["excluded_tags"] = excl_arr;

    godot::Array rules_arr;
    for (int i = 0; i < associated_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(associated_rules[i]));
        if (rule.is_valid()) {
            rules_arr.append(rule->save_rule());
        }
    }
    data["associated_rules"] = rules_arr;

    return data;
}

void GameplayResource::load_resource(const godot::Dictionary &p_data) {
    if (p_data.has("title")) {
        title = p_data["title"];
    }
    if (p_data.has("resource_id")) {
        resource_id = godot::StringName(godot::String(p_data["resource_id"]));
    }
    if (p_data.has("description")) {
        description = p_data["description"];
    }
    if (p_data.has("category")) {
        category = godot::StringName(godot::String(p_data["category"]));
    }
    if (p_data.has("enabled")) {
        enabled = p_data["enabled"];
    }
    if (p_data.has("config_section")) {
        config_section = p_data["config_section"];
    }

    if (p_data.has("option_tags") && p_data["option_tags"].get_type() == godot::Variant::ARRAY) {
        option_tags.clear();
        godot::Array arr = p_data["option_tags"];
        for (int i = 0; i < arr.size(); ++i) {
            option_tags.append(godot::StringName(godot::String(arr[i])));
        }
    }

    if (p_data.has("required_tags") && p_data["required_tags"].get_type() == godot::Variant::ARRAY) {
        required_tags.clear();
        godot::Array arr = p_data["required_tags"];
        for (int i = 0; i < arr.size(); ++i) {
            required_tags.append(godot::StringName(godot::String(arr[i])));
        }
    }

    if (p_data.has("excluded_tags") && p_data["excluded_tags"].get_type() == godot::Variant::ARRAY) {
        excluded_tags.clear();
        godot::Array arr = p_data["excluded_tags"];
        for (int i = 0; i < arr.size(); ++i) {
            excluded_tags.append(godot::StringName(godot::String(arr[i])));
        }
    }

    if (p_data.has("associated_rules") && p_data["associated_rules"].get_type() == godot::Variant::ARRAY) {
        associated_rules.clear();
        godot::Array arr = p_data["associated_rules"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayRule> rule;
                rule.instantiate();
                rule->load_rule(arr[i]);
                associated_rules.append(rule);
            }
        }
    }

    emit_changed();
}

bool GameplayResource::save_to_config(const godot::Ref<godot::ConfigFile> &p_config, const godot::String &p_section) const {
    if (!p_config.is_valid()) {
        return false;
    }

    godot::String sec = p_section.is_empty() ? config_section : p_section;
    if (sec.is_empty()) {
        sec = "gameplay";
    }

    godot::Dictionary data = save_resource();
    godot::Array keys = data.keys();
    for (int i = 0; i < keys.size(); ++i) {
        godot::String key = keys[i];
        p_config->set_value(sec, key, data[key]);
    }
    return true;
}

bool GameplayResource::load_from_config(const godot::Ref<godot::ConfigFile> &p_config, const godot::String &p_section) {
    if (!p_config.is_valid()) {
        return false;
    }

    godot::String sec = p_section.is_empty() ? config_section : p_section;
    if (sec.is_empty()) {
        sec = "gameplay";
    }

    if (!p_config->has_section(sec)) {
        return false;
    }

    godot::Dictionary data = save_resource();
    godot::PackedStringArray section_keys = p_config->get_section_keys(sec);
    for (int i = 0; i < section_keys.size(); ++i) {
        const godot::String &key = section_keys[i];
        data[key] = p_config->get_value(sec, key);
    }

    load_resource(data);
    return true;
}

godot::Error GameplayResource::save_to_config_file(const godot::String &p_path, const godot::String &p_section) const {
    if (p_path.is_empty()) {
        return godot::ERR_INVALID_PARAMETER;
    }

    godot::Ref<godot::ConfigFile> cfg;
    cfg.instantiate();
    // Preserve existing sections in the file if it already exists on disk
    cfg->load(p_path);
    save_to_config(cfg, p_section);
    return cfg->save(p_path);
}

godot::Error GameplayResource::load_from_config_file(const godot::String &p_path, const godot::String &p_section) {
    if (p_path.is_empty()) {
        return godot::ERR_INVALID_PARAMETER;
    }

    godot::Ref<godot::ConfigFile> cfg;
    cfg.instantiate();
    godot::Error err = cfg->load(p_path);
    if (err != godot::OK) {
        return err;
    }

    if (!load_from_config(cfg, p_section)) {
        return godot::ERR_DOES_NOT_EXIST;
    }
    return godot::OK;
}

} // namespace ideam::godot_ext
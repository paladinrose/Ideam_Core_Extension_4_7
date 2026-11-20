#include "tutorial_context_indexer.h"

#include <godot_cpp/classes/config_file.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void TutorialContextIndexer::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("register_tutorial", "tutorial"), &TutorialContextIndexer::register_tutorial);
    ClassDB::bind_method(D_METHOD("unregister_tutorial", "tutorial_id"), &TutorialContextIndexer::unregister_tutorial);
    ClassDB::bind_method(D_METHOD("clear_tutorials"), &TutorialContextIndexer::clear_tutorials);

    ClassDB::bind_method(D_METHOD("find_tutorial_by_id", "tutorial_id"), &TutorialContextIndexer::find_tutorial_by_id);
    ClassDB::bind_method(
        D_METHOD("find_tutorial_for_help_topic", "help_topic_id"),
        &TutorialContextIndexer::find_tutorial_for_help_topic
    );

    ClassDB::bind_method(
        D_METHOD("get_active_ecosystem_plugins"),
        &TutorialContextIndexer::get_active_ecosystem_plugins
    );
    ClassDB::bind_method(D_METHOD("build_context_snapshot"), &TutorialContextIndexer::build_context_snapshot);
}

void TutorialContextIndexer::register_tutorial(const godot::Ref<TutorialResource> &p_tutorial) {
    if (p_tutorial.is_null() || p_tutorial->get_tutorial_id() == godot::StringName()) {
        return;
    }
    for (auto &existing : tutorials_) {
        if (existing.is_valid() && existing->get_tutorial_id() == p_tutorial->get_tutorial_id()) {
            existing = p_tutorial;
            return;
        }
    }
    tutorials_.push_back(p_tutorial);
}

void TutorialContextIndexer::unregister_tutorial(const godot::StringName &p_tutorial_id) {
    std::erase_if(tutorials_, [&](const godot::Ref<TutorialResource> &t) {
        return t.is_null() || t->get_tutorial_id() == p_tutorial_id;
    });
}

void TutorialContextIndexer::clear_tutorials() {
    tutorials_.clear();
}

godot::Ref<TutorialResource> TutorialContextIndexer::find_tutorial_by_id(
    const godot::StringName &p_tutorial_id
) const {
    for (const auto &t : tutorials_) {
        if (t.is_valid() && t->get_tutorial_id() == p_tutorial_id) {
            return t;
        }
    }
    return godot::Ref<TutorialResource>();
}

godot::Ref<TutorialResource> TutorialContextIndexer::find_tutorial_for_help_topic(
    const godot::StringName &p_help_topic_id
) const {
    if (p_help_topic_id == godot::StringName()) {
        return godot::Ref<TutorialResource>();
    }
    const godot::String topic_str(p_help_topic_id);
    for (const auto &t : tutorials_) {
        if (t.is_null()) {
            continue;
        }
        if (t->get_tutorial_id() == p_help_topic_id || t->get_help_topic_ids().has(topic_str)) {
            return t;
        }
    }
    return godot::Ref<TutorialResource>();
}

godot::PackedStringArray TutorialContextIndexer::get_active_ecosystem_plugins() const {
    godot::PackedStringArray active;
    godot::Ref<godot::ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load("res://.ideam_registry.cfg") == godot::OK && cfg->has_section("ActivePlugins")) {
        const godot::PackedStringArray keys = cfg->get_section_keys("ActivePlugins");
        for (int i = 0; i < keys.size(); ++i) {
            if (static_cast<bool>(cfg->get_value("ActivePlugins", keys[i], false))) {
                active.push_back(keys[i]);
            }
        }
    }
    return active;
}

godot::Dictionary TutorialContextIndexer::build_context_snapshot() const {
    godot::Dictionary snap;
    snap["active_plugins"] = get_active_ecosystem_plugins();
    snap["tools"]          = ToolRegistry::get_singleton().export_tool_catalog_schema();

    godot::Array tut_list;
    for (const auto &t : tutorials_) {
        if (t.is_null()) {
            continue;
        }
        godot::Dictionary entry;
        entry["tutorial_id"]    = godot::String(t->get_tutorial_id());
        entry["title"]          = t->get_title();
        entry["description"]    = t->get_description();
        entry["help_topic_ids"] = t->get_help_topic_ids();
        entry["semantic_tags"]  = t->get_semantic_tags();
        entry["step_count"]     = t->get_steps().size();
        tut_list.append(entry);
    }
    snap["tutorials"] = tut_list;
    return snap;
}

} // namespace ideam::godot_ext

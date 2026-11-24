#include "linguistic_object_profiler.h"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/script.hpp>
#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_set>

namespace ideam::godot_ext {

namespace {

[[nodiscard]] bool is_generic_base_class(const godot::StringName &p_class) {
    const godot::String s(p_class);
    return s.is_empty() || s == "Object" || s == "RefCounted" ||
           s == "Resource" || s == "Node" || s == "InstancePlaceholder";
}

[[nodiscard]] bool is_boilerplate_method_name(const godot::String &p_name) {
    if (p_name.is_empty() || p_name.begins_with("_") ||
        p_name.begins_with("set_") || p_name.begins_with("get_")) {
        return true;
    }
    return godot::ClassDB::class_has_method("Object", p_name) ||
           godot::ClassDB::class_has_method("Node", p_name) ||
           godot::ClassDB::class_has_method("Resource", p_name) ||
           godot::ClassDB::class_has_method("RefCounted", p_name) ||
           godot::ClassDB::class_has_method("CanvasItem", p_name) ||
           godot::ClassDB::class_has_method("Node3D", p_name) ||
           godot::ClassDB::class_has_method("Control", p_name);
}

[[nodiscard]] bool is_boilerplate_signal_name(const godot::String &p_name) {
    if (p_name.is_empty() || p_name.begins_with("_")) {
        return true;
    }
    return p_name == "script_changed" || p_name == "property_list_changed" ||
           p_name == "tree_entered" || p_name == "tree_exiting" ||
           p_name == "tree_exited" || p_name == "ready" ||
           p_name == "renamed" || p_name == "child_entered_tree" ||
           p_name == "child_exiting_tree" || p_name == "child_order_changed" ||
           p_name == "replacing_by" || p_name == "editor_state_changed";
}

[[nodiscard]] bool is_boilerplate_property_name(const godot::String &p_name) {
    if (p_name.is_empty() || p_name.begins_with("_") || p_name.contains("/")) {
        return true;
    }
    return p_name == "script" || p_name == "resource_local_to_scene" ||
           p_name == "resource_path" || p_name == "resource_name" ||
           p_name == "resource_scene_unique_id" || p_name == "owner" ||
           p_name == "multiplayer" || p_name == "process_mode" ||
           p_name == "process_priority" || p_name == "process_physics_priority" ||
           p_name == "process_thread_group" || p_name == "physics_interpolation_mode" ||
           p_name == "auto_translate_mode" || p_name == "editor_description";
}

// src/godot/linguistics/linguistic_object_profiler.cpp (after line 55)
[[nodiscard]] bool is_engine_base_property(const godot::String &p_name) {
    static const std::unordered_set<std::string> base_props = []() {
        std::unordered_set<std::string> props;
        const char *classes[] = { "Node3D", "Node2D", "Control", "CanvasItem" };
        for (const char *cls : classes) {
            const godot::TypedArray<godot::Dictionary> plist =
                godot::ClassDB::class_get_property_list(cls, false);
            for (int i = 0; i < plist.size(); ++i) {
                const godot::Dictionary entry = plist[i];
                const godot::String prop_name = entry.get("name", "");
                if (!prop_name.is_empty()) {
                    props.insert(prop_name.utf8().get_data());
                }
            }
        }
        return props;
    }();
    return base_props.find(p_name.utf8().get_data()) != base_props.end();
}

void map_godot_class_to_structural_semes(
    const godot::String &p_class_name,
    const godot::Ref<SemanticTagSetResource> &r_tag_set
) {
    if (p_class_name.contains("3D")) {
        r_tag_set->add_nuclear_tag("3d");
    } else if (p_class_name.contains("2D")) {
        r_tag_set->add_nuclear_tag("2d");
    } else if (p_class_name.contains("4D")) {
        r_tag_set->add_nuclear_tag("4d");
    } else if (p_class_name.contains("1D")) {
        r_tag_set->add_nuclear_tag("1d");
    }

    if (p_class_name.begins_with("XR") || p_class_name.contains("OpenXR")) {
        r_tag_set->add_nuclear_tag("spatial_xr_immersive");
        r_tag_set->add_nuclear_tag("3d");
    }

    if (p_class_name == "CharacterBody3D" || p_class_name == "CharacterBody2D" ||
        p_class_name == "GameAgent" || p_class_name == "MindAgentNode" ||
        p_class_name == "Character" || p_class_name == "LoomisCharacterNode3D") {
        r_tag_set->add_nuclear_tag("agent");
        r_tag_set->add_nuclear_tag("movable");
    } else if (p_class_name == "RigidBody3D" || p_class_name == "RigidBody2D" ||
               p_class_name == "AnimatableBody3D" || p_class_name == "AnimatableBody2D") {
        r_tag_set->add_nuclear_tag("prop");
        r_tag_set->add_nuclear_tag("movable");
    } else if (p_class_name == "StaticBody3D" || p_class_name == "StaticBody2D" ||
               p_class_name.begins_with("CSG") || p_class_name == "SemanticStructure3D" ||
               p_class_name == "Container3D") {
        r_tag_set->add_nuclear_tag("structure");
    } else if (p_class_name == "Area3D" || p_class_name == "Area2D" ||
               p_class_name == "RayCast3D" || p_class_name == "RayCast2D" ||
               p_class_name == "ShapeCast3D" || p_class_name == "ShapeCast2D") {
        r_tag_set->add_nuclear_tag("sensor");
    } else if (p_class_name.contains("Light")) {
        r_tag_set->add_nuclear_tag("light");
    } else if (p_class_name.contains("AudioStreamPlayer") ||
               p_class_name.contains("CommunicationTransmitter") ||
               p_class_name.contains("CommunicationReceiver")) {
        r_tag_set->add_nuclear_tag("audio");
        r_tag_set->add_peripheral_tag("communicative");
    } else if (p_class_name == "Label" || p_class_name == "Label3D" ||
               p_class_name == "RichTextLabel") {
        r_tag_set->add_nuclear_tag("ui_display_text");
        r_tag_set->add_peripheral_tag("communicative");
    } else if (p_class_name == "LineEdit" || p_class_name == "TextEdit" ||
               p_class_name == "CodeEdit") {
        r_tag_set->add_nuclear_tag("ui_text_input");
        r_tag_set->add_nuclear_tag("interactive");
    } else if (p_class_name.contains("Button") || p_class_name == "CheckBox") {
        r_tag_set->add_nuclear_tag("ui_action_trigger");
        r_tag_set->add_nuclear_tag("interactive");
    } else if (p_class_name == "Slider" || p_class_name == "HSlider" ||
               p_class_name == "VSlider" || p_class_name == "SpinBox" ||
               p_class_name == "ProgressBar" || p_class_name == "CircularGauge" ||
               p_class_name == "GamePropertyGauge") {
        r_tag_set->add_nuclear_tag("ui_value_gauge");
    }
}

} // namespace

void LinguisticObjectProfiler::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_prompt_parser", "parser"), &LinguisticObjectProfiler::set_prompt_parser);
    ClassDB::bind_method(D_METHOD("get_prompt_parser"), &LinguisticObjectProfiler::get_prompt_parser);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "prompt_parser", PROPERTY_HINT_RESOURCE_TYPE, "SemanticPromptParser"),
        "set_prompt_parser",
        "get_prompt_parser"
    );

    ClassDB::bind_method(D_METHOD("set_metadata_manager", "manager"), &LinguisticObjectProfiler::set_metadata_manager);
    ClassDB::bind_method(D_METHOD("get_metadata_manager"), &LinguisticObjectProfiler::get_metadata_manager);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "metadata_manager", PROPERTY_HINT_RESOURCE_TYPE, "MetadataLinguisticsManager"),
        "set_metadata_manager",
        "get_metadata_manager"
    );

    ClassDB::bind_method(D_METHOD("set_enabled_channels_mask", "mask"), &LinguisticObjectProfiler::set_enabled_channels_mask);
    ClassDB::bind_method(D_METHOD("get_enabled_channels_mask"), &LinguisticObjectProfiler::get_enabled_channels_mask);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "enabled_channels_mask",
            PROPERTY_HINT_FLAGS,
            "Class Hierarchy:1,Object & Node Name:2,Node Groups:4,UI Text Controls:8,Reflected Properties:16,Reflected Methods:32,Reflected Signals:64,Object Metadata:128,Explicit Resources:256,SceneState:512"
        ),
        "set_enabled_channels_mask",
        "get_enabled_channels_mask"
    );

    ClassDB::bind_method(D_METHOD("set_max_inheritance_depth", "depth"), &LinguisticObjectProfiler::set_max_inheritance_depth);
    ClassDB::bind_method(D_METHOD("get_max_inheritance_depth"), &LinguisticObjectProfiler::get_max_inheritance_depth);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "max_inheritance_depth", PROPERTY_HINT_RANGE, "1,16,1"),
        "set_max_inheritance_depth",
        "get_max_inheritance_depth"
    );

    ClassDB::bind_method(D_METHOD("set_max_ui_items_to_sample", "max_items"), &LinguisticObjectProfiler::set_max_ui_items_to_sample);
    ClassDB::bind_method(D_METHOD("get_max_ui_items_to_sample"), &LinguisticObjectProfiler::get_max_ui_items_to_sample);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "max_ui_items_to_sample", PROPERTY_HINT_RANGE, "1,128,1"),
        "set_max_ui_items_to_sample",
        "get_max_ui_items_to_sample"
    );

    ClassDB::bind_method(D_METHOD("set_include_engine_base_members", "include_base"), &LinguisticObjectProfiler::set_include_engine_base_members);
    ClassDB::bind_method(D_METHOD("get_include_engine_base_members"), &LinguisticObjectProfiler::get_include_engine_base_members);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "include_engine_base_members"),
        "set_include_engine_base_members",
        "get_include_engine_base_members"
    );

    ClassDB::bind_method(D_METHOD("set_include_child_ui_labels", "include_children"), &LinguisticObjectProfiler::set_include_child_ui_labels);
    ClassDB::bind_method(D_METHOD("get_include_child_ui_labels"), &LinguisticObjectProfiler::get_include_child_ui_labels);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "include_child_ui_labels"),
        "set_include_child_ui_labels",
        "get_include_child_ui_labels"
    );

    ClassDB::bind_method(D_METHOD("set_auto_cache_pod_in_metadata", "auto_cache"), &LinguisticObjectProfiler::set_auto_cache_pod_in_metadata);
    ClassDB::bind_method(D_METHOD("get_auto_cache_pod_in_metadata"), &LinguisticObjectProfiler::get_auto_cache_pod_in_metadata);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_cache_pod_in_metadata"),
        "set_auto_cache_pod_in_metadata",
        "get_auto_cache_pod_in_metadata"
    );

    // Core Profiling Methods
    ClassDB::bind_method(
        D_METHOD("profile_object", "object", "use_cached_metadata_if_present"),
        &LinguisticObjectProfiler::profile_object,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("inspect_object_linguistic_breakdown", "object"),
        &LinguisticObjectProfiler::inspect_object_linguistic_breakdown
    );
    ClassDB::bind_method(
        D_METHOD("profile_node_subtree", "root_node", "max_depth"),
        &LinguisticObjectProfiler::profile_node_subtree,
        DEFVAL(4)
    );
    ClassDB::bind_method(
        D_METHOD("profile_packed_scene", "scene"),
        &LinguisticObjectProfiler::profile_packed_scene
    );
    ClassDB::bind_method(
        D_METHOD("profile_scene_state_node", "state", "node_idx"),
        &LinguisticObjectProfiler::profile_scene_state_node
    );
    ClassDB::bind_method(
        D_METHOD("compute_semantic_similarity_between_objects", "object_a", "object_b", "nuclear_weight", "peripheral_weight"),
        &LinguisticObjectProfiler::compute_semantic_similarity_between_objects,
        DEFVAL(0.75f),
        DEFVAL(0.25f)
    );
    ClassDB::bind_method(
        D_METHOD("does_object_match_prompt", "object", "natural_language_prompt", "min_affinity"),
        &LinguisticObjectProfiler::does_object_match_prompt,
        DEFVAL(0.35f)
    );
    ClassDB::bind_method(
        D_METHOD("build_reflection_signature_dictionary", "object"),
        &LinguisticObjectProfiler::build_reflection_signature_dictionary
    );
    ClassDB::bind_method(
        D_METHOD("profile_objects_to_dod_buffer", "objects", "cache_in_metadata"),
        &LinguisticObjectProfiler::profile_objects_to_dod_buffer,
        DEFVAL(true)
    );
}

void LinguisticObjectProfiler::_ensure_helpers() const {
    if (prompt_parser_.is_null()) {
        prompt_parser_.instantiate();
    }
    if (metadata_manager_.is_null()) {
        metadata_manager_.instantiate();
        metadata_manager_->set_prompt_parser(prompt_parser_);
    }
}

godot::Ref<SemanticPromptParser> LinguisticObjectProfiler::get_prompt_parser() const {
    _ensure_helpers();
    return prompt_parser_;
}

godot::Ref<MetadataLinguisticsManager> LinguisticObjectProfiler::get_metadata_manager() const {
    _ensure_helpers();
    return metadata_manager_;
}

void LinguisticObjectProfiler::_harvest_class_hierarchy(
    const godot::Object *p_object,
    const godot::Ref<SemanticTagSetResource> &r_tag_set,
    godot::PackedStringArray &r_class_tokens
) const {
    if (!p_object) return;

    // 1. Check attached Script global class_name and filename stem
    const godot::Variant script_var = p_object->get_script();
    if (script_var.get_type() == godot::Variant::OBJECT) {
        if (const godot::Script *scr = godot::Object::cast_to<godot::Script>(script_var)) {
            const godot::StringName global_name = scr->get_global_name();
            if (!global_name.is_empty()) {
                const godot::String g_str(global_name);
                r_class_tokens.push_back(g_str);
                r_tag_set->merge_from(prompt_parser_->parse_identifier_to_tag_set(g_str, true));
            }
            const godot::String scr_path = scr->get_path();
            if (!scr_path.is_empty()) {
                const godot::String stem = scr_path.get_file().get_basename();
                if (!stem.is_empty()) {
                    r_class_tokens.push_back(stem);
                    r_tag_set->merge_from(prompt_parser_->parse_identifier_to_tag_set(stem, true));
                }
            }
        }
    }

    // 2. Walk ClassDB inheritance chain up to max_inheritance_depth_
    godot::StringName curr_class(p_object->get_class());
    int depth = 0;
    while (!is_generic_base_class(curr_class) && depth < max_inheritance_depth_) {
        const godot::String cls_str(curr_class);
        r_class_tokens.push_back(cls_str);
        map_godot_class_to_structural_semes(cls_str, r_tag_set);

        godot::Ref<SemanticTagSetResource> cls_tags =
            prompt_parser_->parse_identifier_to_tag_set(cls_str, true);
        if (depth == 0) {
            r_tag_set->merge_from(cls_tags);
        } else {
            // Ancestor classes contribute their tags as peripheral context
            const godot::PackedStringArray anc_nuc = cls_tags->get_all_nuclear_tags();
            for (int i = 0; i < anc_nuc.size(); ++i) {
                r_tag_set->add_peripheral_tag(anc_nuc[i]);
            }
        }

        curr_class = godot::ClassDB::get_parent_class(curr_class);
        ++depth;
    }
}

void LinguisticObjectProfiler::_harvest_names_and_groups(
    const godot::Object *p_object,
    const godot::Ref<SemanticTagSetResource> &r_tag_set,
    godot::PackedStringArray &r_name_tokens
) const {
    if (!p_object) return;

    auto harvest_identifier = [&](const godot::String &p_id_str, bool p_is_primary) {
        if (p_id_str.is_empty()) return;
        const godot::PackedStringArray parts = SemanticPromptParser::split_compound_identifier(p_id_str);
        for (int i = 0; i < parts.size(); ++i) {
            if (!r_name_tokens.has(parts[i])) {
                r_name_tokens.push_back(parts[i]);
            }
        }
        godot::Ref<SemanticTagSetResource> id_tags =
            prompt_parser_->parse_identifier_to_tag_set(p_id_str, !p_is_primary);
        r_tag_set->merge_from(id_tags);
    };

    if ((enabled_channels_mask_ & core::LING_SOURCE_OBJECT_AND_NODE_NAME) != 0u) {
        if (const auto *node = godot::Object::cast_to<godot::Node>(p_object)) {
            harvest_identifier(godot::String(node->get_name()), true);
            const godot::String scene_path = node->get_scene_file_path();
            if (!scene_path.is_empty()) {
                harvest_identifier(scene_path.get_file().get_basename(), false);
            }
        }
        if (const auto *res = godot::Object::cast_to<godot::Resource>(p_object)) {
            harvest_identifier(res->get_name(), true);
            const godot::String res_path = res->get_path();
            if (!res_path.is_empty()) {
                harvest_identifier(res_path.get_file().get_basename(), false);
            }
        }
    }

    if ((enabled_channels_mask_ & core::LING_SOURCE_NODE_GROUPS) != 0u) {
        if (const auto *node = godot::Object::cast_to<godot::Node>(p_object)) {
            const godot::TypedArray<godot::StringName> groups = node->get_groups();
            for (int i = 0; i < groups.size(); ++i) {
                const godot::String g_str(groups[i]);
                if (!g_str.begins_with("_")) {
                    harvest_identifier(g_str, true);
                }
            }
        }
    }
}

void LinguisticObjectProfiler::_harvest_ui_text_controls(
    godot::Object *p_object,
    const godot::Ref<SemanticTagSetResource> &r_tag_set,
    godot::PackedStringArray &r_ui_texts,
    int p_child_depth
) const {
    if (!p_object) return;

    auto add_ui_phrase = [&](const godot::String &p_text, bool p_as_nuclear) {
        godot::String trimmed = p_text.strip_edges();
        if (trimmed.is_empty()) return;
        if (!r_ui_texts.has(trimmed)) {
            r_ui_texts.push_back(trimmed);
        }
        godot::Ref<SemanticTagSetResource> parsed = prompt_parser_->parse_prompt_to_tag_set(trimmed);
        if (p_as_nuclear) {
            r_tag_set->merge_from(parsed);
        } else {
            const godot::PackedStringArray nuc = parsed->get_all_nuclear_tags();
            for (int i = 0; i < nuc.size(); ++i) {
                r_tag_set->add_peripheral_tag(nuc[i]);
            }
            const godot::PackedStringArray per = parsed->get_all_peripheral_tags();
            for (int i = 0; i < per.size(); ++i) {
                r_tag_set->add_peripheral_tag(per[i]);
            }
        }
    };

    // 1. Node editor_description
    if (const auto *node = godot::Object::cast_to<godot::Node>(p_object)) {
        const godot::String ed_desc = node->get_editor_description();
        if (!ed_desc.is_empty()) {
            add_ui_phrase(ed_desc, false);
        }
    }

    // 2. Control tooltip_text
    if (const auto *ctrl = godot::Object::cast_to<godot::Control>(p_object)) {
        const godot::String tooltip = ctrl->get_tooltip_text();
        if (!tooltip.is_empty()) {
            add_ui_phrase(tooltip, false);
        }
    }

    // 3. RichTextLabel (`get_parsed_text` strips BBCode tags cleanly)
    if (p_object->is_class("RichTextLabel") && p_object->has_method("get_parsed_text")) {
        add_ui_phrase(godot::String(p_object->call("get_parsed_text")), p_child_depth == 0);
    } else if (p_object->is_class("Label") || p_object->is_class("Label3D") ||
               p_object->is_class("Button") || p_object->is_class("LinkButton") ||
               p_object->is_class("CheckBox") || p_object->is_class("CheckButton")) {
        add_ui_phrase(godot::String(p_object->get("text")), true);
    } else if (p_object->is_class("LineEdit") || p_object->is_class("TextEdit") ||
               p_object->is_class("CodeEdit")) {
        // Placeholder text expresses the structural semantic role of the input field (Nuclear),
        // while current user-typed text is dynamic state (Peripheral)
        const godot::Variant ph = p_object->get("placeholder_text");
        if (ph.get_type() == godot::Variant::STRING) {
            add_ui_phrase(godot::String(ph), true);
        }
        const godot::Variant txt = p_object->get("text");
        if (txt.get_type() == godot::Variant::STRING) {
            add_ui_phrase(godot::String(txt), false);
        }
        if (p_object->is_class("LineEdit") && static_cast<bool>(p_object->get("secret"))) {
            r_tag_set->add_nuclear_tag("hidden");
        }
    } else if (p_object->is_class("Window") || p_object->is_class("AcceptDialog")) {
        add_ui_phrase(godot::String(p_object->get("title")), true);
        if (p_object->is_class("AcceptDialog")) {
            add_ui_phrase(godot::String(p_object->get("dialog_text")), false);
        }
    }

    // 4. ItemList / OptionButton item texts
    if ((p_object->is_class("ItemList") || p_object->is_class("OptionButton")) &&
        p_object->has_method("get_item_count") && p_object->has_method("get_item_text")) {
        const int count = std::min(static_cast<int>(p_object->call("get_item_count")), max_ui_items_to_sample_);
        for (int i = 0; i < count; ++i) {
            add_ui_phrase(godot::String(p_object->call("get_item_text", i)), false);
        }
    }

    // 5. TabBar / TabContainer tab titles
    if ((p_object->is_class("TabBar") || p_object->is_class("TabContainer")) &&
        p_object->has_method("get_tab_count") && p_object->has_method("get_tab_title")) {
        const int count = std::min(static_cast<int>(p_object->call("get_tab_count")), max_ui_items_to_sample_);
        for (int i = 0; i < count; ++i) {
            add_ui_phrase(godot::String(p_object->call("get_tab_title", i)), false);
        }
    }

    // 6. Optional immediate child UI Label/Button harvesting for composite UI containers
    if (include_child_ui_labels_ && p_child_depth == 0) {
        if (auto *node = godot::Object::cast_to<godot::Node>(p_object)) {
            const int child_count = std::min(node->get_child_count(), max_ui_items_to_sample_);
            for (int c = 0; c < child_count; ++c) {
                if (godot::Node *child = node->get_child(c)) {
                    if (child->is_class("Label") || child->is_class("Button") ||
                        child->is_class("LineEdit") || child->is_class("RichTextLabel")) {
                        _harvest_ui_text_controls(child, r_tag_set, r_ui_texts, p_child_depth + 1);
                    }
                }
            }
        }
    }
}

void LinguisticObjectProfiler::_harvest_reflected_properties(
    const godot::Object *p_object,
    const godot::Ref<SemanticTagSetResource> &r_tag_set,
    godot::PackedStringArray &r_property_names,
    uint32_t &r_property_trait_mask
) const {
    if (!p_object) return;

    const godot::TypedArray<godot::Dictionary> prop_list = p_object->get_property_list();
    for (int i = 0; i < prop_list.size(); ++i) {
        const godot::Dictionary prop = prop_list[i];
        const godot::String name     = prop.get("name", "");
        const int type               = static_cast<int>(prop.get("type", godot::Variant::NIL));
        const int hint               = static_cast<int>(prop.get("hint", godot::PROPERTY_HINT_NONE));
        const godot::String hint_str = prop.get("hint_string", "");
        const uint32_t usage         = static_cast<uint32_t>(static_cast<int64_t>(prop.get("usage", 0)));

        if (is_boilerplate_property_name(name)) {
            continue;
        }

        // Skip category/group pseudo-properties
        if (type == godot::Variant::NIL &&
            (usage & (godot::PROPERTY_USAGE_CATEGORY | godot::PROPERTY_USAGE_GROUP | godot::PROPERTY_USAGE_SUBGROUP)) != 0u) {
            continue;
        }

        const bool is_script_var = (usage & godot::PROPERTY_USAGE_SCRIPT_VARIABLE) != 0u;
        const bool is_stored_ed  = (usage & godot::PROPERTY_USAGE_EDITOR) != 0u &&
                                   (usage & godot::PROPERTY_USAGE_STORAGE) != 0u;
        
        if (!include_engine_base_members_ && !is_script_var) {
            if (!is_stored_ed || is_engine_base_property(name)) {
                continue;
            }
        }

        r_property_names.push_back(name);
        const uint32_t prop_hash = core::hash_linguistic_token(name.utf8().get_data());
        r_property_trait_mask |= (1u << (prop_hash & 31u));

        // Check if this property holds an embedded SememeResource or SemanticTagSetResource
        if (type == godot::Variant::OBJECT &&
            (enabled_channels_mask_ & core::LING_SOURCE_EXPLICIT_RESOURCE) != 0u) {
            const godot::Variant obj_val = p_object->get(godot::StringName(name));
            if (obj_val.get_type() == godot::Variant::OBJECT) {
                if (const auto *sem = godot::Object::cast_to<SememeResource>(obj_val)) {
                    r_tag_set->add_sememe(godot::Ref<SememeResource>(const_cast<SememeResource *>(sem)));
                    continue;
                }
                if (const auto *tset = godot::Object::cast_to<SemanticTagSetResource>(obj_val)) {
                    r_tag_set->merge_from(godot::Ref<SemanticTagSetResource>(const_cast<SemanticTagSetResource *>(tset)));
                    continue;
                }
            }
        }

        // Boolean state properties (e.g. `is_locked`, `can_fly`, `has_key`, `enabled`)
        if (type == godot::Variant::BOOL) {
            const bool val = static_cast<bool>(p_object->get(godot::StringName(name)));
            godot::String pred = name;
            if (pred.begins_with("is_"))        pred = pred.substr(3);
            else if (pred.begins_with("has_"))  pred = pred.substr(4);
            else if (pred.begins_with("can_"))  pred = pred.substr(4);
            else if (pred.begins_with("allow_"))pred = pred.substr(6);

            godot::Ref<SemanticTagSetResource> p_tags =
                prompt_parser_->parse_identifier_to_tag_set(pred, false);
            const godot::PackedStringArray nuc = p_tags->get_all_nuclear_tags();
            if (val) {
                for (int k = 0; k < nuc.size(); ++k) {
                    r_tag_set->add_peripheral_tag(nuc[k]);
                }
            } else {
                for (int k = 0; k < nuc.size(); ++k) {
                    r_tag_set->add_excluded_tag(nuc[k]);
                }
            }
            continue;
        }

        // Enum & Bitfield properties (`PROPERTY_HINT_ENUM`, `PROPERTY_HINT_FLAGS`):
        // Resolve the active integer value against `hint_string` to harvest the active enum label!
        if (type == godot::Variant::INT && !hint_str.is_empty() &&
            (hint == godot::PROPERTY_HINT_ENUM || hint == godot::PROPERTY_HINT_FLAGS)) {
            const int64_t cur_val = static_cast<int64_t>(p_object->get(godot::StringName(name)));
            const godot::PackedStringArray items = hint_str.split(",", false);
            for (int idx = 0; idx < items.size(); ++idx) {
                godot::String item = items[idx].strip_edges();
                int64_t item_val   = idx;
                const int colon    = item.find(":");
                if (colon >= 0) {
                    item_val = item.substr(colon + 1).to_int();
                    item     = item.substr(0, colon).strip_edges();
                }
                const bool is_match = (hint == godot::PROPERTY_HINT_ENUM)
                                          ? (cur_val == item_val)
                                          : ((cur_val & (1LL << idx)) != 0LL);
                if (is_match && !item.is_empty() && item.to_lower() != "none" && item.to_lower() != "undefined") {
                    r_tag_set->merge_from(prompt_parser_->parse_identifier_to_tag_set(item, false));
                }
            }
        }

        // String / StringName identity properties (`title`, `canonical_label`, `rule_name`, etc.)
        if (type == godot::Variant::STRING || type == godot::Variant::STRING_NAME) {
            const godot::String s_val = godot::String(p_object->get(godot::StringName(name))).strip_edges();
            if (!s_val.is_empty() && s_val.length() <= 120 && !s_val.begins_with("res://")) {
                if (name == "title" || name == "canonical_label" || name == "rule_name" ||
                    name == "lexicon_name" || name == "language_name") {
                    r_tag_set->merge_from(prompt_parser_->parse_prompt_to_tag_set(s_val));
                } else {
                    godot::Ref<SemanticTagSetResource> val_tags = prompt_parser_->parse_prompt_to_tag_set(s_val);
                    const godot::PackedStringArray nuc = val_tags->get_all_nuclear_tags();
                    for (int k = 0; k < nuc.size(); ++k) {
                        r_tag_set->add_peripheral_tag(nuc[k]);
                    }
                }
            }
        }

        // Decompose the member property name itself into peripheral semantic traits
        godot::Ref<SemanticTagSetResource> prop_name_tags =
            prompt_parser_->parse_identifier_to_tag_set(name, true);
        const godot::PackedStringArray pn_nuc = prop_name_tags->get_all_nuclear_tags();
        for (int k = 0; k < pn_nuc.size(); ++k) {
            r_tag_set->add_peripheral_tag(pn_nuc[k]);
        }
    }
}

void LinguisticObjectProfiler::_harvest_reflected_methods(
    const godot::Object *p_object,
    const godot::Ref<SemanticTagSetResource> &r_tag_set,
    godot::PackedStringArray &r_method_verbs,
    uint32_t &r_method_trait_mask
) const {
    if (!p_object) return;

    const godot::TypedArray<godot::Dictionary> method_list = p_object->get_method_list();
    for (int i = 0; i < method_list.size(); ++i) {
        const godot::Dictionary m = method_list[i];
        const godot::String name  = m.get("name", "");
        if (!include_engine_base_members_ && is_boilerplate_method_name(name)) {
            continue;
        }

        r_method_verbs.push_back(name);
        const uint32_t m_hash = core::hash_linguistic_token(name.utf8().get_data());
        r_method_trait_mask |= (1u << (m_hash & 31u));

        const godot::PackedStringArray parts = SemanticPromptParser::split_compound_identifier(name);
        if (!parts.is_empty()) {
            // In method names (e.g. `attack_target`, `open_door`, `recenter_player`),
            // the leading token is typically the action verb (affordance)
            const godot::String lead_verb = parts[0];
            godot::Ref<SemanticTagSetResource> m_tags =
                prompt_parser_->parse_identifier_to_tag_set(name, false);
            const godot::PackedStringArray nuc = m_tags->get_all_nuclear_tags();
            for (int k = 0; k < nuc.size(); ++k) {
                r_tag_set->add_peripheral_tag(nuc[k]);
            }
        }
    }

    if (!r_method_verbs.is_empty()) {
        r_tag_set->add_peripheral_tag("refl_has_methods");
    }
}

void LinguisticObjectProfiler::_harvest_reflected_signals(
    const godot::Object *p_object,
    const godot::Ref<SemanticTagSetResource> &r_tag_set,
    godot::PackedStringArray &r_signal_events,
    uint32_t &r_signal_trait_mask
) const {
    if (!p_object) return;

    const godot::TypedArray<godot::Dictionary> sig_list = p_object->get_signal_list();
    for (int i = 0; i < sig_list.size(); ++i) {
        const godot::Dictionary s = sig_list[i];
        const godot::String name  = s.get("name", "");
        if (!include_engine_base_members_ && is_boilerplate_signal_name(name)) {
            continue;
        }

        r_signal_events.push_back(name);
        const uint32_t s_hash = core::hash_linguistic_token(name.utf8().get_data());
        r_signal_trait_mask |= (1u << (s_hash & 31u));

        godot::Ref<SemanticTagSetResource> s_tags =
            prompt_parser_->parse_identifier_to_tag_set(name, false);
        const godot::PackedStringArray nuc = s_tags->get_all_nuclear_tags();
        for (int k = 0; k < nuc.size(); ++k) {
            r_tag_set->add_peripheral_tag(nuc[k]);
        }
    }

    if (!r_signal_events.is_empty()) {
        r_tag_set->add_peripheral_tag("refl_has_signals");
    }
}

godot::Ref<SemanticTagSetResource> LinguisticObjectProfiler::profile_object(
    godot::Object *p_object,
    bool p_use_cached_metadata_if_present
) const {
    _ensure_helpers();
    if (!p_object) {
        godot::Ref<SemanticTagSetResource> empty_set;
        empty_set.instantiate();
        return empty_set;
    }

    if (p_use_cached_metadata_if_present && MetadataLinguisticsManager::has_linguistic_metadata(p_object)) {
        return MetadataLinguisticsManager::read_tag_set_from_metadata(p_object);
    }

    godot::Ref<SemanticTagSetResource> tag_set;
    tag_set.instantiate();

    godot::PackedStringArray class_tokens;
    godot::PackedStringArray name_tokens;
    godot::PackedStringArray ui_texts;
    godot::PackedStringArray prop_names;
    godot::PackedStringArray method_verbs;
    godot::PackedStringArray signal_events;
    uint32_t prop_mask   = 0u;
    uint32_t method_mask = 0u;
    uint32_t signal_mask = 0u;

    if ((enabled_channels_mask_ & core::LING_SOURCE_CLASS_HIERARCHY) != 0u) {
        _harvest_class_hierarchy(p_object, tag_set, class_tokens);
    }
    if ((enabled_channels_mask_ & (core::LING_SOURCE_OBJECT_AND_NODE_NAME | core::LING_SOURCE_NODE_GROUPS)) != 0u) {
        _harvest_names_and_groups(p_object, tag_set, name_tokens);
    }
    if ((enabled_channels_mask_ & core::LING_SOURCE_UI_TEXT_CONTROLS) != 0u) {
        _harvest_ui_text_controls(p_object, tag_set, ui_texts, 0);
    }
    if ((enabled_channels_mask_ & core::LING_SOURCE_REFLECTED_PROPERTIES) != 0u) {
        _harvest_reflected_properties(p_object, tag_set, prop_names, prop_mask);
    }
    if ((enabled_channels_mask_ & core::LING_SOURCE_REFLECTED_METHODS) != 0u) {
        _harvest_reflected_methods(p_object, tag_set, method_verbs, method_mask);
    }
    if ((enabled_channels_mask_ & core::LING_SOURCE_REFLECTED_SIGNALS) != 0u) {
        _harvest_reflected_signals(p_object, tag_set, signal_events, signal_mask);
    }
    if ((enabled_channels_mask_ & core::LING_SOURCE_OBJECT_METADATA) != 0u) {
        godot::Ref<SemanticTagSetResource> meta_tags =
            metadata_manager_->harvest_user_metadata_semantics(p_object, false);
        tag_set->merge_from(meta_tags);
    }

    if (auto_cache_pod_in_metadata_) {
        const core::LinguisticProfilePOD pod = tag_set->to_linguistic_profile_pod(
            static_cast<uint16_t>(enabled_channels_mask_ & 0xFFFFu)
        );
        MetadataLinguisticsManager::write_profile_pod_to_metadata(p_object, pod);
    }

    return tag_set;
}

godot::Dictionary LinguisticObjectProfiler::inspect_object_linguistic_breakdown(
    godot::Object *p_object
) const {
    _ensure_helpers();
    godot::Dictionary report;
    if (!p_object) {
        return report;
    }

    godot::Ref<SemanticTagSetResource> tag_set;
    tag_set.instantiate();

    godot::PackedStringArray class_tokens;
    godot::PackedStringArray name_tokens;
    godot::PackedStringArray ui_texts;
    godot::PackedStringArray prop_names;
    godot::PackedStringArray method_verbs;
    godot::PackedStringArray signal_events;
    uint32_t prop_mask   = 0u;
    uint32_t method_mask = 0u;
    uint32_t signal_mask = 0u;

    _harvest_class_hierarchy(p_object, tag_set, class_tokens);
    _harvest_names_and_groups(p_object, tag_set, name_tokens);
    _harvest_ui_text_controls(p_object, tag_set, ui_texts, 0);
    _harvest_reflected_properties(p_object, tag_set, prop_names, prop_mask);
    _harvest_reflected_methods(p_object, tag_set, method_verbs, method_mask);
    _harvest_reflected_signals(p_object, tag_set, signal_events, signal_mask);
    godot::Ref<SemanticTagSetResource> meta_tags =
        metadata_manager_->harvest_user_metadata_semantics(p_object, false);
    tag_set->merge_from(meta_tags);

    report["class_name"]           = p_object->get_class();
    report["class_lineage_tokens"] = class_tokens;
    report["name_and_group_tokens"]= name_tokens;
    report["ui_harvested_texts"]   = ui_texts;
    report["reflected_properties"] = prop_names;
    report["reflected_methods"]    = method_verbs;
    report["reflected_signals"]    = signal_events;
    report["nuclear_seme_tags"]    = tag_set->get_all_nuclear_tags();
    report["peripheral_seme_tags"] = tag_set->get_all_peripheral_tags();
    report["nuclear_bitmask"]      = tag_set->get_nuclear_bitmask_words();
    report["peripheral_bitmask"]   = tag_set->get_peripheral_bitmask_words();
    report["coherence_mask32"]     = tag_set->get_coherence_trait_mask32();
    report["aggregate_valence"]    = tag_set->get_aggregate_valence();
    report["compiled_tag_set"]     = tag_set;

    return report;
}

godot::Ref<SemanticTagSetResource> LinguisticObjectProfiler::profile_node_subtree(
    godot::Node *p_root_node,
    int p_max_depth
) const {
    godot::Ref<SemanticTagSetResource> root_tags = profile_object(p_root_node, false);
    if (!p_root_node || p_max_depth <= 0) {
        return root_tags;
    }

    const int child_count = p_root_node->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        if (godot::Node *child = p_root_node->get_child(i)) {
            godot::Ref<SemanticTagSetResource> child_tags =
                profile_node_subtree(child, p_max_depth - 1);
            // Child nodes contribute their nuclear identity into the parent subtree's peripheral context
            const godot::PackedStringArray child_nuc = child_tags->get_all_nuclear_tags();
            for (int k = 0; k < child_nuc.size(); ++k) {
                root_tags->add_peripheral_tag(child_nuc[k]);
            }
        }
    }
    return root_tags;
}

godot::Ref<SemanticTagSetResource> LinguisticObjectProfiler::profile_scene_state_node(
    const godot::Ref<godot::SceneState> &p_state,
    int p_node_idx
) const {
    _ensure_helpers();
    godot::Ref<SemanticTagSetResource> tag_set;
    tag_set.instantiate();
    if (p_state.is_null() || p_node_idx < 0 || p_node_idx >= p_state->get_node_count()) {
        return tag_set;
    }

    // 1. Node Type & Name
    const godot::String node_type(p_state->get_node_type(p_node_idx));
    const godot::String node_name(p_state->get_node_name(p_node_idx));
    if (!node_type.is_empty()) {
        map_godot_class_to_structural_semes(node_type, tag_set);
        tag_set->merge_from(prompt_parser_->parse_identifier_to_tag_set(node_type, true));
    }
    if (!node_name.is_empty()) {
        tag_set->merge_from(prompt_parser_->parse_identifier_to_tag_set(node_name, false));
    }

    // 2. Node Groups
    const godot::PackedStringArray groups = p_state->get_node_groups(p_node_idx);
    for (int g = 0; g < groups.size(); ++g) {
        if (!groups[g].begins_with("_")) {
            tag_set->merge_from(prompt_parser_->parse_identifier_to_tag_set(groups[g], false));
        }
    }

    // 3. Serialized Node Properties (including UI `text`, `placeholder_text`, `title`, and metadata)
    const int prop_count = p_state->get_node_property_count(p_node_idx);
    for (int p = 0; p < prop_count; ++p) {
        const godot::String prop_name(p_state->get_node_property_name(p_node_idx, p));
        const godot::Variant prop_val = p_state->get_node_property_value(p_node_idx, p);

        if (prop_name == "text" || prop_name == "placeholder_text" || prop_name == "title") {
            if (prop_val.get_type() == godot::Variant::STRING || prop_val.get_type() == godot::Variant::STRING_NAME) {
                tag_set->merge_from(prompt_parser_->parse_prompt_to_tag_set(godot::String(prop_val)));
            }
        } else if (prop_name.begins_with("metadata/")) {
            const godot::String meta_key = prop_name.substr(9);
            if (!meta_key.begins_with("_")) {
                tag_set->merge_from(prompt_parser_->parse_identifier_to_tag_set(meta_key, true));
                if (prop_val.get_type() == godot::Variant::STRING || prop_val.get_type() == godot::Variant::STRING_NAME) {
                    tag_set->merge_from(prompt_parser_->parse_prompt_to_tag_set(godot::String(prop_val)));
                }
            }
        }
    }

    return tag_set;
}

godot::Ref<SemanticTagSetResource> LinguisticObjectProfiler::profile_packed_scene(
    const godot::Ref<godot::PackedScene> &p_scene
) const {
    godot::Ref<SemanticTagSetResource> scene_tags;
    scene_tags.instantiate();
    if (p_scene.is_null()) {
        return scene_tags;
    }

    const godot::Ref<godot::SceneState> state = p_scene->get_state();
    if (state.is_null() || state->get_node_count() == 0) {
        return scene_tags;
    }

    // Root node defines primary nuclear tags; child nodes contribute peripheral tags
    scene_tags = profile_scene_state_node(state, 0);
    const int node_count = state->get_node_count();
    for (int idx = 1; idx < node_count; ++idx) {
        godot::Ref<SemanticTagSetResource> child_tags = profile_scene_state_node(state, idx);
        const godot::PackedStringArray child_nuc = child_tags->get_all_nuclear_tags();
        for (int k = 0; k < child_nuc.size(); ++k) {
            scene_tags->add_peripheral_tag(child_nuc[k]);
        }
    }
    return scene_tags;
}

float LinguisticObjectProfiler::compute_semantic_similarity_between_objects(
    godot::Object *p_object_a,
    godot::Object *p_object_b,
    float p_nuclear_weight,
    float p_peripheral_weight
) const {
    if (!p_object_a || !p_object_b) {
        return 0.0f;
    }
    const core::LinguisticProfilePOD pod_a = compile_linguistic_profile_pod(p_object_a, auto_cache_pod_in_metadata_);
    const core::LinguisticProfilePOD pod_b = compile_linguistic_profile_pod(p_object_b, auto_cache_pod_in_metadata_);
    return core::SemanticLexiconCore::compute_descriptor_affinity(
        pod_a.to_sememe_descriptor(),
        pod_b.to_sememe_descriptor(),
        p_nuclear_weight,
        p_peripheral_weight
    );
}

bool LinguisticObjectProfiler::does_object_match_prompt(
    godot::Object *p_object,
    const godot::String &p_natural_language_prompt,
    float p_min_affinity
) const {
    if (!p_object) return false;
    _ensure_helpers();
    godot::Ref<SemanticTagSetResource> prompt_tags =
        prompt_parser_->parse_prompt_to_tag_set(p_natural_language_prompt);
    godot::Ref<SemanticTagSetResource> obj_tags = profile_object(p_object, true);
    return prompt_tags->compute_tversky_affinity_with(obj_tags, 0.75f, 0.25f) >= p_min_affinity;
}

core::LinguisticProfilePOD LinguisticObjectProfiler::compile_linguistic_profile_pod(
    godot::Object *p_object,
    bool p_cache_in_metadata
) const {
    if (!p_object) {
        return core::LinguisticProfilePOD{};
    }
    core::LinguisticProfilePOD cached{};
    if (MetadataLinguisticsManager::read_profile_pod_from_metadata(p_object, cached)) {
        return cached;
    }
    godot::Ref<SemanticTagSetResource> tag_set = profile_object(p_object, false);
    core::LinguisticProfilePOD pod = tag_set->to_linguistic_profile_pod(
        static_cast<uint16_t>(enabled_channels_mask_ & 0xFFFFu)
    );
    if (const auto *node = godot::Object::cast_to<godot::Node>(p_object)) {
        pod.object_name_hash = core::hash_linguistic_token(godot::String(node->get_name()).utf8().get_data());
    } else if (const auto *res = godot::Object::cast_to<godot::Resource>(p_object)) {
        pod.object_name_hash = core::hash_linguistic_token(res->get_name().utf8().get_data());
    }
    if (p_cache_in_metadata) {
        MetadataLinguisticsManager::write_profile_pod_to_metadata(p_object, pod);
    }
    return pod;
}

core::ReflectionSignaturePOD LinguisticObjectProfiler::compile_reflection_signature_pod(
    const godot::Object *p_object
) const {
    core::ReflectionSignaturePOD sig{};
    if (!p_object) {
        return sig;
    }

    const godot::String cls_str = p_object->get_class();
    sig.class_name_hash = core::hash_linguistic_token(cls_str.utf8().get_data());

    godot::String obj_name;
    if (const auto *node = godot::Object::cast_to<godot::Node>(p_object)) {
        obj_name = godot::String(node->get_name());
    } else if (const auto *res = godot::Object::cast_to<godot::Resource>(p_object)) {
        obj_name = res->get_name();
    }
    if (!obj_name.is_empty()) {
        sig.object_name_hash = core::hash_linguistic_token(obj_name.utf8().get_data());
        const godot::PackedStringArray parts = SemanticPromptParser::split_compound_identifier(obj_name);
        if (!parts.is_empty()) {
            sig.name_prefix_hash = core::hash_linguistic_token(parts[0].utf8().get_data());
            sig.name_suffix_hash = core::hash_linguistic_token(parts[parts.size() - 1].utf8().get_data());
        }
    }

    godot::Ref<SemanticTagSetResource> dummy_set;
    dummy_set.instantiate();
    godot::PackedStringArray props;
    godot::PackedStringArray methods;
    godot::PackedStringArray signals;
    _harvest_reflected_properties(p_object, dummy_set, props, sig.property_trait_mask);
    _harvest_reflected_methods(p_object, dummy_set, methods, sig.method_trait_mask);
    _harvest_reflected_signals(p_object, dummy_set, signals, sig.signal_trait_mask);

    core::LinguisticProfilePOD prof{};
    if (MetadataLinguisticsManager::read_profile_pod_from_metadata(p_object, prof)) {
        sig.group_or_tag_mask = prof.to_sememe_descriptor().compute_coherence_trait_mask32();
    } else {
        sig.group_or_tag_mask = dummy_set->get_compiled_descriptor().compute_coherence_trait_mask32();
    }

    return sig;
}

godot::Dictionary LinguisticObjectProfiler::build_reflection_signature_dictionary(
    const godot::Object *p_object
) const {
    const core::ReflectionSignaturePOD sig = compile_reflection_signature_pod(p_object);
    godot::Dictionary d;
    d["class_name_hash"]     = static_cast<int64_t>(sig.class_name_hash);
    d["object_name_hash"]    = static_cast<int64_t>(sig.object_name_hash);
    d["name_prefix_hash"]    = static_cast<int64_t>(sig.name_prefix_hash);
    d["name_suffix_hash"]    = static_cast<int64_t>(sig.name_suffix_hash);
    d["group_or_tag_mask"]   = static_cast<int64_t>(sig.group_or_tag_mask);
    d["property_trait_mask"] = static_cast<int64_t>(sig.property_trait_mask);
    d["method_trait_mask"]   = static_cast<int64_t>(sig.method_trait_mask);
    d["signal_trait_mask"]   = static_cast<int64_t>(sig.signal_trait_mask);
    return d;
}

godot::PackedByteArray LinguisticObjectProfiler::profile_objects_to_dod_buffer(
    const godot::Array &p_objects,
    bool p_cache_in_metadata
) const {
    godot::PackedByteArray buffer;
    buffer.resize(static_cast<int64_t>(p_objects.size()) * static_cast<int64_t>(sizeof(core::LinguisticProfilePOD)));
    uint8_t *dst = buffer.ptrw();

    for (int i = 0; i < p_objects.size(); ++i) {
        core::LinguisticProfilePOD pod{};
        if (p_objects[i].get_type() == godot::Variant::OBJECT) {
            godot::Object *obj = p_objects[i];
            if (obj) {
                pod = compile_linguistic_profile_pod(obj, p_cache_in_metadata);
            }
        }
        std::memcpy(
            dst + static_cast<size_t>(i) * sizeof(core::LinguisticProfilePOD),
            &pod,
            sizeof(core::LinguisticProfilePOD)
        );
    }
    return buffer;
}

} // namespace ideam::godot_ext

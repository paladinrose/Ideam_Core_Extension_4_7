#include "gameplay_rule_editor_inspector_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "../game.h"
#include "../game_entity.h"
#include "../game_property.h"

namespace ideam::godot_ext {

void GameplayRuleEditorInspectorPlugin::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_select_matching_entities_pressed"),
        &GameplayRuleEditorInspectorPlugin::_on_select_matching_entities_pressed
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_target_class_selected", "index", "picker"),
        &GameplayRuleEditorInspectorPlugin::_on_target_class_selected
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_target_title_selected", "index", "picker"),
        &GameplayRuleEditorInspectorPlugin::_on_target_title_selected
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_target_property_selected", "index", "picker"),
        &GameplayRuleEditorInspectorPlugin::_on_target_property_selected
    );
}

GameplayRuleEditorInspectorPlugin::GameplayRuleEditorInspectorPlugin() {}

GameplayRuleEditorInspectorPlugin::~GameplayRuleEditorInspectorPlugin() {}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *GameplayRuleEditorInspectorPlugin::get_undo_redo() const {
    if (godot::EditorInterface::get_singleton()) {
        return godot::EditorInterface::get_singleton()->get_editor_undo_redo();
    }
    return nullptr;
}
#endif

GameplayRule *GameplayRuleEditorInspectorPlugin::_get_current_rule() const {
    if (_current_rule_id.is_null()) {
        return nullptr;
    }
    return godot::Object::cast_to<GameplayRule>(godot::ObjectDB::get_instance(_current_rule_id));
}

bool GameplayRuleEditorInspectorPlugin::_can_handle(godot::Object *p_object) {
    return godot::Object::cast_to<GameplayRule>(p_object) != nullptr;
}

void GameplayRuleEditorInspectorPlugin::_collect_matching_nodes(
    godot::Node *p_node,
    GameplayRule *p_rule,
    godot::Node *p_scene_root,
    godot::TypedArray<godot::Node> &r_matches
) {
    if (!p_node || !p_rule) {
        return;
    }

    bool is_host = (p_node == p_scene_root);
    if (p_rule->matches_object(p_node, is_host)) {
        r_matches.append(p_node);
    }

    int32_t child_count = p_node->get_child_count();
    for (int32_t i = 0; i < child_count; ++i) {
        _collect_matching_nodes(p_node->get_child(i), p_rule, p_scene_root, r_matches);
    }
}

void GameplayRuleEditorInspectorPlugin::_collect_scene_titles_and_groups(
    godot::Node *p_node,
    godot::PackedStringArray &r_titles,
    godot::PackedStringArray &r_groups
) {
    if (!p_node) {
        return;
    }

    if (p_node->has_method("get_title")) {
        godot::String t = p_node->call("get_title");
        if (!t.is_empty() && !r_titles.has(t)) {
            r_titles.append(t);
        }
    }

    godot::TypedArray<godot::StringName> node_groups = p_node->get_groups();
    for (int i = 0; i < node_groups.size(); ++i) {
        godot::String grp_str = node_groups[i];
        if (!grp_str.is_empty() && !grp_str.begins_with("_") && !r_groups.has(grp_str)) {
            r_groups.append(grp_str);
        }
    }

    if (p_node->has_method("get_competes_with_groups")) {
        godot::TypedArray<godot::String> comp_groups = p_node->call("get_competes_with_groups");
        for (int i = 0; i < comp_groups.size(); ++i) {
            godot::String cg_str = comp_groups[i];
            if (!cg_str.is_empty() && !r_groups.has(cg_str)) {
                r_groups.append(cg_str);
            }
        }
    }

    int32_t child_count = p_node->get_child_count();
    for (int32_t i = 0; i < child_count; ++i) {
        _collect_scene_titles_and_groups(p_node->get_child(i), r_titles, r_groups);
    }
}

void GameplayRuleEditorInspectorPlugin::_collect_scene_property_titles(
    godot::Node *p_node,
    godot::PackedStringArray &r_prop_titles
) {
    if (!p_node) {
        return;
    }

    if (GameProperty *gp = godot::Object::cast_to<GameProperty>(p_node)) {
        godot::String prop_title = gp->get_title();
        if (!prop_title.is_empty() && !r_prop_titles.has(prop_title)) {
            r_prop_titles.append(prop_title);
        }
    }

    int32_t child_count = p_node->get_child_count();
    for (int32_t i = 0; i < child_count; ++i) {
        _collect_scene_property_titles(p_node->get_child(i), r_prop_titles);
    }
}

void GameplayRuleEditorInspectorPlugin::_parse_begin(godot::Object *p_object) {
    GameplayRule *rule = godot::Object::cast_to<GameplayRule>(p_object);
    if (!rule) {
        return;
    }

    _current_rule_id = godot::ObjectID(rule->get_instance_id());

    // Build header Specificity & Scope Badge Pill
    godot::PanelContainer *banner = memnew(godot::PanelContainer);
    godot::Ref<godot::StyleBoxFlat> box_style;
    box_style.instantiate();
    box_style->set_bg_color(godot::Color(0.11f, 0.14f, 0.19f, 0.95f));
    box_style->set_border_color(godot::Color(0.24f, 0.56f, 0.88f, 0.75f));
    box_style->set_border_width_all(1);
    box_style->set_corner_radius_all(5);
    box_style->set_content_margin_all(8);
    banner->add_theme_stylebox_override("panel", box_style);

    godot::VBoxContainer *vbox = memnew(godot::VBoxContainer);
    vbox->add_theme_constant_override("separation", 6);
    banner->add_child(vbox);

    // Row 1: Selector summary & Specificity Tuple
    int spec = rule->get_specificity();
    int id_bit = (spec / 100) % 10;
    int group_bit = (spec / 10) % 10;
    int class_bit = spec % 10;

    godot::String scope_badge = "*";
    RuleScope sc = rule->get_scope();
    if (sc == RuleScope::SELF) {
        scope_badge = ":host";
    } else if (sc == RuleScope::ALL) {
        scope_badge = ":host, *";
    }

    godot::String layer_badge = "Default";
    switch (rule->get_cascade_layer()) {
        case RuleCascadeLayer::LAYER_BASE: layer_badge = "@layer base"; break;
        case RuleCascadeLayer::LAYER_MODE: layer_badge = "@layer mode"; break;
        case RuleCascadeLayer::LAYER_DIFFICULTY: layer_badge = "@layer difficulty"; break;
        case RuleCascadeLayer::LAYER_PLAYER_ACCESSIBILITY: layer_badge = "@layer accessibility"; break;
        case RuleCascadeLayer::LAYER_INLINE_LOCAL: layer_badge = "@layer override"; break;
        default: break;
    }
    if (rule->get_is_important()) {
        layer_badge += " !important";
    }

    godot::Label *spec_label = memnew(godot::Label);
    spec_label->set_text(
        "Specificity: [" + godot::String::num_int64(id_bit) + ", " +
        godot::String::num_int64(group_bit) + ", " +
        godot::String::num_int64(class_bit) + "] (" +
        godot::String::num_int64(spec) + ")  |  Scope: " + scope_badge +
        "  |  " + layer_badge
    );
    spec_label->add_theme_color_override("font_color", godot::Color(0.55f, 0.82f, 1.0f));
    vbox->add_child(spec_label);

    // Row 2: Live Scene Match Counter & Select Action
    int match_count = 0;
#ifdef TOOLS_ENABLED
    if (godot::EditorInterface::get_singleton()) {
        godot::Node *scene_root = godot::EditorInterface::get_singleton()->get_edited_scene_root();
        if (scene_root) {
            godot::TypedArray<godot::Node> matches;
            _collect_matching_nodes(scene_root, rule, scene_root, matches);
            match_count = matches.size();
        }
    }
#endif

    godot::Button *match_btn = memnew(godot::Button);
    match_btn->set_text(
        "Matches " + godot::String::num_int64(match_count) +
        " entities in current scene (Click to Select)"
    );
    match_btn->set_disabled(match_count == 0);
    match_btn->connect(
        "pressed",
        godot::Callable(this, "_on_select_matching_entities_pressed")
    );
    vbox->add_child(match_btn);

    add_custom_control(banner);
}

bool GameplayRuleEditorInspectorPlugin::_parse_property(
    godot::Object *p_object,
    godot::Variant::Type p_type,
    const godot::String &p_name,
    godot::PropertyHint p_hint_type,
    const godot::String &p_hint_string,
    godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
    bool p_wide
) {
    GameplayRule *rule = godot::Object::cast_to<GameplayRule>(p_object);
    if (!rule) {
        return false;
    }

    _current_rule_id = godot::ObjectID(rule->get_instance_id());

    // 1. Smart Selector Picker for target_class
    if (p_name == "target_class") {
        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        godot::Label *lbl = memnew(godot::Label);
        lbl->set_text("Quick Class Preset:");
        lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        row->add_child(lbl);

        godot::OptionButton *picker = memnew(godot::OptionButton);
        picker->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        picker->add_item("* (Any Class)");
        picker->set_item_metadata(0, godot::String(""));

        const char *classes[] = {
            "Game",
            "GameEntity",
            "GameBoard",
            "GameAgent",
            "GamePiece",
            "GamePlayer",
            "GameAgentAction",
            "GamePieceAction",
            "GameInteraction",
            "GameProperty"
        };
        godot::String current_cls = rule->get_target_class();
        int selected_idx = 0;
        for (int i = 0; i < 10; ++i) {
            godot::String c_name(classes[i]);
            picker->add_item(c_name);
            picker->set_item_metadata(i + 1, c_name);
            if (current_cls == c_name) {
                selected_idx = i + 1;
            }
        }
        picker->select(selected_idx);
        picker->connect(
            "item_selected",
            godot::Callable(this, "_on_target_class_selected").bind(picker)
        );
        row->add_child(picker);
        add_custom_control(row);
        return false; // Keep native String field editable below
    }

    // 2. Smart Selector Picker for target_title_or_group
    if (p_name == "target_title_or_group") {
        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        godot::Label *lbl = memnew(godot::Label);
        lbl->set_text("Scene Title / Group:");
        lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        row->add_child(lbl);

        godot::OptionButton *picker = memnew(godot::OptionButton);
        picker->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        picker->add_item("* (Any Title / Group)");
        picker->set_item_metadata(0, godot::String(""));

        godot::PackedStringArray titles;
        godot::PackedStringArray groups;
#ifdef TOOLS_ENABLED
        if (godot::EditorInterface::get_singleton()) {
            godot::Node *scene_root = godot::EditorInterface::get_singleton()->get_edited_scene_root();
            if (scene_root) {
                _collect_scene_titles_and_groups(scene_root, titles, groups);
            }
        }
#endif
        int idx = 1;
        int selected_idx = 0;
        godot::String current_sel = rule->get_target_title_or_group();

        for (int i = 0; i < titles.size(); ++i) {
            godot::String t = titles[i];
            picker->add_item("#" + t + " (Title)");
            picker->set_item_metadata(idx, t);
            if (current_sel == t || current_sel == ("#" + t)) {
                selected_idx = idx;
            }
            ++idx;
        }
        for (int i = 0; i < groups.size(); ++i) {
            godot::String g = groups[i];
            picker->add_item("." + g + " (Group)");
            picker->set_item_metadata(idx, "." + g);
            if (current_sel == g || current_sel == ("." + g)) {
                selected_idx = idx;
            }
            ++idx;
        }

        picker->select(selected_idx);
        picker->connect(
            "item_selected",
            godot::Callable(this, "_on_target_title_selected").bind(picker)
        );
        row->add_child(picker);
        add_custom_control(row);
        return false;
    }

    // 3. Smart Selector Picker for target_property
    if (p_name == "target_property") {
        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        godot::Label *lbl = memnew(godot::Label);
        lbl->set_text("Known Property / Stat:");
        lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        row->add_child(lbl);

        godot::OptionButton *picker = memnew(godot::OptionButton);
        picker->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        picker->add_item("-- Select Property or GameProperty --");
        picker->set_item_metadata(0, godot::String(""));

        int idx = 1;
        int selected_idx = 0;
        godot::String current_prop = rule->get_target_property();

        // Collect scene GameProperty titles (+ :max_value, :min_value, :locked)
        godot::PackedStringArray prop_titles;
#ifdef TOOLS_ENABLED
        if (godot::EditorInterface::get_singleton()) {
            godot::Node *scene_root = godot::EditorInterface::get_singleton()->get_edited_scene_root();
            if (scene_root) {
                _collect_scene_property_titles(scene_root, prop_titles);
            }
        }
#endif
        for (int i = 0; i < prop_titles.size(); ++i) {
            godot::String pt = prop_titles[i];
            const char *sub_suffixes[] = { "", ":max_value", ":min_value", ":locked" };
            for (int s = 0; s < 4; ++s) {
                godot::String full_prop = pt + godot::String(sub_suffixes[s]);
                picker->add_item("[GameProperty] " + full_prop);
                picker->set_item_metadata(idx, full_prop);
                if (current_prop == full_prop) {
                    selected_idx = idx;
                }
                ++idx;
            }
        }

        // Reflect ClassDB properties for target_class (or GameEntity default)
        godot::String target_cls = rule->get_target_class();
        if (target_cls.is_empty()) {
            target_cls = "GameEntity";
        }
        if (godot::ClassDB::class_exists(target_cls)) {
            godot::TypedArray<godot::Dictionary> prop_list =
                godot::ClassDB::class_get_property_list(target_cls, false);
            for (int i = 0; i < prop_list.size(); ++i) {
                godot::Dictionary d = prop_list[i];
                godot::String prop_name = d.get("name", "");
                int usage = d.get("usage", 0);
                if (!prop_name.is_empty() &&
                    !prop_name.begins_with("_") &&
                    (usage & godot::PROPERTY_USAGE_EDITOR)) {
                    picker->add_item("[" + target_cls + "] " + prop_name);
                    picker->set_item_metadata(idx, prop_name);
                    if (current_prop == prop_name) {
                        selected_idx = idx;
                    }
                    ++idx;
                }
            }
        }

        picker->select(selected_idx);
        picker->connect(
            "item_selected",
            godot::Callable(this, "_on_target_property_selected").bind(picker)
        );
        row->add_child(picker);
        add_custom_control(row);
        return false;
    }

    return false;
}

void GameplayRuleEditorInspectorPlugin::_on_select_matching_entities_pressed() {
#ifdef TOOLS_ENABLED
    GameplayRule *rule = _get_current_rule();
    if (!rule || !godot::EditorInterface::get_singleton()) {
        return;
    }

    godot::Node *scene_root = godot::EditorInterface::get_singleton()->get_edited_scene_root();
    godot::EditorSelection *selection = godot::EditorInterface::get_singleton()->get_selection();
    if (!scene_root || !selection) {
        return;
    }

    godot::TypedArray<godot::Node> matches;
    _collect_matching_nodes(scene_root, rule, scene_root, matches);
    if (matches.is_empty()) {
        return;
    }

    selection->clear();
    for (int i = 0; i < matches.size(); ++i) {
        godot::Node *n = godot::Object::cast_to<godot::Node>(matches[i]);
        if (n) {
            selection->add_node(n);
        }
    }
#endif
}

void GameplayRuleEditorInspectorPlugin::_apply_rule_property_undoable(
    GameplayRule *p_rule,
    const godot::StringName &p_property,
    const godot::Variant &p_new_value,
    const godot::String &p_action_name
) {
    if (!p_rule) {
        return;
    }
    godot::Variant old_value = p_rule->get(p_property);
    if (old_value == p_new_value) {
        return;
    }

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) {
        ur->create_action(p_action_name);
        ur->add_do_property(p_rule, p_property, p_new_value);
        ur->add_undo_property(p_rule, p_property, old_value);
        ur->commit_action();
        p_rule->notify_property_list_changed();
        return;
    }
#endif
    p_rule->set(p_property, p_new_value);
    p_rule->notify_property_list_changed();
}

void GameplayRuleEditorInspectorPlugin::_on_target_class_selected(
    int p_index,
    godot::OptionButton *p_picker
) {
    GameplayRule *rule = _get_current_rule();
    if (!rule || !p_picker) {
        return;
    }
    godot::String chosen = p_picker->get_item_metadata(p_index);
    _apply_rule_property_undoable(rule, "target_class", chosen, "Set GameplayRule Target Class");
}

void GameplayRuleEditorInspectorPlugin::_on_target_title_selected(
    int p_index,
    godot::OptionButton *p_picker
) {
    GameplayRule *rule = _get_current_rule();
    if (!rule || !p_picker) {
        return;
    }
    godot::String chosen = p_picker->get_item_metadata(p_index);
    _apply_rule_property_undoable(
        rule,
        "target_title_or_group",
        chosen,
        "Set GameplayRule Target Title/Group"
    );
}

void GameplayRuleEditorInspectorPlugin::_on_target_property_selected(
    int p_index,
    godot::OptionButton *p_picker
) {
    GameplayRule *rule = _get_current_rule();
    if (!rule || !p_picker || p_index <= 0) {
        return;
    }
    godot::String chosen = p_picker->get_item_metadata(p_index);
    _apply_rule_property_undoable(
        rule,
        "target_property",
        chosen,
        "Set GameplayRule Target Property"
    );
}

} // namespace ideam::godot_ext
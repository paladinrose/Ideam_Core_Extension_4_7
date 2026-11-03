#include "gameplay_resource_editor_inspector_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "../gameplay/gameplay_accessibility_option.h"
#include "../gameplay/gameplay_control_map.h"
#include "../gameplay/gameplay_difficulty.h"
#include "../gameplay/gameplay_perspective.h"
#include "../ui/accessibility_live_preview_card.h"
#include "../ui/control_stick_deadzone_visualizer.h"
#include "../ui/difficulty_profile_radar.h"
#include "../ui/perspective_frustum_visualizer.h"

namespace ideam::godot_ext {

void GameplayResourceEditorInspectorPlugin::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_refresh_rules_pressed"),
        &GameplayResourceEditorInspectorPlugin::_on_refresh_rules_pressed
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_inspect_rule_pressed", "rule"),
        &GameplayResourceEditorInspectorPlugin::_on_inspect_rule_pressed
    );
}

GameplayResourceEditorInspectorPlugin::GameplayResourceEditorInspectorPlugin() {}

GameplayResourceEditorInspectorPlugin::~GameplayResourceEditorInspectorPlugin() {}

GameplayResource *GameplayResourceEditorInspectorPlugin::_get_current_resource() const {
    if (_current_resource_id.is_null()) {
        return nullptr;
    }
    return godot::Object::cast_to<GameplayResource>(
        godot::ObjectDB::get_instance(_current_resource_id)
    );
}

bool GameplayResourceEditorInspectorPlugin::_can_handle(godot::Object *p_object) {
    if (!p_object) {
        return false;
    }
    // Exclude GameplayRule itself (handled by GameplayRuleEditorInspectorPlugin)
    if (godot::Object::cast_to<GameplayRule>(p_object) != nullptr) {
        return false;
    }
    return godot::Object::cast_to<GameplayResource>(p_object) != nullptr;
}

void GameplayResourceEditorInspectorPlugin::_parse_begin(godot::Object *p_object) {
    GameplayResource *res = godot::Object::cast_to<GameplayResource>(p_object);
    if (!res) {
        return;
    }

    _current_resource_id = godot::ObjectID(res->get_instance_id());

    godot::VBoxContainer *root_vbox = memnew(godot::VBoxContainer);
    root_vbox->add_theme_constant_override("separation", 8);

    // 1. Embed specialized Phase 2 interactive preview control at top of Inspector
    if (GameplayControlMap *ctrl_map = godot::Object::cast_to<GameplayControlMap>(res)) {
        ControlStickDeadzoneVisualizer *vis = memnew(ControlStickDeadzoneVisualizer);
        vis->set_custom_minimum_size(godot::Vector2(0.0f, 180.0f));
        vis->set_control_map(godot::Ref<GameplayControlMap>(ctrl_map));
        root_vbox->add_child(vis);
    } else if (GameplayDifficulty *diff = godot::Object::cast_to<GameplayDifficulty>(res)) {
        DifficultyProfileRadar *radar = memnew(DifficultyProfileRadar);
        radar->set_custom_minimum_size(godot::Vector2(0.0f, 190.0f));
        radar->set_difficulty(godot::Ref<GameplayDifficulty>(diff));
        root_vbox->add_child(radar);
    } else if (GameplayPerspective *persp = godot::Object::cast_to<GameplayPerspective>(res)) {
        PerspectiveFrustumVisualizer *frustum = memnew(PerspectiveFrustumVisualizer);
        frustum->set_custom_minimum_size(godot::Vector2(0.0f, 180.0f));
        frustum->set_perspective(godot::Ref<GameplayPerspective>(persp));
        root_vbox->add_child(frustum);
    } else if (GameplayAccessibilityOption *acc =
                   godot::Object::cast_to<GameplayAccessibilityOption>(res)) {
        AccessibilityLivePreviewCard *card = memnew(AccessibilityLivePreviewCard);
        card->set_custom_minimum_size(godot::Vector2(0.0f, 175.0f));
        card->set_accessibility_option(godot::Ref<GameplayAccessibilityOption>(acc));
        root_vbox->add_child(card);
    }

    // 2. Synthesized Cascade Rules (collect_rules()) Preview Box
    godot::PanelContainer *rules_panel = memnew(godot::PanelContainer);
    godot::Ref<godot::StyleBoxFlat> box_style;
    box_style.instantiate();
    box_style->set_bg_color(godot::Color(0.10f, 0.12f, 0.16f, 0.95f));
    box_style->set_border_color(godot::Color(0.25f, 0.32f, 0.42f, 0.80f));
    box_style->set_border_width_all(1);
    box_style->set_corner_radius_all(5);
    box_style->set_content_margin_all(8);
    rules_panel->add_theme_stylebox_override("panel", box_style);

    godot::VBoxContainer *panel_vbox = memnew(godot::VBoxContainer);
    panel_vbox->add_theme_constant_override("separation", 6);
    rules_panel->add_child(panel_vbox);

    godot::HBoxContainer *header_row = memnew(godot::HBoxContainer);
    godot::Label *title_lbl = memnew(godot::Label);
    title_lbl->set_text("Synthesized Cascade Rules (collect_rules())");
    title_lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    title_lbl->add_theme_color_override("font_color", godot::Color(0.65f, 0.85f, 1.0f));
    header_row->add_child(title_lbl);

    godot::Button *refresh_btn = memnew(godot::Button);
    refresh_btn->set_text("Refresh");
    refresh_btn->connect(
        "pressed",
        godot::Callable(this, "_on_refresh_rules_pressed")
    );
    header_row->add_child(refresh_btn);
    panel_vbox->add_child(header_row);

    godot::VBoxContainer *rules_list_vbox = memnew(godot::VBoxContainer);
    rules_list_vbox->add_theme_constant_override("separation", 3);
    panel_vbox->add_child(rules_list_vbox);
    _rules_list_vbox_id = godot::ObjectID(rules_list_vbox->get_instance_id());

    root_vbox->add_child(rules_panel);
    add_custom_control(root_vbox);

    _rebuild_synthesized_rules_list();
}

bool GameplayResourceEditorInspectorPlugin::_parse_property(
    godot::Object *p_object,
    godot::Variant::Type p_type,
    const godot::String &p_name,
    godot::PropertyHint p_hint_type,
    const godot::String &p_hint_string,
    godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
    bool p_wide
) {
    return false;
}

void GameplayResourceEditorInspectorPlugin::_rebuild_synthesized_rules_list() {
    if (_rules_list_vbox_id.is_null()) {
        return;
    }
    godot::VBoxContainer *list_vbox = godot::Object::cast_to<godot::VBoxContainer>(
        godot::ObjectDB::get_instance(_rules_list_vbox_id)
    );
    GameplayResource *res = _get_current_resource();
    if (!list_vbox || !res) {
        return;
    }

    // Clear existing rows
    int32_t child_count = list_vbox->get_child_count();
    for (int32_t i = child_count - 1; i >= 0; --i) {
        godot::Node *child = list_vbox->get_child(i);
        list_vbox->remove_child(child);
        child->queue_free();
    }

    godot::TypedArray<GameplayRule> rules = res->collect_rules();
    if (rules.is_empty()) {
        godot::Label *empty_lbl = memnew(godot::Label);
        empty_lbl->set_text("No active rules emitted by collect_rules().");
        empty_lbl->add_theme_color_override("font_color", godot::Color(0.55f, 0.58f, 0.65f));
        list_vbox->add_child(empty_lbl);
        return;
    }

    int max_preview = rules.size() > 16 ? 16 : rules.size();
    for (int i = 0; i < max_preview; ++i) {
        godot::Ref<GameplayRule> r = rules[i];
        if (r.is_null()) {
            continue;
        }

        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        row->add_theme_constant_override("separation", 6);

        godot::String sel = r->get_target_class().is_empty() ? "*" : r->get_target_class();
        if (!r->get_target_title_or_group().is_empty()) {
            sel += "#" + r->get_target_title_or_group();
        }
        godot::String prop = r->get_target_property();
        godot::String val_str = r->get_value().stringify();

        godot::Label *rule_lbl = memnew(godot::Label);
        rule_lbl->set_text(
            sel + " { " + prop + ": " + val_str + " }  [spec " +
            godot::String::num_int64(r->get_specificity()) + "]"
        );
        rule_lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        rule_lbl->set_clip_text(true);
        row->add_child(rule_lbl);

        godot::Button *inspect_btn = memnew(godot::Button);
        inspect_btn->set_text("Inspect");
        inspect_btn->connect(
            "pressed",
            godot::Callable(this, "_on_inspect_rule_pressed").bind(r)
        );
        row->add_child(inspect_btn);

        list_vbox->add_child(row);
    }

    if (rules.size() > max_preview) {
        godot::Label *more_lbl = memnew(godot::Label);
        more_lbl->set_text(
            "... +" + godot::String::num_int64(rules.size() - max_preview) + " additional rules"
        );
        more_lbl->add_theme_color_override("font_color", godot::Color(0.55f, 0.60f, 0.68f));
        list_vbox->add_child(more_lbl);
    }
}

void GameplayResourceEditorInspectorPlugin::_on_refresh_rules_pressed() {
    _rebuild_synthesized_rules_list();
}

void GameplayResourceEditorInspectorPlugin::_on_inspect_rule_pressed(
    const godot::Ref<GameplayRule> &p_rule
) {
#ifdef TOOLS_ENABLED
    if (p_rule.is_valid() && godot::EditorInterface::get_singleton()) {
        godot::EditorInterface::get_singleton()->inspect_object(p_rule.ptr());
    }
#endif
}

} // namespace ideam::godot_ext
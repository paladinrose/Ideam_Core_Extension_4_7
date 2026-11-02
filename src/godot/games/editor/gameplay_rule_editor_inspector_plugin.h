#pragma once

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../gameplay/gameplay_rule.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

/**
 * @brief GameplayRuleEditorInspectorPlugin — Phase 4 Editor Inspector plugin for GameplayRule.
 *
 * Responsibilities:
 *   1. Header Specificity & Scope Badge Pill:
 *      Displays computed CSS-like specificity (e.g., [0, 1, 1] -> 110), scope (:host, *, :host,*),
 *      cascade @layer, and a live "Matches N entities in current scene (Select)" button.
 *   2. Smart Selector Autocomplete Pickers:
 *      - target_class: Dropdown of Game, GameEntity, GameBoard, GameAgent, GamePiece,
 *        GameAgentAction, GamePieceAction, GameInteraction, GameProperty.
 *      - target_title_or_group: Dropdown populated from active scene GameEntity titles,
 *        Godot Node groups, and competes_with_groups tags.
 *      - target_property: Dropdown populated from reflected ClassDB properties on target_class
 *        plus active scene GameProperty titles (including :max_value, :min_value, :locked).
 */
class GameplayRuleEditorInspectorPlugin : public IdeamEditorInspectorPlugin {
    GDCLASS(GameplayRuleEditorInspectorPlugin, IdeamEditorInspectorPlugin)

private:
    godot::ObjectID _current_rule_id;

    // Scene traversal helpers (stateless, on-demand during inspector construction/click)
    static void _collect_matching_nodes(
        godot::Node *p_node,
        GameplayRule *p_rule,
        godot::Node *p_scene_root,
        godot::TypedArray<godot::Node> &r_matches
    );
    static void _collect_scene_titles_and_groups(
        godot::Node *p_node,
        godot::PackedStringArray &r_titles,
        godot::PackedStringArray &r_groups
    );
    static void _collect_scene_property_titles(
        godot::Node *p_node,
        godot::PackedStringArray &r_prop_titles
    );

    GameplayRule *_get_current_rule() const;

    // UI Callbacks
    void _on_select_matching_entities_pressed();
    void _on_target_class_selected(int p_index, godot::OptionButton *p_picker);
    void _on_target_title_selected(int p_index, godot::OptionButton *p_picker);
    void _on_target_property_selected(int p_index, godot::OptionButton *p_picker);

    void _apply_rule_property_undoable(
        GameplayRule *p_rule,
        const godot::StringName &p_property,
        const godot::Variant &p_new_value,
        const godot::String &p_action_name
    );

protected:
    static void _bind_methods();

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const;
#endif

public:
    GameplayRuleEditorInspectorPlugin();
    ~GameplayRuleEditorInspectorPlugin() override;

    virtual bool _can_handle(godot::Object *p_object) override;
    virtual void _parse_begin(godot::Object *p_object) override;
    virtual bool _parse_property(
        godot::Object *p_object,
        godot::Variant::Type p_type,
        const godot::String &p_name,
        godot::PropertyHint p_hint_type,
        const godot::String &p_hint_string,
        godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
        bool p_wide
    ) override;
};

} // namespace ideam::godot_ext
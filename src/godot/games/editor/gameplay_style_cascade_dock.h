#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../gameplay/gameplay_rule.h"
#include "../gameplay/gameplay_style.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: GameplayStyleCascadeDock provides a Browser DevTools-style ("CSS Inspector")
 * bottom panel in the Godot Editor for inspecting the `GameplayStyle` and `GameplayRule`
 * cascade on any `Game` or `GameEntity` (`GameBoard`, `GameAgent`, `GamePiece`,
 * `GameAgentAction`, `GameInteraction`) in the active Scene Tree.
 *
 * When a developer selects a node in the Scene Tree (or picks one from the dock's
 * entity dropdown), GameplayStyleCascadeDock:
 * 1. Simulates `GameplayStyle::merge_with_child()` from the scene root down through all
 *    ancestor `Game` / `GameEntity` nodes to produce a non-destructive simulated
 *    `_resolved_gameplay_style`.
 * 2. Left Pane (Computed Properties): Lists direct reflected properties (`time_scale`,
 *    `priority`, `maximum_interactions`, etc.) and child `GameProperty` nodes (`Health`,
 *    `HearingRange`, `ChargeTime`, etc.), displaying `Authoritative Base Value -> Final Computed Value`
 *    and a clickable link to inspect the winning `GameplayRule`.
 * 3. Right Pane (Cascade Rule Waterfall): Lists every matching `GameplayRule` in precedence
 *    order (`!important`, `RuleCascadeLayer`, `priority`, `get_specificity()`). Rules shadowed
 *    or overridden by a higher-precedence `RuleOp::SET` rule are rendered with a `[SUPERSEDED]`
 *    badge and strikethrough-style formatting.
 */
class GameplayStyleCascadeDock : public godot::VBoxContainer {
    GDCLASS(GameplayStyleCascadeDock, godot::VBoxContainer)

protected:
    static void _bind_methods();

private:
    godot::ObjectID _selected_node_id;
    godot::ObjectID _scene_root_id;

    // --- Header Bar Controls ---
    godot::Label *_target_path_label = nullptr;
    godot::OptionButton *_entity_picker = nullptr;
    godot::Button *_refresh_button = nullptr;
    godot::Button *_apply_live_button = nullptr;
    godot::Button *_revert_live_button = nullptr;

    // --- Split Panes ---
    godot::HSplitContainer *_split_container = nullptr;
    godot::VBoxContainer *_computed_props_vbox = nullptr;
    godot::VBoxContainer *_waterfall_rules_vbox = nullptr;

    // Cached list of selectable Game / GameEntity ObjectIDs in the current scene
    godot::Array _picker_node_ids;

    // --- Internal Helpers ---
    void _build_ui();
    void _clear_container(godot::Control *p_container);
    void _collect_scene_entities_recursive(godot::Node *p_node, godot::Array &r_nodes);
    godot::Ref<GameplayStyle> _simulate_resolved_style_for_node(godot::Node *p_target, godot::Array &r_chain_names) const;

    void _populate_computed_properties_pane(godot::Node *p_target, const godot::Ref<GameplayStyle> &p_resolved, const godot::Array &p_cascade_report);
    void _populate_waterfall_pane(godot::Node *p_target, const godot::Ref<GameplayStyle> &p_resolved, const godot::Array &p_cascade_report, const godot::Array &p_chain_names);

    static godot::String _layer_to_badge_string(int p_layer);
    static godot::String _op_to_symbol_string(int p_op);
    static godot::String _scope_to_symbol_string(int p_scope);

    // UI Callbacks
    void _on_entity_picker_selected(int p_index);
    void _on_refresh_pressed();
    void _on_apply_live_pressed();
    void _on_revert_live_pressed();
    void _on_inspect_resource_pressed(godot::Object *p_resource);

public:
    GameplayStyleCascadeDock();
    ~GameplayStyleCascadeDock() override;

    virtual void _ready() override;

    void set_scene_root(godot::Node *p_root);
    godot::Node *get_scene_root() const;

    void inspect_node(godot::Node *p_node);
    godot::Node *get_inspected_node() const;

    void refresh_cascade_view();
};

} // namespace ideam::godot_ext
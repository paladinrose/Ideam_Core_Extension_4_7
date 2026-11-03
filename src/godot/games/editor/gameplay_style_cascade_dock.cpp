#include "gameplay_style_cascade_dock.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "../game.h"
#include "../game_entity.h"
#include "../game_entities/game_piece.h"
#include "../game_property.h"

namespace ideam::godot_ext {

void GameplayStyleCascadeDock::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_scene_root", "root"), &GameplayStyleCascadeDock::set_scene_root);
    godot::ClassDB::bind_method(godot::D_METHOD("get_scene_root"), &GameplayStyleCascadeDock::get_scene_root);
    godot::ClassDB::bind_method(godot::D_METHOD("inspect_node", "node"), &GameplayStyleCascadeDock::inspect_node);
    godot::ClassDB::bind_method(godot::D_METHOD("get_inspected_node"), &GameplayStyleCascadeDock::get_inspected_node);
    godot::ClassDB::bind_method(godot::D_METHOD("refresh_cascade_view"), &GameplayStyleCascadeDock::refresh_cascade_view);

    godot::ClassDB::bind_method(godot::D_METHOD("_on_entity_picker_selected", "index"), &GameplayStyleCascadeDock::_on_entity_picker_selected);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_refresh_pressed"), &GameplayStyleCascadeDock::_on_refresh_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_apply_live_pressed"), &GameplayStyleCascadeDock::_on_apply_live_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_revert_live_pressed"), &GameplayStyleCascadeDock::_on_revert_live_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_inspect_resource_pressed", "resource"), &GameplayStyleCascadeDock::_on_inspect_resource_pressed);
}

GameplayStyleCascadeDock::GameplayStyleCascadeDock() {
    set_name("Gameplay Cascade DevTools");
    set_custom_minimum_size(godot::Vector2(680.0f, 240.0f));
}

GameplayStyleCascadeDock::~GameplayStyleCascadeDock() {
}

void GameplayStyleCascadeDock::_ready() {
    _build_ui();
    refresh_cascade_view();
}

void GameplayStyleCascadeDock::_build_ui() {
    if (_split_container) {
        return;
    }

    // --- 1. Top Toolbar ---
    godot::HBoxContainer *toolbar = memnew(godot::HBoxContainer);
    toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(toolbar);

    godot::Label *title_lbl = memnew(godot::Label);
    title_lbl->set_text("CSS Cascade Inspector:");
    toolbar->add_child(title_lbl);

    _entity_picker = memnew(godot::OptionButton);
    _entity_picker->set_custom_minimum_size(godot::Vector2(220.0f, 0.0f));
    _entity_picker->connect("item_selected", godot::Callable(this, "_on_entity_picker_selected"));
    toolbar->add_child(_entity_picker);

    _target_path_label = memnew(godot::Label);
    _target_path_label->set_text("Select a Game or GameEntity node in the Scene Tree...");
    _target_path_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _target_path_label->set_clip_text(true);
    toolbar->add_child(_target_path_label);

    _refresh_button = memnew(godot::Button);
    _refresh_button->set_text("Refresh Cascade");
    _refresh_button->connect("pressed", godot::Callable(this, "_on_refresh_pressed"));
    toolbar->add_child(_refresh_button);

    _apply_live_button = memnew(godot::Button);
    _apply_live_button->set_text("Test Apply Style");
    _apply_live_button->set_tooltip_text("Applies the resolved GameplayStyle cascade to the selected node so you can preview Computed Values live.");
    _apply_live_button->connect("pressed", godot::Callable(this, "_on_apply_live_pressed"));
    toolbar->add_child(_apply_live_button);

    _revert_live_button = memnew(godot::Button);
    _revert_live_button->set_text("Restore Base Values");
    _revert_live_button->set_tooltip_text("Restores cached authoritative Base Values and removes rule-injected modifiers on the selected node.");
    _revert_live_button->connect("pressed", godot::Callable(this, "_on_revert_live_pressed"));
    toolbar->add_child(_revert_live_button);

    add_child(memnew(godot::HSeparator));

    // --- 2. Main Split Container (Left: Computed Properties, Right: Cascade Rule Waterfall) ---
    _split_container = memnew(godot::HSplitContainer);
    _split_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _split_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _split_container->set_split_offset(320);
    add_child(_split_container);

    // Left Pane: Computed Properties
    godot::VBoxContainer *left_col = memnew(godot::VBoxContainer);
    left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _split_container->add_child(left_col);

    godot::Label *left_header = memnew(godot::Label);
    left_header->set_text("Computed Properties (Base Value -> Final Computed Value)");
    left_col->add_child(left_header);

    godot::ScrollContainer *left_scroll = memnew(godot::ScrollContainer);
    left_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    left_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    left_col->add_child(left_scroll);

    _computed_props_vbox = memnew(godot::VBoxContainer);
    _computed_props_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    left_scroll->add_child(_computed_props_vbox);

    // Right Pane: Cascade Rule Waterfall
    godot::VBoxContainer *right_col = memnew(godot::VBoxContainer);
    right_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    right_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _split_container->add_child(right_col);

    godot::Label *right_header = memnew(godot::Label);
    right_header->set_text("Cascade Rule Waterfall (Sorted by Precedence: !important > @layer > priority > specificity)");
    right_col->add_child(right_header);

    godot::ScrollContainer *right_scroll = memnew(godot::ScrollContainer);
    right_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    right_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    right_col->add_child(right_scroll);

    _waterfall_rules_vbox = memnew(godot::VBoxContainer);
    _waterfall_rules_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    right_scroll->add_child(_waterfall_rules_vbox);
}

void GameplayStyleCascadeDock::_clear_container(godot::Control *p_container) {
    if (!p_container) return;
    int count = p_container->get_child_count();
    for (int i = count - 1; i >= 0; --i) {
        if (godot::Node *child = p_container->get_child(i)) {
            p_container->remove_child(child);
            child->queue_free();
        }
    }
}

void GameplayStyleCascadeDock::set_scene_root(godot::Node *p_root) {
    _scene_root_id = p_root ? godot::ObjectID(p_root->get_instance_id()) : godot::ObjectID();
    refresh_cascade_view();
}

godot::Node *GameplayStyleCascadeDock::get_scene_root() const {
    if (_scene_root_id.is_valid()) {
        if (godot::Node *n = godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(_scene_root_id))) {
            return n;
        }
    }
    if (godot::EditorInterface *ei = godot::EditorInterface::get_singleton()) {
        return ei->get_edited_scene_root();
    }
    return nullptr;
}

void GameplayStyleCascadeDock::inspect_node(godot::Node *p_node) {
    _selected_node_id = p_node ? godot::ObjectID(p_node->get_instance_id()) : godot::ObjectID();
    refresh_cascade_view();
}

godot::Node *GameplayStyleCascadeDock::get_inspected_node() const {
    if (!_selected_node_id.is_valid()) {
        return nullptr;
    }
    return godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(_selected_node_id));
}

void GameplayStyleCascadeDock::_collect_scene_entities_recursive(godot::Node *p_node, godot::Array &r_nodes) {
    if (!p_node) return;
    if (godot::Object::cast_to<Game>(p_node) || godot::Object::cast_to<GameEntity>(p_node)) {
        r_nodes.append(p_node);
    }
    int count = p_node->get_child_count();
    for (int i = 0; i < count; ++i) {
        _collect_scene_entities_recursive(p_node->get_child(i), r_nodes);
    }
}

godot::Ref<GameplayStyle> GameplayStyleCascadeDock::_simulate_resolved_style_for_node(
    godot::Node *p_target,
    godot::Array &r_chain_names
) const {
    r_chain_names.clear();
    if (!p_target) {
        return godot::Ref<GameplayStyle>();
    }

    godot::Array ancestors;
    godot::Node *cursor = p_target;
    while (cursor) {
        if (godot::Object::cast_to<Game>(cursor) || godot::Object::cast_to<GameEntity>(cursor)) {
            ancestors.insert(0, cursor);
        }
        cursor = cursor->get_parent();
    }

    godot::Ref<GameplayStyle> simulated;
    for (int i = 0; i < ancestors.size(); ++i) {
        godot::Node *anc = godot::Object::cast_to<godot::Node>(ancestors[i]);
        if (!anc) continue;

        godot::Ref<GameplayStyle> local_style;
        godot::String node_label = godot::String(anc->get_name()) + " (" + anc->get_class() + ")";
        if (Game *g = godot::Object::cast_to<Game>(anc)) {
            local_style = g->get_gameplay_style();
        } else if (GameEntity *ent = godot::Object::cast_to<GameEntity>(anc)) {
            local_style = ent->get_gameplay_style();
            if (!ent->get_title().is_empty()) {
                node_label = ent->get_title() + " [" + anc->get_class() + "]";
            }
        }

        r_chain_names.append(node_label);

        if (simulated.is_valid()) {
            simulated = simulated->merge_with_child(local_style);
        } else if (local_style.is_valid()) {
            simulated.instantiate();
            simulated->apply_style(local_style);
        }
    }

    return simulated;
}

void GameplayStyleCascadeDock::refresh_cascade_view() {
    _build_ui();
    _clear_container(_computed_props_vbox);
    _clear_container(_waterfall_rules_vbox);

    _entity_picker->clear();
    _picker_node_ids.clear();

    godot::Node *root = get_scene_root();
    godot::Array entities;
    if (root) {
        _collect_scene_entities_recursive(root, entities);
    }

    godot::Node *target = get_inspected_node();
    if (!target && !entities.is_empty()) {
        target = godot::Object::cast_to<godot::Node>(entities[0]);
        if (target) {
            _selected_node_id = godot::ObjectID(target->get_instance_id());
        }
    }

    int selected_idx = -1;
    for (int i = 0; i < entities.size(); ++i) {
        godot::Node *n = godot::Object::cast_to<godot::Node>(entities[i]);
        if (!n) continue;

        godot::String item_text = godot::String(n->get_name()) + " (" + n->get_class() + ")";
        if (GameEntity *ent = godot::Object::cast_to<GameEntity>(n)) {
            if (!ent->get_title().is_empty()) {
                item_text += " — \"" + ent->get_title() + "\"";
            }
        }
        _entity_picker->add_item(item_text, i);
        _picker_node_ids.append(static_cast<uint64_t>(n->get_instance_id()));
        if (n == target) {
            selected_idx = i;
        }
    }

    if (selected_idx >= 0) {
        _entity_picker->select(selected_idx);
    }

    if (!target) {
        _target_path_label->set_text("No Game or GameEntity found in active scene.");
        godot::Label *empty_lbl = memnew(godot::Label);
        empty_lbl->set_text("Select a Game, GameBoard, GameAgent, GamePiece, GameAgentAction, or GameInteraction node to inspect its GameplayStyle cascade.");
        _computed_props_vbox->add_child(empty_lbl);
        return;
    }

    godot::Array chain_names;
    godot::Ref<GameplayStyle> resolved = _simulate_resolved_style_for_node(target, chain_names);

    godot::String chain_str;
    for (int i = 0; i < chain_names.size(); ++i) {
        if (i > 0) chain_str += " -> ";
        chain_str += godot::String(chain_names[i]);
    }
    _target_path_label->set_text("DOM Chain: " + chain_str);

    godot::Array cascade_report;
    if (resolved.is_valid()) {
        cascade_report = resolved->inspect_cascade_for_object(target, true);
    }

    _populate_computed_properties_pane(target, resolved, cascade_report);
    _populate_waterfall_pane(target, resolved, cascade_report, chain_names);
}

void GameplayStyleCascadeDock::_populate_computed_properties_pane(
    godot::Node *p_target,
    const godot::Ref<GameplayStyle> &p_resolved,
    const godot::Array &p_cascade_report
) {
    auto find_winning_rule = [&p_cascade_report](const godot::String &p_prop_name) -> godot::Ref<GameplayRule> {
        godot::Ref<GameplayRule> winner;
        for (int i = 0; i < p_cascade_report.size(); ++i) {
            godot::Dictionary entry = p_cascade_report[i];
            if (static_cast<bool>(entry.get("overridden", false))) {
                continue;
            }
            godot::String tprop(entry.get("target_property", ""));
            int colon = tprop.find(":");
            godot::String base_tprop = (colon > 0) ? tprop.substr(0, colon) : tprop;
            if (tprop == p_prop_name || base_tprop == p_prop_name) {
                winner = godot::Ref<GameplayRule>(godot::Object::cast_to<GameplayRule>(entry["rule"]));
            }
        }
        return winner;
    };

    auto add_computed_row = [this](
        const godot::String &p_category,
        const godot::String &p_prop_label,
        const godot::String &p_base_str,
        const godot::String &p_computed_str,
        bool p_modified,
        const godot::Ref<GameplayRule> &p_winner
    ) {
        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        row->set_h_size_flags(Control::SIZE_EXPAND_FILL);

        godot::Label *name_lbl = memnew(godot::Label);
        name_lbl->set_text("[" + p_category + "] " + p_prop_label);
        name_lbl->set_custom_minimum_size(godot::Vector2(140.0f, 0.0f));
        row->add_child(name_lbl);

        godot::Label *val_lbl = memnew(godot::Label);
        if (p_modified) {
            val_lbl->set_text(
                godot::String(p_base_str) + godot::String("  ->  ") + godot::String(p_computed_str)
            );
            val_lbl->set_modulate(godot::Color(0.35f, 0.95f, 0.62f, 1.0f));
        } else {
            val_lbl->set_text(
                godot::String(p_computed_str) + godot::String(" (unmodified)")
            );
            val_lbl->set_modulate(godot::Color(0.75f, 0.80f, 0.86f, 0.85f));
        }
        val_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        row->add_child(val_lbl);

        if (p_winner.is_valid()) {
            godot::Button *rule_btn = memnew(godot::Button);
            godot::String r_title = p_winner->get_title().is_empty() ? godot::String("Rule") : p_winner->get_title();
            rule_btn->set_text(r_title + " (" + _layer_to_badge_string(static_cast<int>(p_winner->get_cascade_layer())) + ")");
            godot::Array bind_args;
            bind_args.append(p_winner.ptr());
            rule_btn->connect("pressed", godot::Callable(this, "_on_inspect_resource_pressed").bindv(bind_args));
            row->add_child(rule_btn);
        }

        _computed_props_vbox->add_child(row);
    };

    // 1. Direct Key Properties on Game / GameEntity
    godot::Array direct_keys;
    direct_keys.append("time_scale");
    direct_keys.append("enabled");
    if (godot::Object::cast_to<Game>(p_target)) {
        direct_keys.append("maximum_interactions");
        direct_keys.append("save_options");
        direct_keys.append("load_options");
    }
    if (p_target->is_class("GameAgent")) {
        direct_keys.append("exclusivity");
        direct_keys.append("player_reposition_time");
    }
    if (p_target->is_class("GameAgentAction")) {
        direct_keys.append("priority");
        direct_keys.append("competes_with_groups");
    }

    for (int i = 0; i < direct_keys.size(); ++i) {
        godot::String key_str = direct_keys[i];
        godot::StringName key_sn(key_str);
        godot::Variant current_val = p_target->get(key_sn);
        godot::Variant base_val = GameplayRule::get_cached_base_value(p_target, key_sn, current_val);
        godot::Ref<GameplayRule> winner = find_winning_rule(key_str);
        bool modified = (base_val != current_val) || winner.is_valid();
        add_computed_row("Direct", key_str, godot::String(base_val), godot::String(current_val), modified, winner);
    }

    // 2. Child & Owned GameProperty Nodes
    godot::Array game_props;
    if (GamePiece *piece = godot::Object::cast_to<GamePiece>(p_target)) {
        godot::TypedArray<GameProperty> gp_arr = piece->get_game_properties();
        for (int i = 0; i < gp_arr.size(); ++i) {
            if (GameProperty *gp = godot::Object::cast_to<GameProperty>(gp_arr[i])) {
                if (!game_props.has(gp)) game_props.append(gp);
            }
        }
    }
    int child_count = p_target->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        if (GameProperty *gp = godot::Object::cast_to<GameProperty>(p_target->get_child(i))) {
            if (!game_props.has(gp)) game_props.append(gp);
        }
    }

    for (int i = 0; i < game_props.size(); ++i) {
        GameProperty *gp = godot::Object::cast_to<GameProperty>(game_props[i]);
        if (!gp) continue;

        godot::String gp_name = gp->get_title().is_empty() ? godot::String(gp->get_name()) : gp->get_title();
        int raw_val = gp->get_value();
        int base_val = static_cast<int>(GameplayRule::get_cached_base_value(gp, godot::StringName("value"), raw_val));
        int modded_val = gp->get_modded_value();
        int max_val = gp->get_modded_max_value();

        godot::Ref<GameplayRule> winner = find_winning_rule(gp_name);
        bool is_mod = (base_val != modded_val) || winner.is_valid();
        add_computed_row(
            "GameProperty",
            gp_name,
            godot::String::num_int64(base_val) + "/" + godot::String::num_int64(gp->get_max_value()),
            godot::String::num_int64(modded_val) + "/" + godot::String::num_int64(max_val),
            is_mod,
            winner
        );
    }
}

void GameplayStyleCascadeDock::_populate_waterfall_pane(
    godot::Node *p_target,
    const godot::Ref<GameplayStyle> &p_resolved,
    const godot::Array &p_cascade_report,
    const godot::Array &p_chain_names
) {
    if (!p_resolved.is_valid()) {
        godot::Label *lbl = memnew(godot::Label);
        lbl->set_text("No GameplayStyle assigned on this node or its ancestors.");
        _waterfall_rules_vbox->add_child(lbl);
        return;
    }

    godot::TypedArray<GameplayRule> all_rules = p_resolved->collect_rules();
    godot::Label *summary_lbl = memnew(godot::Label);
    summary_lbl->set_text(
        "Total Rules in Resolved Cascade: " + godot::String::num_int64(all_rules.size()) +
        "  |  Matched on Target: " + godot::String::num_int64(p_cascade_report.size())
    );
    summary_lbl->set_modulate(godot::Color(0.68f, 0.84f, 0.98f, 1.0f));
    _waterfall_rules_vbox->add_child(summary_lbl);

    for (int i = p_cascade_report.size() - 1; i >= 0; --i) {
        godot::Dictionary entry = p_cascade_report[i];
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(entry["rule"]));
        if (!rule.is_valid()) continue;

        bool overridden = static_cast<bool>(entry.get("overridden", false));
        godot::Ref<GameplayRule> superseded_by(godot::Object::cast_to<GameplayRule>(entry.get("superseded_by", godot::Variant())));

        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        row->set_h_size_flags(Control::SIZE_EXPAND_FILL);

        godot::String badge_text = "[" + _layer_to_badge_string(static_cast<int>(rule->get_cascade_layer())) +
                                   " | prio:" + godot::String::num_int64(rule->get_priority()) +
                                   " | spec:" + godot::String::num_int64(rule->get_specificity()) + "]";
        if (rule->get_is_important()) {
            badge_text = "[!IMPORTANT] " + badge_text;
        }

        godot::Label *badge_lbl = memnew(godot::Label);
        badge_lbl->set_text(badge_text);
        badge_lbl->set_custom_minimum_size(godot::Vector2(185.0f, 0.0f));
        badge_lbl->set_modulate(
            overridden
                ? godot::Color(0.55f, 0.55f, 0.60f, 0.65f)
                : (rule->get_is_important() ? godot::Color(1.0f, 0.78f, 0.22f, 1.0f) : godot::Color(0.55f, 0.82f, 0.98f, 1.0f))
        );
        row->add_child(badge_lbl);

        godot::String sel_str = _scope_to_symbol_string(static_cast<int>(rule->get_scope())) + " ";
        if (!rule->get_target_class().is_empty()) sel_str += godot::String(rule->get_target_class());
        else sel_str += "*";
        if (!rule->get_target_title_or_group().is_empty()) sel_str += "." + godot::String(rule->get_target_title_or_group());
        if (!rule->get_pseudo_state().is_empty()) sel_str += godot::String(rule->get_pseudo_state());

        godot::String decl_str = " { " + godot::String(rule->get_target_property()) + " " +
                                 _op_to_symbol_string(static_cast<int>(rule->get_operation())) + " " +
                                 godot::String(rule->get_value()) + " }";

        godot::String full_line = (rule->get_title().is_empty() ? godot::String("Rule") : rule->get_title()) + ": " + sel_str + decl_str;
        if (overridden) {
            godot::String sup_title = (superseded_by.is_valid() && !superseded_by->get_title().is_empty())
                ? superseded_by->get_title()
                : godot::String("Higher Rule");
            full_line = "~~ " + full_line + " ~~  [SUPERSEDED BY: " + sup_title + "]";
        }

        godot::Label *rule_lbl = memnew(godot::Label);
        rule_lbl->set_text(full_line);
        rule_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        rule_lbl->set_modulate(overridden ? godot::Color(0.62f, 0.55f, 0.55f, 0.65f) : godot::Color(0.92f, 0.96f, 0.98f, 1.0f));
        row->add_child(rule_lbl);

        godot::Button *insp_btn = memnew(godot::Button);
        insp_btn->set_text("Inspect");
        godot::Array bind_args;
        bind_args.append(rule.ptr());
        insp_btn->connect("pressed", godot::Callable(this, "_on_inspect_resource_pressed").bindv(bind_args));
        row->add_child(insp_btn);

        _waterfall_rules_vbox->add_child(row);
    }
}

godot::String GameplayStyleCascadeDock::_layer_to_badge_string(int p_layer) {
    switch (p_layer) {
        case RuleCascadeLayer::LAYER_BASE: return "@base";
        case RuleCascadeLayer::LAYER_MODE: return "@mode";
        case RuleCascadeLayer::LAYER_DIFFICULTY: return "@difficulty";
        case RuleCascadeLayer::LAYER_ENVIRONMENT: return "@board";
        case RuleCascadeLayer::LAYER_PLAYER_ACCESSIBILITY: return "@assist";
        case RuleCascadeLayer::LAYER_INLINE_LOCAL: return "@inline";
        default: return "@layer";
    }
}

godot::String GameplayStyleCascadeDock::_op_to_symbol_string(int p_op) {
    switch (p_op) {
        case RuleOp::SET: return ":=";
        case RuleOp::ADD: return "+=";
        case RuleOp::MULTIPLY: return "*=";
        case RuleOp::MODIFIER_ADD: return "+mod";
        case RuleOp::CLAMP: return "clamp";
        case RuleOp::MODIFIER_REMOVE: return "-mod";
        case RuleOp::GROUP_ADD: return "+grp";
        case RuleOp::GROUP_REMOVE: return "-grp";
        case RuleOp::INTERACTION_WEIGHT: return "weight";
        case RuleOp::SIGNAL_CONNECT: return "connect";
        case RuleOp::NODE_RETARGET: return "retarget";
        default: return "=";
    }
}

godot::String GameplayStyleCascadeDock::_scope_to_symbol_string(int p_scope) {
    switch (p_scope) {
        case RuleScope::SELF: return ":host";
        case RuleScope::DESCENDANTS: return "*";
        case RuleScope::ALL: return ":host,*";
        default: return "";
    }
}

void GameplayStyleCascadeDock::_on_entity_picker_selected(int p_index) {
    if (p_index < 0 || p_index >= _picker_node_ids.size()) return;
    uint64_t raw_id = _picker_node_ids[p_index];
    _selected_node_id = godot::ObjectID(raw_id);
    refresh_cascade_view();
}

void GameplayStyleCascadeDock::_on_refresh_pressed() {
    refresh_cascade_view();
}

void GameplayStyleCascadeDock::_on_apply_live_pressed() {
    godot::Node *target = get_inspected_node();
    if (!target) return;

    godot::Array chain_names;
    godot::Ref<GameplayStyle> resolved = _simulate_resolved_style_for_node(target, chain_names);
    if (resolved.is_valid()) {
        resolved->apply_rules_to_object(target, true, true);
        refresh_cascade_view();
    }
}

void GameplayStyleCascadeDock::_on_revert_live_pressed() {
    godot::Node *target = get_inspected_node();
    if (!target) return;
    GameplayRule::revert_all_from_object(target);
    refresh_cascade_view();
}

void GameplayStyleCascadeDock::_on_inspect_resource_pressed(godot::Object *p_resource) {
    if (!p_resource) return;
    if (godot::EditorInterface *ei = godot::EditorInterface::get_singleton()) {
        ei->inspect_object(p_resource);
    }
}

} // namespace ideam::godot_ext
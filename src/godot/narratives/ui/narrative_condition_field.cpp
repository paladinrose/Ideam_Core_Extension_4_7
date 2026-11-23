#include "narrative_condition_field.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/rect2i.hpp>

using namespace godot;

namespace ideam::godot_ext {

void NarrativeConditionField::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_narrative", "narrative"), &NarrativeConditionField::set_narrative);
    ClassDB::bind_method(D_METHOD("get_narrative"), &NarrativeConditionField::get_narrative);
    ClassDB::bind_method(D_METHOD("set_condition", "condition"), &NarrativeConditionField::set_condition);
    ClassDB::bind_method(D_METHOD("get_condition"), &NarrativeConditionField::get_condition);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &NarrativeConditionField::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &NarrativeConditionField::get_tool_context);

    ClassDB::bind_method(D_METHOD("set_subject_filter_mode", "mode"), &NarrativeConditionField::set_subject_filter_mode);
    ClassDB::bind_method(D_METHOD("get_subject_filter_mode"), &NarrativeConditionField::get_subject_filter_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "subject_filter_mode"), "set_subject_filter_mode", "get_subject_filter_mode");

    ClassDB::bind_method(D_METHOD("set_object_filter_mode", "mode"), &NarrativeConditionField::set_object_filter_mode);
    ClassDB::bind_method(D_METHOD("get_object_filter_mode"), &NarrativeConditionField::get_object_filter_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "object_filter_mode"), "set_object_filter_mode", "get_object_filter_mode");

    ClassDB::bind_method(D_METHOD("refresh_display"), &NarrativeConditionField::refresh_display);

    ClassDB::bind_method(D_METHOD("_on_subject_btn_pressed"), &NarrativeConditionField::_on_subject_btn_pressed);
    ClassDB::bind_method(D_METHOD("_on_object_btn_pressed"), &NarrativeConditionField::_on_object_btn_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_object_pressed"), &NarrativeConditionField::_on_clear_object_pressed);
    ClassDB::bind_method(D_METHOD("_on_subject_popup_id_pressed", "id"), &NarrativeConditionField::_on_subject_popup_id_pressed);
    ClassDB::bind_method(D_METHOD("_on_object_popup_id_pressed", "id"), &NarrativeConditionField::_on_object_popup_id_pressed);
    ClassDB::bind_method(D_METHOD("_on_predicate_selected", "index"), &NarrativeConditionField::_on_predicate_selected);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &NarrativeConditionField::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_condition_resource_changed"), &NarrativeConditionField::_on_condition_resource_changed);

    ADD_SIGNAL(MethodInfo(
        "condition_edited",
        PropertyInfo(Variant::OBJECT, "condition", PROPERTY_HINT_RESOURCE_TYPE, "Narrative_Condition")
    ));
}

NarrativeConditionField::NarrativeConditionField() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);
    domain_tool::stamp_ui_selector(this, "narratives.condition_field");
    domain_tool::attach_help_topic(this, StringName("NarrativeConditionField"));
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());
    _rebuild_ui();
}

PackedStringArray NarrativeConditionField::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("narrative");
    tags.append("condition");
    tags.append("predicate");
    tags.append("semantics");
    tags.append("framenet");
    return tags;
}

void NarrativeConditionField::_rebuild_ui() {
    subject_btn_ = memnew(Button);
    subject_btn_->set_text("[Subject...]");
    subject_btn_->set_tooltip_text("Select Subject Narreme (filterable by Literal / Symbolic)");
    subject_btn_->set_custom_minimum_size(Vector2(130, 0));
    domain_tool::stamp_ui_selector(subject_btn_, "narratives.condition_field.subject");
    subject_btn_->connect("pressed", Callable(this, "_on_subject_btn_pressed"));
    add_child(subject_btn_);

    predicate_opt_ = memnew(OptionButton);
    predicate_opt_->set_h_size_flags(SIZE_EXPAND_FILL);
    predicate_opt_->set_tooltip_text("Condition predicate (language-agnostic semantic relation)");
    domain_tool::stamp_ui_selector(predicate_opt_, "narratives.condition_field.predicate");
    predicate_opt_->connect("item_selected", Callable(this, "_on_predicate_selected"));
    add_child(predicate_opt_);

    object_btn_ = memnew(Button);
    object_btn_->set_text("[+ Object]");
    object_btn_->set_tooltip_text("Optional Object Narreme (switches predicate to binary form)");
    object_btn_->set_custom_minimum_size(Vector2(125, 0));
    domain_tool::stamp_ui_selector(object_btn_, "narratives.condition_field.object");
    object_btn_->connect("pressed", Callable(this, "_on_object_btn_pressed"));
    add_child(object_btn_);

    clear_object_btn_ = memnew(Button);
    clear_object_btn_->set_text("x");
    clear_object_btn_->set_tooltip_text("Clear Object (return to sole-subject condition)");
    clear_object_btn_->set_visible(false);
    domain_tool::stamp_ui_selector(clear_object_btn_, "narratives.condition_field.clear_object");
    clear_object_btn_->connect("pressed", Callable(this, "_on_clear_object_pressed"));
    add_child(clear_object_btn_);

    semantic_badge_lbl_ = memnew(Label);
    semantic_badge_lbl_->set_text("");
    semantic_badge_lbl_->add_theme_color_override("font_color", Color(0.62f, 0.85f, 0.96f));
    domain_tool::stamp_ui_selector(semantic_badge_lbl_, "narratives.condition_field.semantic_badge");
    add_child(semantic_badge_lbl_);

    help_btn_ = memnew(Button);
    help_btn_->set_text("?");
    help_btn_->set_tooltip_text("Help: Narrative Condition & Semantics");
    domain_tool::stamp_ui_selector(help_btn_, "narratives.condition_field.help");
    help_btn_->connect("pressed", Callable(this, "_on_help_pressed"));
    add_child(help_btn_);

    subject_popup_ = memnew(PopupMenu);
    subject_popup_->connect("id_pressed", Callable(this, "_on_subject_popup_id_pressed"));
    add_child(subject_popup_);

    object_popup_ = memnew(PopupMenu);
    object_popup_->connect("id_pressed", Callable(this, "_on_object_popup_id_pressed"));
    add_child(object_popup_);
}

void NarrativeConditionField::bind_domain_asset(
    const Ref<Resource> &p_asset,
    const Ref<ToolContext> &p_context
) {
    set_tool_context(p_context);
    if (Ref<Narrative_Condition> nc = p_asset; nc.is_valid()) {
        set_condition(nc);
    } else if (Ref<Narrative> nar = p_asset; nar.is_valid()) {
        set_narrative(nar);
    }
}

void NarrativeConditionField::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
    if (condition_.is_valid() && tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        condition_->set_undo_redo(tool_context_->get_undo_redo());
    }
}

void NarrativeConditionField::set_narrative(const Ref<Narrative> &p_narrative) {
    narrative_ = p_narrative;
    refresh_display();
}

void NarrativeConditionField::set_condition(const Ref<Narrative_Condition> &p_condition) {
    if (condition_.is_valid() && condition_->is_connected("changed", Callable(this, "_on_condition_resource_changed"))) {
        condition_->disconnect("changed", Callable(this, "_on_condition_resource_changed"));
    }
    condition_ = p_condition;
    if (condition_.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
            condition_->set_undo_redo(tool_context_->get_undo_redo());
        }
        condition_->connect("changed", Callable(this, "_on_condition_resource_changed"));
    }
    refresh_display();
}

void NarrativeConditionField::set_subject_filter_mode(NarremeFilterMode p_mode) {
    subject_filter_mode_ = p_mode;
}

void NarrativeConditionField::set_object_filter_mode(NarremeFilterMode p_mode) {
    object_filter_mode_ = p_mode;
}

String NarrativeConditionField::_format_narreme_label(const Ref<Narreme> &p_narreme) const {
    if (!p_narreme.is_valid()) {
        return String();
    }
    String name = p_narreme->get_official_name();
    if (name.is_empty()) {
        name = p_narreme->get_class_name_str();
    }
    if (narrative_.is_valid() && narrative_->is_narreme_symbolic(p_narreme)) {
        return String("[Sym] ") + name;
    }
    return name;
}

void NarrativeConditionField::refresh_display() {
    updating_ui_ = true;

    if (!condition_.is_valid()) {
        subject_btn_->set_text("[No Condition]");
        object_btn_->set_text("[+ Object]");
        clear_object_btn_->set_visible(false);
        predicate_opt_->clear();
        semantic_badge_lbl_->set_text("");
        updating_ui_ = false;
        return;
    }

    const Ref<Narreme> subj = condition_->get_subject();
    const Ref<Narreme> obj  = condition_->get_object();

    subject_btn_->set_text(subj.is_valid() ? _format_narreme_label(subj) : String("[Subject...]"));
    if (obj.is_valid()) {
        object_btn_->set_text(_format_narreme_label(obj));
        clear_object_btn_->set_visible(true);
    } else {
        object_btn_->set_text("[+ Object]");
        clear_object_btn_->set_visible(false);
    }

    _refresh_predicate_options();
    _refresh_semantic_badge();

    updating_ui_ = false;
}

void NarrativeConditionField::_refresh_predicate_options() {
    predicate_opt_->clear();
    active_descriptors_.clear();

    if (!condition_.is_valid() || !condition_->get_subject().is_valid()) {
        predicate_opt_->add_item("(pick a subject first)", -1);
        predicate_opt_->set_disabled(true);
        return;
    }

    predicate_opt_->set_disabled(false);
    const Ref<Narreme> subj = condition_->get_subject();
    const Ref<Narreme> obj  = condition_->get_object();
    active_descriptors_     = subj->get_narrative_condition_descriptors(obj.ptr());

    if (active_descriptors_.is_empty()) {
        predicate_opt_->add_item("(no valid conditions)", -1);
        predicate_opt_->set_disabled(true);
        return;
    }

    int selected_idx = 0;
    const int current_id = condition_->get_condition();
    for (int i = 0; i < active_descriptors_.size(); ++i) {
        const Dictionary desc = active_descriptors_[i];
        const int cid         = static_cast<int>(desc.get("condition_id", i));
        const String frag     = String(desc.get("display_fragment", ""));
        predicate_opt_->add_item(frag, cid);
        if (cid == current_id) {
            selected_idx = i;
        }
    }
    predicate_opt_->select(selected_idx);
}

void NarrativeConditionField::_refresh_semantic_badge() {
    if (!condition_.is_valid() || !condition_->get_subject().is_valid()) {
        semantic_badge_lbl_->set_text("");
        semantic_badge_lbl_->set_tooltip_text("");
        return;
    }

    const Dictionary desc = condition_->get_semantic_descriptor();
    if (desc.is_empty()) {
        semantic_badge_lbl_->set_text("");
        semantic_badge_lbl_->set_tooltip_text("");
        return;
    }

    const String frame_name = String(desc.get("frame_name", ""));
    const String key        = String(desc.get("canonical_key", ""));
    const String subj_role  = String(desc.get("subject_role", ""));
    const String obj_role   = String(desc.get("object_role", ""));
    const PackedStringArray tags = desc.get("seme_tags", PackedStringArray());

    semantic_badge_lbl_->set_text(String("[") + frame_name + String("]"));
    String tip = String("Canonical Key: ") + key + String("\nFrameNet Frame: ") + frame_name;
    if (!subj_role.is_empty()) {
        tip += String("\nRoles: ") + subj_role;
        if (!obj_role.is_empty()) {
            tip += String(" -> ") + obj_role;
        }
    }
    if (!tags.is_empty()) {
        tip += String("\nSemes: ") + String(", ").join(tags);
    }
    semantic_badge_lbl_->set_tooltip_text(tip);
}

void NarrativeConditionField::_populate_picker_popup(
    PopupMenu *p_popup,
    NarremeFilterMode p_filter,
    bool p_allow_clear,
    std::vector<Ref<Narreme>> &r_candidates
) {
    p_popup->clear();
    r_candidates.clear();

    // Filter scope toggles at the top of the popup
    p_popup->add_radio_check_item("Scope: All Narremes", POPUP_FILTER_ALL);
    p_popup->set_item_checked(p_popup->get_item_index(POPUP_FILTER_ALL), p_filter == FILTER_ALL);

    p_popup->add_radio_check_item("Scope: Literal (Real) Only", POPUP_FILTER_LITERAL);
    p_popup->set_item_checked(p_popup->get_item_index(POPUP_FILTER_LITERAL), p_filter == FILTER_LITERAL_ONLY);

    p_popup->add_radio_check_item("Scope: Symbolic Forms Only", POPUP_FILTER_SYMBOLIC);
    p_popup->set_item_checked(p_popup->get_item_index(POPUP_FILTER_SYMBOLIC), p_filter == FILTER_SYMBOLIC_ONLY);

    if (p_allow_clear) {
        p_popup->add_separator();
        p_popup->add_item("(Clear Object — Sole-Subject Mode)", POPUP_CLEAR_SELECTION);
    }

    p_popup->add_separator();

    if (!narrative_.is_valid()) {
        p_popup->add_item("(No Narrative bound)", -1);
        p_popup->set_item_disabled(p_popup->get_item_count() - 1, true);
        return;
    }

    const TypedArray<Narreme> all = narrative_->get_all_narremes(p_filter);
    if (all.is_empty()) {
        p_popup->add_item("(No matching narremes)", -1);
        p_popup->set_item_disabled(p_popup->get_item_count() - 1, true);
        return;
    }

    for (int i = 0; i < all.size(); ++i) {
        Ref<Narreme> n = all[i];
        if (!n.is_valid()) continue;
        const int idx = static_cast<int>(r_candidates.size());
        r_candidates.push_back(n);

        const String cls   = n->get_class_name_str();
        const String label = _format_narreme_label(n) + String("  (") + cls + String(")");
        p_popup->add_item(label, idx);
    }
}

void NarrativeConditionField::_on_subject_btn_pressed() {
    _populate_picker_popup(subject_popup_, subject_filter_mode_, false, subject_candidates_);
    const Vector2 pos = subject_btn_->get_screen_position();
    const Vector2 size = subject_btn_->get_size();
    subject_popup_->set_position(Vector2i(static_cast<int>(pos.x), static_cast<int>(pos.y + size.y)));
    subject_popup_->popup();
}

void NarrativeConditionField::_on_object_btn_pressed() {
    _populate_picker_popup(object_popup_, object_filter_mode_, true, object_candidates_);
    const Vector2 pos = object_btn_->get_screen_position();
    const Vector2 size = object_btn_->get_size();
    object_popup_->set_position(Vector2i(static_cast<int>(pos.x), static_cast<int>(pos.y + size.y)));
    object_popup_->popup();
}

void NarrativeConditionField::_on_clear_object_pressed() {
    if (!condition_.is_valid() || !condition_->get_object().is_valid()) return;
    condition_->commit_configuration(
        condition_->get_subject(),
        0,
        Ref<Narreme>(),
        condition_->get_subject_is_symbolic(),
        false,
        "Clear Condition Object"
    );
    emit_signal("condition_edited", condition_);
}

void NarrativeConditionField::_on_subject_popup_id_pressed(int p_id) {
    if (p_id == POPUP_FILTER_ALL) {
        subject_filter_mode_ = FILTER_ALL;
        _on_subject_btn_pressed();
        return;
    }
    if (p_id == POPUP_FILTER_LITERAL) {
        subject_filter_mode_ = FILTER_LITERAL_ONLY;
        _on_subject_btn_pressed();
        return;
    }
    if (p_id == POPUP_FILTER_SYMBOLIC) {
        subject_filter_mode_ = FILTER_SYMBOLIC_ONLY;
        _on_subject_btn_pressed();
        return;
    }
    if (!condition_.is_valid() || p_id < 0 || p_id >= static_cast<int>(subject_candidates_.size())) {
        return;
    }

    const Ref<Narreme> chosen = subject_candidates_[static_cast<size_t>(p_id)];
    const bool is_sym         = narrative_.is_valid() && narrative_->is_narreme_symbolic(chosen);
    condition_->commit_configuration(
        chosen,
        0,
        condition_->get_object(),
        is_sym,
        condition_->get_object_is_symbolic(),
        "Set Condition Subject"
    );
    emit_signal("condition_edited", condition_);
}

void NarrativeConditionField::_on_object_popup_id_pressed(int p_id) {
    if (p_id == POPUP_FILTER_ALL) {
        object_filter_mode_ = FILTER_ALL;
        _on_object_btn_pressed();
        return;
    }
    if (p_id == POPUP_FILTER_LITERAL) {
        object_filter_mode_ = FILTER_LITERAL_ONLY;
        _on_object_btn_pressed();
        return;
    }
    if (p_id == POPUP_FILTER_SYMBOLIC) {
        object_filter_mode_ = FILTER_SYMBOLIC_ONLY;
        _on_object_btn_pressed();
        return;
    }
    if (p_id == POPUP_CLEAR_SELECTION) {
        _on_clear_object_pressed();
        return;
    }
    if (!condition_.is_valid() || p_id < 0 || p_id >= static_cast<int>(object_candidates_.size())) {
        return;
    }

    const Ref<Narreme> chosen = object_candidates_[static_cast<size_t>(p_id)];
    const bool is_sym         = narrative_.is_valid() && narrative_->is_narreme_symbolic(chosen);
    condition_->commit_configuration(
        condition_->get_subject(),
        0,
        chosen,
        condition_->get_subject_is_symbolic(),
        is_sym,
        "Set Condition Object"
    );
    emit_signal("condition_edited", condition_);
}

void NarrativeConditionField::_on_predicate_selected(int p_index) {
    if (updating_ui_ || !condition_.is_valid() || p_index < 0) return;
    const int cid = predicate_opt_->get_item_id(p_index);
    if (cid < 0) return;

    condition_->commit_configuration(
        condition_->get_subject(),
        cid,
        condition_->get_object(),
        condition_->get_subject_is_symbolic(),
        condition_->get_object_is_symbolic(),
        "Change Condition Predicate"
    );
    emit_signal("condition_edited", condition_);
}

void NarrativeConditionField::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void NarrativeConditionField::_on_condition_resource_changed() {
    refresh_display();
}

} // namespace ideam::godot_ext

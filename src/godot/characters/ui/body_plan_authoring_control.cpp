#include "body_plan_authoring_control.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void BodyPlanAuthoringControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_body_plan", "body_plan"), &BodyPlanAuthoringControl::set_body_plan);
    ClassDB::bind_method(D_METHOD("get_body_plan"), &BodyPlanAuthoringControl::get_body_plan);
    ClassDB::bind_method(D_METHOD("set_gallery_bank", "bank"), &BodyPlanAuthoringControl::set_gallery_bank);
    ClassDB::bind_method(D_METHOD("get_gallery_bank"), &BodyPlanAuthoringControl::get_gallery_bank);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &BodyPlanAuthoringControl::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &BodyPlanAuthoringControl::get_tool_context);

    ClassDB::bind_method(D_METHOD("_on_bank_tab_changed", "tab"), &BodyPlanAuthoringControl::_on_bank_tab_changed);
    ClassDB::bind_method(D_METHOD("_on_search_changed", "text"), &BodyPlanAuthoringControl::_on_search_changed);
    ClassDB::bind_method(D_METHOD("_on_preset_selected", "index"), &BodyPlanAuthoringControl::_on_preset_selected);
    ClassDB::bind_method(D_METHOD("_on_apply_preset_pressed"), &BodyPlanAuthoringControl::_on_apply_preset_pressed);
    ClassDB::bind_method(D_METHOD("_on_starter_humanoid_pressed"), &BodyPlanAuthoringControl::_on_starter_humanoid_pressed);
    ClassDB::bind_method(D_METHOD("_on_starter_quadruped_pressed"), &BodyPlanAuthoringControl::_on_starter_quadruped_pressed);
    ClassDB::bind_method(D_METHOD("_on_starter_avian_pressed"), &BodyPlanAuthoringControl::_on_starter_avian_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &BodyPlanAuthoringControl::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_plan_changed"), &BodyPlanAuthoringControl::_on_plan_changed);
}

BodyPlanAuthoringControl::BodyPlanAuthoringControl() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);

    gallery_bank_.instantiate();

    domain_tool::stamp_ui_selector(this, "characters.plan_authoring");
    domain_tool::attach_help_topic(this, get_domain_tool_id());
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());

    _build_ui();
    _refresh_bank_list();
}

PackedStringArray BodyPlanAuthoringControl::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("characters");
    tags.append("body_plan");
    tags.append("gallery_banks");
    tags.append("loomis");
    return tags;
}

void BodyPlanAuthoringControl::_build_ui() {
    auto *top_bar = memnew(HBoxContainer);
    top_bar->add_theme_constant_override("separation", 6);
    add_child(top_bar);

    auto *title = memnew(Label);
    title->set_text("Plan Authoring — Loomis Gallery Banks");
    title->set_h_size_flags(SIZE_EXPAND_FILL);
    top_bar->add_child(title);

    auto add_starter = [&](const char *label, const char *sel, const char *method) {
        auto *b = memnew(Button);
        b->set_text(label);
        domain_tool::stamp_ui_selector(b, sel);
        b->connect("pressed", Callable(this, method));
        top_bar->add_child(b);
    };
    add_starter("Humanoid Base",  "characters.plan_authoring.starter_humanoid",  "_on_starter_humanoid_pressed");
    add_starter("Quadruped Base", "characters.plan_authoring.starter_quadruped", "_on_starter_quadruped_pressed");
    add_starter("Avian Base",     "characters.plan_authoring.starter_avian",     "_on_starter_avian_pressed");

    auto *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for BodyPlan Authoring & Gallery Banks");
    domain_tool::stamp_ui_selector(help_btn, "characters.plan_authoring.help_btn");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed"));
    top_bar->add_child(help_btn);

    bank_tabs_ = memnew(TabBar);
    bank_tabs_->add_tab("ScaffoldBank");
    bank_tabs_->add_tab("TopologyBank");
    bank_tabs_->add_tab("FeatureBank");
    bank_tabs_->add_tab("DetailBank");
    domain_tool::stamp_ui_selector(bank_tabs_, "characters.banks.tabs");
    bank_tabs_->connect("tab_changed", Callable(this, "_on_bank_tab_changed"));
    add_child(bank_tabs_);

    auto *search_row = memnew(HBoxContainer);
    search_row->add_theme_constant_override("separation", 6);
    add_child(search_row);

    search_edit_ = memnew(LineEdit);
    search_edit_->set_placeholder("Filter presets by name or tag (e.g. loomis, asaro, horn, avian)...");
    search_edit_->set_h_size_flags(SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(search_edit_, "characters.banks.search");
    search_edit_->connect("text_changed", Callable(this, "_on_search_changed"));
    search_row->add_child(search_edit_);

    apply_btn_ = memnew(Button);
    apply_btn_->set_text("Apply Selected Preset");
    apply_btn_->set_disabled(true);
    domain_tool::stamp_ui_selector(apply_btn_, "characters.banks.apply_btn");
    apply_btn_->connect("pressed", Callable(this, "_on_apply_preset_pressed"));
    search_row->add_child(apply_btn_);

    preset_list_ = memnew(ItemList);
    preset_list_->set_custom_minimum_size(Vector2(280.0f, 180.0f));
    preset_list_->set_v_size_flags(SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(preset_list_, "characters.banks.preset_list");
    preset_list_->connect("item_selected", Callable(this, "_on_preset_selected"));
    add_child(preset_list_);

    preset_desc_lbl_ = memnew(Label);
    preset_desc_lbl_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    preset_desc_lbl_->add_theme_color_override("font_color", Color(0.75f, 0.88f, 1.0f));
    add_child(preset_desc_lbl_);

    plan_summary_lbl_ = memnew(Label);
    plan_summary_lbl_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(plan_summary_lbl_);
}

void BodyPlanAuthoringControl::bind_domain_asset(const Ref<Resource> &p_asset, const Ref<ToolContext> &p_context) {
    set_tool_context(p_context);
    if (Ref<BodyPlanResource> bp = p_asset; bp.is_valid()) {
        set_body_plan(bp);
    } else if (Ref<CharacterGalleryBankResource> bank = p_asset; bank.is_valid()) {
        set_gallery_bank(bank);
    }
}

void BodyPlanAuthoringControl::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
    if (body_plan_.is_valid() && tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        body_plan_->set_undo_redo(tool_context_->get_undo_redo());
    }
}

void BodyPlanAuthoringControl::set_body_plan(const Ref<BodyPlanResource> &p_plan) {
    const Callable cb(this, "_on_plan_changed");
    if (body_plan_.is_valid() && body_plan_->is_connected("changed", cb)) body_plan_->disconnect("changed", cb);
    body_plan_ = p_plan;
    if (body_plan_.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
            body_plan_->set_undo_redo(tool_context_->get_undo_redo());
        }
        if (!body_plan_->is_connected("changed", cb)) body_plan_->connect("changed", cb);
    }
    _refresh_plan_summary();
}

void BodyPlanAuthoringControl::set_gallery_bank(const Ref<CharacterGalleryBankResource> &p_bank) {
    if (p_bank.is_valid()) {
        gallery_bank_ = p_bank;
        _refresh_bank_list();
    }
}

void BodyPlanAuthoringControl::_refresh_bank_list() {
    if (!preset_list_ || !gallery_bank_.is_valid()) return;
    preset_list_->clear();
    displayed_preset_ids_.clear();

    const String filter = search_edit_ ? search_edit_->get_text() : String();
    const TypedArray<Dictionary> items = gallery_bank_->get_presets_for_bank(active_bank_, filter);
    for (int64_t i = 0; i < items.size(); ++i) {
        const Dictionary d = items[i];
        preset_list_->add_item(String(d.get("display_name", "Preset")));
        displayed_preset_ids_.append(String(d.get("id", "")));
    }
    if (apply_btn_) apply_btn_->set_disabled(true);
    if (preset_desc_lbl_) preset_desc_lbl_->set_text("Select a preset to inspect and apply.");
}

void BodyPlanAuthoringControl::_refresh_plan_summary() {
    if (!plan_summary_lbl_) return;
    if (!body_plan_.is_valid()) {
        plan_summary_lbl_->set_text("No BodyPlanResource bound — open or create a Body Plan to assemble gallery presets.");
        return;
    }
    plan_summary_lbl_->set_text(
        "Active BodyPlan: " + body_plan_->get_species_name() +
        "  |  Param Bounds: " + String::num_int64(body_plan_->get_parameter_bounds_list().size()) +
        "  |  Landmarks: " + String::num_int64(body_plan_->get_topology_landmarks_list().size()) +
        "  |  Bridge Rules: " + String::num_int64(body_plan_->get_bridge_rules_list().size()) +
        "  |  Features: " + String::num_int64(body_plan_->get_features_list().size())
    );
}

void BodyPlanAuthoringControl::_on_bank_tab_changed(int p_tab) {
    active_bank_ = static_cast<CharacterBankCategory>(p_tab);
    _refresh_bank_list();
}

void BodyPlanAuthoringControl::_on_search_changed(const String &p_text) {
    (void)p_text;
    _refresh_bank_list();
}

void BodyPlanAuthoringControl::_on_preset_selected(int p_index) {
    if (p_index < 0 || p_index >= displayed_preset_ids_.size() || !gallery_bank_.is_valid()) return;
    const Dictionary d = gallery_bank_->find_preset_by_id(displayed_preset_ids_[p_index]);
    if (preset_desc_lbl_) preset_desc_lbl_->set_text(String(d.get("description", "")));
    if (apply_btn_) apply_btn_->set_disabled(!body_plan_.is_valid() || active_bank_ == BANK_DETAIL);
}

void BodyPlanAuthoringControl::_on_apply_preset_pressed() {
    if (!body_plan_.is_valid() || !gallery_bank_.is_valid() || !preset_list_) return;
    const PackedInt32Array sel = preset_list_->get_selected_items();
    if (sel.is_empty()) return;
    const int idx = sel[0];
    if (idx >= 0 && idx < displayed_preset_ids_.size()) {
        gallery_bank_->apply_preset_to_body_plan(displayed_preset_ids_[idx], body_plan_);
    }
}

void BodyPlanAuthoringControl::_on_starter_humanoid_pressed()  { if (body_plan_.is_valid()) body_plan_->setup_loomis_humanoid_plan(); }
void BodyPlanAuthoringControl::_on_starter_quadruped_pressed() { if (body_plan_.is_valid()) body_plan_->setup_loomis_quadruped_plan(); }
void BodyPlanAuthoringControl::_on_starter_avian_pressed()     { if (body_plan_.is_valid()) body_plan_->setup_loomis_avian_plan(); }

void BodyPlanAuthoringControl::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void BodyPlanAuthoringControl::_on_plan_changed() {
    _refresh_plan_summary();
}

} // namespace ideam::godot_ext

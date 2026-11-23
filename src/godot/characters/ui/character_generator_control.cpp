#include "character_generator_control.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

using namespace godot;

namespace ideam::godot_ext {

void CharacterGeneratorControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_character_instance", "instance"), &CharacterGeneratorControl::set_character_instance);
    ClassDB::bind_method(D_METHOD("get_character_instance"), &CharacterGeneratorControl::get_character_instance);
    ClassDB::bind_method(D_METHOD("set_gallery_bank", "bank"), &CharacterGeneratorControl::set_gallery_bank);
    ClassDB::bind_method(D_METHOD("get_gallery_bank"), &CharacterGeneratorControl::get_gallery_bank);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &CharacterGeneratorControl::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &CharacterGeneratorControl::get_tool_context);

    ClassDB::bind_method(D_METHOD("_on_slider_changed", "value", "param_name"), &CharacterGeneratorControl::_on_slider_changed);
    ClassDB::bind_method(D_METHOD("_on_randomize_pressed"), &CharacterGeneratorControl::_on_randomize_pressed);
    ClassDB::bind_method(D_METHOD("_on_reset_defaults_pressed"), &CharacterGeneratorControl::_on_reset_defaults_pressed);
    ClassDB::bind_method(D_METHOD("_on_lod_selected", "index"), &CharacterGeneratorControl::_on_lod_selected);
    ClassDB::bind_method(D_METHOD("_on_clear_details_pressed"), &CharacterGeneratorControl::_on_clear_details_pressed);
    ClassDB::bind_method(D_METHOD("_on_apply_detail_preset_pressed", "preset_id"), &CharacterGeneratorControl::_on_apply_detail_preset_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &CharacterGeneratorControl::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_instance_changed"), &CharacterGeneratorControl::_on_instance_changed);
}

CharacterGeneratorControl::CharacterGeneratorControl() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);

    gallery_bank_.instantiate();

    domain_tool::stamp_ui_selector(this, "characters.generator");
    domain_tool::attach_help_topic(this, get_domain_tool_id());
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());

    _build_ui();
}

PackedStringArray CharacterGeneratorControl::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("characters");
    tags.append("generator");
    tags.append("sliders");
    tags.append("loomis");
    return tags;
}

void CharacterGeneratorControl::_build_ui() {
    auto *bar = memnew(HBoxContainer);
    bar->add_theme_constant_override("separation", 6);
    add_child(bar);

    auto *rand_btn = memnew(Button);
    rand_btn->set_text("Randomize");
    domain_tool::stamp_ui_selector(rand_btn, "characters.generator.randomize_btn");
    rand_btn->connect("pressed", Callable(this, "_on_randomize_pressed"));
    bar->add_child(rand_btn);

    auto *seed_lbl = memnew(Label);
    seed_lbl->set_text("Seed:");
    bar->add_child(seed_lbl);

    seed_spin_ = memnew(SpinBox);
    seed_spin_->set_min(0);
    seed_spin_->set_max(999999);
    seed_spin_->set_value(1337);
    domain_tool::stamp_ui_selector(seed_spin_, "characters.generator.seed_spin");
    bar->add_child(seed_spin_);

    auto *reset_btn = memnew(Button);
    reset_btn->set_text("Reset Defaults");
    domain_tool::stamp_ui_selector(reset_btn, "characters.generator.reset_btn");
    reset_btn->connect("pressed", Callable(this, "_on_reset_defaults_pressed"));
    bar->add_child(reset_btn);

    lod_opt_ = memnew(OptionButton);
    lod_opt_->add_item("LOD 0 (Full)", 0);
    lod_opt_->add_item("LOD 1 (50%)", 1);
    lod_opt_->add_item("LOD 2 (25%)", 2);
    lod_opt_->add_item("LOD 3 (Cage)", 3);
    domain_tool::stamp_ui_selector(lod_opt_, "characters.generator.lod_selector");
    lod_opt_->connect("item_selected", Callable(this, "_on_lod_selected"));
    bar->add_child(lod_opt_);

    auto *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for Character Generator Mode");
    domain_tool::stamp_ui_selector(help_btn, "characters.generator.help_btn");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed"));
    bar->add_child(help_btn);

    auto *scroll = memnew(ScrollContainer);
    scroll->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll->set_custom_minimum_size(Vector2(300.0f, 180.0f));
    add_child(scroll);

    sliders_vbox_ = memnew(VBoxContainer);
    sliders_vbox_->set_h_size_flags(SIZE_EXPAND_FILL);
    sliders_vbox_->add_theme_constant_override("separation", 4);
    scroll->add_child(sliders_vbox_);

    // Quick Layer 4 Asymmetric Detail Bank Strip
    auto *detail_bar = memnew(HBoxContainer);
    detail_bar->add_theme_constant_override("separation", 6);
    add_child(detail_bar);

    auto *dlbl = memnew(Label);
    dlbl->set_text("Layer 4 Details:");
    detail_bar->add_child(dlbl);

    const TypedArray<Dictionary> details = gallery_bank_->get_presets_for_bank(BANK_DETAIL);
    for (int64_t i = 0; i < details.size(); ++i) {
        const Dictionary d = details[i];
        auto *b = memnew(Button);
        b->set_text(String("+ ") + String(d.get("display_name", "Detail")));
        b->connect("pressed", Callable(this, "_on_apply_detail_preset_pressed").bind(String(d.get("id", ""))));
        detail_bar->add_child(b);
    }

    auto *clear_btn = memnew(Button);
    clear_btn->set_text("Clear Layer 4");
    domain_tool::stamp_ui_selector(clear_btn, "characters.generator.clear_details_btn");
    clear_btn->connect("pressed", Callable(this, "_on_clear_details_pressed"));
    detail_bar->add_child(clear_btn);

    bridge_telemetry_lbl_ = memnew(Label);
    bridge_telemetry_lbl_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    bridge_telemetry_lbl_->add_theme_color_override("font_color", Color(0.75f, 0.90f, 1.0f));
    add_child(bridge_telemetry_lbl_);

    balance_telemetry_lbl_ = memnew(Label);
    balance_telemetry_lbl_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(balance_telemetry_lbl_);
}

void CharacterGeneratorControl::bind_domain_asset(const Ref<Resource> &p_asset, const Ref<ToolContext> &p_context) {
    set_tool_context(p_context);
    if (Ref<CharacterInstanceResource> ci = p_asset; ci.is_valid()) {
        set_character_instance(ci);
    }
}

void CharacterGeneratorControl::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
    if (instance_.is_valid() && tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        instance_->set_undo_redo(tool_context_->get_undo_redo());
    }
}

void CharacterGeneratorControl::set_character_instance(const Ref<CharacterInstanceResource> &p_instance) {
    const Callable cb(this, "_on_instance_changed");
    if (instance_.is_valid() && instance_->is_connected("changed", cb)) instance_->disconnect("changed", cb);
    instance_ = p_instance;
    if (instance_.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
            instance_->set_undo_redo(tool_context_->get_undo_redo());
        }
        if (!instance_->is_connected("changed", cb)) instance_->connect("changed", cb);
    }
    _rebuild_parameter_sliders();
    _refresh_telemetry();
}

void CharacterGeneratorControl::_rebuild_parameter_sliders() {
    if (!sliders_vbox_) return;
    rebuilding_sliders_ = true;

    while (sliders_vbox_->get_child_count() > 0) {
        Node *ch = sliders_vbox_->get_child(0);
        sliders_vbox_->remove_child(ch);
        ch->queue_free();
    }

    if (!instance_.is_valid() || !instance_->get_body_plan().is_valid()) {
        auto *lbl = memnew(Label);
        lbl->set_text("Bind a CharacterInstanceResource with a BodyPlanResource to edit bounded parameters.");
        sliders_vbox_->add_child(lbl);
        rebuilding_sliders_ = false;
        return;
    }

    const TypedArray<Dictionary> bounds = instance_->get_body_plan()->get_parameter_bounds_list();
    for (int64_t i = 0; i < bounds.size(); ++i) {
        const Dictionary b = bounds[i];
        const String pname = b.get("name", "param");
        const double min_v = double(b.get("min_value", 0.0));
        const double max_v = double(b.get("max_value", 1.0));
        const double cur_v = static_cast<double>(instance_->get_parameter_value(pname));

        auto *row = memnew(HBoxContainer);
        row->add_theme_constant_override("separation", 8);
        sliders_vbox_->add_child(row);

        auto *lbl = memnew(Label);
        lbl->set_text(pname);
        lbl->set_custom_minimum_size(Vector2(130.0f, 0.0f));
        row->add_child(lbl);

        auto *sld = memnew(HSlider);
        sld->set_min(min_v);
        sld->set_max(max_v);
        sld->set_step((max_v - min_v) * 0.01);
        sld->set_value(cur_v);
        sld->set_h_size_flags(SIZE_EXPAND_FILL);
        sld->connect("value_changed", Callable(this, "_on_slider_changed").bind(pname));
        row->add_child(sld);

        auto *val_lbl = memnew(Label);
        val_lbl->set_text(String::num(cur_v, 2));
        val_lbl->set_custom_minimum_size(Vector2(48.0f, 0.0f));
        row->add_child(val_lbl);
    }

    rebuilding_sliders_ = false;
}

void CharacterGeneratorControl::_refresh_telemetry() {
    if (!instance_.is_valid() || !instance_->get_body_plan().is_valid()) return;

    const TypedArray<Dictionary> regimes = instance_->get_evaluated_bridge_regimes();
    String br_text = "Topological Bridges: ";
    for (int64_t i = 0; i < regimes.size(); ++i) {
        const Dictionary d = regimes[i];
        const int r = int(d.get("active_regime", 0));
        const char *r_str = (r == 2) ? "DEEP_FOLD" : ((r == 1) ? "CREASE" : "SMOOTH");
        if (i > 0) br_text += "  |  ";
        br_text += String(d.get("name", "Bridge")) + String(": ") + String(r_str) +
                   String(" (") + String::num(double(d.get("evaluated_metric", 0.0)), 2) + String(")");
    }
    if (bridge_telemetry_lbl_) bridge_telemetry_lbl_->set_text(br_text);

    const Dictionary bal = instance_->get_balance_report();
    if (balance_telemetry_lbl_) {
        balance_telemetry_lbl_->set_text(
            String("Character: ") + instance_->get_character_name() +
            String("  |  LOD ") + String::num_int64(active_lod_) +
            String("  |  Layer 4 Deltas: ") + String::num_int64(instance_->get_detail_deltas().size()) +
            String("  |  Loomis Balance: ") + (bool(bal.get("is_balanced", true)) ? String("Balanced") : String("Off-Balance"))
        );
    }
}

void CharacterGeneratorControl::_on_slider_changed(double p_val, const String &p_param_name) {
    if (rebuilding_sliders_ || !instance_.is_valid()) return;
    instance_->commit_set_parameter_value(p_param_name, static_cast<real_t>(p_val));
}

void CharacterGeneratorControl::_on_randomize_pressed() {
    if (!instance_.is_valid()) return;
    const int64_t seed = seed_spin_ ? static_cast<int64_t>(seed_spin_->get_value()) : 1337;
    instance_->randomize_within_bounds(seed);
    if (seed_spin_) seed_spin_->set_value(static_cast<double>((seed + 1) % 999999));
    _rebuild_parameter_sliders();
}

void CharacterGeneratorControl::_on_reset_defaults_pressed() {
    if (!instance_.is_valid() || !instance_->get_body_plan().is_valid()) return;
    const TypedArray<Dictionary> bounds = instance_->get_body_plan()->get_parameter_bounds_list();
    for (int64_t i = 0; i < bounds.size(); ++i) {
        const Dictionary b = bounds[i];
        instance_->set_parameter_value(String(b.get("name", "")), static_cast<real_t>(double(b.get("default_value", 0.0))));
    }
    _rebuild_parameter_sliders();
}

void CharacterGeneratorControl::_on_lod_selected(int p_index) {
    active_lod_ = std::clamp(p_index, 0, 3);
    _refresh_telemetry();
}

void CharacterGeneratorControl::_on_clear_details_pressed() {
    if (instance_.is_valid()) instance_->clear_detail_deltas();
}

void CharacterGeneratorControl::_on_apply_detail_preset_pressed(const String &p_preset_id) {
    if (instance_.is_valid() && gallery_bank_.is_valid()) {
        gallery_bank_->apply_preset_to_character_instance(p_preset_id, instance_);
    }
}

void CharacterGeneratorControl::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void CharacterGeneratorControl::_on_instance_changed() {
    _refresh_telemetry();
}

} // namespace ideam::godot_ext

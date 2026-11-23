#include "character_morph_slider_panel.h"
#include "../ideam_characters_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void CharacterMorphSliderPanel::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        if (get_child_count() == 0) {
            _rebuild_ui();
        }
        _sync_from_blueprint();
    }
}

void CharacterMorphSliderPanel::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &CharacterMorphSliderPanel::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &CharacterMorphSliderPanel::get_blueprint);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "blueprint", PROPERTY_HINT_RESOURCE_TYPE, "CharacterBlueprintResource"),
        "set_blueprint",
        "get_blueprint"
    );

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &CharacterMorphSliderPanel::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &CharacterMorphSliderPanel::get_undo_redo);

    ClassDB::bind_method(D_METHOD("refresh_from_blueprint"), &CharacterMorphSliderPanel::refresh_from_blueprint);
    ClassDB::bind_method(D_METHOD("_on_archetype_selected", "index"), &CharacterMorphSliderPanel::_on_archetype_selected);
    ClassDB::bind_method(D_METHOD("_on_lod_value_changed", "value"), &CharacterMorphSliderPanel::_on_lod_value_changed);
    ClassDB::bind_method(D_METHOD("_on_morph_slider_changed", "value", "channel"), &CharacterMorphSliderPanel::_on_morph_slider_changed);
    ClassDB::bind_method(D_METHOD("_on_reset_morphs_pressed"), &CharacterMorphSliderPanel::_on_reset_morphs_pressed);

    ADD_SIGNAL(MethodInfo("morph_sliders_changed"));
}

void CharacterMorphSliderPanel::set_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint) {
    blueprint_ = p_blueprint;
    if (get_child_count() == 0) {
        _rebuild_ui();
    }
    _sync_from_blueprint();
}

void CharacterMorphSliderPanel::refresh_from_blueprint() {
    _sync_from_blueprint();
}

void CharacterMorphSliderPanel::_rebuild_ui() {
    using namespace godot;

    for (int i = get_child_count() - 1; i >= 0; --i) {
        Node *c = get_child(i);
        remove_child(c);
        memdelete(c);
    }
    channel_sliders_.clear();
    channel_value_labels_.clear();

    add_theme_constant_override("separation", 6);

    // Top row: Body Archetype + Subdivision LOD + Reset Button
    HBoxContainer *top_row = memnew(HBoxContainer);
    top_row->add_theme_constant_override("separation", 8);
    add_child(top_row);

    Label *arch_lbl = memnew(Label);
    arch_lbl->set_text("Archetype:");
    top_row->add_child(arch_lbl);

    archetype_selector_ = memnew(OptionButton);
    archetype_selector_->add_item("Biped", 0);
    archetype_selector_->add_item("Quadruped", 1);
    archetype_selector_->add_item("Avian", 2);
    archetype_selector_->add_item("FlyingEye", 3);
    archetype_selector_->add_item("Serpentine", 4);
    archetype_selector_->add_item("Custom", 5);
    archetype_selector_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    archetype_selector_->connect("item_selected", Callable(this, "_on_archetype_selected"));
    top_row->add_child(archetype_selector_);

    Label *lod_lbl = memnew(Label);
    lod_lbl->set_text("LOD:");
    top_row->add_child(lod_lbl);

    lod_spin_box_ = memnew(SpinBox);
    lod_spin_box_->set_min(0.0);
    lod_spin_box_->set_max(3.0);
    lod_spin_box_->set_step(1.0);
    lod_spin_box_->set_value(1.0);
    lod_spin_box_->connect("value_changed", Callable(this, "_on_lod_value_changed"));
    top_row->add_child(lod_spin_box_);

    Button *reset_btn = memnew(Button);
    reset_btn->set_text("Reset Sliders");
    reset_btn->connect("pressed", Callable(this, "_on_reset_morphs_pressed"));
    top_row->add_child(reset_btn);

    telemetry_label_ = memnew(Label);
    telemetry_label_->add_theme_color_override("font_color", Color(0.72f, 0.85f, 0.98f));
    add_child(telemetry_label_);

    // 12 Anatomical Sparse Morph Sliders
    const int n_channels = static_cast<int>(core::MORPH_DEFAULT_CHANNEL_COUNT);
    channel_sliders_.resize(static_cast<size_t>(n_channels), nullptr);
    channel_value_labels_.resize(static_cast<size_t>(n_channels), nullptr);

    for (int ch = 0; ch < n_channels; ++ch) {
        HBoxContainer *row = memnew(HBoxContainer);
        row->add_theme_constant_override("separation", 6);
        add_child(row);

        const std::string_view ch_name = core::morph_channel_to_string(static_cast<uint32_t>(ch));
        Label *name_lbl = memnew(Label);
        name_lbl->set_text(String::utf8(ch_name.data(), static_cast<int>(ch_name.size())));
        name_lbl->set_custom_minimum_size(Vector2(155.0f, 0.0f));
        row->add_child(name_lbl);

        HSlider *slider = memnew(HSlider);
        slider->set_min(-1.0);
        slider->set_max(1.0);
        slider->set_step(0.01);
        slider->set_value(0.0);
        slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        slider->connect("value_changed", Callable(this, "_on_morph_slider_changed").bind(ch));
        row->add_child(slider);
        channel_sliders_[static_cast<size_t>(ch)] = slider;

        Label *val_lbl = memnew(Label);
        val_lbl->set_text("0.00");
        val_lbl->set_custom_minimum_size(Vector2(44.0f, 0.0f));
        row->add_child(val_lbl);
        channel_value_labels_[static_cast<size_t>(ch)] = val_lbl;
    }
}

void CharacterMorphSliderPanel::_sync_from_blueprint() {
    if (blueprint_.is_null()) {
        if (telemetry_label_) {
            telemetry_label_->set_text("No CharacterBlueprintResource assigned.");
        }
        return;
    }

    updating_ui_ = true;
    if (archetype_selector_) {
        archetype_selector_->select(blueprint_->get_body_archetype());
    }
    if (lod_spin_box_) {
        lod_spin_box_->set_value(static_cast<double>(blueprint_->get_subdivision_lod_level()));
    }
    if (telemetry_label_) {
        telemetry_label_->set_text(
            godot::String("Quad-Loop Vertices: ") + godot::String::num_int64(blueprint_->get_base_vertex_count()) +
            godot::String("  |  Quads: ") + godot::String::num_int64(blueprint_->get_base_quad_count()) +
            godot::String("  |  Sparse Morph Deltas: ") + godot::String::num_int64(blueprint_->get_sparse_morph_delta_count())
        );
    }

    for (size_t ch = 0; ch < channel_sliders_.size(); ++ch) {
        const float w = blueprint_->get_morph_channel_weight(static_cast<int>(ch));
        if (channel_sliders_[ch]) {
            channel_sliders_[ch]->set_value(static_cast<double>(w));
        }
        if (channel_value_labels_[ch]) {
            channel_value_labels_[ch]->set_text(godot::String::num(w, 2));
        }
    }
    updating_ui_ = false;
}

void CharacterMorphSliderPanel::_on_archetype_selected(int p_index) {
    if (updating_ui_ || blueprint_.is_null()) return;
    blueprint_->commit_set_body_archetype(p_index, undo_redo_);
    _sync_from_blueprint();
    emit_signal("morph_sliders_changed");
}

void CharacterMorphSliderPanel::_on_lod_value_changed(double p_value) {
    if (updating_ui_ || blueprint_.is_null()) return;
    blueprint_->commit_set_subdivision_lod_level(static_cast<int>(p_value), undo_redo_);
    _sync_from_blueprint();
    emit_signal("morph_sliders_changed");
}

void CharacterMorphSliderPanel::_on_morph_slider_changed(double p_value, int p_channel) {
    if (updating_ui_ || blueprint_.is_null()) return;
    const float w = static_cast<float>(p_value);
    if (p_channel >= 0 && static_cast<size_t>(p_channel) < channel_value_labels_.size() &&
        channel_value_labels_[static_cast<size_t>(p_channel)]) {
        channel_value_labels_[static_cast<size_t>(p_channel)]->set_text(godot::String::num(w, 2));
    }
    blueprint_->commit_set_morph_channel_weight(p_channel, w, undo_redo_);
    emit_signal("morph_sliders_changed");
}

void CharacterMorphSliderPanel::_on_reset_morphs_pressed() {
    if (blueprint_.is_null()) return;
    godot::PackedFloat32Array zeroes;
    zeroes.resize(blueprint_->get_morph_channel_count());
    for (int i = 0; i < zeroes.size(); ++i) {
        zeroes.set(i, 0.0f);
    }
    blueprint_->commit_set_morph_weights(zeroes, undo_redo_);
    _sync_from_blueprint();
    emit_signal("morph_sliders_changed");
}

} // namespace ideam::godot_ext

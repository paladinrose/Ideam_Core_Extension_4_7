#include "procedural_generator_inspector_base.h"
#include "../ideam_procedural_generation_plugin.h"
#include "../ui/procedural_composer.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void ProceduralGeneratorInspectorBase::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_open_in_procedural_composer", "resource"),
        &ProceduralGeneratorInspectorBase::_on_open_in_procedural_composer
    );
}

void ProceduralGeneratorInspectorBase::_on_open_in_procedural_composer(const godot::Ref<godot::Resource> &p_resource) {
    if (p_resource.is_valid()) {
        ProceduralComposer::open_procedural_resource(p_resource);
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *ProceduralGeneratorInspectorBase::get_undo_redo() const {
    return IdeamProceduralGenerationPlugin::get_static_undo_redo();
}
#endif

bool ProceduralGeneratorInspectorBase::_can_handle(godot::Object *p_object) {
    if (!p_object) return false;
    return godot::Object::cast_to<ProceduralGeneratorResource>(p_object) != nullptr ||
           godot::Object::cast_to<AnalysisRule>(p_object) != nullptr ||
           godot::Object::cast_to<AnalysisStyle>(p_object) != nullptr ||
           godot::Object::cast_to<GenerationRule>(p_object) != nullptr;
}

void ProceduralGeneratorInspectorBase::_parse_begin(godot::Object *p_object) {
    using namespace godot;
    if (!p_object) return;

    auto *panel = memnew(PanelContainer);
    auto *vbox  = memnew(VBoxContainer);
    panel->add_child(vbox);

    auto *header_row = memnew(HBoxContainer);
    header_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    auto *title = memnew(Label);
    title->set_text("IdeamProceduralGeneration — " + p_object->get_class());
    title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    header_row->add_child(title);

    if (auto *res = Object::cast_to<Resource>(p_object)) {
        Ref<Resource> res_ref(res);
        auto *open_btn = memnew(Button);
        open_btn->set_text("Open in ProceduralComposer");
        open_btn->set_tooltip_text("Author and execute generators, WFC tensors, and Quantum Darwinism stabilizers in ProceduralComposer.");
        open_btn->connect("pressed", Callable(this, "_on_open_in_procedural_composer").bind(res_ref));
        header_row->add_child(open_btn);
    }
    vbox->add_child(header_row);

    if (auto *pat = Object::cast_to<ProceduralPattern>(p_object)) {
        auto *info = memnew(Label);
        info->set_text(
            "Discovered Tokens: " + String::num_int64(pat->get_discovered_tokens().size()) +
            " | Generation Rules: " + String::num_int64(pat->get_generation_rule_count()) +
            " | Seed: " + String::num_int64(pat->get_master_seed())
        );
        vbox->add_child(info);
    } else if (auto *grule = Object::cast_to<GenerationRule>(p_object)) {
        auto *info = memnew(Label);
        info->set_text(
            "CSS Specificity: " + String::num_int64(grule->compute_specificity()) +
            " | Layer: " + String::num_int64(grule->get_cascade_layer()) +
            " | Cardinality: [" + String::num_int64(grule->get_min_occurrences()) +
            ".." + String::num_int64(grule->get_max_occurrences()) + "]"
        );
        vbox->add_child(info);
    } else if (auto *arule = Object::cast_to<AnalysisRule>(p_object)) {
        auto *info = memnew(Label);
        info->set_text(
            "Analysis Specificity: " + String::num_int64(arule->compute_specificity()) +
            " | Child Rules: " + String::num_int64(arule->get_child_rules().size())
        );
        vbox->add_child(info);
    }

    add_custom_control(panel);
}

bool ProceduralGeneratorInspectorBase::_parse_property(
    godot::Object *p_object,
    godot::Variant::Type p_type,
    const godot::String &p_name,
    godot::PropertyHint p_hint_type,
    const godot::String &p_hint_string,
    godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
    bool p_wide
) {
    (void)p_object;
    (void)p_type;
    (void)p_name;
    (void)p_hint_type;
    (void)p_hint_string;
    (void)p_usage_flags;
    (void)p_wide;
    return false;
}

} // namespace ideam::godot_ext

#include "procedural_generator_inspector_base.h"
#include "../ideam_procedural_generation_plugin.h"
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

void ProceduralGeneratorInspectorBase::_bind_methods() {}

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
    if (!p_object) return;

    auto *panel = memnew(godot::PanelContainer);
    auto *vbox  = memnew(godot::VBoxContainer);
    panel->add_child(vbox);

    auto *title = memnew(godot::Label);
    title->set_text("IdeamProceduralGeneration — CSS Cascade Diagnostics");
    vbox->add_child(title);

    if (auto *pat = godot::Object::cast_to<ProceduralPattern>(p_object)) {
        auto *info = memnew(godot::Label);
        info->set_text(
            "Discovered Tokens: " + godot::String::num_int64(pat->get_discovered_tokens().size()) +
            " | Generation Rules: " + godot::String::num_int64(pat->get_generation_rule_count()) +
            " | Seed: " + godot::String::num_int64(pat->get_master_seed())
        );
        vbox->add_child(info);
    } else if (auto *grule = godot::Object::cast_to<GenerationRule>(p_object)) {
        auto *info = memnew(godot::Label);
        info->set_text(
            "CSS Specificity: " + godot::String::num_int64(grule->compute_specificity()) +
            " | Layer: " + godot::String::num_int64(grule->get_cascade_layer()) +
            " | Cardinality: [" + godot::String::num_int64(grule->get_min_occurrences()) +
            ".." + godot::String::num_int64(grule->get_max_occurrences()) + "]"
        );
        vbox->add_child(info);
    } else if (auto *arule = godot::Object::cast_to<AnalysisRule>(p_object)) {
        auto *info = memnew(godot::Label);
        info->set_text(
            "Analysis Specificity: " + godot::String::num_int64(arule->compute_specificity()) +
            " | Child Rules: " + godot::String::num_int64(arule->get_child_rules().size())
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

#include "narratives_inspector.h"
#include "../ideam_narratives_plugin.h"
#include "../ui/narratives_composer.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_registry.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

void NarrativesInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("_on_open_in_composer_pressed"), &NarrativesInspector::_on_open_in_composer_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &NarrativesInspector::_on_help_pressed);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *NarrativesInspector::get_undo_redo() const {
    return IdeamNarrativesPlugin::get_static_undo_redo();
}
#endif

bool NarrativesInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) return false;
    return godot::Object::cast_to<Narreme>(p_object) != nullptr ||
           godot::Object::cast_to<Relationship>(p_object) != nullptr ||
           godot::Object::cast_to<Incident_Condition>(p_object) != nullptr ||
           godot::Object::cast_to<Plot_Event>(p_object) != nullptr;
}

void NarrativesInspector::_parse_begin(godot::Object *p_object) {
    if (!p_object) return;

    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    godot::Ref<IdeamUndoRedo> ur = IdeamNarrativesPlugin::create_undo_redo_wrapper();
    if (auto *nar = godot::Object::cast_to<Narrative>(p_object)) {
        nar->propagate_undo_redo(ur);
    } else if (auto *narreme = godot::Object::cast_to<Narreme>(p_object)) {
        narreme->set_undo_redo(ur);
    } else if (auto *rel = godot::Object::cast_to<Relationship>(p_object)) {
        rel->set_undo_redo(ur);
    } else if (auto *cond = godot::Object::cast_to<Incident_Condition>(p_object)) {
        cond->set_undo_redo(ur);
    } else if (auto *pe = godot::Object::cast_to<Plot_Event>(p_object)) {
        pe->set_undo_redo(ur);
    }

    current_object_id_ = godot::ObjectID(p_object->get_instance_id());

    godot::PanelContainer *panel = memnew(godot::PanelContainer);
    godot::Ref<godot::StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(godot::Color(0.12f, 0.11f, 0.16f, 0.95f));
    style->set_border_color(godot::Color(0.46f, 0.36f, 0.64f, 0.85f));
    style->set_border_width_all(1);
    style->set_corner_radius_all(5);
    style->set_content_margin_all(8);
    panel->add_theme_stylebox_override("panel", style);
    domain_tool::stamp_ui_selector(panel, "narratives.inspector.header");

    godot::VBoxContainer *vbox = memnew(godot::VBoxContainer);
    vbox->add_theme_constant_override("separation", 4);
    panel->add_child(vbox);

    godot::HBoxContainer *top_row = memnew(godot::HBoxContainer);
    vbox->add_child(top_row);

    godot::Label *title_lbl = memnew(godot::Label);
    title_lbl->set_text("IdeamNarratives — Narreme & Semantics Inspector");
    title_lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    title_lbl->add_theme_color_override("font_color", godot::Color(0.84f, 0.76f, 1.0f));
    top_row->add_child(title_lbl);

    godot::Button *open_btn = memnew(godot::Button);
    open_btn->set_text("Open in Composer");
    open_btn->set_tooltip_text("Open this resource in the Narratives Composer window");
    domain_tool::stamp_ui_selector(open_btn, "narratives.inspector.open_composer_btn");
    open_btn->connect("pressed", godot::Callable(this, "_on_open_in_composer_pressed"));
    top_row->add_child(open_btn);

    godot::Button *help_btn = memnew(godot::Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for this Narreme");
    domain_tool::stamp_ui_selector(help_btn, "narratives.inspector.help_btn");
    help_btn->connect("pressed", godot::Callable(this, "_on_help_pressed"));
    top_row->add_child(help_btn);

    godot::Label *summary_lbl = memnew(godot::Label);
    summary_lbl->set_autowrap_mode(godot::TextServer::AUTOWRAP_WORD_SMART);
    summary_lbl->add_theme_color_override("font_color", godot::Color(0.80f, 0.82f, 0.88f));
    vbox->add_child(summary_lbl);
    summary_label_id_ = godot::ObjectID(summary_lbl->get_instance_id());

    add_custom_control(panel);
    _refresh_summary_panel();
}

bool NarrativesInspector::_parse_property(
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

void NarrativesInspector::_refresh_summary_panel() {
    if (current_object_id_.is_null() || summary_label_id_.is_null()) {
        return;
    }
    godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_);
    godot::Label *lbl  = godot::Object::cast_to<godot::Label>(godot::ObjectDB::get_instance(summary_label_id_));
    if (!obj || !lbl) {
        return;
    }

    auto format_sem_tags = [](const godot::Ref<SemanticTagSetResource> &p_tags) -> godot::String {
        if (!p_tags.is_valid()) return "Untagged";
        return godot::String::num_int64(p_tags->get_nuclear_popcount()) + godot::String(" nuclear / ") +
               godot::String::num_int64(p_tags->get_peripheral_popcount()) + godot::String(" peripheral semes");
    };

    if (const auto *loc = godot::Object::cast_to<Location>(obj)) {
        lbl->set_text(
            godot::String("Location: ") + (loc->get_official_name().is_empty() ? godot::String("<Unnamed>") : loc->get_official_name()) +
            godot::String("  |  Connections: ") + godot::String::num_int64(loc->get_connection_count()) +
            godot::String("  |  Characters: ") + godot::String::num_int64(loc->get_characters().size()) +
            godot::String("  |  Props: ") + godot::String::num_int64(loc->get_props().size()) +
            godot::String("  |  Semantics: ") + format_sem_tags(loc->get_semantic_tags())
        );
    } else if (const auto *ch = godot::Object::cast_to<Character>(obj)) {
        lbl->set_text(
            godot::String("Character: ") + (ch->get_official_name().is_empty() ? godot::String("<Unnamed>") : ch->get_official_name()) +
            godot::String("  |  Relationships: ") + godot::String::num_int64(ch->get_relationships().size()) +
            godot::String("  |  Goals: ") + godot::String::num_int64(ch->get_goals().size()) +
            godot::String("  |  Semantics: ") + format_sem_tags(ch->get_semantic_tags())
        );
    } else if (const auto *inc = godot::Object::cast_to<Incident>(obj)) {
        const godot::String frame_str = inc->get_semantic_frame().is_valid()
            ? godot::String(inc->get_semantic_frame()->get_frame_name())
            : godot::String("None");
        lbl->set_text(
            godot::String("Incident: ") + (inc->get_official_name().is_empty() ? godot::String("<Unnamed>") : inc->get_official_name()) +
            godot::String("  |  Conditions: ") + godot::String::num_int64(inc->get_conditions().size()) +
            godot::String("  |  FrameNet: ") + frame_str +
            godot::String("  |  Semantics: ") + format_sem_tags(inc->get_semantic_tags())
        );
    } else if (const auto *nar = godot::Object::cast_to<Narrative>(obj)) {
        lbl->set_text(
            godot::String("Narrative: ") + (nar->get_official_name().is_empty() ? godot::String("<Unnamed>") : nar->get_official_name()) +
            godot::String("  |  Chars/Roles: ") + godot::String::num_int64(nar->get_characters().size()) + godot::String("/") + godot::String::num_int64(nar->get_roles().size()) +
            godot::String("  |  Locs/Places: ") + godot::String::num_int64(nar->get_locations().size()) + godot::String("/") + godot::String::num_int64(nar->get_places().size()) +
            godot::String("  |  Plots/Stories: ") + godot::String::num_int64(nar->get_plots().size()) + godot::String("/") + godot::String::num_int64(nar->get_stories().size())
        );
    } else if (const auto *narreme = godot::Object::cast_to<Narreme>(obj)) {
        lbl->set_text(
            narreme->get_class_name_str() + godot::String(": ") +
            (narreme->get_official_name().is_empty() ? godot::String("<Unnamed>") : narreme->get_official_name()) +
            godot::String("  |  Semantics: ") + format_sem_tags(narreme->get_semantic_tags())
        );
    } else if (const auto *narcon = godot::Object::cast_to<Narrative_Condition>(obj)) {
        const godot::String key_str = narcon->get_semantic_predicate_key() != godot::StringName()
            ? godot::String(narcon->get_semantic_predicate_key())
            : godot::String("Unbound");
        lbl->set_text(
            godot::String("Condition: \"") + narcon->format_sentence() + godot::String("\"  |  Key: ") + key_str
        );
    } else if (const auto *rel = godot::Object::cast_to<Relationship>(obj)) {
        lbl->set_text(
            godot::String("Relationship: ") + (rel->get_title().is_empty() ? godot::String("<Untitled>") : rel->get_title()) +
            godot::String("  |  Semantics: ") + format_sem_tags(rel->get_semantic_tags())
        );
    } else if (const auto *pe = godot::Object::cast_to<Plot_Event>(obj)) {
        lbl->set_text(
            godot::String("Plot Event: ") + (pe->get_title().is_empty() ? godot::String("<Untitled>") : pe->get_title())
        );
    }
}

void NarrativesInspector::_on_open_in_composer_pressed() {
    if (current_object_id_.is_null()) return;
    if (godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_)) {
        if (auto *res = godot::Object::cast_to<godot::Resource>(obj)) {
            NarrativesComposer::open_narrative_resource(godot::Ref<godot::Resource>(res));
        }
    }
}

void NarrativesInspector::_on_help_pressed() {
    godot::StringName topic_id("IdeamNarratives");
    godot::Dictionary ctx;
    if (!current_object_id_.is_null()) {
        if (godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_)) {
            ctx["object_class"] = obj->get_class();
            if (const auto *narreme = godot::Object::cast_to<Narreme>(obj)) {
                topic_id = narreme->get_help_topic_id();
            } else {
                topic_id = godot::StringName(obj->get_class());
            }
        }
    }
    ToolRegistry::get_singleton().dispatch_help_request(topic_id, "narratives.inspector.header", ctx);
}

} // namespace ideam::godot_ext

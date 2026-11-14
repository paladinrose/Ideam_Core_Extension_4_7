#include "linguistics_inspector.h"
#include "../ideam_linguistics_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void LinguisticsInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_populate_builtin_vocab_pressed"),
        &LinguisticsInspector::_on_populate_builtin_vocab_pressed
    );
    ClassDB::bind_method(
        D_METHOD("_on_test_input_submitted", "text"),
        &LinguisticsInspector::_on_test_input_submitted
    );
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *LinguisticsInspector::get_undo_redo() const {
    return IdeamLinguisticsPlugin::get_static_undo_redo();
}
#endif

bool LinguisticsInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) return false;
    return godot::Object::cast_to<LanguageAssetResource>(p_object) != nullptr ||
           godot::Object::cast_to<LexiconResource>(p_object) != nullptr ||
           godot::Object::cast_to<GrammarRuleResource>(p_object) != nullptr ||
           godot::Object::cast_to<SememeResource>(p_object) != nullptr ||
           godot::Object::cast_to<SemanticTagSetResource>(p_object) != nullptr ||
           godot::Object::cast_to<SemanticFrameResource>(p_object) != nullptr;
}

void LinguisticsInspector::_parse_begin(godot::Object *p_object) {
    if (!p_object) return;

    // 1. Inject IdeamUndoRedo into `p_object->get("undo_redo")`
    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    current_object_id_ = godot::ObjectID(p_object->get_instance_id());

    // 2. Build interactive Linguistics Inspector Header Panel
    godot::PanelContainer *panel = memnew(godot::PanelContainer);
    godot::Ref<godot::StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(godot::Color(0.10f, 0.13f, 0.17f, 0.95f));
    style->set_border_color(godot::Color(0.28f, 0.42f, 0.58f, 0.85f));
    style->set_border_width_all(1);
    style->set_corner_radius_all(5);
    style->set_content_margin_all(8);
    panel->add_theme_stylebox_override("panel", style);

    godot::VBoxContainer *vbox = memnew(godot::VBoxContainer);
    vbox->add_theme_constant_override("separation", 6);
    panel->add_child(vbox);

    godot::Label *title_lbl = memnew(godot::Label);
    title_lbl->set_text("IdeamLinguistics — NLP, MRL Binary & FrameNet Telemetry");
    title_lbl->add_theme_color_override("font_color", godot::Color(0.68f, 0.88f, 1.0f));
    vbox->add_child(title_lbl);

    godot::Label *summary_lbl = memnew(godot::Label);
    summary_lbl->set_autowrap_mode(godot::TextServer::AUTOWRAP_WORD_SMART);
    vbox->add_child(summary_lbl);
    summary_label_id_ = godot::ObjectID(summary_lbl->get_instance_id());

    if (godot::Object::cast_to<LanguageAssetResource>(p_object) != nullptr ||
        godot::Object::cast_to<LexiconResource>(p_object) != nullptr) {
        godot::Button *pop_btn = memnew(godot::Button);
        pop_btn->set_text("Populate Built-In English / WordNet / FrameNet / Wizard Vocabulary");
        pop_btn->connect("pressed", godot::Callable(this, "_on_populate_builtin_vocab_pressed"));
        vbox->add_child(pop_btn);
    }

    godot::LineEdit *test_input = memnew(godot::LineEdit);
    test_input->set_placeholder("Test prompt, clause, or identifier (e.g. 'player attack monster with sword')...");
    test_input->connect("text_changed", godot::Callable(this, "_on_test_input_submitted"));
    vbox->add_child(test_input);

    godot::Label *result_lbl = memnew(godot::Label);
    result_lbl->set_autowrap_mode(godot::TextServer::AUTOWRAP_WORD_SMART);
    result_lbl->add_theme_color_override("font_color", godot::Color(0.75f, 0.82f, 0.90f));
    vbox->add_child(result_lbl);
    test_result_label_id_ = godot::ObjectID(result_lbl->get_instance_id());

    add_custom_control(panel);
    _refresh_summary_panel();
}

bool LinguisticsInspector::_parse_property(
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

void LinguisticsInspector::_refresh_summary_panel() {
    if (current_object_id_.is_null() || summary_label_id_.is_null()) {
        return;
    }
    godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_);
    godot::Label *lbl  = godot::Object::cast_to<godot::Label>(godot::ObjectDB::get_instance(summary_label_id_));
    if (!obj || !lbl) {
        return;
    }

    if (const auto *sem = godot::Object::cast_to<SememeResource>(obj)) {
        lbl->set_text(
            "UPOS: " + godot::String(core::upos_to_string(static_cast<core::UniversalPOS>(sem->get_upos_tag())).data()) +
            "  |  Nuclear Bits: " + godot::String::num_int64(sem->get_nuclear_popcount()) +
            " / 192  |  MRL Dims: " + godot::String::num_int64(sem->get_mrl_float_embedding().size()) +
            "  |  Coherence Mask32: 0x" + godot::String::num_uint64(static_cast<uint64_t>(sem->get_coherence_trait_mask32()), 16)
        );
    } else if (const auto *tset = godot::Object::cast_to<SemanticTagSetResource>(obj)) {
        lbl->set_text(
            "Nuclear Bits Set: " + godot::String::num_int64(tset->get_nuclear_popcount()) +
            " / 192  |  Peripheral Bits Set: " + godot::String::num_int64(tset->get_peripheral_popcount()) +
            " / 192  |  Valence: " + godot::String::num(tset->get_aggregate_valence(), 2)
        );
    } else if (const auto *frame = godot::Object::cast_to<SemanticFrameResource>(obj)) {
        lbl->set_text(
            "FrameNet Frame: " + godot::String(frame->get_frame_name()) +
            "  |  Evoking Verbs: " + godot::String::num_int64(frame->get_evoking_lexical_units().size()) +
            "  |  Valence: " + godot::String::num(frame->get_valence_polarity(), 2)
        );
    } else if (const auto *lex = godot::Object::cast_to<LexiconResource>(obj)) {
        lbl->set_text(
            "Phonemes: " + godot::String::num_int64(lex->get_phoneme_count()) +
            "  |  Semes: " + godot::String::num_int64(lex->get_seme_count()) +
            "  |  Sememes: " + godot::String::num_int64(lex->get_sememe_count()) +
            "  |  Frames: " + godot::String::num_int64(lex->get_semantic_frame_count()) +
            "  |  Synset Edges: " + godot::String::num_int64(lex->get_synset_relation_count()) +
            "  |  Lexemes: " + godot::String::num_int64(lex->get_lexeme_count())
        );
    } else if (const auto *lang = godot::Object::cast_to<LanguageAssetResource>(obj)) {
        const auto &core_lex = lang->get_unified_core_lexicon();
        lbl->set_text(
            "Unified Phonemes: " + godot::String::num_int64(static_cast<int64_t>(core_lex.phonemes().size())) +
            "  |  Semes: " + godot::String::num_int64(static_cast<int64_t>(core_lex.semes().size())) +
            "  |  Sememes: " + godot::String::num_int64(static_cast<int64_t>(core_lex.sememes().size())) +
            "  |  Frames: " + godot::String::num_int64(static_cast<int64_t>(core_lex.semantic_frames().size())) +
            "  |  Lexemes: " + godot::String::num_int64(static_cast<int64_t>(core_lex.lexemes().size()))
        );
    } else if (const auto *gram = godot::Object::cast_to<GrammarRuleResource>(obj)) {
        lbl->set_text(
            "CSS Specificity: " + godot::String::num_int64(gram->get_specificity()) +
            "  |  Cascade Layer: " + godot::String::num_int64(gram->get_cascade_layer()) +
            "  |  Priority: " + godot::String::num_int64(gram->get_priority())
        );
    }
}

void LinguisticsInspector::_on_populate_builtin_vocab_pressed() {
    if (current_object_id_.is_null()) return;
    godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_);
    if (!obj) return;

    if (auto *lang = godot::Object::cast_to<LanguageAssetResource>(obj)) {
        lang->populate_builtin_godot_and_wizard_vocabulary();
    } else if (auto *lex = godot::Object::cast_to<LexiconResource>(obj)) {
        lex->populate_default_english_ipa_inventory();
    }
    _refresh_summary_panel();
}

void LinguisticsInspector::_on_test_input_submitted(const godot::String &p_text) {
    if (current_object_id_.is_null() || test_result_label_id_.is_null()) {
        return;
    }
    godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_);
    godot::Label *res_lbl = godot::Object::cast_to<godot::Label>(
        godot::ObjectDB::get_instance(test_result_label_id_)
    );
    if (!obj || !res_lbl) {
        return;
    }
    if (p_text.strip_edges().is_empty()) {
        res_lbl->set_text("");
        return;
    }

    godot::Ref<SemanticPromptParser> parser;
    parser.instantiate();
    if (auto *lang = godot::Object::cast_to<LanguageAssetResource>(obj)) {
        parser->set_language_asset(godot::Ref<LanguageAssetResource>(lang));
    }

    godot::Ref<SemanticTagSetResource> parsed_set = parser->parse_prompt_to_tag_set(p_text);
    const godot::PackedStringArray nuc_tags = parsed_set->get_all_nuclear_tags();
    const godot::PackedStringArray per_tags = parsed_set->get_all_peripheral_tags();

    if (auto *sem = godot::Object::cast_to<SememeResource>(obj)) {
        godot::Ref<SemanticTagSetResource> self_set;
        self_set.instantiate();
        self_set->add_sememe(godot::Ref<SememeResource>(sem));
        const float aff = parsed_set->compute_affinity_with(self_set);
        res_lbl->set_text(
            "Hybrid Symbolic+MRL Affinity: " + godot::String::num(aff * 100.0f, 1) + "%  |  Parsed Nuclear: [" +
            godot::String(", ").join(nuc_tags) + "]"
        );
    } else if (auto *tset = godot::Object::cast_to<SemanticTagSetResource>(obj)) {
        const float aff = parsed_set->compute_affinity_with(godot::Ref<SemanticTagSetResource>(tset));
        res_lbl->set_text(
            "Affinity with TagSet: " + godot::String::num(aff * 100.0f, 1) + "%  |  Parsed Nuclear: [" +
            godot::String(", ").join(nuc_tags) + "]"
        );
    } else if (auto *frame = godot::Object::cast_to<SemanticFrameResource>(obj)) {
        const godot::Dictionary clause = parser->parse_prompt_to_semantic_frame(p_text);
        const float fit = frame->evaluate_role_fit(
            parser->parse_prompt_to_tag_set(clause.get("agent_token", "")),
            parser->parse_prompt_to_tag_set(clause.get("patient_token", "")),
            parser->parse_prompt_to_tag_set(clause.get("instrument_token", ""))
        );
        res_lbl->set_text(
            "Frame Role Fit: " + godot::String::num(fit * 100.0f, 1) + "%  |  Agent: " +
            godot::String(clause.get("agent_token", "")) + "  |  Patient: " +
            godot::String(clause.get("patient_token", "")) + "  |  Instrument: " +
            godot::String(clause.get("instrument_token", ""))
        );
    } else {
        const godot::PackedInt32Array ph_masks = parser->get_language_asset()->encode_text_to_phoneme_masks(p_text);
        const godot::Dictionary clause = parser->parse_prompt_to_semantic_frame(p_text);
        res_lbl->set_text(
            "Nuclear Semes: [" + godot::String(", ").join(nuc_tags) + "]  |  Frame: " +
            godot::String(clause.get("frame_name", "None")) + "  |  Phoneme Count: " +
            godot::String::num_int64(ph_masks.size())
        );
    }
}

} // namespace ideam::godot_ext

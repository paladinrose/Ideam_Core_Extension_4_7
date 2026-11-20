#include "semantic_style_rubric.h"

#include <algorithm>

namespace ideam::godot_ext {

void SemanticStyleRubric::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_rubric_id", "id"), &SemanticStyleRubric::set_rubric_id);
    ClassDB::bind_method(D_METHOD("get_rubric_id"), &SemanticStyleRubric::get_rubric_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "rubric_id"), "set_rubric_id", "get_rubric_id");

    ClassDB::bind_method(D_METHOD("set_prompt_summary", "summary"), &SemanticStyleRubric::set_prompt_summary);
    ClassDB::bind_method(D_METHOD("get_prompt_summary"), &SemanticStyleRubric::get_prompt_summary);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "prompt_summary", PROPERTY_HINT_MULTILINE_TEXT),
        "set_prompt_summary",
        "get_prompt_summary"
    );

    ClassDB::bind_method(D_METHOD("set_prioritized_tags", "tags"), &SemanticStyleRubric::set_prioritized_tags);
    ClassDB::bind_method(D_METHOD("get_prioritized_tags"), &SemanticStyleRubric::get_prioritized_tags);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "prioritized_tags"), "set_prioritized_tags", "get_prioritized_tags");

    ClassDB::bind_method(
        D_METHOD("set_coverage_thresholds", "thresholds"),
        &SemanticStyleRubric::set_coverage_thresholds
    );
    ClassDB::bind_method(D_METHOD("get_coverage_thresholds"), &SemanticStyleRubric::get_coverage_thresholds);
    ADD_PROPERTY(
        PropertyInfo(Variant::DICTIONARY, "coverage_thresholds"),
        "set_coverage_thresholds",
        "get_coverage_thresholds"
    );

    ClassDB::bind_method(D_METHOD("set_provenance_notes", "notes"), &SemanticStyleRubric::set_provenance_notes);
    ClassDB::bind_method(D_METHOD("get_provenance_notes"), &SemanticStyleRubric::get_provenance_notes);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "provenance_notes"), "set_provenance_notes", "get_provenance_notes");

    ClassDB::bind_method(D_METHOD("set_excluded_tags", "tags"), &SemanticStyleRubric::set_excluded_tags);
    ClassDB::bind_method(D_METHOD("get_excluded_tags"), &SemanticStyleRubric::get_excluded_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "excluded_tags"), "set_excluded_tags", "get_excluded_tags");

    ClassDB::bind_method(
        D_METHOD("set_candidate_external_sources", "sources"),
        &SemanticStyleRubric::set_candidate_external_sources
    );
    ClassDB::bind_method(
        D_METHOD("get_candidate_external_sources"),
        &SemanticStyleRubric::get_candidate_external_sources
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "candidate_external_sources"),
        "set_candidate_external_sources",
        "get_candidate_external_sources"
    );

    ClassDB::bind_method(
        D_METHOD("add_tag", "tag", "priority", "min_threshold", "provenance_note"),
        &SemanticStyleRubric::add_tag,
        DEFVAL(0.75f),
        DEFVAL(0.5f),
        DEFVAL(String())
    );
    ClassDB::bind_method(D_METHOD("to_dictionary"), &SemanticStyleRubric::to_dictionary);
}

void SemanticStyleRubric::add_tag(
    const godot::String &p_tag,
    float p_priority,
    float p_min_threshold,
    const godot::String &p_provenance_note
) {
    if (p_tag.is_empty()) {
        return;
    }
    prioritized_tags_[p_tag]    = std::clamp(p_priority, 0.0f, 1.0f);
    coverage_thresholds_[p_tag] = std::clamp(p_min_threshold, 0.0f, 1.0f);
    if (!p_provenance_note.is_empty()) {
        provenance_notes_[p_tag] = p_provenance_note;
    }
}

godot::Dictionary SemanticStyleRubric::to_dictionary() const {
    godot::Dictionary d;
    d["rubric_id"]                  = godot::String(rubric_id_);
    d["prompt_summary"]             = prompt_summary_;
    d["prioritized_tags"]           = prioritized_tags_;
    d["coverage_thresholds"]        = coverage_thresholds_;
    d["provenance_notes"]           = provenance_notes_;
    d["excluded_tags"]              = excluded_tags_;
    d["candidate_external_sources"] = candidate_external_sources_;
    return d;
}

} // namespace ideam::godot_ext

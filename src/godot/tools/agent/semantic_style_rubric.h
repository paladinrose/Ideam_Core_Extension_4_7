#pragma once

/**
 * semantic_style_rubric.h
 * Tier 5 (`AgentTool`) prioritized semantic rubric (`src/godot/tools/agent/`).
 *
 * Produced by an SLM/LLM agent from a user's natural-language brief (e.g. genres, historical/cultural
 * inspirations, disambiguated artifacts, exclusions) and passed down to a Tier 4 `WorkflowTool`
 * to generate a `SemanticCoverageReport` heatmap before gap-filling and metacontent generation.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class SemanticStyleRubric : public godot::Resource {
    GDCLASS(SemanticStyleRubric, godot::Resource)

private:
    godot::StringName rubric_id_;
    godot::String prompt_summary_;
    godot::Dictionary prioritized_tags_;       // tag (String) -> priority weight float [0.0, 1.0]
    godot::Dictionary coverage_thresholds_;    // tag (String) -> minimum coverage float [0.0, 1.0]
    godot::Dictionary provenance_notes_;       // tag/concept (String) -> research rationale/source note
    godot::PackedStringArray excluded_tags_;   // Explicit negative constraints
    godot::Array candidate_external_sources_;  // Array of Dictionaries (title, url, license, target_tags)

protected:
    static void _bind_methods();

public:
    SemanticStyleRubric() = default;
    ~SemanticStyleRubric() override = default;

    void set_rubric_id(const godot::StringName &p_id) { rubric_id_ = p_id; }
    [[nodiscard]] godot::StringName get_rubric_id() const { return rubric_id_; }

    void set_prompt_summary(const godot::String &p_summary) { prompt_summary_ = p_summary; }
    [[nodiscard]] godot::String get_prompt_summary() const { return prompt_summary_; }

    void set_prioritized_tags(const godot::Dictionary &p_tags) { prioritized_tags_ = p_tags; }
    [[nodiscard]] godot::Dictionary get_prioritized_tags() const { return prioritized_tags_; }

    void set_coverage_thresholds(const godot::Dictionary &p_thresholds) { coverage_thresholds_ = p_thresholds; }
    [[nodiscard]] godot::Dictionary get_coverage_thresholds() const { return coverage_thresholds_; }

    void set_provenance_notes(const godot::Dictionary &p_notes) { provenance_notes_ = p_notes; }
    [[nodiscard]] godot::Dictionary get_provenance_notes() const { return provenance_notes_; }

    void set_excluded_tags(const godot::PackedStringArray &p_tags) { excluded_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_excluded_tags() const { return excluded_tags_; }

    void set_candidate_external_sources(const godot::Array &p_sources) { candidate_external_sources_ = p_sources; }
    [[nodiscard]] godot::Array get_candidate_external_sources() const { return candidate_external_sources_; }

    void add_tag(
        const godot::String &p_tag,
        float p_priority = 0.75f,
        float p_min_threshold = 0.5f,
        const godot::String &p_provenance_note = ""
    );

    [[nodiscard]] godot::Dictionary to_dictionary() const;
};

} // namespace ideam::godot_ext

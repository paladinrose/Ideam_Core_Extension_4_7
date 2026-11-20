#pragma once

/**
 * semantic_coverage_report.h
 * Tier 4 (`WorkflowTool`) -> Tier 5 (`AgentTool`) semantic coverage heatmap contract
 * (`src/godot/tools/workflow/`).
 *
 * Captures how strongly local project resources cover each prioritized tag in a
 * `SemanticStyleRubric`, identifies deficit tags below their priority-adjusted thresholds,
 * and exports normalized float arrays ready to visualize in `HeatmapGrid`.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class SemanticCoverageReport : public godot::Resource {
    GDCLASS(SemanticCoverageReport, godot::Resource)

private:
    godot::StringName rubric_id_;
    godot::PackedStringArray ordered_tags_;
    godot::Dictionary tag_scores_;            // tag (String) -> float [0.0, 1.0]
    godot::Dictionary tag_thresholds_;        // tag (String) -> float [0.0, 1.0]
    godot::Dictionary matched_assets_by_tag_; // tag (String) -> PackedStringArray of res:// paths
    godot::PackedStringArray deficit_tags_;
    float overall_coverage_ = 0.0f;

protected:
    static void _bind_methods();

public:
    SemanticCoverageReport() = default;
    ~SemanticCoverageReport() override = default;

    void set_rubric_id(const godot::StringName &p_id) { rubric_id_ = p_id; }
    [[nodiscard]] godot::StringName get_rubric_id() const { return rubric_id_; }

    void set_ordered_tags(const godot::PackedStringArray &p_tags) { ordered_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_ordered_tags() const { return ordered_tags_; }

    void set_tag_scores(const godot::Dictionary &p_scores) { tag_scores_ = p_scores; }
    [[nodiscard]] godot::Dictionary get_tag_scores() const { return tag_scores_; }

    void set_tag_thresholds(const godot::Dictionary &p_thresholds) { tag_thresholds_ = p_thresholds; }
    [[nodiscard]] godot::Dictionary get_tag_thresholds() const { return tag_thresholds_; }

    void set_matched_assets_by_tag(const godot::Dictionary &p_map) { matched_assets_by_tag_ = p_map; }
    [[nodiscard]] godot::Dictionary get_matched_assets_by_tag() const { return matched_assets_by_tag_; }

    void set_deficit_tags(const godot::PackedStringArray &p_deficits) { deficit_tags_ = p_deficits; }
    [[nodiscard]] godot::PackedStringArray get_deficit_tags() const { return deficit_tags_; }

    void set_overall_coverage(float p_value) { overall_coverage_ = p_value; }
    [[nodiscard]] float get_overall_coverage() const noexcept { return overall_coverage_; }

    void record_tag_coverage(
        const godot::String &p_tag,
        float p_score,
        float p_threshold,
        const godot::PackedStringArray &p_matched_assets = godot::PackedStringArray()
    );
    void recompute_deficits();
    [[nodiscard]] bool meets_all_thresholds() const { return deficit_tags_.is_empty(); }
    [[nodiscard]] godot::PackedFloat32Array to_heatmap_values() const;
    [[nodiscard]] godot::Dictionary to_summary_dictionary() const;
};

} // namespace ideam::godot_ext

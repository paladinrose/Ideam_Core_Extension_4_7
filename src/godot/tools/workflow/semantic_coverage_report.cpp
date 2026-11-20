#include "semantic_coverage_report.h"

#include <algorithm>

namespace ideam::godot_ext {

void SemanticCoverageReport::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_rubric_id", "id"), &SemanticCoverageReport::set_rubric_id);
    ClassDB::bind_method(D_METHOD("get_rubric_id"), &SemanticCoverageReport::get_rubric_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "rubric_id"), "set_rubric_id", "get_rubric_id");

    ClassDB::bind_method(D_METHOD("set_ordered_tags", "tags"), &SemanticCoverageReport::set_ordered_tags);
    ClassDB::bind_method(D_METHOD("get_ordered_tags"), &SemanticCoverageReport::get_ordered_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "ordered_tags"), "set_ordered_tags", "get_ordered_tags");

    ClassDB::bind_method(D_METHOD("set_tag_scores", "scores"), &SemanticCoverageReport::set_tag_scores);
    ClassDB::bind_method(D_METHOD("get_tag_scores"), &SemanticCoverageReport::get_tag_scores);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "tag_scores"), "set_tag_scores", "get_tag_scores");

    ClassDB::bind_method(D_METHOD("set_tag_thresholds", "thresholds"), &SemanticCoverageReport::set_tag_thresholds);
    ClassDB::bind_method(D_METHOD("get_tag_thresholds"), &SemanticCoverageReport::get_tag_thresholds);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "tag_thresholds"), "set_tag_thresholds", "get_tag_thresholds");

    ClassDB::bind_method(
        D_METHOD("set_matched_assets_by_tag", "map"),
        &SemanticCoverageReport::set_matched_assets_by_tag
    );
    ClassDB::bind_method(
        D_METHOD("get_matched_assets_by_tag"),
        &SemanticCoverageReport::get_matched_assets_by_tag
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::DICTIONARY, "matched_assets_by_tag"),
        "set_matched_assets_by_tag",
        "get_matched_assets_by_tag"
    );

    ClassDB::bind_method(D_METHOD("set_deficit_tags", "deficits"), &SemanticCoverageReport::set_deficit_tags);
    ClassDB::bind_method(D_METHOD("get_deficit_tags"), &SemanticCoverageReport::get_deficit_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "deficit_tags"), "set_deficit_tags", "get_deficit_tags");

    ClassDB::bind_method(D_METHOD("set_overall_coverage", "value"), &SemanticCoverageReport::set_overall_coverage);
    ClassDB::bind_method(D_METHOD("get_overall_coverage"), &SemanticCoverageReport::get_overall_coverage);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "overall_coverage"), "set_overall_coverage", "get_overall_coverage");

    ClassDB::bind_method(
        D_METHOD("record_tag_coverage", "tag", "score", "threshold", "matched_assets"),
        &SemanticCoverageReport::record_tag_coverage,
        DEFVAL(PackedStringArray())
    );
    ClassDB::bind_method(D_METHOD("recompute_deficits"), &SemanticCoverageReport::recompute_deficits);
    ClassDB::bind_method(D_METHOD("meets_all_thresholds"), &SemanticCoverageReport::meets_all_thresholds);
    ClassDB::bind_method(D_METHOD("to_heatmap_values"), &SemanticCoverageReport::to_heatmap_values);
    ClassDB::bind_method(D_METHOD("to_summary_dictionary"), &SemanticCoverageReport::to_summary_dictionary);
}

void SemanticCoverageReport::record_tag_coverage(
    const godot::String &p_tag,
    float p_score,
    float p_threshold,
    const godot::PackedStringArray &p_matched_assets
) {
    if (p_tag.is_empty()) {
        return;
    }
    if (!ordered_tags_.has(p_tag)) {
        ordered_tags_.push_back(p_tag);
    }
    tag_scores_[p_tag]            = std::clamp(p_score, 0.0f, 1.0f);
    tag_thresholds_[p_tag]        = std::clamp(p_threshold, 0.0f, 1.0f);
    matched_assets_by_tag_[p_tag] = p_matched_assets;
    recompute_deficits();
}

void SemanticCoverageReport::recompute_deficits() {
    deficit_tags_.clear();
    if (ordered_tags_.is_empty()) {
        overall_coverage_ = 0.0f;
        return;
    }
    float total = 0.0f;
    for (int i = 0; i < ordered_tags_.size(); ++i) {
        const godot::String &tag = ordered_tags_[i];
        const float score        = tag_scores_.has(tag) ? static_cast<float>(tag_scores_[tag]) : 0.0f;
        const float threshold    = tag_thresholds_.has(tag) ? static_cast<float>(tag_thresholds_[tag]) : 0.5f;
        total += score;
        if (score < threshold) {
            deficit_tags_.push_back(tag);
        }
    }
    overall_coverage_ = total / static_cast<float>(ordered_tags_.size());
}

godot::PackedFloat32Array SemanticCoverageReport::to_heatmap_values() const {
    godot::PackedFloat32Array values;
    values.resize(ordered_tags_.size());
    for (int i = 0; i < ordered_tags_.size(); ++i) {
        const godot::String &tag = ordered_tags_[i];
        values.set(i, tag_scores_.has(tag) ? static_cast<float>(tag_scores_[tag]) : 0.0f);
    }
    return values;
}

godot::Dictionary SemanticCoverageReport::to_summary_dictionary() const {
    godot::Dictionary d;
    d["rubric_id"]            = godot::String(rubric_id_);
    d["overall_coverage"]     = overall_coverage_;
    d["meets_all_thresholds"] = meets_all_thresholds();
    d["deficit_tags"]         = deficit_tags_;
    d["tag_scores"]           = tag_scores_;
    d["tag_thresholds"]       = tag_thresholds_;
    return d;
}

} // namespace ideam::godot_ext

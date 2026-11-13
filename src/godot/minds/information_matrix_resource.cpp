#include "information_matrix_resource.h"
#include <algorithm>

namespace ideam::godot_ext {

void InformationMatrixResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_default_tome_id", "tome_id"), &InformationMatrixResource::set_default_tome_id);
    ClassDB::bind_method(D_METHOD("get_default_tome_id"), &InformationMatrixResource::get_default_tome_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_tome_id"), "set_default_tome_id", "get_default_tome_id");

    ClassDB::bind_method(D_METHOD("clear_facts"), &InformationMatrixResource::clear_facts);
    ClassDB::bind_method(
        D_METHOD("register_akashic_tome", "tome_id", "authority_weight", "domain_flags"),
        &InformationMatrixResource::register_akashic_tome,
        DEFVAL(0),
        DEFVAL(1.0f),
        DEFVAL(0)
    );
    ClassDB::bind_method(D_METHOD("get_tome_count"), &InformationMatrixResource::get_tome_count);

    ClassDB::bind_method(
        D_METHOD("register_fact", "subject_id", "predicate_id", "truth_value", "location_id", "salience"),
        &InformationMatrixResource::register_fact,
        DEFVAL(0),
        DEFVAL(1.0f)
    );
    ClassDB::bind_method(
        D_METHOD("register_fact_in_tome", "tome_id", "subject_id", "predicate_id", "truth_value", "location_id", "salience"),
        &InformationMatrixResource::register_fact_in_tome,
        DEFVAL(0),
        DEFVAL(1.0f)
    );
    ClassDB::bind_method(
        D_METHOD("get_fact_truth_value", "fact_id", "tome_id"),
        &InformationMatrixResource::get_fact_truth_value,
        DEFVAL(0)
    );
    ClassDB::bind_method(D_METHOD("get_fact_count"), &InformationMatrixResource::get_fact_count);

    ClassDB::bind_method(
        D_METHOD("register_hypothesis", "hypothesis_fact_id", "theory_group_id", "required_observation_mask", "consistency_score"),
        &InformationMatrixResource::register_hypothesis,
        DEFVAL(0),
        DEFVAL(1.0f)
    );
    ClassDB::bind_method(
        D_METHOD("register_syllogism", "major_fact_id", "minor_fact_id", "conclusion_fact_id", "figure", "major_q", "minor_q", "conclusion_q", "tome_id"),
        &InformationMatrixResource::register_syllogism,
        DEFVAL(0)
    );
    ClassDB::bind_static_method(
        "InformationMatrixResource",
        D_METHOD("is_syllogism_form_valid", "figure", "major_q", "minor_q", "conclusion_q"),
        &InformationMatrixResource::is_syllogism_form_valid
    );
}

void InformationMatrixResource::set_default_tome_id(int p_tome_id) {
    default_tome_id_ = std::clamp(p_tome_id, 0, 65535);
    emit_changed();
}

void InformationMatrixResource::clear_facts() {
    core_matrix_.clear();
    emit_changed();
}

int InformationMatrixResource::register_akashic_tome(int p_tome_id, float p_authority_weight, int p_domain_flags) {
    const uint16_t id = core_matrix_.register_akashic_tome(
        static_cast<uint16_t>(std::clamp(p_tome_id, 0, 65535)),
        p_authority_weight,
        static_cast<uint16_t>(p_domain_flags)
    );
    emit_changed();
    return static_cast<int>(id);
}

int InformationMatrixResource::get_tome_count() const {
    return static_cast<int>(core_matrix_.akashic_tomes().size());
}

int InformationMatrixResource::register_fact(
    int p_subject_id,
    int p_predicate_id,
    int64_t p_truth_value,
    int p_location_id,
    float p_salience
) {
    return register_fact_in_tome(default_tome_id_, p_subject_id, p_predicate_id, p_truth_value, p_location_id, p_salience);
}

int InformationMatrixResource::register_fact_in_tome(
    int p_tome_id,
    int p_subject_id,
    int p_predicate_id,
    int64_t p_truth_value,
    int p_location_id,
    float p_salience
) {
    core::FactPod fact{};
    fact.subject_id    = static_cast<uint32_t>(std::max(0, p_subject_id));
    fact.predicate_id  = static_cast<uint16_t>(std::clamp(p_predicate_id, 0, 65535));
    fact.truth_value   = p_truth_value;
    fact.location_id   = static_cast<uint32_t>(std::max(0, p_location_id));
    fact.base_salience = p_salience;
    const uint16_t tome_id = static_cast<uint16_t>(std::clamp(p_tome_id, 0, 255));
    fact.set_tome_id(tome_id);

    const uint32_t id = core_matrix_.upsert_ground_truth_fact(fact, tome_id);
    emit_changed();
    return static_cast<int>(id);
}

int64_t InformationMatrixResource::get_fact_truth_value(int p_fact_id, int p_tome_id) const {
    const core::FactPod *f = core_matrix_.find_ground_truth_fact(
        static_cast<uint32_t>(std::max(0, p_fact_id)),
        static_cast<uint16_t>(std::clamp(p_tome_id, 0, 255))
    );
    return f ? f->truth_value : 0;
}

int InformationMatrixResource::get_fact_count() const {
    return static_cast<int>(core_matrix_.ground_truth_facts().size());
}

int InformationMatrixResource::register_hypothesis(
    int p_hypothesis_fact_id,
    int p_theory_group_id,
    int p_required_observation_mask,
    float p_consistency_score
) {
    core::HypothesisPod h{};
    h.hypothesis_fact_id        = static_cast<uint32_t>(std::max(0, p_hypothesis_fact_id));
    h.theory_group_id           = static_cast<uint32_t>(std::max(0, p_theory_group_id));
    h.required_observation_mask = static_cast<uint32_t>(p_required_observation_mask);
    h.consistency_score         = std::clamp(p_consistency_score, 0.0f, 1.0f);
    const uint32_t slot = core_matrix_.register_hypothesis(h);
    emit_changed();
    return static_cast<int>(slot);
}

int InformationMatrixResource::register_syllogism(
    int p_major_fact_id,
    int p_minor_fact_id,
    int p_conclusion_fact_id,
    int p_figure,
    int p_major_q,
    int p_minor_q,
    int p_conclusion_q,
    int p_tome_id
) {
    core::SyllogisticPropositionPod s{};
    s.major_premise_fact_id = static_cast<uint32_t>(std::max(0, p_major_fact_id));
    s.minor_premise_fact_id = static_cast<uint32_t>(std::max(0, p_minor_fact_id));
    s.conclusion_fact_id    = static_cast<uint32_t>(std::max(0, p_conclusion_fact_id));
    s.packed_syllogism_form = core::pack_syllogistic_form(
        static_cast<core::SyllogisticFigure>(p_figure & 0x03),
        static_cast<core::LogicalQuantifier>(p_major_q & 0x03),
        static_cast<core::LogicalQuantifier>(p_minor_q & 0x03),
        static_cast<core::LogicalQuantifier>(p_conclusion_q & 0x03)
    );
    s.tome_id = static_cast<uint16_t>(std::clamp(p_tome_id, 0, 255));
    const uint32_t slot = core_matrix_.register_syllogism(s);
    emit_changed();
    return static_cast<int>(slot);
}

bool InformationMatrixResource::is_syllogism_form_valid(int p_figure, int p_major_q, int p_minor_q, int p_conclusion_q) {
    return core::is_syllogism_valid(
        static_cast<core::SyllogisticFigure>(p_figure & 0x03),
        static_cast<core::LogicalQuantifier>(p_major_q & 0x03),
        static_cast<core::LogicalQuantifier>(p_minor_q & 0x03),
        static_cast<core::LogicalQuantifier>(p_conclusion_q & 0x03)
    );
}

} // namespace ideam::godot_ext

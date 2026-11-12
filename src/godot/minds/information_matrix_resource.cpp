#include "information_matrix_resource.h"

namespace ideam::godot_ext {

void InformationMatrixResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("clear_facts"), &InformationMatrixResource::clear_facts);
    ClassDB::bind_method(
        D_METHOD("register_fact", "subject_id", "predicate_id", "truth_value", "location_id", "salience"),
        &InformationMatrixResource::register_fact,
        DEFVAL(0),
        DEFVAL(1.0f)
    );
    ClassDB::bind_method(D_METHOD("get_fact_count"), &InformationMatrixResource::get_fact_count);
}

void InformationMatrixResource::clear_facts() {
    core_matrix_.clear();
    emit_changed();
}

int InformationMatrixResource::register_fact(
    int p_subject_id,
    int p_predicate_id,
    int64_t p_truth_value,
    int p_location_id,
    float p_salience
) {
    core::FactPod fact{};
    fact.subject_id    = static_cast<uint32_t>(p_subject_id);
    fact.predicate_id  = static_cast<uint16_t>(p_predicate_id);
    fact.truth_value   = p_truth_value;
    fact.location_id   = static_cast<uint32_t>(p_location_id);
    fact.base_salience = p_salience;
    const uint32_t id  = core_matrix_.upsert_ground_truth_fact(fact);
    emit_changed();
    return static_cast<int>(id);
}

int InformationMatrixResource::get_fact_count() const {
    return static_cast<int>(core_matrix_.ground_truth_facts().size());
}

} // namespace ideam::godot_ext

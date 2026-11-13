#pragma once

/**
 * information_matrix_resource.h
 * Shared world `InformationMatrixResource` wrapping `ideam::core::InformationMatrixDOD`
 * (`src/godot/minds/`).
 *
 * Exposes Multi-Source Akashic Tomes (`register_akashic_tome`, `register_fact_in_tome`),
 * Reasoning Registries (`register_hypothesis`, `register_syllogism`, `is_syllogism_valid`),
 * and Per-Agent Reference Catalogue inspection (including misreferencing detection and
 * `is_agent_belief_knowledge`).
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../core/minds/information_matrix_dod.h"
#include "../utilities/ideam_undo_redo.h"

namespace ideam::godot_ext {

class InformationMatrixResource : public godot::Resource {
    GDCLASS(InformationMatrixResource, godot::Resource)

private:
    core::InformationMatrixDOD core_matrix_;
    godot::Ref<IdeamUndoRedo> undo_redo_;
    int default_tome_id_ = static_cast<int>(core::TOME_PHYSICAL_REALITY);

protected:
    static void _bind_methods();

public:
    InformationMatrixResource() = default;
    ~InformationMatrixResource() override = default;

    [[nodiscard]] const core::InformationMatrixDOD &get_core_matrix() const noexcept { return core_matrix_; }
    [[nodiscard]] core::InformationMatrixDOD &get_core_matrix_mut() noexcept { return core_matrix_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_default_tome_id(int p_tome_id);
    [[nodiscard]] int get_default_tome_id() const noexcept { return default_tome_id_; }

    // --- Akashic Tome & Ground-Truth Fact API ---
    void clear_facts();
    int register_akashic_tome(int p_tome_id = 0, float p_authority_weight = 1.0f, int p_domain_flags = 0);
    [[nodiscard]] int get_tome_count() const;

    int register_fact(int p_subject_id, int p_predicate_id, int64_t p_truth_value, int p_location_id = 0, float p_salience = 1.0f);
    int register_fact_in_tome(int p_tome_id, int p_subject_id, int p_predicate_id, int64_t p_truth_value, int p_location_id = 0, float p_salience = 1.0f);
    [[nodiscard]] int64_t get_fact_truth_value(int p_fact_id, int p_tome_id = 0) const;
    [[nodiscard]] int get_fact_count() const;

    // --- Reasoning & Syllogism API ---
    int register_hypothesis(int p_hypothesis_fact_id, int p_theory_group_id, int p_required_observation_mask = 0, float p_consistency_score = 1.0f);
    int register_syllogism(int p_major_fact_id, int p_minor_fact_id, int p_conclusion_fact_id, int p_figure, int p_major_q, int p_minor_q, int p_conclusion_q, int p_tome_id = 0);
    [[nodiscard]] static bool is_syllogism_form_valid(int p_figure, int p_major_q, int p_minor_q, int p_conclusion_q);
};

} // namespace ideam::godot_ext

#pragma once

/**
 * information_matrix_resource.h
 * Shared world `InformationMatrixResource` wrapping `ideam::core::InformationMatrixDOD`
 * (`src/godot/minds/`).
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

protected:
    static void _bind_methods();

public:
    InformationMatrixResource() = default;
    ~InformationMatrixResource() override = default;

    [[nodiscard]] const core::InformationMatrixDOD &get_core_matrix() const noexcept { return core_matrix_; }
    [[nodiscard]] core::InformationMatrixDOD &get_core_matrix_mut() noexcept { return core_matrix_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void clear_facts();
    int register_fact(int p_subject_id, int p_predicate_id, int64_t p_truth_value, int p_location_id = 0, float p_salience = 1.0f);
    [[nodiscard]] int get_fact_count() const;
};

} // namespace ideam::godot_ext

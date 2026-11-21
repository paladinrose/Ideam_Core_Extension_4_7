#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>
#include "../narreme.h" // Needed for NarremeConditionStatus & IdeamUndoRedo

namespace ideam::godot_ext {

class Incident_Condition : public godot::Resource {
    GDCLASS(Incident_Condition, godot::Resource)

private:
    godot::String title;
    NarremeConditionStatus last_status = NarremeConditionStatus::UNKNOWN;

    void _set_title_direct(const godot::String &p_title);

protected:
    bool _is_ready = false;
    godot::Ref<IdeamUndoRedo> undo_redo_;
    bool undo_redo_suppressed_ = false;

    [[nodiscard]] bool _can_record_undo() const noexcept {
        return undo_redo_.is_valid() && !undo_redo_suppressed_;
    }

    static void _bind_methods();

public:
    Incident_Condition();
    ~Incident_Condition() override;

    virtual void initialize();

    // Undo / Redo Injection
    virtual void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_undo_redo_suppressed(bool p_suppressed) noexcept { undo_redo_suppressed_ = p_suppressed; }
    [[nodiscard]] bool is_undo_redo_suppressed() const noexcept { return undo_redo_suppressed_; }

    // Getters & Setters
    void set_title(const godot::String &p_title);
    [[nodiscard]] godot::String get_title() const;
    void commit_title(const godot::String &p_title, const godot::String &p_action_name = "Rename Condition");

    void set_last_status(NarremeConditionStatus p_status);
    [[nodiscard]] NarremeConditionStatus get_last_status() const;

    // DOD NOTE: Virtual functions cause vtable lookups and pipeline stalls.
    // In IDEAM Core, we will replace polymorphic condition evaluation with
    // data-driven 'Strategy' execution mapped over contiguous SoA arrays.
    virtual NarremeConditionStatus evaluate_condition();
};

} // namespace ideam::godot_ext
#pragma once

#include "../narremes/incident.h"
#include "../../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

enum IncidentRequirement : int8_t {
    INCIDENT_REQ_HAS_NOT_HAPPENED = 0,
    INCIDENT_REQ_HAS_HAPPENED = 1,
    INCIDENT_REQ_CANNOT_HAPPEN = 2,
    INCIDENT_REQ_INSTANT_SUCCESS = 3,
    INCIDENT_REQ_INSTANT_FAILURE = 4
};

class Plot_Event : public godot::Resource {
    GDCLASS(Plot_Event, godot::Resource)

private:
    godot::String title;
    godot::Ref<Incident> incident;
    IncidentRequirement requirement = INCIDENT_REQ_HAS_NOT_HAPPENED;

    bool _is_ready = false;
    bool _instant  = false;

    godot::Ref<IdeamUndoRedo> undo_redo_;
    bool undo_redo_suppressed_ = false;

    [[nodiscard]] bool _can_record_undo() const noexcept {
        return undo_redo_.is_valid() && !undo_redo_suppressed_;
    }

    void _apply_edit_direct(
        const godot::String &p_title,
        const godot::Ref<Incident> &p_incident,
        int p_requirement
    );

protected:
    static void _bind_methods();

public:
    Plot_Event();
    ~Plot_Event() override;

    void initialize();

    // Undo / Redo Injection
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    // Getters & Setters
    void set_title(const godot::String &p_title);
    [[nodiscard]] godot::String get_title() const;

    void set_incident(const godot::Ref<Incident> &p_incident);
    [[nodiscard]] godot::Ref<Incident> get_incident() const;

    void set_requirement(IncidentRequirement p_requirement);
    [[nodiscard]] IncidentRequirement get_requirement() const;

    // Reversible Edit API (`IdeamUndoRedo`, used by PlotOutliner)
    void commit_edit(
        const godot::String &p_title,
        const godot::Ref<Incident> &p_incident,
        IncidentRequirement p_requirement,
        const godot::String &p_action_name = "Edit Plot Event"
    );

    // Methods
    bool check_requirement();

    void _connect_to_incident();
    void _disconnect_from_incident();

    void _cannot_happen();
    void _happened();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::IncidentRequirement);
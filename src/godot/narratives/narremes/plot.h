#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Plot_Event;
class Incident;

enum PlotStatus : int8_t {
    PLOT_NOT_BEGUN = 0,
    PLOT_IN_PROGRESS = 1,
    PLOT_COMPLETE = 2,
    PLOT_CANNOT_COMPLETE = 3,
    PLOT_FAILED = 4
};

class Plot : public Narreme {
    GDCLASS(Plot, Narreme)

private:
    PlotStatus status = PLOT_NOT_BEGUN;
    godot::TypedArray<Plot_Event> plot_events;

    void _insert_plot_event_at_direct(int p_index, const godot::Ref<Plot_Event> &p_event);
    void _remove_plot_event_at_direct(int p_index);

protected:
    static void _bind_methods();

public:
    Plot();
    ~Plot() override;

    virtual void initialize() override;

    // Getters & Setters
    void set_status(PlotStatus p_status);
    [[nodiscard]] PlotStatus get_status() const;

    void set_plot_events(const godot::TypedArray<Plot_Event> &p_events);
    [[nodiscard]] godot::TypedArray<Plot_Event> get_plot_events() const;

    // Methods (IdeamUndoRedo-aware)
    PlotStatus evaluate_plot_status();
    void gather_plot_events();

    int add_plot_event(const godot::Ref<Plot_Event> &new_plot_event);
    [[nodiscard]] int get_plot_event_id(const godot::Ref<Plot_Event> &plot_event) const;
    [[nodiscard]] bool has_plot_event(const godot::Ref<Plot_Event> &plot_event) const;

    bool remove_plot_event(const godot::Ref<Plot_Event> &plot_event);
    bool remove_plot_event_at(int plot_event_ID);

    void _connect_to_plot_event(const godot::Ref<Plot_Event> &plot_event);
    void _disconnect_from_plot_event(const godot::Ref<Plot_Event> &plot_event);
    void _plot_event_change(const godot::Ref<Plot_Event> &plot_event);

    // Overrides
    virtual godot::TypedArray<godot::Dictionary> get_narrative_condition_descriptors(Narreme *p_narreme = nullptr) const override;
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::PlotStatus);
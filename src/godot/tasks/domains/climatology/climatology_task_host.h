#pragma once

/**
 * @file climatology_task_host.h
 * @brief Scene-tree Node host that orchestrates DataOrientedClimatology within IdeamTasks.
 */

#include <godot_cpp/classes/node.hpp>

#include "godot/tasks/domains/climatology/data_oriented_climatology.h"

namespace ideam::godot_ext {

class ClimatologyTaskHost : public godot::Node {
    GDCLASS(ClimatologyTaskHost, godot::Node)

private:
    godot::Ref<DataOrientedClimatology> dod_climatology_;
    godot::Ref<ClimatologyAtmosphereResource> atmosphere_resource_;
    bool auto_step_in_process_ = false;
    float synoptic_dt_per_step_sec_ = 30.0f;
    float vortex_dt_per_step_sec_ = 10.0f;

protected:
    static void _bind_methods();

public:
    ClimatologyTaskHost();
    ~ClimatologyTaskHost() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    void set_atmosphere_resource(const godot::Ref<ClimatologyAtmosphereResource>& p_atmosphere);
    godot::Ref<ClimatologyAtmosphereResource> get_atmosphere_resource() const;

    godot::Ref<DataOrientedClimatology> get_dod_climatology() const;

    void set_auto_step_in_process(bool p_enabled);
    bool get_auto_step_in_process() const;

    void set_synoptic_dt_per_step_sec(float p_dt);
    float get_synoptic_dt_per_step_sec() const;

    void set_vortex_dt_per_step_sec(float p_dt);
    float get_vortex_dt_per_step_sec() const;

    void step_once();
};

} // namespace ideam::godot_ext

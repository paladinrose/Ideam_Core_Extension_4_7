#pragma once

/**
 * @file geomorphology_task_host.h
 * @brief Scene-tree Node host that orchestrates DataOrientedGeomorphology within IdeamTasks.
 */

#include <godot_cpp/classes/node.hpp>

#include "godot/tasks/domains/geomorphology/data_oriented_geomorphology.h"

namespace ideam::godot_ext {

class GeomorphologyTaskHost : public godot::Node {
    GDCLASS(GeomorphologyTaskHost, godot::Node)

private:
    godot::Ref<DataOrientedGeomorphology> dod_geomorphology_;
    godot::Ref<GeomorphologyTerrainResource> terrain_resource_;
    bool auto_step_in_process_ = false;
    float geological_dt_per_step_myr_ = 0.01f;
    float hydrological_dt_per_step_sec_ = 0.20f;
    float precipitation_rate_ = 0.015f;

protected:
    static void _bind_methods();

public:
    GeomorphologyTaskHost();
    ~GeomorphologyTaskHost() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    void set_terrain_resource(const godot::Ref<GeomorphologyTerrainResource>& p_terrain);
    godot::Ref<GeomorphologyTerrainResource> get_terrain_resource() const;

    godot::Ref<DataOrientedGeomorphology> get_dod_geomorphology() const;

    void set_auto_step_in_process(bool p_enabled);
    bool get_auto_step_in_process() const;

    void set_geological_dt_per_step_myr(float p_dt);
    float get_geological_dt_per_step_myr() const;

    void set_hydrological_dt_per_step_sec(float p_dt);
    float get_hydrological_dt_per_step_sec() const;

    void set_precipitation_rate(float p_rate);
    float get_precipitation_rate() const;

    void step_once();
};

} // namespace ideam::godot_ext

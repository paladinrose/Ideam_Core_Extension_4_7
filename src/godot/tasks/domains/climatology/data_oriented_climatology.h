#pragma once

/**
 * @file data_oriented_climatology.h
 * @brief Godot-facing Data-Oriented container for high-throughput Climatology simulation.
 *
 * Bridges ClimatologyAtmosphereResource into the IdeamTasks DOD execution pipeline.
 */

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "core/tasks/domains/climatology/climatology_dod.h"
#include "godot/climatology/climatology_weather_resource.h"

namespace ideam::tasks::climatology {

class DataOrientedClimatology : public godot::RefCounted {
    GDCLASS(DataOrientedClimatology, godot::RefCounted)

private:
    godot::Ref<ideam::climatology::ClimatologyAtmosphereResource> active_atmosphere_;
    uint32_t stage_mask_ = static_cast<uint32_t>(ClimatologyStageFlags::ALL_STAGES);

protected:
    static void _bind_methods();

public:
    DataOrientedClimatology() = default;
    ~DataOrientedClimatology() override = default;

    void bind_atmosphere_resource(const godot::Ref<ideam::climatology::ClimatologyAtmosphereResource>& p_atmosphere);
    godot::Ref<ideam::climatology::ClimatologyAtmosphereResource> get_bound_atmosphere_resource() const;

    void set_stage_mask(int p_mask);
    int get_stage_mask() const;

    void initialize_from_seed();
    void execute_pipeline_step(float p_dt_synoptic_sec, float p_dt_vortex_sec);
    godot::Dictionary get_pipeline_telemetry() const;
};

} // namespace ideam::tasks::climatology

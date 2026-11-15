#pragma once

/**
 * @file data_oriented_geomorphology.h
 * @brief Godot-facing Data-Oriented container for high-throughput Geomorphology simulation.
 *
 * Bridges GeomorphologyTerrainResource into the IdeamTasks DOD execution pipeline
 * without heap allocations on the worker threads.
 */

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "core/tasks/domains/geomorphology/geomorphology_dod.h"
#include "godot/geomorphology/geomorphology_terrain_resource.h"

namespace ideam::tasks::geomorphology {

class DataOrientedGeomorphology : public godot::RefCounted {
    GDCLASS(DataOrientedGeomorphology, godot::RefCounted)

private:
    godot::Ref<ideam::geomorphology::GeomorphologyTerrainResource> active_terrain_;
    uint32_t stage_mask_ = static_cast<uint32_t>(GeomorphologyStageFlags::ALL_STAGES);

protected:
    static void _bind_methods();

public:
    DataOrientedGeomorphology() = default;
    ~DataOrientedGeomorphology() override = default;

    void bind_terrain_resource(const godot::Ref<ideam::geomorphology::GeomorphologyTerrainResource>& p_terrain);
    godot::Ref<ideam::geomorphology::GeomorphologyTerrainResource> get_bound_terrain_resource() const;

    void set_stage_mask(int p_mask);
    int get_stage_mask() const;

    void initialize_from_seed();
    void execute_pipeline_step(float p_dt_geological_myr, float p_dt_hydrological_sec, float p_precipitation_rate);
    godot::Dictionary get_pipeline_telemetry() const;
};

} // namespace ideam::tasks::geomorphology

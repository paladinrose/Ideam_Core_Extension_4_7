#pragma once

/**
 * @file data_oriented_ecology.h
 * @brief Godot-facing Data-Oriented container for high-throughput Ecology simulation.
 *
 * Bridges EcologyBiosphereResource into the IdeamTasks DOD execution pipeline.
 */

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "core/tasks/domains/ecology/ecology_dod.h"
#include "godot/ecology/ecology_biosphere_resource.h"

namespace ideam::tasks::ecology {

class DataOrientedEcology : public godot::RefCounted {
    GDCLASS(DataOrientedEcology, godot::RefCounted)

private:
    godot::Ref<ideam::ecology::EcologyBiosphereResource> active_biosphere_;
    uint32_t stage_mask_ = static_cast<uint32_t>(EcologyStageFlags::ALL_STAGES);

protected:
    static void _bind_methods();

public:
    DataOrientedEcology() = default;
    ~DataOrientedEcology() override = default;

    void bind_biosphere_resource(const godot::Ref<ideam::ecology::EcologyBiosphereResource>& p_biosphere);
    godot::Ref<ideam::ecology::EcologyBiosphereResource> get_bound_biosphere_resource() const;

    void set_stage_mask(int p_mask);
    int get_stage_mask() const;

    void initialize_from_seed();
    void execute_pipeline_step(float p_dt_years);
    godot::Dictionary get_pipeline_telemetry() const;
};

} // namespace ideam::tasks::ecology

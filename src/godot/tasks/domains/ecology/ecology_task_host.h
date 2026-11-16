#pragma once

/**
 * @file ecology_task_host.h
 * @brief Scene-tree Node host that orchestrates DataOrientedEcology within IdeamTasks.
 */

#include <godot_cpp/classes/node.hpp>

#include "godot/tasks/domains/ecology/data_oriented_ecology.h"

namespace ideam::godot_ext {

class EcologyTaskHost : public godot::Node {
    GDCLASS(EcologyTaskHost, godot::Node)

private:
    godot::Ref<DataOrientedEcology> dod_ecology_;
    godot::Ref<EcologyBiosphereResource> biosphere_resource_;
    bool auto_step_in_process_ = false;
    float dt_years_per_step_ = 0.25f;

protected:
    static void _bind_methods();

public:
    EcologyTaskHost();
    ~EcologyTaskHost() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    void set_biosphere_resource(const godot::Ref<EcologyBiosphereResource>& p_biosphere);
    godot::Ref<EcologyBiosphereResource> get_biosphere_resource() const;

    godot::Ref<DataOrientedEcology> get_dod_ecology() const;

    void set_auto_step_in_process(bool p_enabled);
    bool get_auto_step_in_process() const;

    void set_dt_years_per_step(float p_dt);
    float get_dt_years_per_step() const;

    void step_once();
};

} // namespace ideam::godot_ext

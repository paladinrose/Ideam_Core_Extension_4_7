#pragma once

/**
 * wfc_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedWaveFunctionCollapse`
 * (`src/godot/tasks/domains/wave_function_collapse/`).
 */

#include "data_oriented_wfc.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class WaveFunctionCollapseTaskHost : public TaskGraphHost {
    GDCLASS(WaveFunctionCollapseTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedWaveFunctionCollapse> domain_resource_;
    int active_stage_mask_    = static_cast<int>(core::WFC_STAGE_DEFAULT_PIPELINE_MASK);
    bool auto_stage_on_ready_ = true;

protected:
    static void _bind_methods();

public:
    WaveFunctionCollapseTaskHost() = default;
    ~WaveFunctionCollapseTaskHost() override = default;

    void _ready() override;

    void set_domain_resource(const godot::Ref<DataOrientedWaveFunctionCollapse> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedWaveFunctionCollapse> get_domain_resource() const { return domain_resource_; }

    void set_active_stage_mask(int p_mask) { active_stage_mask_ = p_mask; }
    [[nodiscard]] int get_active_stage_mask() const noexcept { return active_stage_mask_; }

    void set_auto_stage_on_ready(bool p_auto) { auto_stage_on_ready_ = p_auto; }
    [[nodiscard]] bool get_auto_stage_on_ready() const noexcept { return auto_stage_on_ready_; }

    bool step_wfc_pipeline();
};

} // namespace ideam::godot_ext

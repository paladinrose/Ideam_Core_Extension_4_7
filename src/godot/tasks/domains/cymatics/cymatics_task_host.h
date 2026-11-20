#pragma once

/**
 * cymatics_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedCymatics`
 * (`src/godot/tasks/domains/cymatics/`).
 */

#include "data_oriented_cymatics.h"
#include "../../task_graph_host.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class CymaticsTaskHost : public TaskGraphHost {
    GDCLASS(CymaticsTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedCymatics> domain_resource_;
    int active_stage_mask_    = static_cast<int>(core::CYM_STAGE_DEFAULT_MASK);
    bool auto_stage_on_ready_ = true;

protected:
    static void _bind_methods();

public:
    CymaticsTaskHost() = default;
    ~CymaticsTaskHost() override = default;

    void _ready() override;

    void set_domain_resource(const godot::Ref<DataOrientedCymatics> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedCymatics> get_domain_resource() const { return domain_resource_; }

    void set_active_stage_mask(int p_mask) { active_stage_mask_ = p_mask; }
    [[nodiscard]] int get_active_stage_mask() const noexcept { return active_stage_mask_; }

    void set_auto_stage_on_ready(bool p_auto) { auto_stage_on_ready_ = p_auto; }
    [[nodiscard]] bool get_auto_stage_on_ready() const noexcept { return auto_stage_on_ready_; }

    bool step_cymatics_pipeline();
};

} // namespace ideam::godot_ext

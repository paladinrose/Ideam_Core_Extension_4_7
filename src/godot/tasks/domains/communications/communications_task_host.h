#pragma once

/**
 * communications_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedCommunications`
 * (`src/godot/tasks/domains/communications/`).
 */

#include "data_oriented_communications.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class CommunicationsTaskHost : public TaskGraphHost {
    GDCLASS(CommunicationsTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedCommunications> domain_resource_;
    int active_stage_mask_ = static_cast<int>(core::COMM_STAGE_DEFAULT_PIPELINE_MASK);
    bool auto_stage_on_ready_ = true;

protected:
    static void _bind_methods();

public:
    CommunicationsTaskHost() = default;
    ~CommunicationsTaskHost() override = default;

    void _ready() override;

    void set_domain_resource(const godot::Ref<DataOrientedCommunications> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedCommunications> get_domain_resource() const { return domain_resource_; }

    void set_active_stage_mask(int p_mask) { active_stage_mask_ = p_mask; }
    [[nodiscard]] int get_active_stage_mask() const noexcept { return active_stage_mask_; }

    void set_auto_stage_on_ready(bool p_auto) { auto_stage_on_ready_ = p_auto; }
    [[nodiscard]] bool get_auto_stage_on_ready() const noexcept { return auto_stage_on_ready_; }

    bool step_communications_pipeline();
};

} // namespace ideam::godot_ext

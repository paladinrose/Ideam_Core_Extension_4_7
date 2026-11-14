#pragma once

/**
 * linguistics_task_host.h
 * SceneTree `TaskGraphHost` node for `DataOrientedLinguistics` (`src/godot/tasks/domains/linguistics/`).
 *
 * Extends `ideam::godot_ext::TaskGraphHost` so a `DataOrientedLinguistics` Resource can execute
 * inside the Godot SceneTree, share its `MemoryManagerDOD` and `TaskGraphDOD` with another
 * `TaskGraphHost` (such as `MindTaskHost`, `CommunicationsTaskHost`, `ProceduralGenerationTaskHost`,
 * or `WaveFunctionCollapseTaskHost`), and drive frame-by-frame or event-driven NLP, phonetic DTW,
 * MRL binary embedding search, FrameNet validation, and SLM mask evaluation.
 */

#include "data_oriented_linguistics.h"
#include "../../task_graph_host.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace ideam::godot_ext {

class LinguisticsTaskHost : public TaskGraphHost {
    GDCLASS(LinguisticsTaskHost, TaskGraphHost)

private:
    godot::Ref<DataOrientedLinguistics> domain_resource_;
    godot::NodePath shared_host_node_path_;
    bool auto_setup_on_ready_     = true;
    bool auto_execute_in_process_ = false;
    bool share_host_memory_       = true;

    void _wire_shared_host_if_present();

protected:
    static void _bind_methods();

public:
    LinguisticsTaskHost() = default;
    ~LinguisticsTaskHost() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    // --- Inspector Properties ---
    void set_domain_resource(const godot::Ref<DataOrientedLinguistics> &p_res) { domain_resource_ = p_res; }
    [[nodiscard]] godot::Ref<DataOrientedLinguistics> get_domain_resource() const { return domain_resource_; }

    void set_shared_host_node_path(const godot::NodePath &p_path) { shared_host_node_path_ = p_path; }
    [[nodiscard]] godot::NodePath get_shared_host_node_path() const { return shared_host_node_path_; }

    void set_auto_setup_on_ready(bool p_auto_setup) { auto_setup_on_ready_ = p_auto_setup; }
    [[nodiscard]] bool get_auto_setup_on_ready() const noexcept { return auto_setup_on_ready_; }

    void set_auto_execute_in_process(bool p_auto_exec) { auto_execute_in_process_ = p_auto_exec; }
    [[nodiscard]] bool get_auto_execute_in_process() const noexcept { return auto_execute_in_process_; }

    void set_share_host_memory(bool p_share) { share_host_memory_ = p_share; }
    [[nodiscard]] bool get_share_host_memory() const noexcept { return share_host_memory_; }

    // --- Host Lifecycle & Cross-Domain Bridging ---
    void setup_linguistics_host();
    void execute_linguistics_step(double p_delta);
    bool bind_shared_task_host(TaskGraphHost *p_other_host);

    [[nodiscard]] core::LinguisticsDOD *get_linguistics_dod() const noexcept {
        return domain_resource_.is_valid() ? domain_resource_->get_dod_engine() : nullptr;
    }
};

} // namespace ideam::godot_ext

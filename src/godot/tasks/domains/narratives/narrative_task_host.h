#pragma once

/**
 * narrative_task_host.h
 * SceneTree Node host for DataOrientedNarrative in src/godot/tasks/domains/narratives/.
 *
 * Extends ideam::godot_ext::TaskGraphHost so a DataOrientedNarrative (Resource) can
 * execute inside the Godot SceneTree, optionally share a MemoryManagerDOD / TaskGraphDOD
 * with another TaskGraphHost or DataOrientedGame, wire live GamePropertyBuffer columns
 * into N4 (GameplayConditionBridgeTransformLogic), and drive frame-by-frame evaluation.
 */

#include "data_oriented_narrative.h"
#include "../../task_graph_host.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace ideam::godot_ext {

class DataOrientedGame;

class NarrativeTaskHost : public TaskGraphHost {
    GDCLASS(NarrativeTaskHost, TaskGraphHost)

protected:
    static void _bind_methods();

private:
    godot::Ref<DataOrientedNarrative> narrative_resource_;
    godot::NodePath game_node_path_;
    bool auto_setup_on_ready_       = true;
    bool auto_execute_in_process_   = true;
    bool share_game_memory_manager_ = true;

    void _wire_game_node_if_present();

public:
    NarrativeTaskHost() = default;
    ~NarrativeTaskHost() override = default;

    virtual void _ready() override;
    virtual void _process(double p_delta) override;

    // --- Inspector Properties ---
    void set_narrative_resource(const godot::Ref<DataOrientedNarrative>& p_narrative);
    godot::Ref<DataOrientedNarrative> get_narrative_resource() const;

    void set_game_node_path(const godot::NodePath& p_path);
    godot::NodePath get_game_node_path() const;

    void set_auto_setup_on_ready(bool p_auto_setup);
    bool get_auto_setup_on_ready() const;

    void set_auto_execute_in_process(bool p_auto_execute);
    bool get_auto_execute_in_process() const;

    void set_share_game_memory_manager(bool p_share);
    bool get_share_game_memory_manager() const;

    // --- Host Lifecycle & Cross-Domain Bridging ---
    void setup_narrative_host();
    void execute_narrative_step(double p_delta);
    bool bind_data_oriented_game(DataOrientedGame* p_game);

    [[nodiscard]] core::NarrativeDOD* get_narrative_dod() const noexcept {
        return narrative_resource_.is_valid() ? narrative_resource_->get_narrative_dod() : nullptr;
    }
};

} // namespace ideam::godot_ext
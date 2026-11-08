#include "narrative_task_host.h"
#include "../games/data_oriented_game.h"

#include <godot_cpp/classes/engine.hpp>

namespace ideam::godot_ext {

void NarrativeTaskHost::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo("narrative_host_ready"));
    ADD_SIGNAL(MethodInfo("narrative_step_completed", PropertyInfo(Variant::FLOAT, "delta")));

#define IDEAM_BIND_HOST_PROP(m_type, m_name, m_setter, m_getter, ...) \
    ClassDB::bind_method(D_METHOD(#m_setter, "value"), &NarrativeTaskHost::m_setter); \
    ClassDB::bind_method(D_METHOD(#m_getter), &NarrativeTaskHost::m_getter); \
    ADD_PROPERTY(PropertyInfo(m_type, m_name, ##__VA_ARGS__), #m_setter, #m_getter)

    IDEAM_BIND_HOST_PROP(Variant::OBJECT, "narrative_resource", set_narrative_resource, get_narrative_resource, PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedNarrative");
    IDEAM_BIND_HOST_PROP(Variant::NODE_PATH, "game_node_path", set_game_node_path, get_game_node_path, PROPERTY_HINT_NODE_PATH_VALID_TYPES, "DataOrientedGame");
    IDEAM_BIND_HOST_PROP(Variant::BOOL, "auto_setup_on_ready", set_auto_setup_on_ready, get_auto_setup_on_ready);
    IDEAM_BIND_HOST_PROP(Variant::BOOL, "auto_execute_in_process", set_auto_execute_in_process, get_auto_execute_in_process);
    IDEAM_BIND_HOST_PROP(Variant::BOOL, "share_game_memory_manager", set_share_game_memory_manager, get_share_game_memory_manager);

#undef IDEAM_BIND_HOST_PROP

    ClassDB::bind_method(D_METHOD("setup_narrative_host"), &NarrativeTaskHost::setup_narrative_host);
    ClassDB::bind_method(D_METHOD("execute_narrative_step", "delta"), &NarrativeTaskHost::execute_narrative_step);
    ClassDB::bind_method(D_METHOD("bind_data_oriented_game", "game"), &NarrativeTaskHost::bind_data_oriented_game);
}

void NarrativeTaskHost::_ready() {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_setup_on_ready_) {
        setup_narrative_host();
    }
}

void NarrativeTaskHost::_process(double p_delta) {
    if (godot::Engine::get_singleton() && godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (auto_execute_in_process_) {
        execute_narrative_step(p_delta);
    }
}

void NarrativeTaskHost::set_narrative_resource(const godot::Ref<DataOrientedNarrative>& p_narrative) {
    narrative_resource_ = p_narrative;
}
godot::Ref<DataOrientedNarrative> NarrativeTaskHost::get_narrative_resource() const {
    return narrative_resource_;
}

void NarrativeTaskHost::set_game_node_path(const godot::NodePath& p_path) {
    game_node_path_ = p_path;
}
godot::NodePath NarrativeTaskHost::get_game_node_path() const {
    return game_node_path_;
}

void NarrativeTaskHost::set_auto_setup_on_ready(bool p_auto_setup) {
    auto_setup_on_ready_ = p_auto_setup;
}
bool NarrativeTaskHost::get_auto_setup_on_ready() const {
    return auto_setup_on_ready_;
}

void NarrativeTaskHost::set_auto_execute_in_process(bool p_auto_execute) {
    auto_execute_in_process_ = p_auto_execute;
}
bool NarrativeTaskHost::get_auto_execute_in_process() const {
    return auto_execute_in_process_;
}

void NarrativeTaskHost::set_share_game_memory_manager(bool p_share) {
    share_game_memory_manager_ = p_share;
}
bool NarrativeTaskHost::get_share_game_memory_manager() const {
    return share_game_memory_manager_;
}

void NarrativeTaskHost::_wire_game_node_if_present() {
    if (game_node_path_.is_empty()) return;
    if (godot::Node* node = get_node_or_null(game_node_path_)) {
        if (DataOrientedGame* game = godot::Object::cast_to<DataOrientedGame>(node)) {
            bind_data_oriented_game(game);
        }
    }
}

void NarrativeTaskHost::setup_narrative_host() {
    if (graph_resource.is_valid()) {
        TaskGraphHost::setup();
    }

    if (narrative_resource_.is_null()) {
        return;
    }

    std::shared_ptr<core::MemoryManagerDOD> target_mgr = active_manager;
    DataOrientedGame* linked_game = nullptr;

    if (!game_node_path_.is_empty()) {
        if (godot::Node* node = get_node_or_null(game_node_path_)) {
            linked_game = godot::Object::cast_to<DataOrientedGame>(node);
            if (linked_game && share_game_memory_manager_ && linked_game->get_game_dod()) {
                target_mgr = linked_game->get_game_dod()->get_memory_manager();
            }
        }
    }

    if (is_shared && !shared_host_path.is_empty() && !target_mgr) {
        if (godot::Node* host_node = get_node_or_null(shared_host_path)) {
            if (TaskGraphHost* shared_host = godot::Object::cast_to<TaskGraphHost>(host_node)) {
                if (!shared_host->is_ready()) shared_host->setup();
                target_mgr   = shared_host->get_active_manager();
                active_graph = shared_host->get_active_graph();
            }
        }
    }

    narrative_resource_->setup_dod_pipeline_with_manager(target_mgr, active_graph);

    if (linked_game) {
        bind_data_oriented_game(linked_game);
    }

    active_manager = narrative_resource_->get_memory_manager_shared();
    active_graph   = narrative_resource_->get_active_task_graph_shared();

    if (narrative_resource_->get_begin_narrative_on_ready()) {
        narrative_resource_->begin_narrative();
    }

    emit_signal("narrative_host_ready");
}

bool NarrativeTaskHost::bind_data_oriented_game(DataOrientedGame* p_game) {
    if (!p_game || !p_game->get_game_dod() || narrative_resource_.is_null()) return false;

    core::GameDOD* game_dod = p_game->get_game_dod();
    if (share_game_memory_manager_ &&
        (!narrative_resource_->get_narrative_dod() ||
         narrative_resource_->get_memory_manager_shared() != game_dod->get_memory_manager())) {
        narrative_resource_->setup_dod_pipeline_with_manager(game_dod->get_memory_manager(), active_graph);
    }

    core::NarrativeDOD* nar_dod = narrative_resource_->get_narrative_dod();
    if (!nar_dod) return false;

    const core::GamesBufferManifest& g_man = game_dod->get_buffer_manifest();
    nar_dod->bind_external_gameplay_property_buffers(
        g_man.prop_modded_value_buffer_id,
        g_man.prop_modded_min_buffer_id,
        g_man.prop_modded_max_buffer_id,
        g_man.prop_flags_buffer_id,
        game_dod->get_property_high_water_mark()
    );

    narrative_resource_->rebuild_dod_task_graph();
    active_manager = narrative_resource_->get_memory_manager_shared();
    active_graph   = narrative_resource_->get_active_task_graph_shared();
    return true;
}

void NarrativeTaskHost::execute_narrative_step(double p_delta) {
    if (narrative_resource_.is_valid()) {
        if (!narrative_resource_->get_narrative_dod()) {
            setup_narrative_host();
        }
        narrative_resource_->execute_dod_step(p_delta);
        if (graph_resource.is_valid() && active_graph) {
            graph_resource->sync_execution_snapshots(active_graph, ui_to_dod_map);
        }
        emit_signal("narrative_step_completed", p_delta);
    } else if (is_ready()) {
        TaskGraphHost::execute_graph(p_delta);
        emit_signal("narrative_step_completed", p_delta);
    }
}

} // namespace ideam::godot_ext
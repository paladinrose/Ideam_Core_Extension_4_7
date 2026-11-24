#include "ideam_undo_redo.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_interface.hpp>
#endif

#include <algorithm>

namespace ideam::godot_ext {

std::vector<UndoRedoCommitObserver> &IdeamUndoRedo::commit_observers_() {
    static std::vector<UndoRedoCommitObserver> observers;
    return observers;
}

void IdeamUndoRedo::register_commit_observer(UndoRedoCommitObserver p_observer) {
    auto &obs = commit_observers_();
    for (auto &existing : obs) {
        if (existing.observer_id == p_observer.observer_id) {
            existing = std::move(p_observer);
            return;
        }
    }
    obs.push_back(std::move(p_observer));
}

void IdeamUndoRedo::unregister_commit_observer(const godot::StringName &p_observer_id) {
    auto &obs = commit_observers_();
    std::erase_if(obs, [&](const UndoRedoCommitObserver &o) {
        return o.observer_id == p_observer_id;
    });
}

IdeamUndoRedo::IdeamUndoRedo() {
    runtime_undo_redo = memnew(UndoRedo);
}

IdeamUndoRedo::~IdeamUndoRedo() {
    if (runtime_undo_redo) {
        memdelete(runtime_undo_redo);
        runtime_undo_redo = nullptr;
    }
}

void IdeamUndoRedo::_bind_methods() {
    ADD_SIGNAL(MethodInfo(
        "action_committed",
        PropertyInfo(Variant::STRING, "action_name"),
        PropertyInfo(Variant::BOOL, "executed")
    ));
    ADD_SIGNAL(MethodInfo("history_changed"));

    ClassDB::bind_method(D_METHOD("is_using_editor_undo_redo"), &IdeamUndoRedo::is_using_editor_undo_redo);
    ClassDB::bind_method(D_METHOD("undo"), &IdeamUndoRedo::undo);
    ClassDB::bind_method(D_METHOD("redo"), &IdeamUndoRedo::redo);
    ClassDB::bind_method(D_METHOD("has_undo"), &IdeamUndoRedo::has_undo);
    ClassDB::bind_method(D_METHOD("has_redo"), &IdeamUndoRedo::has_redo);
    ClassDB::bind_method(D_METHOD("clear_history", "increase_version"), &IdeamUndoRedo::clear_history, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_current_action_name"), &IdeamUndoRedo::get_current_action_name);
}

#ifdef TOOLS_ENABLED
void IdeamUndoRedo::set_editor_undo_redo(EditorUndoRedoManager *p_manager) {
    editor_undo_redo = p_manager;
}
#endif

bool IdeamUndoRedo::is_using_editor_undo_redo() const {
#ifdef TOOLS_ENABLED
    return Engine::get_singleton()->is_editor_hint() && editor_undo_redo != nullptr;
#else
    return false;
#endif
}

void IdeamUndoRedo::create_action(const String &name, UndoRedo::MergeMode merge_mode, Object *custom_context) {
    pending_action_name_ = name;
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            if (custom_context) {
                editor_undo_redo->create_action(name, merge_mode, custom_context);
            } else {
                editor_undo_redo->create_action(name, merge_mode);
            }
            return;
        }
    }
#endif

    if (runtime_undo_redo) {
        runtime_undo_redo->create_action(name, merge_mode);
    }
}

void IdeamUndoRedo::add_do_method(const Callable &callable) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_do_method(callable);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_do_method(callable);
    }
}

void IdeamUndoRedo::add_undo_method(const Callable &callable) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_undo_method(callable);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_undo_method(callable);
    }
}

void IdeamUndoRedo::add_do_property(Object *object, const StringName &property, const Variant &value) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_do_property(object, property, value);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_do_property(object, property, value);
    }
}

void IdeamUndoRedo::add_undo_property(Object *object, const StringName &property, const Variant &value) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_undo_property(object, property, value);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_undo_property(object, property, value);
    }
}

void IdeamUndoRedo::commit_action(bool execute) {
    const String committed_name = pending_action_name_;
    pending_action_name_        = String();

#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->commit_action(execute);
            emit_signal("action_committed", committed_name, execute);
            emit_signal("history_changed");
            const auto observers_copy = commit_observers_();
            for (const auto &obs : observers_copy) {
                if (obs.on_committed) {
                    obs.on_committed(committed_name, execute);
                }
            }
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->commit_action(execute);
    }
    emit_signal("action_committed", committed_name, execute);
    emit_signal("history_changed");
    const auto observers_copy = commit_observers_();
    for (const auto &obs : observers_copy) {
        if (obs.on_committed) {
            obs.on_committed(committed_name, execute);
        }
    }
}

bool IdeamUndoRedo::undo() {
#ifdef TOOLS_ENABLED
    if (is_using_editor_undo_redo()) {
        if (UndoRedo *ur = editor_undo_redo->get_history_undo_redo(EditorUndoRedoManager::GLOBAL_HISTORY)) {
            const bool ok = ur->undo();
            if (ok) emit_signal("history_changed");
            return ok;
        }
        return false;
    }
#endif
    if (runtime_undo_redo) {
        const bool ok = runtime_undo_redo->undo();
        if (ok) emit_signal("history_changed");
        return ok;
    }
    return false;
}

bool IdeamUndoRedo::redo() {
#ifdef TOOLS_ENABLED
    if (is_using_editor_undo_redo()) {
        if (UndoRedo *ur = editor_undo_redo->get_history_undo_redo(EditorUndoRedoManager::GLOBAL_HISTORY)) {
            const bool ok = ur->redo();
            if (ok) emit_signal("history_changed");
            return ok;
        }
        return false;
    }
#endif
    if (runtime_undo_redo) {
        const bool ok = runtime_undo_redo->redo();
        if (ok) emit_signal("history_changed");
        return ok;
    }
    return false;
}

bool IdeamUndoRedo::has_undo() const {
#ifdef TOOLS_ENABLED
    if (is_using_editor_undo_redo()) {
        if (UndoRedo *ur = editor_undo_redo->get_history_undo_redo(EditorUndoRedoManager::GLOBAL_HISTORY)) {
            return ur->has_undo();
        }
        return false;
    }
#endif
    return runtime_undo_redo ? runtime_undo_redo->has_undo() : false;
}

bool IdeamUndoRedo::has_redo() const {
#ifdef TOOLS_ENABLED
    if (is_using_editor_undo_redo()) {
        if (UndoRedo *ur = editor_undo_redo->get_history_undo_redo(EditorUndoRedoManager::GLOBAL_HISTORY)) {
            return ur->has_redo();
        }
        return false;
    }
#endif
    return runtime_undo_redo ? runtime_undo_redo->has_redo() : false;
}

void IdeamUndoRedo::clear_history(bool p_increase_version) {
#ifdef TOOLS_ENABLED
    if (is_using_editor_undo_redo()) {
        editor_undo_redo->clear_history(EditorUndoRedoManager::GLOBAL_HISTORY, p_increase_version);
        emit_signal("history_changed");
        return;
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->clear_history(p_increase_version);
        emit_signal("history_changed");
    }
}

String IdeamUndoRedo::get_current_action_name() const {
#ifdef TOOLS_ENABLED
    if (is_using_editor_undo_redo()) {
        if (UndoRedo *ur = editor_undo_redo->get_history_undo_redo(EditorUndoRedoManager::GLOBAL_HISTORY)) {
            return ur->get_current_action_name();
        }
        return String();
    }
#endif
    return runtime_undo_redo ? runtime_undo_redo->get_current_action_name() : String();
}

} // namespace ideam::godot_ext

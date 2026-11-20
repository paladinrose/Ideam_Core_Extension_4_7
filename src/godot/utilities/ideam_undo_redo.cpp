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
}

#ifdef TOOLS_ENABLED
void IdeamUndoRedo::set_editor_undo_redo(EditorUndoRedoManager *p_manager) {
    editor_undo_redo = p_manager;
}
#endif

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
    const auto observers_copy = commit_observers_();
    for (const auto &obs : observers_copy) {
        if (obs.on_committed) {
            obs.on_committed(committed_name, execute);
        }
    }
}
} // namespace ideam::godot_ext

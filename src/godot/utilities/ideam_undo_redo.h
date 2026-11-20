#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/variant/string_name.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif

#include <functional>
#include <vector>

using namespace godot;

namespace ideam::godot_ext {

struct UndoRedoCommitObserver {
    godot::StringName observer_id;
    std::function<void(const godot::String &p_action_name, bool p_executed)> on_committed;
};

class IdeamUndoRedo : public RefCounted {
    GDCLASS(IdeamUndoRedo, RefCounted);

private:
    UndoRedo *runtime_undo_redo = nullptr;
    godot::String pending_action_name_;

#ifdef TOOLS_ENABLED
    EditorUndoRedoManager *editor_undo_redo = nullptr;
#endif

    static std::vector<UndoRedoCommitObserver> &commit_observers_();

protected:
    static void _bind_methods();

public:
    IdeamUndoRedo();
    ~IdeamUndoRedo();

#ifdef TOOLS_ENABLED
    void set_editor_undo_redo(EditorUndoRedoManager *p_manager);
#endif

    // Global observer hooks (used by IdeamTutorials validator & macro recorder in Editor and Runtime)
    static void register_commit_observer(UndoRedoCommitObserver p_observer);
    static void unregister_commit_observer(const godot::StringName &p_observer_id);

    // Unified API
    void create_action(const String &name, UndoRedo::MergeMode merge_mode = UndoRedo::MERGE_DISABLE, Object *custom_context = nullptr);
    void add_do_method(const Callable &callable);
    void add_undo_method(const Callable &callable);

    template <typename... Args>
    void add_do_method(Object *p_object, const StringName &p_method, Args&&... p_args) {
        if (!p_object) return;
        Callable c(p_object, p_method);
        if constexpr (sizeof...(Args) > 0) {
            c = c.bind(std::forward<Args>(p_args)...);
        }
        add_do_method(c);
    }

    template <typename... Args>
    void add_undo_method(Object *p_object, const StringName &p_method, Args&&... p_args) {
        if (!p_object) return;
        Callable c(p_object, p_method);
        if constexpr (sizeof...(Args) > 0) {
            c = c.bind(std::forward<Args>(p_args)...);
        }
        add_undo_method(c);
    }
    void add_do_property(Object *object, const StringName &property, const Variant &value);
    void add_undo_property(Object *object, const StringName &property, const Variant &value);
    void commit_action(bool execute = true);
};
} // namespace ideam::godot_ext

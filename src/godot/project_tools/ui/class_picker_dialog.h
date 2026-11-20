#pragma once

/**
 * class_picker_dialog.h
 * Modern C++26 searchable class picker dialog for Ideam Project Tools (Phase 2).
 *
 * Mines `ClassDB`, `ProjectSettings::get_global_class_list()`, and `IdeamTier1ControlRegistry`
 * automatically, grouping instantiable classes into:
 *   - Ideam Registered Tool Classes (highlighted with owning Tier 2 workspace)
 *   - Project Global Script Classes
 *   - Godot / GDExtension Built-in Classes
 */

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class ClassPickerDialog : public godot::ConfirmationDialog {
    GDCLASS(ClassPickerDialog, godot::ConfirmationDialog)

private:
    godot::VBoxContainer *root_vbox_ = nullptr;
    godot::LineEdit *search_edit_    = nullptr;
    godot::Tree *class_tree_         = nullptr;

    godot::String base_filter_;
    godot::String selected_class_;

    void _rebuild_tree();

protected:
    static void _bind_methods();

public:
    ClassPickerDialog();
    ~ClassPickerDialog() override = default;

    void set_base_filter(const godot::String &p_base_class);
    [[nodiscard]] godot::String get_base_filter() const { return base_filter_; }
    [[nodiscard]] godot::String get_selected_class() const { return selected_class_; }

    void popup_for_base(const godot::String &p_base_class = "");

    // Signal handlers
    void _on_search_changed(const godot::String &p_text);
    void _on_item_selected();
    void _on_item_activated();
    void _on_confirmed();
};

} // namespace ideam::godot_ext

#include "class_picker_dialog.h"

#include "../../controls/ideam_tier1_control_registry.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <unordered_set>

using namespace godot;

namespace ideam::godot_ext {

void ClassPickerDialog::_bind_methods() {
    ADD_SIGNAL(MethodInfo("class_selected", PropertyInfo(Variant::STRING, "class_name")));

    ClassDB::bind_method(D_METHOD("set_base_filter", "base_class"), &ClassPickerDialog::set_base_filter);
    ClassDB::bind_method(D_METHOD("get_base_filter"), &ClassPickerDialog::get_base_filter);
    ClassDB::bind_method(D_METHOD("get_selected_class"), &ClassPickerDialog::get_selected_class);
    ClassDB::bind_method(D_METHOD("popup_for_base", "base_class"), &ClassPickerDialog::popup_for_base, DEFVAL(String()));

    ClassDB::bind_method(D_METHOD("_on_search_changed", "text"), &ClassPickerDialog::_on_search_changed);
    ClassDB::bind_method(D_METHOD("_on_item_selected"), &ClassPickerDialog::_on_item_selected);
    ClassDB::bind_method(D_METHOD("_on_item_activated"), &ClassPickerDialog::_on_item_activated);
    ClassDB::bind_method(D_METHOD("_on_confirmed"), &ClassPickerDialog::_on_confirmed);
}

ClassPickerDialog::ClassPickerDialog() {
    set_title("Select Class");
    set_min_size(Vector2i(460, 520));

    root_vbox_ = memnew(VBoxContainer);
    root_vbox_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    root_vbox_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(root_vbox_);

    search_edit_ = memnew(LineEdit);
    search_edit_->set_placeholder("Filter classes (e.g. Spline2DResource, GameHub, Node3D)...");
    search_edit_->set_clear_button_enabled(true);
    search_edit_->connect("text_changed", Callable(this, "_on_search_changed"));
    root_vbox_->add_child(search_edit_);

    class_tree_ = memnew(Tree);
    class_tree_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    class_tree_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    class_tree_->set_hide_root(true);
    class_tree_->set_columns(2);
    class_tree_->set_column_title(0, "Class");
    class_tree_->set_column_title(1, "Source / Workspace");
    class_tree_->set_column_titles_visible(true);
    class_tree_->connect("item_selected", Callable(this, "_on_item_selected"));
    class_tree_->connect("item_activated", Callable(this, "_on_item_activated"));
    root_vbox_->add_child(class_tree_);

    connect("confirmed", Callable(this, "_on_confirmed"));
}

void ClassPickerDialog::set_base_filter(const String &p_base_class) {
    base_filter_ = p_base_class;
    _rebuild_tree();
}

void ClassPickerDialog::popup_for_base(const String &p_base_class) {
    base_filter_    = p_base_class;
    selected_class_ = String();
    if (search_edit_) {
        search_edit_->clear();
    }
    _rebuild_tree();
    popup_centered();
    if (search_edit_) {
        search_edit_->grab_focus();
    }
}

void ClassPickerDialog::_rebuild_tree() {
    if (!class_tree_) {
        return;
    }
    class_tree_->clear();
    TreeItem *root = class_tree_->create_item();

    const String filter_lower = search_edit_ ? search_edit_->get_text().strip_edges().to_lower() : String();
    auto matches_filter = [&](const String &p_name) -> bool {
        return filter_lower.is_empty() || p_name.to_lower().contains(filter_lower);
    };

    ClassDB *cdb = ClassDB::get_singleton();
    auto matches_base = [&](const String &p_cls) -> bool {
        if (base_filter_.is_empty()) {
            return true;
        }
        return cdb && (p_cls == base_filter_ || cdb->is_parent_class(p_cls, base_filter_));
    };

    // 1. Ideam Registered Tool Classes (from IdeamTier1ControlRegistry)
    std::unordered_set<std::string> listed_ideam_classes;
    TreeItem *ideam_group = class_tree_->create_item(root);
    ideam_group->set_text(0, "Ideam Tool-Backed Classes");
    ideam_group->set_selectable(0, false);
    ideam_group->set_selectable(1, false);

    const auto &descs = IdeamTier1ControlRegistry::get_singleton().get_all_descriptors();
    int ideam_count   = 0;
    for (const auto &d : descs) {
        const String cls_name(d.resource_class);
        if (!matches_base(cls_name) || !matches_filter(cls_name)) {
            continue;
        }
        if (!listed_ideam_classes.insert(std::string(cls_name.utf8().get_data())).second) {
            continue;
        }
        TreeItem *item = class_tree_->create_item(ideam_group);
        item->set_text(0, cls_name);
        item->set_text(1, String(d.workspace_id) + " (" + d.sidebar_tab + ")");
        item->set_metadata(0, cls_name);
        ++ideam_count;
    }
    if (ideam_count == 0) {
        ideam_group->free();
    }

    // 2. Project Global Script Classes
    TreeItem *script_group = class_tree_->create_item(root);
    script_group->set_text(0, "Project Global Scripts");
    script_group->set_selectable(0, false);
    script_group->set_selectable(1, false);

    int script_count = 0;
    if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
        TypedArray<Dictionary> globals = ps->get_global_class_list();
        for (int i = 0; i < globals.size(); ++i) {
            Dictionary entry  = globals[i];
            const String cls  = entry.get("class", "");
            const String base = entry.get("base", "");
            if (cls.is_empty() || !matches_filter(cls)) {
                continue;
            }
            if (!base_filter_.is_empty() && base != base_filter_ && !(cdb && cdb->is_parent_class(base, base_filter_))) {
                continue;
            }
            TreeItem *item = class_tree_->create_item(script_group);
            item->set_text(0, cls);
            item->set_text(1, String("GDScript : ") + base);
            item->set_metadata(0, cls);
            ++script_count;
        }
    }
    if (script_count == 0) {
        script_group->free();
    }

    // 3. Built-in & GDExtension ClassDB Classes
    TreeItem *builtin_group = class_tree_->create_item(root);
    builtin_group->set_text(0, base_filter_.is_empty() ? "Engine & GDExtension Classes" : (base_filter_ + " Subclasses"));
    builtin_group->set_selectable(0, false);
    builtin_group->set_selectable(1, false);

    if (cdb) {
        PackedStringArray classes = base_filter_.is_empty()
            ? cdb->get_class_list()
            : cdb->get_inheriters_from_class(base_filter_);

        if (!base_filter_.is_empty() && cdb->can_instantiate(base_filter_) && matches_filter(base_filter_)) {
            TreeItem *self_item = class_tree_->create_item(builtin_group);
            self_item->set_text(0, base_filter_);
            self_item->set_text(1, "ClassDB");
            self_item->set_metadata(0, base_filter_);
        }

        for (int i = 0; i < classes.size(); ++i) {
            const String cls = classes[i];
            if (!cdb->can_instantiate(cls) || !matches_filter(cls)) {
                continue;
            }
            if (listed_ideam_classes.contains(std::string(cls.utf8().get_data()))) {
                continue;
            }
            TreeItem *item = class_tree_->create_item(builtin_group);
            item->set_text(0, cls);
            item->set_text(1, "ClassDB");
            item->set_metadata(0, cls);
        }
    }
}

void ClassPickerDialog::_on_search_changed(const String &p_text) {
    (void)p_text;
    _rebuild_tree();
}

void ClassPickerDialog::_on_item_selected() {
    if (!class_tree_) {
        return;
    }
    TreeItem *sel = class_tree_->get_selected();
    if (sel && sel->get_metadata(0).get_type() == Variant::STRING) {
        selected_class_ = sel->get_metadata(0);
    } else {
        selected_class_ = String();
    }
}

void ClassPickerDialog::_on_item_activated() {
    _on_item_selected();
    if (!selected_class_.is_empty()) {
        emit_signal("class_selected", selected_class_);
        hide();
    }
}

void ClassPickerDialog::_on_confirmed() {
    _on_item_selected();
    if (!selected_class_.is_empty()) {
        emit_signal("class_selected", selected_class_);
    }
}

} // namespace ideam::godot_ext

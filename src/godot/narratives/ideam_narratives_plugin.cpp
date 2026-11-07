#include "ideam_narratives_plugin.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>

#include "../../core/tasks/registration/ideam_task_registry.h"
#include "../../core/tasks/domains/narratives/registration/narratives_task_domain_pack.h"

namespace ideam::godot_ext {

IdeamNarrativesPlugin* IdeamNarrativesPlugin::singleton = nullptr;

void IdeamNarrativesPlugin::_bind_methods() {
    // Cross-Plugin Task Domain Handshake
    godot::ClassDB::bind_method(godot::D_METHOD("get_task_domain_name"), &IdeamNarrativesPlugin::get_task_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("register_task_domain_pack"), &IdeamNarrativesPlugin::register_task_domain_pack);

    // Static bindings so the ecosystem can call these without an instance reference
    godot::ClassDB::bind_static_method("IdeamNarrativesPlugin", godot::D_METHOD("undo_redo"), &IdeamNarrativesPlugin::get_static_undo_redo);
}

IdeamNarrativesPlugin::IdeamNarrativesPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamNarrativesPlugin::~IdeamNarrativesPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamNarrativesPlugin::_enter_tree() {
    // Call base class template setup
    validate_script_templates();

    // Ecosystem Handshake: Announce IdeamNarratives is active and offer TaskDomainPack if IdeamTasks is active
    set_plugin_active("IdeamNarratives", true);
    register_to_ecosystem("WizardSettings", "IdeamNarratives", godot::String(NARRATIVES_SETTINGS_PATH.data()));
    register_to_ecosystem("TaskManifests", "IdeamNarratives", godot::String(core::NarrativesTaskDomainPack::MANIFEST_PATH));
    if (is_plugin_active("IdeamTasks")) {
        register_task_domain_pack();
    }

    // Call specialized narrative script template setup
    validate_narrative_script_templates();
}

void IdeamNarrativesPlugin::_exit_tree() {
    // Ecosystem Handshake: Mark IdeamNarratives inactive and unmount its TaskDomainPack in editor
    set_plugin_active("IdeamNarratives", false);
    if (auto* task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamNarratives", false);
    }
}

void IdeamNarrativesPlugin::_create_template(const godot::String& folder_name, const godot::String& default_file_name) {
    godot::String base_path = "res://script_templates/";
    godot::String folder_path = base_path + folder_name;

    godot::Ref<godot::DirAccess> dir = godot::DirAccess::open(base_path);
    if (dir.is_valid() && !dir->dir_exists(folder_name)) {
        dir->make_dir(folder_name);
    }

    godot::Ref<godot::DirAccess> template_dir = godot::DirAccess::open(folder_path);
    if (template_dir.is_valid() && !template_dir->file_exists("default_template.gd")) {
        godot::String source_path = "res://addons/ideam_narratives/resources/" + default_file_name;
        godot::Ref<godot::FileAccess> default_text = godot::FileAccess::open(source_path, godot::FileAccess::READ);

        if (default_text.is_valid()) {
            godot::Ref<godot::FileAccess> new_template = godot::FileAccess::open(folder_path + "/default_template.gd", godot::FileAccess::WRITE);
            if (new_template.is_valid()) {
                new_template->store_string(default_text->get_as_text());
                new_template->close();
            }
            default_text->close();
        }
    }
}

void IdeamNarrativesPlugin::validate_narrative_script_templates() {
    _create_template("Narreme", "default_narreme_template.txt");
    _create_template("Narrative", "default_narrative_template.txt");
    _create_template("Character", "default_character_template.txt");
    _create_template("Location", "default_location_template.txt");
    _create_template("Prop", "default_prop_template.txt");
    _create_template("Incident", "default_incident_template.txt");
    _create_template("Plot", "default_plot_template.txt");
}

void IdeamNarrativesPlugin::register_task_domain_pack() {
    if (auto* task_reg = core::IdeamTaskRegistry::get_singleton()) {
        if (!task_reg->has_domain_pack("IdeamNarratives")) {
            core::NarrativesTaskDomainPack::register_domain();
        }
        task_reg->set_domain_pack_active("IdeamNarratives", true);
    }
}

// --- Static Godot API ---

godot::EditorUndoRedoManager* IdeamNarrativesPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}

} // namespace ideam::godot_ext
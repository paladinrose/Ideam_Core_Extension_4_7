#include "ideam_linguistics_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamLinguisticsPlugin *IdeamLinguisticsPlugin::singleton = nullptr;

void IdeamLinguisticsPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_plugin_domain_name"), &IdeamLinguisticsPlugin::get_plugin_domain_name);
    ClassDB::bind_method(D_METHOD("is_communications_handshake_available"), &IdeamLinguisticsPlugin::is_communications_handshake_available);
    ClassDB::bind_method(D_METHOD("is_minds_handshake_available"), &IdeamLinguisticsPlugin::is_minds_handshake_available);
    ClassDB::bind_method(D_METHOD("is_narratives_handshake_available"), &IdeamLinguisticsPlugin::is_narratives_handshake_available);
    ClassDB::bind_method(D_METHOD("is_procedural_generation_handshake_available"), &IdeamLinguisticsPlugin::is_procedural_generation_handshake_available);
    ClassDB::bind_method(D_METHOD("is_wfc_handshake_available"), &IdeamLinguisticsPlugin::is_wfc_handshake_available);
    ClassDB::bind_method(D_METHOD("is_quantum_darwinism_handshake_available"), &IdeamLinguisticsPlugin::is_quantum_darwinism_handshake_available);
    ClassDB::bind_method(D_METHOD("is_design_handshake_available"), &IdeamLinguisticsPlugin::is_design_handshake_available);
    ClassDB::bind_method(D_METHOD("is_characters_handshake_available"), &IdeamLinguisticsPlugin::is_characters_handshake_available);
    ClassDB::bind_method(D_METHOD("is_games_handshake_available"), &IdeamLinguisticsPlugin::is_games_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tasks_handshake_available"), &IdeamLinguisticsPlugin::is_tasks_handshake_available);

    ClassDB::bind_method(D_METHOD("get_default_language_asset"), &IdeamLinguisticsPlugin::get_default_language_asset);
    ClassDB::bind_method(D_METHOD("get_default_prompt_parser"), &IdeamLinguisticsPlugin::get_default_prompt_parser);
    ClassDB::bind_method(D_METHOD("get_metadata_manager"), &IdeamLinguisticsPlugin::get_metadata_manager);
    ClassDB::bind_method(D_METHOD("get_object_profiler"), &IdeamLinguisticsPlugin::get_object_profiler);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method(
        "IdeamLinguisticsPlugin",
        D_METHOD("undo_redo"),
        &IdeamLinguisticsPlugin::get_static_undo_redo
    );
#endif
    ClassDB::bind_static_method(
        "IdeamLinguisticsPlugin",
        D_METHOD("create_undo_redo_wrapper"),
        &IdeamLinguisticsPlugin::create_undo_redo_wrapper
    );
}

IdeamLinguisticsPlugin::IdeamLinguisticsPlugin() {
    if (singleton == nullptr) singleton = this;
}

IdeamLinguisticsPlugin::~IdeamLinguisticsPlugin() {
    if (singleton == this) singleton = nullptr;
}

void IdeamLinguisticsPlugin::_ensure_services_initialized() const {
    if (default_language_asset_.is_null()) {
        default_language_asset_.instantiate();
        default_language_asset_->populate_builtin_godot_and_wizard_vocabulary();
    }
    if (default_prompt_parser_.is_null()) {
        default_prompt_parser_.instantiate();
        default_prompt_parser_->set_language_asset(default_language_asset_);
    }
    if (metadata_manager_.is_null()) {
        metadata_manager_.instantiate();
        metadata_manager_->set_prompt_parser(default_prompt_parser_);
        metadata_manager_->set_undo_redo(create_undo_redo_wrapper());
    }
    if (object_profiler_.is_null()) {
        object_profiler_.instantiate();
        object_profiler_->set_prompt_parser(default_prompt_parser_);
        object_profiler_->set_metadata_manager(metadata_manager_);
    }
}

void IdeamLinguisticsPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamLinguistics", true);
    register_to_ecosystem("WizardSettings", "IdeamLinguistics", godot::String(LINGUISTICS_SETTINGS_PATH.data()));

    _ensure_services_initialized();

    linguistics_inspector_.instantiate();
    add_inspector_plugin(linguistics_inspector_);
}

void IdeamLinguisticsPlugin::_exit_tree() {
    if (linguistics_inspector_.is_valid()) {
        remove_inspector_plugin(linguistics_inspector_);
        linguistics_inspector_.unref();
    }
    object_profiler_.unref();
    metadata_manager_.unref();
    default_prompt_parser_.unref();
    default_language_asset_.unref();
    set_plugin_active("IdeamLinguistics", false);
}

godot::Ref<LanguageAssetResource> IdeamLinguisticsPlugin::get_default_language_asset() const {
    _ensure_services_initialized();
    return default_language_asset_;
}

godot::Ref<SemanticPromptParser> IdeamLinguisticsPlugin::get_default_prompt_parser() const {
    _ensure_services_initialized();
    return default_prompt_parser_;
}

godot::Ref<MetadataLinguisticsManager> IdeamLinguisticsPlugin::get_metadata_manager() const {
    _ensure_services_initialized();
    return metadata_manager_;
}

godot::Ref<LinguisticObjectProfiler> IdeamLinguisticsPlugin::get_object_profiler() const {
    _ensure_services_initialized();
    return object_profiler_;
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamLinguisticsPlugin::get_static_undo_redo() {
    return singleton ? singleton->get_undo_redo() : nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamLinguisticsPlugin::create_undo_redo_wrapper() {
    godot::Ref<IdeamUndoRedo> ur;
    ur.instantiate();
#ifdef TOOLS_ENABLED
    if (godot::EditorUndoRedoManager *ed_ur = get_static_undo_redo()) {
        ur->set_editor_undo_redo(ed_ur);
    }
#endif
    return ur;
}

} // namespace ideam::godot_ext

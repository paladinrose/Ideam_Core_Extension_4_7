#include "ideam_task_registry.h"

// --- Sub-Registries ---
#include "query_task_registry.h"
#include "transform_task_registry.h"
#include "metadata_task_registry.h"

// --- Manual Tasks ---
#include "../entry_fill_task.h"
#include "../sub_graph_task.h"

// --- Godot Editor Bridging (Resources & UI Graph Nodes) ---
#include "../../../godot/tasks/entry_fill_task_resource.h"
#include "../../../godot/tasks/entry_fill_task_graph_node.h"
#include "../../../godot/tasks/sub_graph_task_resource.h"
#include "../../../godot/tasks/sub_graph_task_graph_node.h"
#include "../../../godot/tasks/task_graph_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::core {

namespace {

inline godot::StringName normalize_domain(const godot::StringName& p_domain) {
    return p_domain.is_empty() ? godot::StringName("Core") : p_domain;
}

} // namespace

godot::Ref<godot::FileAccess> IdeamTaskRegistry::bake_log_file;

void IdeamTaskRegistry::start_bake_log() {
    bake_log_file = godot::FileAccess::open("res://ideam_bake_log.txt", godot::FileAccess::WRITE);
    if (bake_log_file.is_valid()) {
        bake_log_file->store_line("=== TASK REGISTRY BAKE LOG START ===");
    } else {
        godot::UtilityFunctions::printerr("IdeamTasks: Could not open bake_log.txt for writing.");
    }
}

void IdeamTaskRegistry::log_with_registry(const godot::String& p_message) {
    if (bake_log_file.is_valid()) {
        bake_log_file->store_line(p_message);
    }
}

void IdeamTaskRegistry::end_bake_log() {
    if (bake_log_file.is_valid()) {
        bake_log_file->store_line("=== TASK REGISTRY BAKE LOG END ===");
        bake_log_file->flush();
        bake_log_file.unref();
    }
}

IdeamTaskRegistry* IdeamTaskRegistry::singleton = nullptr;

void IdeamTaskRegistry::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("bake_manifest"), &IdeamTaskRegistry::bake_manifest);
    godot::ClassDB::bind_method(godot::D_METHOD("get_manifest_version"), &IdeamTaskRegistry::get_manifest_version);
    godot::ClassDB::bind_method(godot::D_METHOD("get_domain_manifest_version", "domain"), &IdeamTaskRegistry::get_domain_manifest_version);
    godot::ClassDB::bind_method(godot::D_METHOD("has_domain_pack", "domain"), &IdeamTaskRegistry::has_domain_pack);
    godot::ClassDB::bind_method(godot::D_METHOD("set_domain_pack_active", "domain", "active"), &IdeamTaskRegistry::set_domain_pack_active);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_domains"), &IdeamTaskRegistry::get_active_domains);
    godot::ClassDB::bind_method(godot::D_METHOD("get_domain_ui_prefix", "domain"), &IdeamTaskRegistry::get_domain_ui_prefix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_domain_query_matrix", "domain"), &IdeamTaskRegistry::get_domain_query_matrix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_domain_transform_matrix", "domain"), &IdeamTaskRegistry::get_domain_transform_matrix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_domain_metadata_matrix", "domain"), &IdeamTaskRegistry::get_domain_metadata_matrix);
    godot::ClassDB::bind_method(godot::D_METHOD("are_active_manifests_valid"), &IdeamTaskRegistry::are_active_manifests_valid);
    godot::ClassDB::bind_method(godot::D_METHOD("get_outdated_active_domains"), &IdeamTaskRegistry::get_outdated_active_domains);

    ADD_SIGNAL(godot::MethodInfo("manifest_updated"));
}

void IdeamTaskRegistry::init() {
    if (!singleton) {
        singleton = memnew(IdeamTaskRegistry);
        godot::Engine::get_singleton()->register_singleton("IdeamTaskRegistry", singleton);
    }

    // 1. Register Unique Manual Tasks (Populates C++ closures in memory + caches utility matrix definitions)
    singleton->register_task<EntryFillTask, godot_ext::EntryFillTaskResource, godot_ext::EntryFillTaskGraphNode>("Entry Fill"); 
    singleton->register_task<SubGraphTask, godot_ext::SubGraphTaskResource, godot_ext::SubGraphTaskGraphNode>("Sub-Graph");

    // 2. Register the Primary "Core" TaskDomainPack
    TaskDomainPack core_pack;
    core_pack.domain_name = godot::StringName("Core");
    core_pack.ui_menu_prefix = "";
    core_pack.manifest_path = TaskManifest::MANIFEST_PATH;
    core_pack.expected_manifest_version = TaskManifest::CURRENT_MANIFEST_VERSION;
    core_pack.is_core = true;
    core_pack.is_active_in_editor = true;
    core_pack.init_routing = []() {
        QueryTaskRegistry::init_execution_routing();
        TransformTaskRegistry::init_execution_routing();
        MetadataTaskRegistry::init_execution_routing();
    };
    core_pack.cleanup_routing = []() {
        QueryTaskRegistry::cleanup_execution_routing();
        TransformTaskRegistry::cleanup_execution_routing();
        MetadataTaskRegistry::cleanup_execution_routing();
    };
    core_pack.generate_query_ui = &QueryTaskRegistry::generate_ui_matrices;
    core_pack.generate_transform_ui = &TransformTaskRegistry::generate_ui_matrices;
    core_pack.generate_metadata_ui = &MetadataTaskRegistry::generate_ui_matrices;
    core_pack.create_transform = &TransformTaskRegistry::create;
    core_pack.create_query = &QueryTaskRegistry::create;
    core_pack.create_metadata = &MetadataTaskRegistry::create;

    singleton->register_domain_pack(core_pack);
    singleton->sync_active_domains_from_config();
}

void IdeamTaskRegistry::register_domain_pack(const TaskDomainPack& p_pack) {
    if (p_pack.domain_name.is_empty()) {
        godot::UtilityFunctions::printerr("IdeamTaskRegistry: Cannot register a TaskDomainPack with an empty domain_name.");
        return;
    }

    TaskDomainPack pack = p_pack;

    // 1. Initialize fast-path C++ execution pointers immediately
    if (pack.init_routing) {
        pack.init_routing();
    }

    // 2. Load existing manifest from disk if present, otherwise instantiate an in-memory placeholder
    if (!pack.manifest_path.is_empty() && godot::FileAccess::file_exists(pack.manifest_path)) {
        pack.manifest = godot::ResourceLoader::get_singleton()->load(pack.manifest_path);
    }
    if (!pack.manifest.is_valid()) {
        pack.manifest.instantiate();
        pack.manifest->set_plugin_domain(pack.domain_name);
    }

    // 3. Sync initial active state from .ideam_registry.cfg if not explicitly set
    if (!pack.is_core && !pack.is_active_in_editor) {
        godot::Ref<godot::ConfigFile> cfg;
        cfg.instantiate();
        if (godot::FileAccess::file_exists("res://.ideam_registry.cfg") &&
            cfg->load("res://.ideam_registry.cfg") == godot::OK) {
            pack.is_active_in_editor = static_cast<bool>(
                cfg->get_value("ActivePlugins", godot::String(pack.domain_name), false)
            );
        }
    } else if (pack.is_core) {
        pack.is_active_in_editor = true;
    }

    domain_packs[pack.domain_name] = pack;

    if (pack.is_core || pack.domain_name == godot::StringName("Core")) {
        active_manifest = pack.manifest;
    }
}

bool IdeamTaskRegistry::has_domain_pack(const godot::StringName& p_domain) const {
    return domain_packs.has(normalize_domain(p_domain));
}

void IdeamTaskRegistry::set_domain_pack_active(const godot::StringName& p_domain, bool p_active) {
    if (!domain_packs.has(p_domain)) {
        return;
    }

    TaskDomainPack& pack = domain_packs[p_domain];
    if (pack.is_core) {
        return;
    }

    if (pack.is_active_in_editor == p_active) {
        return;
    }

    pack.is_active_in_editor = p_active;

    // Reload manifest from disk when newly activated in case it was generated earlier
    if (p_active && !pack.manifest_path.is_empty() && godot::FileAccess::file_exists(pack.manifest_path)) {
        godot::Ref<TaskManifest> loaded = godot::ResourceLoader::get_singleton()->load(pack.manifest_path);
        if (loaded.is_valid()) {
            pack.manifest = loaded;
        }
    }

    emit_signal("manifest_updated");
}

void IdeamTaskRegistry::sync_active_domains_from_config() {
    godot::Ref<godot::ConfigFile> cfg;
    cfg.instantiate();
    bool has_cfg = godot::FileAccess::file_exists("res://.ideam_registry.cfg") &&
                   cfg->load("res://.ideam_registry.cfg") == godot::OK;

    for (auto& kv : domain_packs) {
        TaskDomainPack& pack = kv.value;
        if (pack.is_core) {
            pack.is_active_in_editor = true;
            continue;
        }

        if (has_cfg) {
            pack.is_active_in_editor = static_cast<bool>(
                cfg->get_value("ActivePlugins", godot::String(pack.domain_name), false)
            );
        } else {
            pack.is_active_in_editor = false;
        }
    }
}

bool IdeamTaskRegistry::are_active_manifests_valid() const {
    if (domain_packs.is_empty()) {
        return false;
    }

    for (const auto& kv : domain_packs) {
        const TaskDomainPack& pack = kv.value;
        if (!pack.is_core && !pack.is_active_in_editor) {
            continue;
        }
        if (!pack.manifest.is_valid() || pack.manifest->get_manifest_version() != pack.expected_manifest_version) {
            return false;
        }
    }
    return true;
}

godot::PackedStringArray IdeamTaskRegistry::get_outdated_active_domains() const {
    godot::PackedStringArray outdated;
    for (const auto& kv : domain_packs) {
        const TaskDomainPack& pack = kv.value;
        if (!pack.is_core && !pack.is_active_in_editor) {
            continue;
        }
        if (!pack.manifest.is_valid() || pack.manifest->get_manifest_version() != pack.expected_manifest_version) {
            outdated.push_back(godot::String(pack.domain_name));
        }
    }
    return outdated;
}

godot::PackedStringArray IdeamTaskRegistry::get_active_domains() const {
    godot::PackedStringArray active;
    if (domain_packs.has("Core")) {
        active.push_back("Core");
    }
    for (const auto& kv : domain_packs) {
        const TaskDomainPack& pack = kv.value;
        if (pack.domain_name == godot::StringName("Core")) {
            continue;
        }
        if (pack.is_core || pack.is_active_in_editor) {
            active.push_back(godot::String(pack.domain_name));
        }
    }
    return active;
}

godot::String IdeamTaskRegistry::get_domain_ui_prefix(const godot::StringName& p_domain) const {
    godot::StringName domain = normalize_domain(p_domain);
    if (domain_packs.has(domain)) {
        return domain_packs[domain].ui_menu_prefix;
    }
    return "";
}

std::unique_ptr<INativeTask> IdeamTaskRegistry::create(const godot::StringName& p_name) {
    if (manual_factories.has(p_name)) {
        return manual_factories[p_name]();
    }
    godot::UtilityFunctions::printerr("IdeamTaskRegistry: Failed to create unregistered manual task '", p_name, "'");
    return nullptr;
}

std::unique_ptr<INativeTask> IdeamTaskRegistry::create_transform_task(
    const godot::StringName& p_domain,
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) const {
    godot::StringName domain = normalize_domain(p_domain);
    if (domain_packs.has(domain)) {
        const TaskDomainPack& pack = domain_packs[domain];
        if (pack.create_transform) {
            return pack.create_transform(p_logic_id, p_view_id, p_strategy_id, p_type_id);
        }
    }
    godot::UtilityFunctions::printerr("IdeamTaskRegistry: Transform dispatcher not found for domain '", domain, "'.");
    return nullptr;
}

std::unique_ptr<INativeTask> IdeamTaskRegistry::create_query_task(
    const godot::StringName& p_domain,
    uint32_t p_op_id,
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) const {
    godot::StringName domain = normalize_domain(p_domain);
    if (domain_packs.has(domain)) {
        const TaskDomainPack& pack = domain_packs[domain];
        if (pack.create_query) {
            return pack.create_query(p_op_id, p_logic_id, p_view_id, p_strategy_id, p_type_id);
        }
    }
    godot::UtilityFunctions::printerr("IdeamTaskRegistry: Query dispatcher not found for domain '", domain, "'.");
    return nullptr;
}

std::unique_ptr<INativeTask> IdeamTaskRegistry::create_metadata_task(
    const godot::StringName& p_domain,
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) const {
    godot::StringName domain = normalize_domain(p_domain);
    if (domain_packs.has(domain)) {
        const TaskDomainPack& pack = domain_packs[domain];
        if (pack.create_metadata) {
            return pack.create_metadata(p_logic_id, p_view_id, p_strategy_id, p_type_id);
        }
    }
    godot::UtilityFunctions::printerr("IdeamTaskRegistry: Metadata dispatcher not found for domain '", domain, "'.");
    return nullptr;
}

void IdeamTaskRegistry::cleanup() {
    if (singleton) {
        godot::Engine::get_singleton()->unregister_singleton("IdeamTaskRegistry");

        for (auto& kv : singleton->domain_packs) {
            if (kv.value.cleanup_routing) {
                kv.value.cleanup_routing();
            }
        }
        singleton->domain_packs.clear();

        memdelete(singleton);
        singleton = nullptr;
    }
}

void IdeamTaskRegistry::bake_manifest() {
    start_bake_log();
    log_with_registry("INIT: Starting multi-domain bake_manifest sequence.");

    sync_active_domains_from_config();

    bool all_saved_ok = true;
    int baked_count = 0;

    for (auto& kv : domain_packs) {
        TaskDomainPack& pack = kv.value;
        if (!pack.is_core && !pack.is_active_in_editor) {
            log_with_registry("SKIP: Domain '" + godot::String(pack.domain_name) + "' is inactive in editor.");
            continue;
        }

        if (!pack.manifest.is_valid()) {
            pack.manifest.instantiate();
        }

        log_with_registry("BAKE: Generating UI matrices for domain '" + godot::String(pack.domain_name) + "'...");

        godot::Dictionary query_matrix;
        godot::Dictionary transform_matrix;
        godot::Dictionary metadata_matrix;

        if (pack.generate_query_ui) {
            pack.generate_query_ui(query_matrix);
        }
        log_with_registry("METRIC [" + godot::String(pack.domain_name) + "]: query_matrix size = " + godot::itos(query_matrix.size()));

        if (pack.generate_transform_ui) {
            pack.generate_transform_ui(transform_matrix);
        }
        log_with_registry("METRIC [" + godot::String(pack.domain_name) + "]: transform_matrix size = " + godot::itos(transform_matrix.size()));

        if (pack.generate_metadata_ui) {
            pack.generate_metadata_ui(metadata_matrix);
        }
        log_with_registry("METRIC [" + godot::String(pack.domain_name) + "]: metadata_matrix size = " + godot::itos(metadata_matrix.size()));

        pack.manifest->set_plugin_domain(pack.domain_name);
        pack.manifest->set_query_matrix(query_matrix);
        pack.manifest->set_transform_matrix(transform_matrix);
        pack.manifest->set_metadata_matrix(metadata_matrix);

        if (pack.is_core) {
            pack.manifest->set_utility_matrix(pending_utility_matrix);
            active_manifest = pack.manifest;
        }

        pack.manifest->set_manifest_version(pack.expected_manifest_version);

        // Ensure target directory exists before saving
        godot::String base_dir = pack.manifest_path.get_base_dir();
        if (!base_dir.is_empty() && !godot::DirAccess::dir_exists_absolute(base_dir)) {
            godot::DirAccess::make_dir_recursive_absolute(base_dir);
        }

        log_with_registry("SAVE [" + godot::String(pack.domain_name) + "]: Saving manifest to " + pack.manifest_path);
        godot::Error err = godot::ResourceSaver::get_singleton()->save(pack.manifest, pack.manifest_path);

        if (err == godot::OK) {
            baked_count++;
            log_with_registry("SUCCESS [" + godot::String(pack.domain_name) + "]: Manifest baked (v" + godot::itos(pack.expected_manifest_version) + ").");
            godot::UtilityFunctions::print("IdeamTasks: Manifest baked for domain '", pack.domain_name, "' -> ", pack.manifest_path);
        } else {
            all_saved_ok = false;
            log_with_registry("ERROR [" + godot::String(pack.domain_name) + "]: Failed to save manifest. Error code: " + godot::itos(err));
            godot::UtilityFunctions::printerr("IdeamTasks: Failed to save manifest for domain '", pack.domain_name, "' at ", pack.manifest_path);
        }
    }

    if (all_saved_ok && baked_count > 0) {
        emit_signal("manifest_updated");
    }

    end_bake_log();
}

godot::Ref<TaskManifest> IdeamTaskRegistry::get_domain_manifest(const godot::StringName& p_domain) const {
    godot::StringName domain = normalize_domain(p_domain);
    if (domain_packs.has(domain)) {
        return domain_packs[domain].manifest;
    }
    return godot::Ref<TaskManifest>();
}

int IdeamTaskRegistry::get_manifest_version() const {
    return get_domain_manifest_version("Core");
}

int IdeamTaskRegistry::get_domain_manifest_version(const godot::StringName& p_domain) const {
    godot::Ref<TaskManifest> man = get_domain_manifest(p_domain);
    return man.is_valid() ? man->get_manifest_version() : 0;
}

godot::Dictionary IdeamTaskRegistry::get_domain_query_matrix(const godot::StringName& p_domain) const {
    godot::Ref<TaskManifest> man = get_domain_manifest(p_domain);
    return man.is_valid() ? man->get_query_matrix() : godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_domain_transform_matrix(const godot::StringName& p_domain) const {
    godot::Ref<TaskManifest> man = get_domain_manifest(p_domain);
    return man.is_valid() ? man->get_transform_matrix() : godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_domain_metadata_matrix(const godot::StringName& p_domain) const {
    godot::Ref<TaskManifest> man = get_domain_manifest(p_domain);
    return man.is_valid() ? man->get_metadata_matrix() : godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_ui_query_matrix(const godot::StringName& p_domain) {
    return singleton ? singleton->get_domain_query_matrix(p_domain) : godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_ui_transform_matrix(const godot::StringName& p_domain) {
    return singleton ? singleton->get_domain_transform_matrix(p_domain) : godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_ui_metadata_matrix(const godot::StringName& p_domain) {
    return singleton ? singleton->get_domain_metadata_matrix(p_domain) : godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_ui_utility_matrix() {
    if (singleton && singleton->active_manifest.is_valid()) {
        return singleton->active_manifest->get_utility_matrix();
    }
    return godot::Dictionary();
}

} // namespace ideam::core
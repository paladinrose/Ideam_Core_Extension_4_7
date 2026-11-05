#include "task_graph_edit.h"
#include "task_graph_resource.h"
#include "metadata_task_resource.h"
#include "query_task_resource.h"
#include "transform_task_resource.h"
#include "transform_task_graph_node.h"
#include "query_task_graph_node.h"
#include "metadata_task_graph_node.h"

#include "../../core/tasks/registration/ideam_task_registry.h"
#include "../../core/tasks/registration/transform_task_registry.h"
#include "../../core/tasks/registration/metadata_task_registry.h"
#include "../../core/tasks/registration/query_task_registry.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

namespace ideam::godot_ext {

void TaskGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_node_duplicate_requested", "node_name"), &TaskGraphEdit::_on_node_duplicate_requested);
    ClassDB::bind_method(D_METHOD("_on_node_delete_requested", "node_name"), &TaskGraphEdit::_on_node_delete_requested);

    ClassDB::bind_method(D_METHOD("set_menu_hierarchy_mode", "mode"), &TaskGraphEdit::set_menu_hierarchy_mode);
    ClassDB::bind_method(D_METHOD("get_menu_hierarchy_mode"), &TaskGraphEdit::get_menu_hierarchy_mode);

    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "menu_hierarchy_mode", PROPERTY_HINT_ENUM, "Axis First,Domain First,Hybrid Auto"),
        "set_menu_hierarchy_mode",
        "get_menu_hierarchy_mode"
    );

    BIND_ENUM_CONSTANT(MENU_AXIS_FIRST);
    BIND_ENUM_CONSTANT(MENU_DOMAIN_FIRST);
    BIND_ENUM_CONSTANT(MENU_HYBRID_AUTO);
}

TaskGraphEdit::TaskGraphEdit() {
}

TaskGraphEdit::~TaskGraphEdit() {
}

void TaskGraphEdit::_ready() {
    MemoryGraphEdit::_ready();
    // Removed the manual popup disconnect/reconnect logic since we now hook the sub-menus
    // directly into the parent's native _popup_select.
}

void TaskGraphEdit::set_menu_hierarchy_mode(int p_mode) {
    if (p_mode < MENU_AXIS_FIRST || p_mode > MENU_HYBRID_AUTO) {
        return;
    }
    menu_hierarchy_mode = static_cast<MenuHierarchyMode>(p_mode);
}

int TaskGraphEdit::get_menu_hierarchy_mode() const {
    return static_cast<int>(menu_hierarchy_mode);
}

void TaskGraphEdit::_notification(int p_what) {
    // Bubble up to MemoryGraphEdit first
    MemoryGraphEdit::_notification(p_what);

    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _update_theme_properties();
        } break;
    }
}

void TaskGraphEdit::_style_popup_menu_recursive(
    PopupMenu* p_menu,
    const Ref<StyleBox>& p_panel_style,
    const Ref<StyleBox>& p_hover_style
) {
    if (!p_menu) return;

    if (p_panel_style.is_valid()) p_menu->add_theme_stylebox_override("panel", p_panel_style);
    if (p_hover_style.is_valid()) p_menu->add_theme_stylebox_override("hover", p_hover_style);

    for (int i = 0; i < p_menu->get_child_count(); ++i) {
        PopupMenu* sub = Object::cast_to<PopupMenu>(p_menu->get_child(i));
        if (sub) {
            _style_popup_menu_recursive(sub, p_panel_style, p_hover_style);
        }
    }
}

void TaskGraphEdit::_update_theme_properties() {
    // 1. Core parent theme configuration (handles context_popup)
    MemoryGraphEdit::_update_theme_properties();

    // 2. Propagate styles recursively down to any active submenu layers of arbitrary depth
    if (context_popup) {
        Ref<StyleBox> panel_style = get_theme_stylebox("popup_menu_panel", "PopupMenu");
        Ref<StyleBox> hover_style = get_theme_stylebox("popup_menu_hover", "PopupMenu");

        for (int i = 0; i < context_popup->get_child_count(); ++i) {
            PopupMenu* sub = Object::cast_to<PopupMenu>(context_popup->get_child(i));
            if (sub) {
                _style_popup_menu_recursive(sub, panel_style, hover_style);
            }
        }
    }
}

bool TaskGraphEdit::_strategy_supports_layout(core::MemoryStrategy p_strategy, core::BufferLayoutType p_layout) const {
    // Exact mapping between structural memory layouts and safe execution strategies.
    // Prevents cache-thrashing tasks from attempting to process fragmented layouts.
    using namespace core;
    BufferLayoutType strategy_layout_mask = BufferLayoutType::NONE;
    
    switch (p_strategy) {
        case MemoryStrategy::FlatStrategy:      strategy_layout_mask = BufferLayoutType::FLAT; break;
        case MemoryStrategy::SoAStrategy:       strategy_layout_mask = BufferLayoutType::SOA; break;
        case MemoryStrategy::AoSStrategy:       strategy_layout_mask = BufferLayoutType::AOS; break;
        case MemoryStrategy::Spatial2DStrategy: 
        case MemoryStrategy::Spatial3DStrategy: 
        case MemoryStrategy::Spatial4DStrategy: strategy_layout_mask = BufferLayoutType::ANY_SPATIAL; break;
        case MemoryStrategy::TiledSoAStrategy:  strategy_layout_mask = BufferLayoutType::TILED_SOA; break;
        case MemoryStrategy::RingStrategy:      strategy_layout_mask = BufferLayoutType::RING; break;
        case MemoryStrategy::PagedStrategy:     strategy_layout_mask = BufferLayoutType::PAGED; break;
        default: break;
    }

    return core::has_layout(p_layout, strategy_layout_mask);
}

String TaskGraphEdit::_resolve_task_menu_path(
    const String& p_axis_name,
    const String& p_raw_category,
    const String& p_domain_prefix,
    bool& r_is_direct_axis_item
) const {
    String raw_cat = p_raw_category.strip_edges();
    bool is_promoted = raw_cat.begins_with("@");
    String clean_cat = is_promoted ? raw_cat.substr(1).strip_edges() : raw_cat;
    bool is_general = clean_cat.is_empty() || clean_cat == "General";

    r_is_direct_axis_item = false;

    // 1. Core Domain Tasks (p_domain_prefix is empty)
    if (p_domain_prefix.is_empty()) {
        if (menu_hierarchy_mode == MENU_HYBRID_AUTO && is_promoted && !is_general) {
            // @Promoted categories mount Domain-First at root: e.g. "Physics/Transform"
            return clean_cat + "/" + p_axis_name;
        }

        if (menu_hierarchy_mode == MENU_DOMAIN_FIRST) {
            if (is_general) {
                r_is_direct_axis_item = true;
                return p_axis_name;
            }
            // Domain-First: e.g. "Image/Transform" or "Model/Transform"
            return clean_cat + "/" + p_axis_name;
        }

        // MENU_AXIS_FIRST or non-promoted MENU_HYBRID_AUTO:
        if (is_general) {
            r_is_direct_axis_item = true;
            return p_axis_name; // Placed directly inside "Transform", "Metadata", "Query", or "Utility"
        }
        return p_axis_name + godot::String("/") + clean_cat; // e.g. "Transform/Image", "Transform/Model"
    }

    // 2. External Plugin Domain Packs (e.g., IdeamGames -> p_domain_prefix = "Games")
    if (menu_hierarchy_mode == MENU_AXIS_FIRST) {
        if (is_general) {
            return p_axis_name + godot::String("/") + p_domain_prefix; // e.g. "Transform/Games"
        }
        return p_axis_name + godot::String("/") + p_domain_prefix + "/" + clean_cat; // e.g. "Transform/Games/Combat"
    }

    // MENU_HYBRID_AUTO or MENU_DOMAIN_FIRST mounts plugin packs under their domain root:
    if (is_general) {
        r_is_direct_axis_item = true;
        return p_domain_prefix + godot::String("/") + p_axis_name; // e.g. "Games/Transform"
    }
    return p_domain_prefix + godot::String("/") + p_axis_name + godot::String("/") + clean_cat; // e.g. "Games/Transform/Combat"
}

TypedArray<String> TaskGraphEdit::_get_new_node_types() const {
    auto* mutable_this = const_cast<TaskGraphEdit*>(this);
    mutable_this->spawn_options_cache.clear();

    if (!mutable_this->context_popup) return TypedArray<String>();

    // Clean up any stale sub-menus from the previous popup invocation.
    // Detach immediately via remove_child() before queue_free() so newly created PopupMenus
    // in the same frame do not collide with pending-deletion node names.
    for (int i = mutable_this->context_popup->get_child_count() - 1; i >= 0; --i) {
        Node* child = mutable_this->context_popup->get_child(i);
        if (Object::cast_to<PopupMenu>(child)) {
            mutable_this->context_popup->remove_child(child);
            child->queue_free();
        }
    }

    Ref<StyleBox> panel_style = mutable_this->get_theme_stylebox("popup_menu_panel", "PopupMenu");
    Ref<StyleBox> hover_style = mutable_this->get_theme_stylebox("popup_menu_hover", "PopupMenu");

    // Recursive path resolver: splits slash-delimited paths (e.g. "Transform/Model/Mesh")
    // and builds or reuses nested PopupMenu chains on demand.
    auto get_or_create_submenu_path = [&](PopupMenu* p_root, const String& p_path) -> PopupMenu* {
        if (!p_root || p_path.is_empty()) return p_root;

        PackedStringArray segments = p_path.split("/", false);
        PopupMenu* current_menu = p_root;

        for (int s = 0; s < segments.size(); ++s) {
            String segment = segments[s].strip_edges();
            if (segment.is_empty()) continue;

            PopupMenu* existing_sub = nullptr;
            for (int c = 0; c < current_menu->get_child_count(); ++c) {
                PopupMenu* candidate = Object::cast_to<PopupMenu>(current_menu->get_child(c));
                if (candidate && candidate->get_name() == StringName(segment)) {
                    existing_sub = candidate;
                    break;
                }
            }

            if (!existing_sub) {
                existing_sub = memnew(PopupMenu);
                existing_sub->set_name(segment);
                current_menu->add_child(existing_sub);
                current_menu->add_submenu_node_item(segment, existing_sub);

                if (panel_style.is_valid()) existing_sub->add_theme_stylebox_override("panel", panel_style);
                if (hover_style.is_valid()) existing_sub->add_theme_stylebox_override("hover", hover_style);

                existing_sub->connect("id_pressed", Callable(mutable_this, "_popup_select"));
            }

            current_menu = existing_sub;
        }

        return current_menu;
    };

    // Context-Aware Selection: Dynamically parse structural traits if dragging out of a port
    mutable_this->active_filter_mask = 0; // Default to zero if right-clicking generic space
    if (drag_source_port != -1 && drag_source_node != String().is_empty()) {
        Node* source_node_base = get_node_or_null(NodePath(drag_source_node));
        TaskGraphNode* source_task_node = Object::cast_to<TaskGraphNode>(source_node_base);
        
        if (source_task_node) {
            Ref<TaskResource> task_res = source_task_node->get_task_node_resource();
            if (task_res.is_valid()) {
                Ref<MemoryGrantResource> grant_res = task_res->get_memory_grant();
                if (grant_res.is_valid()) {
                    TypedArray<GrantPartResource> parts = grant_res->get_configured_parts();
                    if (drag_source_port >= 0 && drag_source_port < parts.size()) {
                        Ref<GrantPartResource> target_part = parts[drag_source_port];
                        if (target_part.is_valid()) {
                            // Extract the layout/buffer type mapping from the serialized layout intent
                            mutable_this->active_filter_mask = static_cast<uint32_t>(target_part->get_buffer_type());
                        }
                    }
                }
            }
        }
        godot::UtilityFunctions::print("Active Filter Mask set to: ", mutable_this->active_filter_mask);
    }
    core::BufferLayoutType layout_requirement = static_cast<core::BufferLayoutType>(mutable_this->active_filter_mask);
    bool check_layout = layout_requirement != core::BufferLayoutType::NONE && layout_requirement != core::BufferLayoutType::ANY;

    int current_global_id = MENU_SPAWN_NODE_START; // Start after reserved IDs for context menu options

    struct PendingMenuEntry {
        String menu_path;
        String display_name;
        uint32_t logic_id;
        bool is_direct_axis_item;
    };

    auto process_matrix = [&](
        const Dictionary& p_matrix,
        TaskCategory p_category,
        const String& p_axis_name,
        const StringName& p_plugin_domain,
        const String& p_domain_prefix
    ) {
        Array keys = p_matrix.keys();
        godot::UtilityFunctions::print(
            "Category Check: ", p_category,
            " | Domain: ", String(p_plugin_domain),
            " | Total Tasks found in Registry Matrix: ", keys.size()
        );

        std::vector<PendingMenuEntry> valid_entries;
        valid_entries.reserve(keys.size());

        for (int i = 0; i < keys.size(); ++i) {
            String logic_str = keys[i];
            uint32_t logic_id = (p_category == CATEGORY_MANUAL) ? UINT32_MAX : static_cast<uint32_t>(logic_str.to_int());
            Dictionary logic_def = p_matrix[keys[i]];

            bool has_combos = logic_def.has("valid_combinations");

            if (check_layout && has_combos) {
                PackedInt64Array valid_hashes = logic_def["valid_combinations"];
                bool has_compatible_strategy = false;

                for (int h = 0; h < valid_hashes.size(); ++h) {
                    uint64_t hash = valid_hashes[h];
                    uint32_t s_index = 0;

                    if (p_category == CATEGORY_METADATA) {
                        using namespace core;
                        // Metadata Matrix uses: (View * S_COUNT * T_COUNT) + (Strategy * T_COUNT) + Type
                        uint64_t sub_stride = hash % (MetadataTaskRegistry::S_COUNT * MetadataTaskRegistry::T_COUNT);
                        s_index = static_cast<uint32_t>(sub_stride / MetadataTaskRegistry::T_COUNT);
                    } 
                    else if (p_category == CATEGORY_TRANSFORM) {
                        using namespace core;
                        uint64_t sub_stride = hash % (TransformTaskRegistry::S_COUNT * TransformTaskRegistry::T_COUNT);
                        s_index = static_cast<uint32_t>(sub_stride / TransformTaskRegistry::T_COUNT);
                    } 
                    else if (p_category == CATEGORY_QUERY) {
                        using namespace core;
                        // Layout: (Op * V_COUNT * S_COUNT * T_COUNT) + (View * S_COUNT * T_COUNT) + (Strategy * T_COUNT) + Type
                        uint64_t op_sub_stride = QueryTaskRegistry::V_COUNT * QueryTaskRegistry::S_COUNT * QueryTaskRegistry::T_COUNT;
                        uint64_t hash_without_op = hash % op_sub_stride;

                        uint64_t view_sub_stride = QueryTaskRegistry::S_COUNT * QueryTaskRegistry::T_COUNT;
                        uint64_t hash_without_view = hash_without_op % view_sub_stride;

                        s_index = static_cast<uint32_t>(hash_without_view / QueryTaskRegistry::T_COUNT);
                    }

                    if (_strategy_supports_layout(static_cast<core::MemoryStrategy>(s_index), layout_requirement)) {
                        has_compatible_strategy = true;
                        break;
                    }
                }

                if (!has_compatible_strategy) continue;
            }

            String display_name = (p_category == CATEGORY_MANUAL)
                ? logic_str
                : String(logic_def.get("name", logic_str));

            String raw_category = (p_category == CATEGORY_MANUAL)
                ? String(logic_def.get("category", "General"))
                : String(logic_def.get("category", "General"));

            bool is_direct_axis_item = false;
            String menu_path = _resolve_task_menu_path(
                p_axis_name,
                raw_category,
                p_domain_prefix,
                is_direct_axis_item
            );

            valid_entries.push_back({
                menu_path,
                display_name,
                logic_id,
                is_direct_axis_item
            });
        }

        auto register_entry = [&](const PendingMenuEntry& p_entry, bool p_insert_separator_if_has_submenus) {
            PopupMenu* target_menu = get_or_create_submenu_path(mutable_this->context_popup, p_entry.menu_path);
            if (!target_menu) return;

            if (p_insert_separator_if_has_submenus && target_menu->get_child_count() > 0) {
                target_menu->add_separator();
            }

            SpawnDescriptor desc;
            desc.plugin_domain = p_plugin_domain;
            desc.category = p_category;
            desc.logic_id = p_entry.logic_id;
            desc.task_name = StringName(p_entry.display_name);
            mutable_this->spawn_options_cache.push_back(desc);

            if (!_menu_has_option(target_menu, p_entry.display_name)) {
                target_menu->add_item(p_entry.display_name, current_global_id);
            }
            current_global_id++;
        };

        // Pass 1: Populate categorized submenus first (e.g., Transform > Image, Transform > Model)
        // so submenus sit cleanly at the top of their parent axis menu.
        for (const PendingMenuEntry& entry : valid_entries) {
            if (!entry.is_direct_axis_item) {
                register_entry(entry, false);
            }
        }

        // Pass 2: Populate direct ("General") items below any category submenus, separated by a divider line.
        bool separator_checked = false;
        for (const PendingMenuEntry& entry : valid_entries) {
            if (entry.is_direct_axis_item) {
                register_entry(entry, !separator_checked);
                separator_checked = true;
            }
        }
    };

    // --- 1. Core Domain Matrices ---
    const StringName core_domain("Core");
    process_matrix(core::IdeamTaskRegistry::get_ui_transform_matrix(), CATEGORY_TRANSFORM, "Transform", core_domain, "");
    process_matrix(core::IdeamTaskRegistry::get_ui_metadata_matrix(), CATEGORY_METADATA, "Metadata", core_domain, "");
    process_matrix(core::IdeamTaskRegistry::get_ui_query_matrix(), CATEGORY_QUERY, "Query", core_domain, "");
    process_matrix(core::IdeamTaskRegistry::get_ui_utility_matrix(), CATEGORY_MANUAL, "Utility", core_domain, "");

    // --- 2. Active Companion Plugin Domain Matrices ---
    if (core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton()) {
        PackedStringArray active_domains = registry->get_active_domains();
        for (int d = 0; d < active_domains.size(); ++d) {
            StringName domain_name(active_domains[d]);
            if (domain_name == core_domain) continue;

            String domain_prefix = registry->get_domain_ui_prefix(domain_name);
            if (domain_prefix.is_empty()) {
                domain_prefix = String(domain_name);
            }
            
            process_matrix(registry->get_domain_transform_matrix(domain_name), CATEGORY_TRANSFORM, "Transform", domain_name, domain_prefix);
            process_matrix(registry->get_domain_metadata_matrix(domain_name), CATEGORY_METADATA, "Metadata", domain_name, domain_prefix);
            process_matrix(registry->get_domain_query_matrix(domain_name), CATEGORY_QUERY, "Query", domain_name, domain_prefix);
        }
    }
    
    return TypedArray<String>();
}

bool TaskGraphEdit::_menu_has_option(PopupMenu *p_menu, const String &p_option) const {
    int32_t count = p_menu->get_item_count();
    
    for (int32_t i = 0; i < count; ++i) {
        // Compare the existing label with your target string
        if (p_menu->get_item_text(i) == p_option) {
            return true;
        }
    }
    
    return false;
}

void TaskGraphEdit::_spawn_node_by_type(int p_type_id) {
    if (current_blueprint.is_null()) return;
    if (p_type_id < 0 || p_type_id >= static_cast<int>(spawn_options_cache.size())) return;

    const SpawnDescriptor& desc = spawn_options_cache[p_type_id];
    StringName unique_name = String("TaskNode_") + String::num_int64(UtilityFunctions::randi());

    // Declare the base reference, but DO NOT instantiate it yet.
    Ref<TaskResource> new_res;

    // Branch to instantiate the highly specialized, memory-packed resource.
    switch (desc.category) {
        case CATEGORY_TRANSFORM: {
            Ref<TransformTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_NATIVE_CPU);
            res->set_logic_id(desc.logic_id); 
            new_res = res;
            break;
        }
        case CATEGORY_METADATA: {
            Ref<MetadataTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_NATIVE_CPU);
            res->set_logic_id(desc.logic_id);
            new_res = res;
            break;
        }
        case CATEGORY_QUERY: {
            Ref<QueryTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_QUERY_CULLER);
            res->set_logic_id(desc.logic_id);
            new_res = res;
            break;
        }
        case CATEGORY_MANUAL: {
            core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton();
            auto* factories = registry->get_ui_factories();
        
            if (factories && factories->has(desc.task_name)) {
                // Deterministic, O(1) instantiation. No casting required.
                new_res = (*factories)[desc.task_name].resource_factory();
                new_res->set_task_type(TASK_NATIVE_CPU);
            } else {
                godot::UtilityFunctions::printerr("TaskGraphEdit: Manual task factory not found for ", desc.task_name);
                return;
            }
            break;
        }
        default:
            return; // Safety bailout
    }

    // Apply the configured properties to our safely typed wrapper
    new_res->set_node_name(unique_name);
    new_res->set_task_name(desc.task_name);
    new_res->set_base_logic_id(desc.logic_id);
    new_res->set_plugin_domain(desc.plugin_domain);

    // Apply scroll offset compensation
    Vector2 spawn_pos = popup_position;
    if (get_zoom() > 0.0f) {
        spawn_pos = (spawn_pos + get_scroll_offset()) / get_zoom();
    }
    new_res->set_position_offset(spawn_pos);

    // Retrieve the matching registry matrix to snatch the cached UI labels
    godot::Dictionary matrix;
    if (desc.category == CATEGORY_METADATA) matrix = core::IdeamTaskRegistry::get_ui_metadata_matrix();
    else if (desc.category == CATEGORY_TRANSFORM) matrix = core::IdeamTaskRegistry::get_ui_transform_matrix();
    else if (desc.category == CATEGORY_QUERY) matrix = core::IdeamTaskRegistry::get_ui_query_matrix();
    // Phase 3 Hook: When get_ui_*_matrix(desc.plugin_domain) is added in Phase 3, pass desc.plugin_domain above.

    godot::String logic_str = godot::String::num_int64(desc.logic_id);
    if (matrix.has(logic_str)) {
        godot::Dictionary logic_def = matrix[logic_str];
        if (logic_def.has("variant_labels")) {
            godot::PackedStringArray labels = logic_def["variant_labels"];
            new_res->set_logic_variant_labels(labels);
        }
    }

    if (!drag_source_node.is_empty()) {
        Node* source_node_base = get_node_or_null(NodePath(drag_source_node));
        TaskGraphNode* source_task_node = Object::cast_to<TaskGraphNode>(source_node_base);
        
        if (source_task_node) {
            Ref<TaskResource> task_res = source_task_node->get_task_node_resource();
            if (task_res.is_valid()) {
                Ref<MemoryGrantResource> grant_res = task_res->get_memory_grant();
                if (grant_res.is_valid()) {
                    new_res->set_memory_grant(grant_res); // Propagate the same grant to the new node
                }
            }
        }

        godot::Dictionary edge;
        edge["from"] = drag_source_node;
        edge["from_port"] = drag_source_port;
        edge["to"] = unique_name;
        // Default to the first input port on the new node
        edge["to_port"] = 0; 
        edge["access_mode"] = static_cast<int>(core::BufferAccessMode::READ);
        
        current_blueprint->action_add_edge(edge);
        
        // Reset tracking markers
        drag_source_node = godot::StringName();
        drag_source_port = -1;
        
        // Sync Visuals
        if (current_blueprint.is_valid()) {
            // Because TaskGraphEdit inherits from MemoryGraphEdit, it has access to these
            _refresh_edge_cache(); 
            queue_redraw();
        }
    }
    
    current_blueprint->action_add_node(new_res);
}

IdeamGraphNode* TaskGraphEdit::_create_graph_node(const godot::Ref<IdeamGraphNodeResource>& p_node_res) {
    Ref<TaskResource> task_res = p_node_res;
    if (task_res.is_null()) {
        return nullptr; // Hard bailout: Keeps type constraints rigid
    }

    IdeamGraphNode* new_node = nullptr;
    StringName res_class = task_res->get_class();

    // 1. Direct Archetype Matching (Hot Path for primary nodes)
    // Using Object::cast_to avoids string hashing overhead where possible.
    if (Object::cast_to<TransformTaskResource>(task_res.ptr())) {
        new_node = memnew(TransformTaskGraphNode);
    } 
    else if (Object::cast_to<QueryTaskResource>(task_res.ptr())) {
        new_node = memnew(QueryTaskGraphNode);
    } 
    else if (Object::cast_to<MetadataTaskResource>(task_res.ptr())) {
        new_node = memnew(MetadataTaskGraphNode);
    } 
    // 2. Dynamic Fallback for Utility / Manual Tasks
    // If it's not a core struct, we query the utility matrix to find its bound UI class.
    else {

        godot::StringName task_name = task_res->get_task_name();
        core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton();
        auto* factories = registry->get_ui_factories();
        if (factories && factories->has(task_name)) {
            new_node = (*factories)[task_name].node_factory();
        }
    }

    if (new_node) {
        // Wire up the dynamic UI population for DOD memory buffers.
        // Because TaskGraphEdit inherits from MemoryGraphEdit, the Callable will
        // natively resolve to the base class's bound method.
        
        new_node->connect("memory_grant_requested", godot::Callable(this, "_on_node_memory_grant_requested"));
        new_node->connect("active_grants_requested", godot::Callable(this, "_on_active_grants_requested"));
        new_node->connect("buffer_names_requested", godot::Callable(this, "_on_buffer_names_requested"));
        new_node->connect("connections_requested", godot::Callable(this, "_on_node_connections_requested"));
        new_node->connect("duplicate_requested", godot::Callable(this, "_on_node_duplicate_requested"));
        new_node->connect("delete_requested", godot::Callable(this, "_on_node_delete_requested"));
    } else {
        godot::UtilityFunctions::printerr("TaskGraphEdit: Failed to resolve UI GraphNode class for data payload: ", res_class);
    }

    // Return the newly instantiated node so the IdeamGraphEdit sync loop can position/add it to the tree
    return new_node;
}

void TaskGraphEdit::_on_node_duplicate_requested(const godot::StringName& p_node_name) {
    if (current_blueprint.is_null()) return;

    // 1. Resolve the requesting node
    godot::Node* target_node = get_node_or_null(godot::NodePath(p_node_name));
    TaskGraphNode* task_node = godot::Object::cast_to<TaskGraphNode>(target_node);
    if (!task_node) return;

    // 2. Extract the data payload
    godot::Ref<TaskResource> source_res = task_node->get_task_node_resource();
    if (source_res.is_null()) return;

    // 3. Deep duplicate the underlying Resource
    // Using true ensures nested properties/dictionaries are copied natively
    godot::Ref<TaskResource> duplicated_res = source_res->duplicate(true);

    // 4. Assign a new unique topological identity
    godot::StringName unique_name = godot::String("TaskNode_") + godot::String::num_int64(godot::UtilityFunctions::randi());
    duplicated_res->set_node_name(unique_name);

    // 5. Apply a visual offset so it doesn't perfectly occlude the original node
    godot::Vector2 new_pos = duplicated_res->get_position_offset() + godot::Vector2(30.0f, 30.0f);
    duplicated_res->set_position_offset(new_pos);

    // 6. Push to the data blueprint (which triggers your UI sync downstream)
    current_blueprint->action_add_node(duplicated_res);
}

void TaskGraphEdit::_on_node_delete_requested(const godot::StringName& p_node_name) {
    if (current_blueprint.is_null()) return;

    current_blueprint->action_remove_node(p_node_name);
}

} // namespace ideam::godot_ext
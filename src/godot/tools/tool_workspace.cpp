#include "tool_workspace.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

using namespace godot;

namespace ideam::godot_ext {

void ToolWorkspace::_bind_methods() {
    ADD_SIGNAL(MethodInfo(
        "asset_opened",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "asset_closed",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "active_asset_changed",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "runtime_save_requested",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "explicit_rebuild_triggered",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "help_requested",
        PropertyInfo(Variant::STRING_NAME, "tool_id"),
        PropertyInfo(Variant::STRING, "ui_selector")
    ));

    ClassDB::bind_method(D_METHOD("set_workspace_id", "id"), &ToolWorkspace::set_workspace_id);
    ClassDB::bind_method(D_METHOD("get_workspace_id"), &ToolWorkspace::get_workspace_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "workspace_id"), "set_workspace_id", "get_workspace_id");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &ToolWorkspace::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &ToolWorkspace::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(
        D_METHOD("request_workspace_help", "ui_selector"),
        &ToolWorkspace::request_workspace_help,
        DEFVAL(String())
    );

    ClassDB::bind_method(D_METHOD("configure_theme_registry", "registry_path"), &ToolWorkspace::configure_theme_registry);
    ClassDB::bind_method(D_METHOD("ensure_sidebar_tab", "tab_name"), &ToolWorkspace::ensure_sidebar_tab);

    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &ToolWorkspace::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &ToolWorkspace::get_tool_context);

    ClassDB::bind_method(
        D_METHOD("set_chrome_visibility", "show_command_bar", "show_sidebar", "show_inspector", "show_ribbon"),
        &ToolWorkspace::set_chrome_visibility,
        DEFVAL(true),
        DEFVAL(true),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("set_dirty_banner", "visible", "message", "action_label"),
        &ToolWorkspace::set_dirty_banner,
        DEFVAL(String()),
        DEFVAL(String("Rebuild"))
    );

    ClassDB::bind_method(
        D_METHOD("open_asset", "asset", "tab_override"),
        &ToolWorkspace::open_asset,
        DEFVAL(String())
    );
    ClassDB::bind_method(D_METHOD("close_asset", "asset"), &ToolWorkspace::close_asset);
    ClassDB::bind_method(D_METHOD("close_all_assets"), &ToolWorkspace::close_all_assets);
    ClassDB::bind_method(D_METHOD("select_asset", "asset"), &ToolWorkspace::select_asset);
    ClassDB::bind_method(D_METHOD("get_active_asset"), &ToolWorkspace::get_active_asset);

    ClassDB::bind_method(D_METHOD("save_asset", "asset"), &ToolWorkspace::save_asset);
    ClassDB::bind_method(D_METHOD("save_active_asset"), &ToolWorkspace::save_active_asset);
    ClassDB::bind_method(D_METHOD("save_all_assets"), &ToolWorkspace::save_all_assets);

    // Internal Signal Handlers
    ClassDB::bind_method(D_METHOD("_on_theme_applied", "theme", "index"), &ToolWorkspace::_on_theme_applied);
    ClassDB::bind_method(D_METHOD("_on_context_theme_changed", "theme", "index"), &ToolWorkspace::_on_context_theme_changed);
    ClassDB::bind_method(D_METHOD("_on_update_policy_selected", "index"), &ToolWorkspace::_on_update_policy_selected);
    ClassDB::bind_method(D_METHOD("_on_context_update_policy_changed", "policy"), &ToolWorkspace::_on_context_update_policy_changed);
    ClassDB::bind_method(D_METHOD("_on_context_asset_dirty_changed", "asset", "dirty"), &ToolWorkspace::_on_context_asset_dirty_changed);
    ClassDB::bind_method(D_METHOD("_on_save_pressed"), &ToolWorkspace::_on_save_pressed);
    ClassDB::bind_method(D_METHOD("_on_save_all_pressed"), &ToolWorkspace::_on_save_all_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &ToolWorkspace::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_sidebar_item_selected", "index", "tab_name"), &ToolWorkspace::_on_sidebar_item_selected);
    ClassDB::bind_method(D_METHOD("_on_sidebar_gui_input", "event", "tab_name"), &ToolWorkspace::_on_sidebar_gui_input);
    ClassDB::bind_method(D_METHOD("_on_sidebar_context_action", "action_id"), &ToolWorkspace::_on_sidebar_context_action);
    ClassDB::bind_method(D_METHOD("_on_domain_commit_completed", "action_name"), &ToolWorkspace::_on_domain_commit_completed);
    ClassDB::bind_method(D_METHOD("_on_dirty_banner_action_pressed"), &ToolWorkspace::_on_dirty_banner_action_pressed);
}

ToolWorkspace::ToolWorkspace() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_process_input(true);

    tool_context_.instantiate();

    // --- Context Menu for Sidebar Asset Lists ---
    sidebar_context_menu_ = memnew(PopupMenu);
    add_child(sidebar_context_menu_);
    sidebar_context_menu_->add_item("Save Asset", CTX_SAVE_ASSET);
    sidebar_context_menu_->add_item("Close Asset", CTX_CLOSE_ASSET);
    sidebar_context_menu_->add_separator();
    sidebar_context_menu_->add_item("Copy Resource Path", CTX_COPY_PATH);
    sidebar_context_menu_->add_item("Copy Resource UID", CTX_COPY_UID);
    sidebar_context_menu_->connect("id_pressed", Callable(this, "_on_sidebar_context_action"));

    // --- Row 1: Command Bar ---
    command_bar_ = memnew(HBoxContainer);
    command_bar_->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(command_bar_);

    theme_selector_ui_ = memnew(ThemeSelector);
    command_bar_->add_child(theme_selector_ui_);
    theme_selector_ui_->connect("theme_applied", Callable(this, "_on_theme_applied"));

    save_btn_ = memnew(Button);
    save_btn_->set_text("Save");
    save_btn_->set_tooltip_text("Save the active asset (Ctrl+S)");
    command_bar_->add_child(save_btn_);
    save_btn_->connect("pressed", Callable(this, "_on_save_pressed"));

    save_all_btn_ = memnew(Button);
    save_all_btn_->set_text("Save All");
    save_all_btn_->set_tooltip_text("Save all open assets (Ctrl+Shift+S)");
    command_bar_->add_child(save_all_btn_);
    save_all_btn_->connect("pressed", Callable(this, "_on_save_all_pressed"));

    update_policy_btn_ = memnew(OptionButton);
    update_policy_btn_->add_item("Updates: Live", static_cast<int>(UPDATE_IMMEDIATE));
    update_policy_btn_->add_item("Updates: On Release", static_cast<int>(UPDATE_ON_COMMIT));
    update_policy_btn_->add_item("Updates: Manual", static_cast<int>(UPDATE_EXPLICIT_TRIGGER));
    update_policy_btn_->select(static_cast<int>(UPDATE_ON_COMMIT));
    update_policy_btn_->set_tooltip_text(
        "Controls when DomainTool edits commit to the underlying Resource and trigger downstream rebuilds."
    );
    command_bar_->add_child(update_policy_btn_);
    update_policy_btn_->connect("item_selected", Callable(this, "_on_update_policy_selected"));

    custom_actions_box_ = memnew(HBoxContainer);
    custom_actions_box_->set_h_size_flags(SIZE_EXPAND_FILL);
    command_bar_->add_child(custom_actions_box_);

    help_btn_ = memnew(Button);
    help_btn_->set_text("?");
    help_btn_->set_tooltip_text("Open guided help & tutorials for this workspace (F1)");
    command_bar_->add_child(help_btn_);
    help_btn_->connect("pressed", Callable(this, "_on_help_pressed"));

    // --- Row 2: Optional Ribbon / Dirty-Pipeline Banner ---
    ribbon_slot_ = memnew(PanelContainer);
    ribbon_slot_->set_h_size_flags(SIZE_EXPAND_FILL);
    ribbon_slot_->set_visible(false);
    add_child(ribbon_slot_);

    dirty_banner_box_ = memnew(HBoxContainer);
    dirty_banner_box_->set_h_size_flags(SIZE_EXPAND_FILL);
    ribbon_slot_->add_child(dirty_banner_box_);

    dirty_banner_label_ = memnew(Label);
    dirty_banner_label_->set_h_size_flags(SIZE_EXPAND_FILL);
    dirty_banner_label_->set_text("Changes pending — downstream pipeline needs rebuild.");
    dirty_banner_box_->add_child(dirty_banner_label_);

    dirty_banner_action_btn_ = memnew(Button);
    dirty_banner_action_btn_->set_text("Rebuild");
    dirty_banner_box_->add_child(dirty_banner_action_btn_);
    dirty_banner_action_btn_->connect("pressed", Callable(this, "_on_dirty_banner_action_pressed"));

    // --- Row 3: 3-Pane Workspace (Left Sidebar | Center DomainTool Host | Right Inspector) ---
    workspace_split_ = memnew(HSplitContainer);
    workspace_split_->set_h_size_flags(SIZE_EXPAND_FILL);
    workspace_split_->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(workspace_split_);

    sidebar_tabs_ = memnew(TabContainer);
    sidebar_tabs_->set_custom_minimum_size(Vector2(210.0f, 0.0f));
    sidebar_tabs_->set_v_size_flags(SIZE_EXPAND_FILL);
    workspace_split_->add_child(sidebar_tabs_);

    workspace_inner_split_ = memnew(HSplitContainer);
    workspace_inner_split_->set_h_size_flags(SIZE_EXPAND_FILL);
    workspace_inner_split_->set_v_size_flags(SIZE_EXPAND_FILL);
    workspace_split_->add_child(workspace_inner_split_);

    center_scroll_ = memnew(ScrollContainer);
    center_scroll_->set_h_size_flags(SIZE_EXPAND_FILL);
    center_scroll_->set_v_size_flags(SIZE_EXPAND_FILL);
    center_scroll_->set_custom_minimum_size(Vector2(320.0f, 0.0f));
    workspace_inner_split_->add_child(center_scroll_);

    center_host_ = memnew(PanelContainer);
    center_host_->set_h_size_flags(SIZE_EXPAND_FILL);
    center_host_->set_v_size_flags(SIZE_EXPAND_FILL);
    center_scroll_->add_child(center_host_);

    empty_state_label_ = memnew(Label);
    empty_state_label_->set_text("No asset selected. Open an asset from the Inspector or Sidebar.");
    empty_state_label_->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    empty_state_label_->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    empty_state_label_->set_h_size_flags(SIZE_EXPAND_FILL);
    empty_state_label_->set_v_size_flags(SIZE_EXPAND_FILL);
    center_host_->add_child(empty_state_label_);

    inspector_panel_ = memnew(PanelContainer);
    inspector_panel_->set_custom_minimum_size(Vector2(260.0f, 0.0f));
    inspector_panel_->set_v_size_flags(SIZE_EXPAND_FILL);
    workspace_inner_split_->add_child(inspector_panel_);

    inspector_vbox_ = memnew(VBoxContainer);
    inspector_vbox_->set_h_size_flags(SIZE_EXPAND_FILL);
    inspector_vbox_->set_v_size_flags(SIZE_EXPAND_FILL);
    inspector_panel_->add_child(inspector_vbox_);

    inspector_title_ = memnew(Label);
    inspector_title_->set_text("Inspector");
    inspector_vbox_->add_child(inspector_title_);

    runtime_inspector_ = memnew(RuntimeInspector);
    runtime_inspector_->set_h_size_flags(SIZE_EXPAND_FILL);
    runtime_inspector_->set_v_size_flags(SIZE_EXPAND_FILL);
    inspector_vbox_->add_child(runtime_inspector_);

    _stamp_ui_selectors();
    _connect_context_signals();
}

ToolWorkspace::~ToolWorkspace() {
    _disconnect_context_signals();
}

void ToolWorkspace::_stamp_ui_selectors() {
    const String prefix = String(workspace_id_) + String("/");
    set_meta("ideam_ui_selector", String(workspace_id_));
    if (command_bar_) command_bar_->set_meta("ideam_ui_selector", prefix + String("command_bar"));
    if (save_btn_) save_btn_->set_meta("ideam_ui_selector", prefix + String("command_bar/save_btn"));
    if (save_all_btn_) save_all_btn_->set_meta("ideam_ui_selector", prefix + String("command_bar/save_all_btn"));
    if (update_policy_btn_) update_policy_btn_->set_meta("ideam_ui_selector", prefix + String("command_bar/update_policy_btn"));
    if (help_btn_) help_btn_->set_meta("ideam_ui_selector", prefix + String("command_bar/help_btn"));
    if (sidebar_tabs_) sidebar_tabs_->set_meta("ideam_ui_selector", prefix + String("sidebar"));
    if (center_host_) center_host_->set_meta("ideam_ui_selector", prefix + String("center_host"));
    if (inspector_panel_) inspector_panel_->set_meta("ideam_ui_selector", prefix + String("inspector"));
}

void ToolWorkspace::_connect_context_signals() {
    if (tool_context_.is_null()) {
        return;
    }
    const Callable theme_cb(this, "_on_context_theme_changed");
    if (!tool_context_->is_connected("theme_changed", theme_cb)) {
        tool_context_->connect("theme_changed", theme_cb);
    }
    const Callable policy_cb(this, "_on_context_update_policy_changed");
    if (!tool_context_->is_connected("update_policy_changed", policy_cb)) {
        tool_context_->connect("update_policy_changed", policy_cb);
    }
    const Callable dirty_cb(this, "_on_context_asset_dirty_changed");
    if (!tool_context_->is_connected("asset_dirty_changed", dirty_cb)) {
        tool_context_->connect("asset_dirty_changed", dirty_cb);
    }
}

void ToolWorkspace::_disconnect_context_signals() {
    if (tool_context_.is_null()) {
        return;
    }
    const Callable theme_cb(this, "_on_context_theme_changed");
    if (tool_context_->is_connected("theme_changed", theme_cb)) {
        tool_context_->disconnect("theme_changed", theme_cb);
    }
    const Callable policy_cb(this, "_on_context_update_policy_changed");
    if (tool_context_->is_connected("update_policy_changed", policy_cb)) {
        tool_context_->disconnect("update_policy_changed", policy_cb);
    }
    const Callable dirty_cb(this, "_on_context_asset_dirty_changed");
    if (tool_context_->is_connected("asset_dirty_changed", dirty_cb)) {
        tool_context_->disconnect("asset_dirty_changed", dirty_cb);
    }
}

void ToolWorkspace::_input(const Ref<InputEvent> &p_event) {
    if (!is_visible_in_tree() || p_event.is_null()) {
        return;
    }
    if (Ref<InputEventKey> key = p_event; key.is_valid() && key->is_pressed() && !key->is_echo()) {
        if (key->get_keycode() == KEY_F1) {
            request_workspace_help();
            get_viewport()->set_input_as_handled();
            return;
        }
        if (key->is_command_or_control_pressed()) {
            if (key->get_keycode() == KEY_S) {
                if (key->is_shift_pressed()) {
                    save_all_assets();
                } else {
                    save_active_asset();
                }
                get_viewport()->set_input_as_handled();
            } else if (key->get_keycode() == KEY_W && active_asset_.is_valid()) {
                close_asset(active_asset_);
                get_viewport()->set_input_as_handled();
            }
        }
    }
}

// ============================================================================
// Identity, Configuration & Embeddability
// ============================================================================

void ToolWorkspace::set_workspace_id(const StringName &p_id) {
    workspace_id_ = p_id;
    _stamp_ui_selectors();
    const auto tabs = ToolRegistry::get_singleton().get_sidebar_tabs_for_workspace(workspace_id_);
    for (const auto &tab : tabs) {
        ensure_sidebar_tab(tab);
    }
}

bool ToolWorkspace::request_workspace_help(const String &p_ui_selector) {
    const StringName topic = help_topic_id_ == StringName() ? workspace_id_ : help_topic_id_;
    const String selector  = p_ui_selector.is_empty() ? String(workspace_id_) : p_ui_selector;

    Dictionary ctx;
    ctx["workspace_id"] = String(workspace_id_);
    if (active_asset_.is_valid()) {
        ctx["active_asset_class"] = active_asset_->get_class();
        ctx["active_asset_path"]  = active_asset_->get_path();
    }

    emit_signal("help_requested", topic, selector);
    if (tool_context_.is_valid()) {
        return tool_context_->request_help(topic, selector, ctx);
    }
    return ToolRegistry::get_singleton().dispatch_help_request(topic, selector, ctx);
}

void ToolWorkspace::configure_theme_registry(const String &p_registry_path) {
    theme_registry_path_ = p_registry_path;
    if (theme_selector_ui_ && !theme_registry_path_.is_empty()) {
        theme_selector_ui_->setup(theme_registry_path_);
        theme_selector_ui_->select_theme(0);
    }
}

void ToolWorkspace::ensure_sidebar_tab(const String &p_tab_name) {
    if (p_tab_name.is_empty()) {
        return;
    }
    (void)_get_or_create_sidebar_bucket(p_tab_name);
}

void ToolWorkspace::set_tool_context(const Ref<ToolContext> &p_context) {
    if (tool_context_ == p_context) {
        return;
    }
    _disconnect_context_signals();
    tool_context_ = p_context;
    if (tool_context_.is_null()) {
        tool_context_.instantiate();
    }
    _connect_context_signals();

    if (tool_context_->get_active_theme().is_valid()) {
        set_theme(tool_context_->get_active_theme());
    }
    if (update_policy_btn_) {
        update_policy_btn_->select(static_cast<int>(tool_context_->get_update_commit_policy()));
    }
    _rebuild_sidebar_lists();
}

void ToolWorkspace::set_chrome_visibility(
    bool p_show_command_bar,
    bool p_show_sidebar,
    bool p_show_inspector,
    bool p_show_ribbon
) {
    if (command_bar_) command_bar_->set_visible(p_show_command_bar);
    if (sidebar_tabs_) sidebar_tabs_->set_visible(p_show_sidebar);
    if (inspector_panel_) inspector_panel_->set_visible(p_show_inspector);
    if (ribbon_slot_) ribbon_slot_->set_visible(p_show_ribbon);
}

void ToolWorkspace::set_dirty_banner(
    bool p_visible,
    const String &p_message,
    const String &p_action_label
) {
    if (!ribbon_slot_) {
        return;
    }
    ribbon_slot_->set_visible(p_visible);
    if (dirty_banner_label_ && !p_message.is_empty()) {
        dirty_banner_label_->set_text(p_message);
    }
    if (dirty_banner_action_btn_ && !p_action_label.is_empty()) {
        dirty_banner_action_btn_->set_text(p_action_label);
    }
}

// ============================================================================
// Sidebar & DomainTool Control Orchestration
// ============================================================================

ToolWorkspace::SidebarTabBucket *ToolWorkspace::_get_or_create_sidebar_bucket(const String &p_tab_name) {
    for (auto &bucket : sidebar_buckets_) {
        if (bucket.tab_name == p_tab_name) {
            return &bucket;
        }
    }
    ItemList *list = memnew(ItemList);
    list->set_name(p_tab_name);
    list->set_h_size_flags(SIZE_EXPAND_FILL);
    list->set_v_size_flags(SIZE_EXPAND_FILL);
    list->set_focus_mode(FOCUS_ALL);
    list->set_meta("ideam_ui_selector", String(workspace_id_) + String("/sidebar/") + p_tab_name);
    sidebar_tabs_->add_child(list);

    list->connect("item_selected", Callable(this, "_on_sidebar_item_selected").bind(p_tab_name));
    list->connect("gui_input", Callable(this, "_on_sidebar_gui_input").bind(p_tab_name));

    sidebar_buckets_.push_back(SidebarTabBucket{p_tab_name, list});
    return &sidebar_buckets_.back();
}

String ToolWorkspace::_resolve_asset_display_name(const Ref<Resource> &p_asset) const {
    if (p_asset.is_null()) {
        return "Invalid Asset";
    }
    String label;
    if (!p_asset->get_name().is_empty()) {
        label = p_asset->get_name();
    } else if (!p_asset->get_path().is_empty()) {
        label = p_asset->get_path().get_file();
    } else {
        label = String(p_asset->get_class()) + String(" #") + String::num_int64(p_asset->get_instance_id() % 10000);
    }
    if (tool_context_.is_valid() && tool_context_->is_asset_dirty(p_asset)) {
        label += " *";
    }
    return label;
}

String ToolWorkspace::_resolve_sidebar_tab_for_asset(
    const Ref<Resource> &p_asset,
    const String &p_tab_override
) const {
    if (!p_tab_override.is_empty()) {
        return p_tab_override;
    }
    if (const auto *desc = ToolRegistry::get_singleton().find_descriptor_for_resource(workspace_id_, p_asset)) {
        if (!desc->sidebar_tab.is_empty()) {
            return desc->sidebar_tab;
        }
    }
    return "General";
}

Control *ToolWorkspace::_create_domain_control_for_asset(const Ref<Resource> &p_asset) {
    const auto *desc = ToolRegistry::get_singleton().find_descriptor_for_resource(workspace_id_, p_asset);
    if (desc && desc->create_control) {
        return desc->create_control(p_asset, tool_context_);
    }
    return nullptr;
}

void ToolWorkspace::_rebuild_sidebar_lists() {
    for (auto &bucket : sidebar_buckets_) {
        if (!bucket.item_list) {
            continue;
        }
        bucket.item_list->clear();
        int idx = 0;
        for (auto &entry : open_assets_) {
            if (entry.sidebar_tab != bucket.tab_name) {
                continue;
            }
            entry.display_name = _resolve_asset_display_name(entry.asset);
            bucket.item_list->add_item(entry.display_name);
            if (entry.asset == active_asset_) {
                bucket.item_list->select(idx);
            }
            ++idx;
        }
    }
}

void ToolWorkspace::_clear_center_view() {
    for (auto &entry : open_assets_) {
        if (entry.domain_control) {
            entry.domain_control->set_visible(false);
        }
    }
    if (empty_state_label_) {
        empty_state_label_->set_visible(true);
    }
}

void ToolWorkspace::_show_asset_in_center(OpenAssetEntry &p_entry) {
    for (auto &entry : open_assets_) {
        if (entry.domain_control) {
            entry.domain_control->set_visible(false);
        }
    }

    if (!p_entry.domain_control) {
        p_entry.domain_control = _create_domain_control_for_asset(p_entry.asset);
        if (p_entry.domain_control) {
            p_entry.domain_control->set_h_size_flags(SIZE_EXPAND_FILL);
            p_entry.domain_control->set_v_size_flags(SIZE_EXPAND_FILL);
            if (!p_entry.domain_control->has_meta("ideam_ui_selector")) {
                p_entry.domain_control->set_meta(
                    "ideam_ui_selector",
                    String(workspace_id_) + String("/center_host/") + String(p_entry.asset->get_class())
                );
            }
            center_host_->add_child(p_entry.domain_control);

            if (tool_context_.is_valid() && p_entry.domain_control->has_method("set_update_commit_policy")) {
                p_entry.domain_control->call(
                    "set_update_commit_policy",
                    static_cast<int>(tool_context_->get_update_commit_policy())
                );
            }
            if (p_entry.domain_control->has_signal("commit_completed")) {
                p_entry.domain_control->connect(
                    "commit_completed",
                    Callable(this, "_on_domain_commit_completed")
                );
            }
        }
    }

    if (p_entry.domain_control) {
        p_entry.domain_control->set_visible(true);
        if (empty_state_label_) {
            empty_state_label_->set_visible(false);
        }
    } else if (empty_state_label_) {
        empty_state_label_->set_text(
            String("No DomainTool editor registered for ") + String(p_entry.asset->get_class()) + String(".")
        );
        empty_state_label_->set_visible(true);
    }
}

void ToolWorkspace::_refresh_inspector_for_asset(const Ref<Resource> &p_asset) {
    if (!inspector_title_ || !runtime_inspector_) {
        return;
    }
    if (p_asset.is_null()) {
        inspector_title_->set_text("Inspector");
        runtime_inspector_->clear_inspector();
        return;
    }
    inspector_title_->set_text(String("Inspecting: ") + _resolve_asset_display_name(p_asset));
    runtime_inspector_->clear_inspector();
}

// ============================================================================
// Asset Lifecycle API
// ============================================================================

bool ToolWorkspace::open_asset(const Ref<Resource> &p_asset, const String &p_tab_override) {
    if (p_asset.is_null()) {
        return false;
    }

    for (auto &entry : open_assets_) {
        if (entry.asset == p_asset) {
            select_asset(p_asset);
            return true;
        }
    }

    const String tab_name = _resolve_sidebar_tab_for_asset(p_asset, p_tab_override);
    (void)_get_or_create_sidebar_bucket(tab_name);

    OpenAssetEntry entry;
    entry.asset        = p_asset;
    entry.sidebar_tab  = tab_name;
    entry.display_name = _resolve_asset_display_name(p_asset);
    open_assets_.push_back(entry);

    emit_signal("asset_opened", p_asset);
    select_asset(p_asset);
    return true;
}

void ToolWorkspace::close_asset(const Ref<Resource> &p_asset) {
    if (p_asset.is_null()) {
        return;
    }
    for (size_t i = 0; i < open_assets_.size(); ++i) {
        if (open_assets_[i].asset == p_asset) {
            if (open_assets_[i].domain_control) {
                if (open_assets_[i].domain_control->get_parent()) {
                    open_assets_[i].domain_control->get_parent()->remove_child(open_assets_[i].domain_control);
                }
                open_assets_[i].domain_control->queue_free();
            }
            open_assets_.erase(open_assets_.begin() + static_cast<ptrdiff_t>(i));
            emit_signal("asset_closed", p_asset);

            if (active_asset_ == p_asset) {
                active_asset_.unref();
                if (!open_assets_.empty()) {
                    select_asset(open_assets_.back().asset);
                } else {
                    _clear_center_view();
                    _refresh_inspector_for_asset(Ref<Resource>());
                    if (tool_context_.is_valid()) {
                        tool_context_->set_active_asset(Ref<Resource>());
                    }
                    emit_signal("active_asset_changed", Ref<Resource>());
                }
            }
            _rebuild_sidebar_lists();
            return;
        }
    }
}

void ToolWorkspace::close_all_assets() {
    for (auto &entry : open_assets_) {
        if (entry.domain_control) {
            if (entry.domain_control->get_parent()) {
                entry.domain_control->get_parent()->remove_child(entry.domain_control);
            }
            entry.domain_control->queue_free();
        }
    }
    open_assets_.clear();
    active_asset_.unref();
    _clear_center_view();
    _refresh_inspector_for_asset(Ref<Resource>());
    _rebuild_sidebar_lists();
    if (tool_context_.is_valid()) {
        tool_context_->set_active_asset(Ref<Resource>());
    }
}

void ToolWorkspace::select_asset(const Ref<Resource> &p_asset) {
    if (p_asset.is_null()) {
        active_asset_.unref();
        _clear_center_view();
        _refresh_inspector_for_asset(Ref<Resource>());
        _rebuild_sidebar_lists();
        return;
    }

    for (auto &entry : open_assets_) {
        if (entry.asset == p_asset) {
            active_asset_ = p_asset;
            for (int t = 0; t < static_cast<int>(sidebar_buckets_.size()); ++t) {
                if (sidebar_buckets_[static_cast<size_t>(t)].tab_name == entry.sidebar_tab) {
                    sidebar_tabs_->set_current_tab(t);
                    break;
                }
            }
            _show_asset_in_center(entry);
            _refresh_inspector_for_asset(active_asset_);
            _rebuild_sidebar_lists();

            if (tool_context_.is_valid()) {
                tool_context_->set_active_asset(active_asset_);
            }
            _on_active_asset_switched(active_asset_);
            emit_signal("active_asset_changed", active_asset_);
            return;
        }
    }
}

bool ToolWorkspace::save_asset(const Ref<Resource> &p_asset) {
    if (p_asset.is_null()) {
        return false;
    }
    if (Engine::get_singleton()->is_editor_hint()) {
        const String res_path = p_asset->get_path();
        if (res_path.is_empty() || res_path.contains("::")) {
            UtilityFunctions::printerr(
                String(workspace_id_) + String(": Cannot save asset without a standalone resource path.")
            );
            return false;
        }
        const Error err = ResourceSaver::get_singleton()->save(p_asset, res_path);
        if (err != OK) {
            UtilityFunctions::printerr(
                String(workspace_id_) + String(": Failed to save resource '") + res_path + String("' (err=") + String::num_int64(err) + String(").")
            );
            return false;
        }
        if (tool_context_.is_valid()) {
            tool_context_->mark_asset_dirty(p_asset, false);
        }
        return true;
    }

    emit_signal("runtime_save_requested", p_asset);
    if (tool_context_.is_valid()) {
        tool_context_->mark_asset_dirty(p_asset, false);
    }
    return true;
}

bool ToolWorkspace::save_active_asset() {
    return save_asset(active_asset_);
}

int ToolWorkspace::save_all_assets() {
    int saved = 0;
    for (const auto &entry : open_assets_) {
        if (save_asset(entry.asset)) {
            ++saved;
        }
    }
    return saved;
}

// ============================================================================
// Signal Handlers
// ============================================================================

void ToolWorkspace::_on_theme_applied(const Ref<Theme> &p_theme, int p_index) {
    if (tool_context_.is_valid()) {
        tool_context_->set_active_theme(p_theme, p_index);
    } else {
        set_theme(p_theme.is_valid() ? p_theme : Ref<Theme>());
    }
}

void ToolWorkspace::_on_context_theme_changed(const Ref<Theme> &p_theme, int p_index) {
    (void)p_index;
    set_theme(p_theme.is_valid() ? p_theme : Ref<Theme>());
}

void ToolWorkspace::_on_update_policy_selected(int p_index) {
    const auto policy = static_cast<UpdateCommitPolicy>(std::clamp(p_index, 0, 2));
    if (tool_context_.is_valid()) {
        tool_context_->set_update_commit_policy(policy);
    }
}

void ToolWorkspace::_on_context_update_policy_changed(int p_policy) {
    if (update_policy_btn_ && update_policy_btn_->get_selected() != p_policy) {
        update_policy_btn_->select(p_policy);
    }
    for (auto &entry : open_assets_) {
        if (entry.domain_control && entry.domain_control->has_method("set_update_commit_policy")) {
            entry.domain_control->call("set_update_commit_policy", p_policy);
        }
    }
}

void ToolWorkspace::_on_context_asset_dirty_changed(const Ref<Resource> &p_asset, bool p_dirty) {
    (void)p_asset;
    _rebuild_sidebar_lists();
    if (tool_context_.is_valid() &&
        tool_context_->get_update_commit_policy() == UPDATE_EXPLICIT_TRIGGER &&
        p_dirty) {
        set_dirty_banner(true);
    }
}

void ToolWorkspace::_on_save_pressed() {
    save_active_asset();
}

void ToolWorkspace::_on_save_all_pressed() {
    save_all_assets();
}

void ToolWorkspace::_on_help_pressed() {
    request_workspace_help();
}

void ToolWorkspace::_on_sidebar_item_selected(int p_index, const String &p_tab_name) {
    int bucket_idx = 0;
    for (const auto &entry : open_assets_) {
        if (entry.sidebar_tab != p_tab_name) {
            continue;
        }
        if (bucket_idx == p_index) {
            select_asset(entry.asset);
            return;
        }
        ++bucket_idx;
    }
}

void ToolWorkspace::_on_sidebar_gui_input(const Ref<InputEvent> &p_event, const String &p_tab_name) {
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_null() || !mb->is_pressed() || mb->get_button_index() != MOUSE_BUTTON_RIGHT) {
        return;
    }

    ItemList *list = nullptr;
    for (const auto &b : sidebar_buckets_) {
        if (b.tab_name == p_tab_name) {
            list = b.item_list;
            break;
        }
    }
    if (!list) {
        return;
    }

    const int item_idx = list->get_item_at_position(mb->get_position(), true);
    if (item_idx < 0) {
        return;
    }

    int bucket_idx = 0;
    for (const auto &entry : open_assets_) {
        if (entry.sidebar_tab != p_tab_name) {
            continue;
        }
        if (bucket_idx == item_idx) {
            context_target_asset_ = entry.asset;
            list->select(item_idx);
            sidebar_context_menu_->set_position(Vector2i(get_screen_position() + get_local_mouse_position()));
            sidebar_context_menu_->popup();
            return;
        }
        ++bucket_idx;
    }
}

void ToolWorkspace::_on_sidebar_context_action(int p_action_id) {
    if (context_target_asset_.is_null()) {
        return;
    }
    switch (static_cast<SidebarContextAction>(p_action_id)) {
        case CTX_SAVE_ASSET: {
            save_asset(context_target_asset_);
            break;
        }
        case CTX_CLOSE_ASSET: {
            close_asset(context_target_asset_);
            break;
        }
        case CTX_COPY_PATH: {
            if (auto *ds = DisplayServer::get_singleton()) {
                ds->clipboard_set(context_target_asset_->get_path());
            }
            break;
        }
        case CTX_COPY_UID: {
            const String path = context_target_asset_->get_path();
            if (!path.is_empty()) {
                const int64_t uid = ResourceLoader::get_singleton()->get_resource_uid(path);
                if (uid != ResourceUID::INVALID_ID) {
                    if (auto *ds = DisplayServer::get_singleton()) {
                        ds->clipboard_set(ResourceUID::get_singleton()->id_to_text(uid));
                    }
                }
            }
            break;
        }
    }
}

void ToolWorkspace::_on_domain_commit_completed(const String &p_action_name) {
    (void)p_action_name;
    if (active_asset_.is_valid() && tool_context_.is_valid()) {
        tool_context_->mark_asset_dirty(active_asset_, true);
    }
}

void ToolWorkspace::_on_dirty_banner_action_pressed() {
    if (tool_context_.is_valid()) {
        tool_context_->request_explicit_rebuild(active_asset_, "User triggered workspace rebuild");
    }
    emit_signal("explicit_rebuild_triggered", active_asset_);
    set_dirty_banner(false);
}

Window *ToolWorkspace::create_workspace_window(
    ToolWorkspace *p_workspace,
    const String &p_title,
    const Vector2i &p_min_size
) {
    Window *win = memnew(Window);
    win->set_title(p_title);
    win->set_min_size(p_min_size);
    win->set_size(p_min_size);
    win->set_transient(false);
    win->connect("close_requested", Callable(win, "queue_free"));

    if (p_workspace) {
        win->add_child(p_workspace);
    }

    if (auto *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop())) {
        if (tree->get_root()) {
            tree->get_root()->add_child(win);
            win->popup_centered();
        }
    }
    return win;
}

} // namespace ideam::godot_ext

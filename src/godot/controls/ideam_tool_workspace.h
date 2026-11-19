#pragma once

/**
 * ideam_tool_workspace.h
 * Reusable Tier 2 orchestration workspace base (`godot::VBoxContainer`) in `src/godot/controls/`.
 *
 * Synthesizes the proven UI patterns of `GraphComposer` (command bar, `ThemeSelector`, save
 * workflow, window hosting) and `MemoryProfiler` (sidebar `TabContainer` of open-asset `ItemList`s,
 * central visualization host, right-hand inspector pane, optional top ribbon/banner, context menu).
 *
 * Design Contract:
 *   - Tier 2 workspaces NEVER mutate domain data directly; they orchestrate Tier 1 controls
 *     registered in `IdeamTier1ControlRegistry` (or via `_create_tier1_control_for_asset()`).
 *   - Embeddable-first: any Tier 2 workspace can run inside its own floating `godot::Window` OR
 *     be embedded directly inside a future Tier 3/4/5 meta-tool with shared `IdeamToolContext`
 *     and configurable chrome visibility (`set_chrome_visibility`).
 */

#include "ideam_tier1_control_registry.h"
#include "ideam_tool_context.h"
#include "runtime_inspector.h"
#include "theme_selector.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <vector>

namespace ideam::godot_ext {

class IdeamToolWorkspace : public godot::VBoxContainer {
    GDCLASS(IdeamToolWorkspace, godot::VBoxContainer)

public:
    enum SidebarContextAction : int32_t {
        CTX_SAVE_ASSET  = 0,
        CTX_CLOSE_ASSET = 1,
        CTX_COPY_PATH   = 2,
        CTX_COPY_UID    = 3
    };

protected:
    struct OpenAssetEntry {
        godot::Ref<godot::Resource> asset;
        godot::String sidebar_tab;
        godot::String display_name;
        godot::Control *tier1_control = nullptr;
    };

    struct SidebarTabBucket {
        godot::String tab_name;
        godot::ItemList *item_list = nullptr;
    };

    godot::StringName workspace_id_ = godot::StringName("IdeamToolWorkspace");
    godot::String theme_registry_path_;
    godot::Ref<IdeamToolContext> tool_context_;

    // Row 1: Command Bar
    godot::HBoxContainer *command_bar_       = nullptr;
    ThemeSelector *theme_selector_ui_        = nullptr;
    godot::OptionButton *update_policy_btn_  = nullptr;
    godot::Button *save_btn_                 = nullptr;
    godot::Button *save_all_btn_             = nullptr;
    godot::HBoxContainer *custom_actions_box_ = nullptr;

    // Row 2: Optional Ribbon / Dirty-Pipeline Banner
    godot::PanelContainer *ribbon_slot_      = nullptr;
    godot::HBoxContainer *dirty_banner_box_  = nullptr;
    godot::Label *dirty_banner_label_        = nullptr;
    godot::Button *dirty_banner_action_btn_  = nullptr;

    // Row 3: 3-Pane Workspace (Left Sidebar | Center Tier-1 Host | Right Inspector)
    godot::HSplitContainer *workspace_split_       = nullptr;
    godot::HSplitContainer *workspace_inner_split_ = nullptr;

    godot::TabContainer *sidebar_tabs_       = nullptr;
    godot::ScrollContainer *center_scroll_   = nullptr;
    godot::PanelContainer *center_host_      = nullptr;
    godot::Label *empty_state_label_         = nullptr;

    godot::PanelContainer *inspector_panel_  = nullptr;
    godot::VBoxContainer *inspector_vbox_    = nullptr;
    godot::Label *inspector_title_           = nullptr;
    RuntimeInspector *runtime_inspector_     = nullptr;

    godot::PopupMenu *sidebar_context_menu_  = nullptr;
    godot::Ref<godot::Resource> context_target_asset_;

    std::vector<SidebarTabBucket> sidebar_buckets_;
    std::vector<OpenAssetEntry> open_assets_;
    godot::Ref<godot::Resource> active_asset_;

    static void _bind_methods();

    // --- Internal Helpers ---
    void _connect_context_signals();
    void _disconnect_context_signals();
    SidebarTabBucket *_get_or_create_sidebar_bucket(const godot::String &p_tab_name);
    void _rebuild_sidebar_lists();
    void _show_asset_in_center(OpenAssetEntry &p_entry);
    void _clear_center_view();
    void _refresh_inspector_for_asset(const godot::Ref<godot::Resource> &p_asset);
    godot::String _resolve_asset_display_name(const godot::Ref<godot::Resource> &p_asset) const;
    godot::String _resolve_sidebar_tab_for_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::String &p_tab_override
    ) const;

    // --- Subclass Customization Hooks ---
    virtual void _populate_command_bar(godot::HBoxContainer *p_custom_actions) { (void)p_custom_actions; }
    virtual godot::Control *_create_tier1_control_for_asset(const godot::Ref<godot::Resource> &p_asset);
    virtual void _on_active_asset_switched(const godot::Ref<godot::Resource> &p_asset) { (void)p_asset; }
    virtual void _on_dirty_banner_action_pressed();

public:
    IdeamToolWorkspace();
    ~IdeamToolWorkspace() override;

    void _input(const godot::Ref<godot::InputEvent> &p_event) override;

    // --- Identity & Configuration ---
    void set_workspace_id(const godot::StringName &p_id);
    [[nodiscard]] godot::StringName get_workspace_id() const { return workspace_id_; }

    void configure_theme_registry(const godot::String &p_registry_path);
    void ensure_sidebar_tab(const godot::String &p_tab_name);

    // --- Shared Context & Tier 3+ Embeddability ---
    void set_tool_context(const godot::Ref<IdeamToolContext> &p_context);
    [[nodiscard]] godot::Ref<IdeamToolContext> get_tool_context() const { return tool_context_; }

    void set_chrome_visibility(
        bool p_show_command_bar,
        bool p_show_sidebar = true,
        bool p_show_inspector = true,
        bool p_show_ribbon = false
    );

    void set_dirty_banner(
        bool p_visible,
        const godot::String &p_message = "",
        const godot::String &p_action_label = "Rebuild"
    );

    // --- Asset Session Management ---
    bool open_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::String &p_tab_override = ""
    );
    void close_asset(const godot::Ref<godot::Resource> &p_asset);
    void close_all_assets();
    void select_asset(const godot::Ref<godot::Resource> &p_asset);
    [[nodiscard]] godot::Ref<godot::Resource> get_active_asset() const { return active_asset_; }

    bool save_asset(const godot::Ref<godot::Resource> &p_asset);
    bool save_active_asset();
    int save_all_assets();

    // --- Signal Handlers ---
    void _on_theme_applied(const godot::Ref<godot::Theme> &p_theme, int p_index);
    void _on_context_theme_changed(const godot::Ref<godot::Theme> &p_theme, int p_index);
    void _on_update_policy_selected(int p_index);
    void _on_context_update_policy_changed(int p_policy);
    void _on_context_asset_dirty_changed(const godot::Ref<godot::Resource> &p_asset, bool p_dirty);
    void _on_save_pressed();
    void _on_save_all_pressed();
    void _on_sidebar_item_selected(int p_index, const godot::String &p_tab_name);
    void _on_sidebar_gui_input(const godot::Ref<godot::InputEvent> &p_event, const godot::String &p_tab_name);
    void _on_sidebar_context_action(int p_action_id);
    void _on_tier1_commit_completed(const godot::String &p_action_name);

    // --- Window Hosting Helper ---
    static godot::Window *create_workspace_window(
        IdeamToolWorkspace *p_workspace,
        const godot::String &p_title,
        const godot::Vector2i &p_min_size = godot::Vector2i(1040, 680)
    );
};

} // namespace ideam::godot_ext

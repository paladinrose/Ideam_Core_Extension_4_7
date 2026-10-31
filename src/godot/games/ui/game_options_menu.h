#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "../gameplay/gameplay_accessibility_option.h"
#include "../gameplay/gameplay_audio_option.h"
#include "../gameplay/gameplay_control_map.h"
#include "../gameplay/gameplay_difficulty.h"
#include "../gameplay/gameplay_graphics_option.h"
#include "../gameplay/gameplay_mode.h"
#include "../gameplay/gameplay_perspective.h"
#include "../gameplay/gameplay_style.h"

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameEntity;
class GameMenu;

/**
 * DOD NOTE: GameOptionsMenu acts as a headless-capable, slot-injected View-Binder and
 * transactional staging controller over the `GameplayStyle` / `GameplayMode` cascade.
 *
 * Rather than hardcoding a monolithic UI tree or mutating live resources directly on
 * every slider tick, GameOptionsMenu:
 * 1. Resolves its target context (`target_entity` such as a `GameBoard` or `GameAgent`,
 *    or the root `Game`, or an explicit `GameplayStyle` / `GameplayMode`).
 * 2. Snapshots a baseline backup and creates a deep staging clone (`_staged_mode`) when
 *    opened, enabling optional real-time previewing (`live_preview_enabled`) while
 *    preserving zero-side-effect rollback on `cancel_options()`.
 * 3. Supports both bespoke designer-injected UI hierarchies (via `@export` container and
 *    button slots) and automatic fallback UI generation (`auto_build_fallback_ui`) across
 *    all six gameplay domains: Graphics, Audio, Control Maps, Difficulty, Perspective,
 *    and Accessibility.
 * 4. Enforces a 15-second display confirmation watchdog when resolution scale or window
 *    mode changes are applied, automatically reverting if unconfirmed.
 */
class GameOptionsMenu : public godot::Control {
    GDCLASS(GameOptionsMenu, godot::Control)

protected:
    static void _bind_methods();

private:
    // --- 1. Context & Target Scope Bindings ---
    Game *game = nullptr;
    GameEntity *target_entity = nullptr;
    GameMenu *game_menu = nullptr;

    godot::Ref<GameplayStyle> target_style;
    godot::Ref<GameplayMode> target_mode;
    int active_mode_index = 0;

    // Optional standalone resource overrides when operating outside a GameplayMode container
    godot::Ref<GameplayGraphicsOption> graphics_option;
    godot::Ref<GameplayAudioOption> audio_option;
    godot::Ref<GameplayControlMap> control_map;
    godot::Ref<GameplayDifficulty> difficulty_option;
    godot::Ref<GameplayPerspective> perspective_option;
    godot::Ref<GameplayAccessibilityOption> accessibility_option;

    // --- 2. Transactional Staging & Persistence Configuration ---
    bool auto_build_fallback_ui = true;
    bool live_preview_enabled = true;
    bool auto_load_config_on_ready = false;
    bool auto_save_config_on_apply = true;
    godot::String config_file_path = "user://settings.cfg";

    // Display mode / resolution change confirmation watchdog
    float display_confirm_timeout = 15.0f;
    float _display_confirm_remaining = 0.0f;
    bool _awaiting_display_confirmation = false;

    // Staged & backup clones for transactional Apply / Cancel / Revert
    godot::Ref<GameplayMode> _staged_mode;
    godot::Ref<GameplayGraphicsOption> _staged_graphics;
    godot::Ref<GameplayAudioOption> _staged_audio;
    godot::Ref<GameplayControlMap> _staged_controls;
    godot::Ref<GameplayDifficulty> _staged_difficulty;
    godot::Ref<GameplayPerspective> _staged_perspective;
    godot::Ref<GameplayAccessibilityOption> _staged_accessibility;

    godot::Dictionary _backup_graphics_data;
    godot::Dictionary _backup_audio_data;
    godot::Dictionary _backup_controls_data;
    godot::Dictionary _backup_difficulty_data;
    godot::Dictionary _backup_perspective_data;
    godot::Dictionary _backup_accessibility_data;

    bool _updating_ui = false;

    // --- 3. Slot-Injected UI Controls (Designer Overrideable) ---
    godot::Label *context_label = nullptr;
    godot::OptionButton *mode_selector = nullptr;
    godot::TabContainer *tabs = nullptr;

    godot::Control *graphics_tab = nullptr;
    godot::Control *audio_tab = nullptr;
    godot::Control *controls_tab = nullptr;
    godot::Control *difficulty_tab = nullptr;
    godot::Control *perspective_tab = nullptr;
    godot::Control *accessibility_tab = nullptr;

    godot::Button *reset_defaults_button = nullptr;
    godot::Button *apply_button = nullptr;
    godot::Button *cancel_button = nullptr;
    godot::ConfirmationDialog *display_confirm_dialog = nullptr;

    // Auto-generated domain content containers
    godot::VBoxContainer *_graphics_vbox = nullptr;
    godot::VBoxContainer *_audio_vbox = nullptr;
    godot::VBoxContainer *_controls_vbox = nullptr;
    godot::VBoxContainer *_difficulty_vbox = nullptr;
    godot::VBoxContainer *_perspective_vbox = nullptr;
    godot::VBoxContainer *_accessibility_vbox = nullptr;

    // --- Internal Helpers ---
    void _resolve_context_resources();
    void _create_staging_clones();
    void _restore_from_backups();
    void _commit_staged_to_authoritative();
    void _apply_staged_to_runtime(bool p_cascade_rules);
    godot::Environment *_find_active_environment() const;
    godot::Node *_find_scene_root() const;

    // UI Construction & Validation
    void _validate_layout_shell();
    godot::VBoxContainer *_ensure_tab_scroll_vbox(godot::Control *&r_tab_slot, const godot::String &p_tab_name);
    void _rebuild_all_domain_tabs();
    void _populate_mode_selector();
    void _update_context_header();

    void _build_graphics_tab_content(godot::VBoxContainer *p_vbox);
    void _build_audio_tab_content(godot::VBoxContainer *p_vbox);
    void _build_controls_tab_content(godot::VBoxContainer *p_vbox);
    void _build_difficulty_tab_content(godot::VBoxContainer *p_vbox);
    void _build_perspective_tab_content(godot::VBoxContainer *p_vbox);
    void _build_accessibility_tab_content(godot::VBoxContainer *p_vbox);

    // Row Factory Helpers
    void _add_section_header(godot::VBoxContainer *p_parent, const godot::String &p_title);
    godot::OptionButton *_add_enum_row(
        godot::VBoxContainer *p_parent,
        const godot::String &p_label,
        const godot::PackedStringArray &p_items,
        const godot::PackedInt32Array &p_ids,
        int p_selected_id,
        const godot::StringName &p_domain,
        const godot::StringName &p_prop
    );
    godot::SpinBox *_add_scalar_row(
        godot::VBoxContainer *p_parent,
        const godot::String &p_label,
        double p_value,
        double p_min,
        double p_max,
        double p_step,
        const godot::StringName &p_domain,
        const godot::StringName &p_prop,
        const godot::String &p_suffix = ""
    );
    godot::CheckBox *_add_bool_row(
        godot::VBoxContainer *p_parent,
        const godot::String &p_label,
        bool p_value,
        const godot::StringName &p_domain,
        const godot::StringName &p_prop
    );

    // Generic Bound UI Signal Callbacks
    void _on_mode_selected(int p_index);
    void _on_enum_changed(int p_index, const godot::StringName &p_domain, const godot::StringName &p_prop, godot::Object *p_option_btn);
    void _on_scalar_changed(double p_value, const godot::StringName &p_domain, const godot::StringName &p_prop);
    void _on_bool_changed(bool p_pressed, const godot::StringName &p_domain, const godot::StringName &p_prop);
    void _on_display_confirmed();
    void _on_display_reverted();

public:
    GameOptionsMenu();
    ~GameOptionsMenu() override;

    virtual void _ready() override;
    virtual void _process(double p_delta) override;

    // --- 1. Context & Target Scope Setters / Getters ---
    void set_game(Game *p_game);
    Game *get_game() const;

    void set_target_entity(GameEntity *p_entity);
    GameEntity *get_target_entity() const;

    void set_game_menu(GameMenu *p_menu);
    GameMenu *get_game_menu() const;

    void set_target_style(const godot::Ref<GameplayStyle> &p_style);
    godot::Ref<GameplayStyle> get_target_style() const;

    void set_target_mode(const godot::Ref<GameplayMode> &p_mode);
    godot::Ref<GameplayMode> get_target_mode() const;

    void set_active_mode_index(int p_index);
    int get_active_mode_index() const;

    // Direct Domain Resource Setters / Getters
    void set_graphics_option(const godot::Ref<GameplayGraphicsOption> &p_opt);
    godot::Ref<GameplayGraphicsOption> get_graphics_option() const;

    void set_audio_option(const godot::Ref<GameplayAudioOption> &p_opt);
    godot::Ref<GameplayAudioOption> get_audio_option() const;

    void set_control_map(const godot::Ref<GameplayControlMap> &p_map);
    godot::Ref<GameplayControlMap> get_control_map() const;

    void set_difficulty_option(const godot::Ref<GameplayDifficulty> &p_opt);
    godot::Ref<GameplayDifficulty> get_difficulty_option() const;

    void set_perspective_option(const godot::Ref<GameplayPerspective> &p_opt);
    godot::Ref<GameplayPerspective> get_perspective_option() const;

    void set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt);
    godot::Ref<GameplayAccessibilityOption> get_accessibility_option() const;

    // --- 2. Behavior & Persistence Setters / Getters ---
    void set_auto_build_fallback_ui(bool p_auto_build);
    bool get_auto_build_fallback_ui() const;

    void set_live_preview_enabled(bool p_enabled);
    bool get_live_preview_enabled() const;

    void set_auto_load_config_on_ready(bool p_auto_load);
    bool get_auto_load_config_on_ready() const;

    void set_auto_save_config_on_apply(bool p_auto_save);
    bool get_auto_save_config_on_apply() const;

    void set_config_file_path(const godot::String &p_path);
    godot::String get_config_file_path() const;

    void set_display_confirm_timeout(float p_seconds);
    float get_display_confirm_timeout() const;

    // --- 3. Slot-Injected Control Setters / Getters ---
    void set_context_label(godot::Label *p_label);
    godot::Label *get_context_label() const;

    void set_mode_selector(godot::OptionButton *p_selector);
    godot::OptionButton *get_mode_selector() const;

    void set_tabs(godot::TabContainer *p_tabs);
    godot::TabContainer *get_tabs() const;

    void set_graphics_tab(godot::Control *p_tab);
    godot::Control *get_graphics_tab() const;

    void set_audio_tab(godot::Control *p_tab);
    godot::Control *get_audio_tab() const;

    void set_controls_tab(godot::Control *p_tab);
    godot::Control *get_controls_tab() const;

    void set_difficulty_tab(godot::Control *p_tab);
    godot::Control *get_difficulty_tab() const;

    void set_perspective_tab(godot::Control *p_tab);
    godot::Control *get_perspective_tab() const;

    void set_accessibility_tab(godot::Control *p_tab);
    godot::Control *get_accessibility_tab() const;

    void set_reset_defaults_button(godot::Button *p_button);
    godot::Button *get_reset_defaults_button() const;

    void set_apply_button(godot::Button *p_button);
    godot::Button *get_apply_button() const;

    void set_cancel_button(godot::Button *p_button);
    godot::Button *get_cancel_button() const;

    void set_display_confirm_dialog(godot::ConfirmationDialog *p_dialog);
    godot::ConfirmationDialog *get_display_confirm_dialog() const;

    // --- 4. Core Lifecycle & Action Methods ---
    void validate_game();
    void build_options_menu();
    void refresh_ui_from_staged();

    void open_options();
    void open_options_for_entity(GameEntity *p_entity);
    void close_options();
    void apply_options();
    void cancel_options();
    void reset_to_defaults();

    bool save_options_to_config(const godot::String &p_path = "");
    bool load_options_from_config(const godot::String &p_path = "");
};

} // namespace ideam::godot_ext
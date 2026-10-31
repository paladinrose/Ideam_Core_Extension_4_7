#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameBoard;
class GameAgent;
class GameMenu;
class GameOptionsMenu;
class GameplayStyle;
class GameplayMode;
class GameplayPerspective;
class GameplayDifficulty;
class GameplayControlMap;

/**
 * DOD NOTE: GamePauseMenu coordinates the runtime pause lifecycle across `Game`,
 * the active `GameBoard`, and the player's possessed `GameAgent`.
 *
 * Following the slot-injection pattern established by `GameMenu`, UI artists can bind
 * custom buttons, quick-switch dropdowns, and telemetry labels via `@export` properties,
 * or allow `validate_pause_menu()` to construct a clean fallback layout automatically.
 * Quick-switch controls operate directly on the active entity's `GameplayMode` and
 * `GameplayStyle` cascade while respecting mid-game lock rules (`allow_mid_game_change`).
 */
class GamePauseMenu : public godot::Control {
    GDCLASS(GamePauseMenu, godot::Control)

protected:
    static void _bind_methods();

private:
    // --- 1. Context & Linked Menus ---
    Game *game = nullptr;
    GameBoard *active_board = nullptr;
    GameAgent *controlled_agent = nullptr;
    GameMenu *game_menu = nullptr;
    GameOptionsMenu *options_menu = nullptr;

    bool auto_build_fallback_ui = true;
    bool handle_pause_input = true;
    godot::StringName fallback_pause_action = godot::StringName("ui_cancel");

    bool _updating_quick_switchers = false;

    // --- 2. Slot-Injected UI Controls (Designer Overrideable) ---
    godot::Label *pause_title_label = nullptr;
    godot::Label *context_status_label = nullptr;
    godot::Label *active_tags_label = nullptr;

    // Quick-Switch Gameplay Controls
    godot::OptionButton *mode_selector = nullptr;
    godot::OptionButton *perspective_selector = nullptr;
    godot::Button *swap_shoulder_button = nullptr;
    godot::OptionButton *difficulty_selector = nullptr;

    // Standard Pause Action Buttons
    godot::Button *resume_button = nullptr;
    godot::Button *save_game_button = nullptr;
    godot::Button *load_game_button = nullptr;
    godot::Button *restart_board_button = nullptr;
    godot::Button *options_button = nullptr;
    godot::Button *main_menu_button = nullptr;
    godot::Button *quit_button = nullptr;

    godot::VBoxContainer *_root_vbox = nullptr;

    // --- Internal Helpers ---
    godot::Ref<GameplayStyle> _resolve_active_style() const;
    godot::Ref<GameplayMode> _resolve_active_mode() const;
    godot::Ref<GameplayPerspective> _resolve_active_perspective() const;
    godot::Ref<GameplayDifficulty> _resolve_active_difficulty() const;
    godot::Ref<GameplayControlMap> _resolve_active_control_map() const;
    void _reapply_active_style_cascade();

    void _validate_buttons();
    void _validate_quick_switch_bar();
    void _connect_game_signals();

    // Signal Callbacks
    void _on_game_paused();
    void _on_game_continued();
    void _on_options_menu_closed();
    void _on_quick_mode_selected(int p_index);
    void _on_quick_perspective_selected(int p_index);
    void _on_quick_difficulty_selected(int p_index);

public:
    GamePauseMenu();
    ~GamePauseMenu() override;

    virtual void _ready() override;
    virtual void _unhandled_input(const godot::Ref<godot::InputEvent> &p_event) override;

    // --- 1. Context & Configuration Setters / Getters ---
    void set_game(Game *p_game);
    Game *get_game() const;

    void set_active_board(GameBoard *p_board);
    GameBoard *get_active_board() const;

    void set_controlled_agent(GameAgent *p_agent);
    GameAgent *get_controlled_agent() const;

    void set_game_menu(GameMenu *p_menu);
    GameMenu *get_game_menu() const;

    void set_options_menu(GameOptionsMenu *p_menu);
    GameOptionsMenu *get_options_menu() const;

    void set_auto_build_fallback_ui(bool p_auto_build);
    bool get_auto_build_fallback_ui() const;

    void set_handle_pause_input(bool p_handle);
    bool get_handle_pause_input() const;

    void set_fallback_pause_action(const godot::StringName &p_action);
    godot::StringName get_fallback_pause_action() const;

    // --- 2. Slot-Injected Control Setters / Getters ---
    void set_pause_title_label(godot::Label *p_label);
    godot::Label *get_pause_title_label() const;

    void set_context_status_label(godot::Label *p_label);
    godot::Label *get_context_status_label() const;

    void set_active_tags_label(godot::Label *p_label);
    godot::Label *get_active_tags_label() const;

    void set_mode_selector(godot::OptionButton *p_selector);
    godot::OptionButton *get_mode_selector() const;

    void set_perspective_selector(godot::OptionButton *p_selector);
    godot::OptionButton *get_perspective_selector() const;

    void set_swap_shoulder_button(godot::Button *p_button);
    godot::Button *get_swap_shoulder_button() const;

    void set_difficulty_selector(godot::OptionButton *p_selector);
    godot::OptionButton *get_difficulty_selector() const;

    void set_resume_button(godot::Button *p_button);
    godot::Button *get_resume_button() const;

    void set_save_game_button(godot::Button *p_button);
    godot::Button *get_save_game_button() const;

    void set_load_game_button(godot::Button *p_button);
    godot::Button *get_load_game_button() const;

    void set_restart_board_button(godot::Button *p_button);
    godot::Button *get_restart_board_button() const;

    void set_options_button(godot::Button *p_button);
    godot::Button *get_options_button() const;

    void set_main_menu_button(godot::Button *p_button);
    godot::Button *get_main_menu_button() const;

    void set_quit_button(godot::Button *p_button);
    godot::Button *get_quit_button() const;

    // --- 3. Lifecycle, Validation & Action Methods ---
    void validate_pause_context();
    void build_pause_menu();
    void refresh_pause_telemetry();

    void open_pause_menu();
    void close_pause_menu();
    void toggle_pause_menu();

    void resume_pressed();
    void save_game_pressed();
    void load_game_pressed();
    void restart_board_pressed();
    void options_pressed();
    void swap_shoulder_pressed();
    void main_menu_pressed();
    void quit_pressed();
};

} // namespace ideam::godot_ext
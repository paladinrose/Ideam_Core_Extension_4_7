#pragma once

/**
 * wallpaper_group_selector_control.h
 * Crystallographic Symmetry & Tactile Isohedral Selector Widget (`src/godot/tiles/ui/`).
 *
 * Responsibilities:
 *   1. Provides a unified selector for `TilingAuthoringModeType` (`Wallpaper Group (17)` vs.
 *      `Isohedral Tactile (81)`), Bravais `TilingLatticeFamily` filter (`All`, `Oblique`,
 *      `Rectangular`, `Rhombic`, `Square`, `Hexagonal`), and `TilingShapePresetType`
 *      (`Tactile Default`, `Quad`, `Brick Hex`, `Right Triangle`, `Equilateral Triangle`,
 *      `Flat Triangle`, `Tall Triangle`, `Rhombus 60`).
 *   2. Renders an interactive 17-button Wallpaper Group matrix (`p1` .. `p6m`) grouped and
 *      color-coded by Bravais lattice family, plus a dynamically filtered `OptionButton`
 *      for the 81 valid Tactile Isohedral types (`IH01` .. `IH93`).
 *   3. Displays live crystallographic telemetry badges (`IUCr`, `Orbifold`, `Rotational Order`,
 *      `Mirror Reflection`, `Glide Reflection`) and commits all user selections through
 *      `WallpaperTilingResource`'s Undo/Redo-aware `commit_set_*` API.
 */

#include "../wallpaper_tiling_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include <array>

namespace ideam::godot_ext {

class WallpaperGroupSelectorControl : public godot::VBoxContainer {
    GDCLASS(WallpaperGroupSelectorControl, godot::VBoxContainer)

private:
    godot::Ref<WallpaperTilingResource> wallpaper_tiling_;
    int lattice_family_filter_ = -1; // -1 = All 5 Bravais lattice families
    bool is_updating_ui_       = false;

    godot::OptionButton *authoring_mode_opt_ = nullptr;
    godot::OptionButton *lattice_filter_opt_ = nullptr;
    godot::OptionButton *shape_preset_opt_   = nullptr;
    godot::OptionButton *ih_type_opt_        = nullptr;
    godot::GridContainer *group_grid_        = nullptr;
    godot::Label *symmetry_badge_label_      = nullptr;

    std::array<godot::Button *, 17> group_buttons_{};

    void _build_ui();
    void _rebuild_ih_dropdown();
    void _update_group_button_states();

    // --- Signal Handlers ---
    void _on_resource_changed();
    void _on_authoring_mode_selected(int p_idx);
    void _on_lattice_filter_selected(int p_idx);
    void _on_shape_preset_selected(int p_idx);
    void _on_ih_type_selected(int p_idx);
    void _on_group_button_pressed(int p_group_idx);

    [[nodiscard]] static TilingLatticeFamily _lattice_for_group(WallpaperGroupType p_group) noexcept;
    [[nodiscard]] static const char *_group_short_label(WallpaperGroupType p_group) noexcept;
    [[nodiscard]] static const char *_group_orbifold_label(WallpaperGroupType p_group) noexcept;

protected:
    static void _bind_methods();

public:
    WallpaperGroupSelectorControl();
    ~WallpaperGroupSelectorControl() override;

    void set_wallpaper_tiling(const godot::Ref<WallpaperTilingResource> &p_tiling);
    [[nodiscard]] godot::Ref<WallpaperTilingResource> get_wallpaper_tiling() const { return wallpaper_tiling_; }

    void set_lattice_family_filter(int p_family);
    [[nodiscard]] int get_lattice_family_filter() const noexcept { return lattice_family_filter_; }

    void refresh_ui();
};

} // namespace ideam::godot_ext

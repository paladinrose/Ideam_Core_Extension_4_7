#pragma once

/**
 * wallpaper_tiling_control.h
 * Composite Reflective Editor & Runtime UI Control (`VBoxContainer`) for inspecting and
 * authoring a `WallpaperTilingResource` (`src/godot/tiles/ui/`).
 *
 * Unites four interactive authoring panels:
 *   1. `WallpaperGroupSelectorControl`: 17 Wallpaper Group matrix, Bravais lattice filter,
 *      81 Tactile `IH` type selector, and shape presets (`Quad`, `Brick Hex`, triangles, `Rhombus 60`).
 *   2. `TileFundamentalDomainCanvas` + Viewport Toolbar: Interactive 2D prototile, lattice
 *      neighborhood (`0..3` ring radius), 3-coloring, `T1`/`T2` basis vectors, symmetry handles,
 *      and `J/U/S/I` edge constraint badges.
 *   3. Dynamic **Tactile Vertex Parameters & Lattice Basis Panel**: Auto-sizing `0..6`
 *      Tactile vertex parameter sliders (`commit_set_tactile_parameter`) and `Scale X/Y`,
 *      `Rotation (deg)`, `Origin X/Y` spinboxes (`commit_set_lattice_*`).
 *   4. **Escher Edge Splines (`IdeamSplines` Handshake) Panel**: Selects any independent
 *      `edge_shape_id` (`0..edge_shape_count-1`), displays its `J / U / S / I` symmetry rule,
 *      provides procedural edge generators (`Reset Straight I`, `Sine Wave S/J`, `Arc Bow U/J`),
 *      and embeds an interactive `Spline2DCanvas` to sculpt the active edge curve live.
 */

#include "tile_fundamental_domain_canvas.h"
#include "wallpaper_group_selector_control.h"
#include "../../splines/spline_2d/spline_2d_control.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include <array>

namespace ideam::godot_ext {

class WallpaperTilingControl : public godot::VBoxContainer {
    GDCLASS(WallpaperTilingControl, godot::VBoxContainer)

private:
    static constexpr size_t MAX_TACTILE_PARAMS = 6;

    godot::Ref<WallpaperTilingResource> wallpaper_tiling_;
    bool is_updating_ui_ = false;

    // 1. Sub-Controls
    WallpaperGroupSelectorControl *group_selector_ = nullptr;
    TileFundamentalDomainCanvas *domain_canvas_    = nullptr;

    // 2. Canvas Viewport Toolbar
    godot::SpinBox *neighborhood_radius_spin_ = nullptr;
    godot::CheckBox *three_color_check_       = nullptr;
    godot::CheckBox *basis_vectors_check_     = nullptr;
    godot::CheckBox *symmetry_handles_check_  = nullptr;
    godot::CheckBox *edge_badges_check_       = nullptr;
    godot::CheckBox *triangulation_check_     = nullptr;

    // 3. Dynamic Tactile Parameter Rows (0..6)
    godot::VBoxContainer *tactile_params_section_ = nullptr;
    std::array<godot::HBoxContainer *, MAX_TACTILE_PARAMS> param_rows_{};
    std::array<godot::HSlider *, MAX_TACTILE_PARAMS> param_sliders_{};
    std::array<godot::SpinBox *, MAX_TACTILE_PARAMS> param_spins_{};

    // 4. Lattice Basis Transform Controls
    godot::SpinBox *scale_x_spin_      = nullptr;
    godot::SpinBox *scale_y_spin_      = nullptr;
    godot::SpinBox *rotation_deg_spin_ = nullptr;
    godot::SpinBox *origin_x_spin_     = nullptr;
    godot::SpinBox *origin_y_spin_     = nullptr;

    // 5. Escher Edge Splines (`IdeamSplines` Lateral Handshake)
    godot::OptionButton *edge_slot_opt_        = nullptr;
    godot::Label *edge_constraint_label_       = nullptr;
    godot::CheckBox *show_edge_canvas_check_   = nullptr;
    Spline2DCanvas *edge_spline_canvas_        = nullptr;

    void _build_ui();
    void _rebuild_edge_slot_dropdown();
    void _sync_active_edge_spline_canvas();

    // --- Callbacks ---
    void _on_resource_changed();
    void _on_fit_view_pressed();
    void _on_neighborhood_radius_changed(double p_val);
    void _on_three_color_toggled(bool p_show);
    void _on_basis_vectors_toggled(bool p_show);
    void _on_symmetry_handles_toggled(bool p_show);
    void _on_edge_badges_toggled(bool p_show);
    void _on_triangulation_toggled(bool p_show);

    void _on_tactile_param_changed(double p_val, int p_param_idx);
    void _on_lattice_transform_edited(double p_unused);
    void _on_reset_lattice_pressed();

    void _on_canvas_edge_shape_selected(int p_edge_shape_id);
    void _on_edge_slot_selected(int p_idx);
    void _on_edge_preset_straight_pressed();
    void _on_edge_preset_s_wave_pressed();
    void _on_edge_preset_u_arc_pressed();
    void _on_clear_all_edges_pressed();
    void _on_show_edge_canvas_toggled(bool p_show);

protected:
    static void _bind_methods();

public:
    WallpaperTilingControl();
    ~WallpaperTilingControl() override;

    void set_wallpaper_tiling(const godot::Ref<WallpaperTilingResource> &p_tiling);
    [[nodiscard]] godot::Ref<WallpaperTilingResource> get_wallpaper_tiling() const { return wallpaper_tiling_; }

    [[nodiscard]] WallpaperGroupSelectorControl *get_group_selector() const noexcept { return group_selector_; }
    [[nodiscard]] TileFundamentalDomainCanvas *get_domain_canvas() const noexcept { return domain_canvas_; }
    [[nodiscard]] Spline2DCanvas *get_edge_spline_canvas() const noexcept { return edge_spline_canvas_; }

    void refresh_ui();
};

} // namespace ideam::godot_ext

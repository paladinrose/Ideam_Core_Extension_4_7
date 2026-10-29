#pragma once

#include "gameplay_resource.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

/**
 * DOD NOTE: Extracting graphics enums at namespace scope with explicit int32_t backing
 * allows them to be stored in flat Structure of Arrays (SoA) configuration blocks and
 * evaluated without pulling in the full GameplayGraphicsOption class definition.
 */
enum GraphicsQualityPreset : int32_t {
    QUALITY_CUSTOM = 0,
    QUALITY_LOW = 1,
    QUALITY_MEDIUM = 2,
    QUALITY_HIGH = 3,
    QUALITY_ULTRA = 4
};

enum ResolutionScalePreset : int32_t {
    RES_SCALE_ULTRA_PERFORMANCE = 0, // 1.0 / 3.0 (~0.333)
    RES_SCALE_PERFORMANCE = 1,       // 1.0 / 2.0 (0.500)
    RES_SCALE_BALANCED = 2,          // 1.0 / 1.7 (~0.588)
    RES_SCALE_QUALITY = 3,           // 1.0 / 1.5 (~0.667)
    RES_SCALE_ULTRA_QUALITY = 4,     // 1.0 / 1.3 (~0.769)
    RES_SCALE_NATIVE = 5,            // 1.000
    RES_SCALE_CUSTOM = 6
};

// Mirrors `Settings.GIType` from the Godot TPS Demo (`settings.gd`)
enum GraphicsGIType : int32_t {
    GI_TYPE_SDFGI = 0,
    GI_TYPE_VOXEL_GI = 1,
    GI_TYPE_LIGHTMAP_GI = 2
};

// Mirrors `Settings.GIQuality` from the Godot TPS Demo (`settings.gd`)
enum GraphicsGIQuality : int32_t {
    GI_QUALITY_DISABLED = 0,
    GI_QUALITY_LOW = 1,
    GI_QUALITY_HIGH = 2
};

// Unified quality tier for SSAO, SSIL, and SSR (-1 = Disabled, 0..4 = RenderingServer tiers)
enum GraphicsEffectQuality : int32_t {
    EFFECT_DISABLED = -1,
    EFFECT_VERY_LOW = 0,
    EFFECT_LOW = 1,
    EFFECT_MEDIUM = 2,
    EFFECT_HIGH = 3,
    EFFECT_ULTRA = 4
};

/**
 * DOD NOTE: GameplayGraphicsOption directly replaces the procedural GDScript logic in
 * the Godot TPS Demo's `settings.gd` and `menu.gd` (`[video]` and `[rendering]` sections)
 * with a native C++26 GameplayResource.
 *
 * Key architectural improvements over the stock TPS Demo `settings.gd`:
 * 1. Non-Destructive Shadow Toggling: Caches each `Light3D`'s original `shadow_enabled`
 *    state inside node metadata (`_ideam_orig_shadow_enabled`) before disabling shadows,
 *    fixing the TPS demo `FIXME` bug where re-enabling shadows in the menu failed to
 *    restore scene lights.
 * 2. Corrected SSAO Quality Dispatch: Fixes the inverted `ENV_SSAO_QUALITY_MEDIUM` vs.
 *    `ENV_SSAO_QUALITY_HIGH` branch in `settings.gd`.
 * 3. Hardware-Aware Upscaler Fallback: Automatically detects Metal rendering driver
 *    support (`is_metalfx_supported()`) and resolves `scale_filter` between MetalFX
 *    and FSR2 without manual UI branching.
 * 4. Dual-Section `user://settings.ini` Compatibility: Reads and writes both the
 *    unified `GameplayResource` section and the TPS demo's split `[video]` + `[rendering]`
 *    sections in `ConfigFile`.
 */
class GameplayGraphicsOption : public GameplayResource {
    GDCLASS(GameplayGraphicsOption, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- 0. Overall Preset ---
    GraphicsQualityPreset quality_preset = GraphicsQualityPreset::QUALITY_MEDIUM;

    // --- 1. Window & Display (`[video]` in TPS Demo) ---
    godot::Window::Mode display_mode = godot::Window::MODE_EXCLUSIVE_FULLSCREEN;
    godot::DisplayServer::VSyncMode vsync_mode = godot::DisplayServer::VSYNC_ENABLED;
    int max_fps = 0; // 0 = Unlimited
    float resolution_scale = 1.0f;
    godot::Viewport::Scaling3DMode scale_filter = godot::Viewport::SCALING_3D_MODE_FSR2;
    float fsr_sharpness = 0.2f;

    // --- 2. Anti-Aliasing (`[rendering]` in TPS Demo) ---
    bool taa_enabled = false;
    godot::Viewport::MSAA msaa_3d = godot::Viewport::MSAA_DISABLED;
    godot::Viewport::ScreenSpaceAA screen_space_aa = godot::Viewport::SCREEN_SPACE_AA_DISABLED;

    // --- 3. Lighting, GI & Shadows (`[rendering]` in TPS Demo) ---
    bool shadow_mapping_enabled = true;
    int positional_shadow_atlas_size = 4096;
    GraphicsGIType gi_type = GraphicsGIType::GI_TYPE_VOXEL_GI;
    GraphicsGIQuality gi_quality = GraphicsGIQuality::GI_QUALITY_LOW;

    // --- 4. Environment & Post-Processing (`[rendering]` in TPS Demo) ---
    GraphicsEffectQuality ssao_quality = GraphicsEffectQuality::EFFECT_MEDIUM;
    GraphicsEffectQuality ssil_quality = GraphicsEffectQuality::EFFECT_DISABLED;
    GraphicsEffectQuality ssr_quality = GraphicsEffectQuality::EFFECT_DISABLED;
    bool bloom_enabled = true;
    bool volumetric_fog_enabled = true;

    // --- 5. LOD & Performance Budgets (Cascaded via GameplayRule) ---
    float mesh_lod_threshold = 1.0f;
    float debris_budget_multiplier = 1.0f;

    // Internal scene traversal helpers for non-destructive shadow & GI toggling
    void _apply_shadow_and_gi_recursive(godot::Node *p_node) const;

public:
    GameplayGraphicsOption();
    ~GameplayGraphicsOption() override;

    // --- Hardware Capability Helpers ---
    static bool is_metalfx_supported();
    godot::Viewport::Scaling3DMode get_effective_scale_filter() const;

    // --- 0. Quality Preset Setters / Getters ---
    void set_quality_preset(GraphicsQualityPreset p_preset);
    GraphicsQualityPreset get_quality_preset() const;
    void apply_quality_preset(GraphicsQualityPreset p_preset);

    // --- 1. Window & Display Setters / Getters ---
    void set_display_mode(godot::Window::Mode p_mode);
    godot::Window::Mode get_display_mode() const;
    void toggle_fullscreen(godot::Window *p_window);

    void set_vsync_mode(godot::DisplayServer::VSyncMode p_mode);
    godot::DisplayServer::VSyncMode get_vsync_mode() const;

    void set_max_fps(int p_max_fps);
    int get_max_fps() const;

    void set_resolution_scale(float p_scale);
    float get_resolution_scale() const;

    void set_resolution_scale_preset(ResolutionScalePreset p_preset);
    ResolutionScalePreset get_resolution_scale_preset() const;
    static float preset_to_resolution_scale(ResolutionScalePreset p_preset);
    static ResolutionScalePreset resolution_scale_to_preset(float p_scale);

    void set_scale_filter(godot::Viewport::Scaling3DMode p_filter);
    godot::Viewport::Scaling3DMode get_scale_filter() const;

    void set_fsr_sharpness(float p_sharpness);
    float get_fsr_sharpness() const;

    // --- 2. Anti-Aliasing Setters / Getters ---
    void set_taa_enabled(bool p_enabled);
    bool get_taa_enabled() const;

    void set_msaa_3d(godot::Viewport::MSAA p_msaa);
    godot::Viewport::MSAA get_msaa_3d() const;

    void set_screen_space_aa(godot::Viewport::ScreenSpaceAA p_aa);
    godot::Viewport::ScreenSpaceAA get_screen_space_aa() const;

    // --- 3. Lighting, GI & Shadows Setters / Getters ---
    void set_shadow_mapping_enabled(bool p_enabled);
    bool get_shadow_mapping_enabled() const;

    void set_positional_shadow_atlas_size(int p_size);
    int get_positional_shadow_atlas_size() const;

    void set_gi_type(GraphicsGIType p_type);
    GraphicsGIType get_gi_type() const;

    void set_gi_quality(GraphicsGIQuality p_quality);
    GraphicsGIQuality get_gi_quality() const;

    // --- 4. Environment & Post-Processing Setters / Getters ---
    void set_ssao_quality(GraphicsEffectQuality p_quality);
    GraphicsEffectQuality get_ssao_quality() const;

    void set_ssil_quality(GraphicsEffectQuality p_quality);
    GraphicsEffectQuality get_ssil_quality() const;

    void set_ssr_quality(GraphicsEffectQuality p_quality);
    GraphicsEffectQuality get_ssr_quality() const;

    void set_bloom_enabled(bool p_enabled);
    bool get_bloom_enabled() const;

    void set_volumetric_fog_enabled(bool p_enabled);
    bool get_volumetric_fog_enabled() const;

    // --- 5. LOD & Performance Budgets Setters / Getters ---
    void set_mesh_lod_threshold(float p_threshold);
    float get_mesh_lod_threshold() const;

    void set_debris_budget_multiplier(float p_multiplier);
    float get_debris_budget_multiplier() const;

    // --- Application, Rule Synthesis & Persistence ---
    void apply_graphics_option(const godot::Ref<GameplayGraphicsOption> &p_other);

    /**
     * Direct C++ replacement for `Settings.apply_graphics_settings(window, environment, scene_root)`
     * in the Godot TPS Demo (`settings.gd`).
     */
    void apply_to_scene(godot::Window *p_window, godot::Environment *p_environment, godot::Node *p_scene_root);

    /**
     * Reads/writes the split `[video]` and `[rendering]` sections used by the Godot TPS
     * Demo's `user://settings.ini` ConfigFile.
     */
    bool save_to_tps_config(const godot::Ref<godot::ConfigFile> &p_config) const;
    bool load_from_tps_config(const godot::Ref<godot::ConfigFile> &p_config);

    virtual godot::TypedArray<GameplayRule> collect_rules() const override;
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::GraphicsQualityPreset);
VARIANT_ENUM_CAST(ideam::godot_ext::ResolutionScalePreset);
VARIANT_ENUM_CAST(ideam::godot_ext::GraphicsGIType);
VARIANT_ENUM_CAST(ideam::godot_ext::GraphicsGIQuality);
VARIANT_ENUM_CAST(ideam::godot_ext::GraphicsEffectQuality);
#include "gameplay_graphics_option.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/lightmap_gi.hpp>
#include <godot_cpp/classes/voxel_gi.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayGraphicsOption::_bind_methods() {
    // --- Enums ---
    BIND_ENUM_CONSTANT(GraphicsQualityPreset::QUALITY_CUSTOM);
    BIND_ENUM_CONSTANT(GraphicsQualityPreset::QUALITY_LOW);
    BIND_ENUM_CONSTANT(GraphicsQualityPreset::QUALITY_MEDIUM);
    BIND_ENUM_CONSTANT(GraphicsQualityPreset::QUALITY_HIGH);
    BIND_ENUM_CONSTANT(GraphicsQualityPreset::QUALITY_ULTRA);

    BIND_ENUM_CONSTANT(ResolutionScalePreset::RES_SCALE_ULTRA_PERFORMANCE);
    BIND_ENUM_CONSTANT(ResolutionScalePreset::RES_SCALE_PERFORMANCE);
    BIND_ENUM_CONSTANT(ResolutionScalePreset::RES_SCALE_BALANCED);
    BIND_ENUM_CONSTANT(ResolutionScalePreset::RES_SCALE_QUALITY);
    BIND_ENUM_CONSTANT(ResolutionScalePreset::RES_SCALE_ULTRA_QUALITY);
    BIND_ENUM_CONSTANT(ResolutionScalePreset::RES_SCALE_NATIVE);
    BIND_ENUM_CONSTANT(ResolutionScalePreset::RES_SCALE_CUSTOM);

    BIND_ENUM_CONSTANT(GraphicsGIType::GI_TYPE_SDFGI);
    BIND_ENUM_CONSTANT(GraphicsGIType::GI_TYPE_VOXEL_GI);
    BIND_ENUM_CONSTANT(GraphicsGIType::GI_TYPE_LIGHTMAP_GI);

    BIND_ENUM_CONSTANT(GraphicsGIQuality::GI_QUALITY_DISABLED);
    BIND_ENUM_CONSTANT(GraphicsGIQuality::GI_QUALITY_LOW);
    BIND_ENUM_CONSTANT(GraphicsGIQuality::GI_QUALITY_HIGH);

    BIND_ENUM_CONSTANT(GraphicsEffectQuality::EFFECT_DISABLED);
    BIND_ENUM_CONSTANT(GraphicsEffectQuality::EFFECT_VERY_LOW);
    BIND_ENUM_CONSTANT(GraphicsEffectQuality::EFFECT_LOW);
    BIND_ENUM_CONSTANT(GraphicsEffectQuality::EFFECT_MEDIUM);
    BIND_ENUM_CONSTANT(GraphicsEffectQuality::EFFECT_HIGH);
    BIND_ENUM_CONSTANT(GraphicsEffectQuality::EFFECT_ULTRA);

    // --- 0. Overall Preset ---
    ADD_GROUP("Quality Preset", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_quality_preset", "preset"), &GameplayGraphicsOption::set_quality_preset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_quality_preset"), &GameplayGraphicsOption::get_quality_preset);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_quality_preset", "preset"), &GameplayGraphicsOption::apply_quality_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "quality_preset", godot::PROPERTY_HINT_ENUM, "Custom,Low,Medium,High,Ultra"),
        "set_quality_preset",
        "get_quality_preset"
    );

    // --- 1. Window & Display ---
    ADD_GROUP("Window & Display", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_display_mode", "mode"), &GameplayGraphicsOption::set_display_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_display_mode"), &GameplayGraphicsOption::get_display_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("toggle_fullscreen", "window"), &GameplayGraphicsOption::toggle_fullscreen);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "display_mode", godot::PROPERTY_HINT_ENUM, "Windowed,Minimized,Maximized,Fullscreen,Exclusive Fullscreen"),
        "set_display_mode",
        "get_display_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vsync_mode", "mode"), &GameplayGraphicsOption::set_vsync_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vsync_mode"), &GameplayGraphicsOption::get_vsync_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "vsync_mode", godot::PROPERTY_HINT_ENUM, "Disabled,Enabled,Adaptive,Mailbox"),
        "set_vsync_mode",
        "get_vsync_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_fps", "max_fps"), &GameplayGraphicsOption::set_max_fps);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_fps"), &GameplayGraphicsOption::get_max_fps);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "max_fps", godot::PROPERTY_HINT_RANGE, "0,500,1"),
        "set_max_fps",
        "get_max_fps"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_resolution_scale", "scale"), &GameplayGraphicsOption::set_resolution_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resolution_scale"), &GameplayGraphicsOption::get_resolution_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "resolution_scale", godot::PROPERTY_HINT_RANGE, "0.25,2.0,0.01"),
        "set_resolution_scale",
        "get_resolution_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_resolution_scale_preset", "preset"), &GameplayGraphicsOption::set_resolution_scale_preset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resolution_scale_preset"), &GameplayGraphicsOption::get_resolution_scale_preset);
    godot::ClassDB::bind_static_method("GameplayGraphicsOption", godot::D_METHOD("preset_to_resolution_scale", "preset"), &GameplayGraphicsOption::preset_to_resolution_scale);
    godot::ClassDB::bind_static_method("GameplayGraphicsOption", godot::D_METHOD("resolution_scale_to_preset", "scale"), &GameplayGraphicsOption::resolution_scale_to_preset);

    godot::ClassDB::bind_method(godot::D_METHOD("set_scale_filter", "filter"), &GameplayGraphicsOption::set_scale_filter);
    godot::ClassDB::bind_method(godot::D_METHOD("get_scale_filter"), &GameplayGraphicsOption::get_scale_filter);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_scale_filter"), &GameplayGraphicsOption::get_effective_scale_filter);
    godot::ClassDB::bind_static_method("GameplayGraphicsOption", godot::D_METHOD("is_metalfx_supported"), &GameplayGraphicsOption::is_metalfx_supported);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "scale_filter", godot::PROPERTY_HINT_ENUM, "Bilinear,FSR 1.0,FSR 2.2,MetalFX Spatial,MetalFX Temporal,Nearest"),
        "set_scale_filter",
        "get_scale_filter"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_fsr_sharpness", "sharpness"), &GameplayGraphicsOption::set_fsr_sharpness);
    godot::ClassDB::bind_method(godot::D_METHOD("get_fsr_sharpness"), &GameplayGraphicsOption::get_fsr_sharpness);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "fsr_sharpness", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.05"),
        "set_fsr_sharpness",
        "get_fsr_sharpness"
    );

    // --- 2. Anti-Aliasing ---
    ADD_GROUP("Anti-Aliasing", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_taa_enabled", "enabled"), &GameplayGraphicsOption::set_taa_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_taa_enabled"), &GameplayGraphicsOption::get_taa_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "taa_enabled"), "set_taa_enabled", "get_taa_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_msaa_3d", "msaa"), &GameplayGraphicsOption::set_msaa_3d);
    godot::ClassDB::bind_method(godot::D_METHOD("get_msaa_3d"), &GameplayGraphicsOption::get_msaa_3d);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "msaa_3d", godot::PROPERTY_HINT_ENUM, "Disabled,2X,4X,8X"),
        "set_msaa_3d",
        "get_msaa_3d"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_screen_space_aa", "aa"), &GameplayGraphicsOption::set_screen_space_aa);
    godot::ClassDB::bind_method(godot::D_METHOD("get_screen_space_aa"), &GameplayGraphicsOption::get_screen_space_aa);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "screen_space_aa", godot::PROPERTY_HINT_ENUM, "Disabled,FXAA,SMAA"),
        "set_screen_space_aa",
        "get_screen_space_aa"
    );

    // --- 3. Lighting, GI & Shadows ---
    ADD_GROUP("Lighting, GI & Shadows", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_shadow_mapping_enabled", "enabled"), &GameplayGraphicsOption::set_shadow_mapping_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_shadow_mapping_enabled"), &GameplayGraphicsOption::get_shadow_mapping_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "shadow_mapping_enabled"), "set_shadow_mapping_enabled", "get_shadow_mapping_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_positional_shadow_atlas_size", "size"), &GameplayGraphicsOption::set_positional_shadow_atlas_size);
    godot::ClassDB::bind_method(godot::D_METHOD("get_positional_shadow_atlas_size"), &GameplayGraphicsOption::get_positional_shadow_atlas_size);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "positional_shadow_atlas_size", godot::PROPERTY_HINT_RANGE, "0,16384,512"),
        "set_positional_shadow_atlas_size",
        "get_positional_shadow_atlas_size"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_gi_type", "type"), &GameplayGraphicsOption::set_gi_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gi_type"), &GameplayGraphicsOption::get_gi_type);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "gi_type", godot::PROPERTY_HINT_ENUM, "SDFGI,VoxelGI,LightmapGI"),
        "set_gi_type",
        "get_gi_type"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_gi_quality", "quality"), &GameplayGraphicsOption::set_gi_quality);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gi_quality"), &GameplayGraphicsOption::get_gi_quality);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "gi_quality", godot::PROPERTY_HINT_ENUM, "Disabled,Low,High"),
        "set_gi_quality",
        "get_gi_quality"
    );

    // --- 4. Environment & Post-Processing ---
    ADD_GROUP("Environment & Post-Processing", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_ssao_quality", "quality"), &GameplayGraphicsOption::set_ssao_quality);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ssao_quality"), &GameplayGraphicsOption::get_ssao_quality);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "ssao_quality", godot::PROPERTY_HINT_ENUM, "Disabled:-1,Very Low:0,Low:1,Medium:2,High:3,Ultra:4"),
        "set_ssao_quality",
        "get_ssao_quality"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ssil_quality", "quality"), &GameplayGraphicsOption::set_ssil_quality);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ssil_quality"), &GameplayGraphicsOption::get_ssil_quality);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "ssil_quality", godot::PROPERTY_HINT_ENUM, "Disabled:-1,Very Low:0,Low:1,Medium:2,High:3,Ultra:4"),
        "set_ssil_quality",
        "get_ssil_quality"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ssr_quality", "quality"), &GameplayGraphicsOption::set_ssr_quality);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ssr_quality"), &GameplayGraphicsOption::get_ssr_quality);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "ssr_quality", godot::PROPERTY_HINT_ENUM, "Disabled:-1,Very Low:0,Low:1,Medium:2,High:3,Ultra:4"),
        "set_ssr_quality",
        "get_ssr_quality"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_bloom_enabled", "enabled"), &GameplayGraphicsOption::set_bloom_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_bloom_enabled"), &GameplayGraphicsOption::get_bloom_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "bloom_enabled"), "set_bloom_enabled", "get_bloom_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_volumetric_fog_enabled", "enabled"), &GameplayGraphicsOption::set_volumetric_fog_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_volumetric_fog_enabled"), &GameplayGraphicsOption::get_volumetric_fog_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "volumetric_fog_enabled"), "set_volumetric_fog_enabled", "get_volumetric_fog_enabled");

    // --- 5. LOD & Performance Budgets ---
    ADD_GROUP("LOD & Performance Budgets", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_mesh_lod_threshold", "threshold"), &GameplayGraphicsOption::set_mesh_lod_threshold);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mesh_lod_threshold"), &GameplayGraphicsOption::get_mesh_lod_threshold);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "mesh_lod_threshold", godot::PROPERTY_HINT_RANGE, "0.0,8.0,0.1"),
        "set_mesh_lod_threshold",
        "get_mesh_lod_threshold"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_debris_budget_multiplier", "multiplier"), &GameplayGraphicsOption::set_debris_budget_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_debris_budget_multiplier"), &GameplayGraphicsOption::get_debris_budget_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "debris_budget_multiplier", godot::PROPERTY_HINT_RANGE, "0.0,4.0,0.05"),
        "set_debris_budget_multiplier",
        "get_debris_budget_multiplier"
    );

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("apply_graphics_option", "other"), &GameplayGraphicsOption::apply_graphics_option);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_to_scene", "window", "environment", "scene_root"), &GameplayGraphicsOption::apply_to_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("save_to_tps_config", "config"), &GameplayGraphicsOption::save_to_tps_config);
    godot::ClassDB::bind_method(godot::D_METHOD("load_from_tps_config", "config"), &GameplayGraphicsOption::load_from_tps_config);
}

GameplayGraphicsOption::GameplayGraphicsOption() {
    set_category(godot::StringName("Graphics"));
    set_config_section("rendering");

    // Default to MetalFX Temporal on Metal driver, or FSR2 on Vulkan/D3D12/OpenGL
    if (is_metalfx_supported()) {
        scale_filter = godot::Viewport::SCALING_3D_MODE_METALFX_TEMPORAL;
    } else {
        scale_filter = godot::Viewport::SCALING_3D_MODE_FSR2;
    }
}

GameplayGraphicsOption::~GameplayGraphicsOption() {
}

// --- Hardware Capability Helpers ---

bool GameplayGraphicsOption::is_metalfx_supported() {
    godot::RenderingServer *rs = godot::RenderingServer::get_singleton();
    if (!rs) {
        return false;
    }
    return rs->get_current_rendering_driver_name() == "metal";
}

godot::Viewport::Scaling3DMode GameplayGraphicsOption::get_effective_scale_filter() const {
    bool metal_supported = is_metalfx_supported();
    if (!metal_supported) {
        if (scale_filter == godot::Viewport::SCALING_3D_MODE_METALFX_SPATIAL) {
            return godot::Viewport::SCALING_3D_MODE_FSR;
        }
        if (scale_filter == godot::Viewport::SCALING_3D_MODE_METALFX_TEMPORAL) {
            return godot::Viewport::SCALING_3D_MODE_FSR2;
        }
    }
    return scale_filter;
}

// --- 0. Quality Preset Setters / Getters ---

void GameplayGraphicsOption::set_quality_preset(GraphicsQualityPreset p_preset) {
    if (quality_preset == p_preset) return;
    quality_preset = p_preset;
    if (p_preset != GraphicsQualityPreset::QUALITY_CUSTOM) {
        apply_quality_preset(p_preset);
    } else {
        emit_changed();
    }
}

GraphicsQualityPreset GameplayGraphicsOption::get_quality_preset() const {
    return quality_preset;
}

void GameplayGraphicsOption::apply_quality_preset(GraphicsQualityPreset p_preset) {
    quality_preset = p_preset;

    switch (p_preset) {
        case GraphicsQualityPreset::QUALITY_LOW: {
            resolution_scale = 1.0f / 1.7f; // Balanced upscaling
            taa_enabled = false;
            msaa_3d = godot::Viewport::MSAA_DISABLED;
            screen_space_aa = godot::Viewport::SCREEN_SPACE_AA_FXAA;
            shadow_mapping_enabled = false;
            positional_shadow_atlas_size = 1024;
            gi_quality = GraphicsGIQuality::GI_QUALITY_DISABLED;
            ssao_quality = GraphicsEffectQuality::EFFECT_DISABLED;
            ssil_quality = GraphicsEffectQuality::EFFECT_DISABLED;
            ssr_quality = GraphicsEffectQuality::EFFECT_DISABLED;
            bloom_enabled = false;
            volumetric_fog_enabled = false;
            mesh_lod_threshold = 2.0f;
            debris_budget_multiplier = 0.25f;
            break;
        }
        case GraphicsQualityPreset::QUALITY_MEDIUM: {
            // Matches the default TPS Demo settings in `settings.gd`
            resolution_scale = 1.0f;
            taa_enabled = false;
            msaa_3d = godot::Viewport::MSAA_DISABLED;
            screen_space_aa = godot::Viewport::SCREEN_SPACE_AA_DISABLED;
            shadow_mapping_enabled = true;
            positional_shadow_atlas_size = 4096;
            gi_type = GraphicsGIType::GI_TYPE_VOXEL_GI;
            gi_quality = GraphicsGIQuality::GI_QUALITY_LOW;
            ssao_quality = GraphicsEffectQuality::EFFECT_MEDIUM;
            ssil_quality = GraphicsEffectQuality::EFFECT_DISABLED;
            ssr_quality = GraphicsEffectQuality::EFFECT_DISABLED;
            bloom_enabled = true;
            volumetric_fog_enabled = true;
            mesh_lod_threshold = 1.0f;
            debris_budget_multiplier = 1.0f;
            break;
        }
        case GraphicsQualityPreset::QUALITY_HIGH: {
            resolution_scale = 1.0f;
            taa_enabled = true;
            msaa_3d = godot::Viewport::MSAA_2X;
            screen_space_aa = godot::Viewport::SCREEN_SPACE_AA_SMAA;
            shadow_mapping_enabled = true;
            positional_shadow_atlas_size = 4096;
            gi_type = GraphicsGIType::GI_TYPE_VOXEL_GI;
            gi_quality = GraphicsGIQuality::GI_QUALITY_HIGH;
            ssao_quality = GraphicsEffectQuality::EFFECT_HIGH;
            ssil_quality = GraphicsEffectQuality::EFFECT_MEDIUM;
            ssr_quality = GraphicsEffectQuality::EFFECT_MEDIUM;
            bloom_enabled = true;
            volumetric_fog_enabled = true;
            mesh_lod_threshold = 0.5f;
            debris_budget_multiplier = 1.5f;
            break;
        }
        case GraphicsQualityPreset::QUALITY_ULTRA: {
            resolution_scale = 1.0f;
            taa_enabled = true;
            msaa_3d = godot::Viewport::MSAA_4X;
            screen_space_aa = godot::Viewport::SCREEN_SPACE_AA_SMAA;
            shadow_mapping_enabled = true;
            positional_shadow_atlas_size = 8192;
            gi_type = GraphicsGIType::GI_TYPE_SDFGI;
            gi_quality = GraphicsGIQuality::GI_QUALITY_HIGH;
            ssao_quality = GraphicsEffectQuality::EFFECT_ULTRA;
            ssil_quality = GraphicsEffectQuality::EFFECT_HIGH;
            ssr_quality = GraphicsEffectQuality::EFFECT_HIGH;
            bloom_enabled = true;
            volumetric_fog_enabled = true;
            mesh_lod_threshold = 0.25f;
            debris_budget_multiplier = 2.0f;
            break;
        }
        case GraphicsQualityPreset::QUALITY_CUSTOM:
        default:
            break;
    }

    emit_changed();
}

// --- 1. Window & Display Setters / Getters ---

void GameplayGraphicsOption::set_display_mode(godot::Window::Mode p_mode) {
    if (display_mode == p_mode) return;
    display_mode = p_mode;
    emit_changed();
}
godot::Window::Mode GameplayGraphicsOption::get_display_mode() const { return display_mode; }

void GameplayGraphicsOption::toggle_fullscreen(godot::Window *p_window) {
    bool is_currently_fullscreen =
        (display_mode == godot::Window::MODE_EXCLUSIVE_FULLSCREEN) ||
        (display_mode == godot::Window::MODE_FULLSCREEN);

    if (p_window) {
        godot::Window::Mode win_mode = p_window->get_mode();
        is_currently_fullscreen =
            (win_mode == godot::Window::MODE_EXCLUSIVE_FULLSCREEN) ||
            (win_mode == godot::Window::MODE_FULLSCREEN);
    }

    set_display_mode(is_currently_fullscreen ? godot::Window::MODE_WINDOWED : godot::Window::MODE_EXCLUSIVE_FULLSCREEN);
    if (p_window) {
        p_window->set_mode(display_mode);
    }
}

void GameplayGraphicsOption::set_vsync_mode(godot::DisplayServer::VSyncMode p_mode) {
    if (vsync_mode == p_mode) return;
    vsync_mode = p_mode;
    emit_changed();
}
godot::DisplayServer::VSyncMode GameplayGraphicsOption::get_vsync_mode() const { return vsync_mode; }

void GameplayGraphicsOption::set_max_fps(int p_max_fps) {
    int clamped = godot::Math::max(0, p_max_fps);
    if (max_fps == clamped) return;
    max_fps = clamped;
    emit_changed();
}
int GameplayGraphicsOption::get_max_fps() const { return max_fps; }

void GameplayGraphicsOption::set_resolution_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.25f, 2.0f);
    if (godot::Math::is_equal_approx(resolution_scale, clamped)) return;
    resolution_scale = clamped;
    emit_changed();
}
float GameplayGraphicsOption::get_resolution_scale() const { return resolution_scale; }

void GameplayGraphicsOption::set_resolution_scale_preset(ResolutionScalePreset p_preset) {
    if (p_preset == ResolutionScalePreset::RES_SCALE_CUSTOM) return;
    set_resolution_scale(preset_to_resolution_scale(p_preset));
}

ResolutionScalePreset GameplayGraphicsOption::get_resolution_scale_preset() const {
    return resolution_scale_to_preset(resolution_scale);
}

float GameplayGraphicsOption::preset_to_resolution_scale(ResolutionScalePreset p_preset) {
    switch (p_preset) {
        case ResolutionScalePreset::RES_SCALE_ULTRA_PERFORMANCE:
            return 1.0f / 3.0f;
        case ResolutionScalePreset::RES_SCALE_PERFORMANCE:
            return 1.0f / 2.0f;
        case ResolutionScalePreset::RES_SCALE_BALANCED:
            return 1.0f / 1.7f;
        case ResolutionScalePreset::RES_SCALE_QUALITY:
            return 1.0f / 1.5f;
        case ResolutionScalePreset::RES_SCALE_ULTRA_QUALITY:
            return 1.0f / 1.3f;
        case ResolutionScalePreset::RES_SCALE_NATIVE:
        default:
            return 1.0f;
    }
}

ResolutionScalePreset GameplayGraphicsOption::resolution_scale_to_preset(float p_scale) {
    if (godot::Math::is_equal_approx(p_scale, 1.0f / 3.0f)) return ResolutionScalePreset::RES_SCALE_ULTRA_PERFORMANCE;
    if (godot::Math::is_equal_approx(p_scale, 1.0f / 2.0f)) return ResolutionScalePreset::RES_SCALE_PERFORMANCE;
    if (godot::Math::is_equal_approx(p_scale, 1.0f / 1.7f)) return ResolutionScalePreset::RES_SCALE_BALANCED;
    if (godot::Math::is_equal_approx(p_scale, 1.0f / 1.5f)) return ResolutionScalePreset::RES_SCALE_QUALITY;
    if (godot::Math::is_equal_approx(p_scale, 1.0f / 1.3f)) return ResolutionScalePreset::RES_SCALE_ULTRA_QUALITY;
    if (godot::Math::is_equal_approx(p_scale, 1.0f)) return ResolutionScalePreset::RES_SCALE_NATIVE;
    return ResolutionScalePreset::RES_SCALE_CUSTOM;
}

void GameplayGraphicsOption::set_scale_filter(godot::Viewport::Scaling3DMode p_filter) {
    if (scale_filter == p_filter) return;
    scale_filter = p_filter;
    emit_changed();
}
godot::Viewport::Scaling3DMode GameplayGraphicsOption::get_scale_filter() const { return scale_filter; }

void GameplayGraphicsOption::set_fsr_sharpness(float p_sharpness) {
    float clamped = godot::Math::clamp(p_sharpness, 0.0f, 2.0f);
    if (godot::Math::is_equal_approx(fsr_sharpness, clamped)) return;
    fsr_sharpness = clamped;
    emit_changed();
}
float GameplayGraphicsOption::get_fsr_sharpness() const { return fsr_sharpness; }

// --- 2. Anti-Aliasing Setters / Getters ---

void GameplayGraphicsOption::set_taa_enabled(bool p_enabled) {
    if (taa_enabled == p_enabled) return;
    taa_enabled = p_enabled;
    emit_changed();
}
bool GameplayGraphicsOption::get_taa_enabled() const { return taa_enabled; }

void GameplayGraphicsOption::set_msaa_3d(godot::Viewport::MSAA p_msaa) {
    if (msaa_3d == p_msaa) return;
    msaa_3d = p_msaa;
    emit_changed();
}
godot::Viewport::MSAA GameplayGraphicsOption::get_msaa_3d() const { return msaa_3d; }

void GameplayGraphicsOption::set_screen_space_aa(godot::Viewport::ScreenSpaceAA p_aa) {
    if (screen_space_aa == p_aa) return;
    screen_space_aa = p_aa;
    emit_changed();
}
godot::Viewport::ScreenSpaceAA GameplayGraphicsOption::get_screen_space_aa() const { return screen_space_aa; }

// --- 3. Lighting, GI & Shadows Setters / Getters ---

void GameplayGraphicsOption::set_shadow_mapping_enabled(bool p_enabled) {
    if (shadow_mapping_enabled == p_enabled) return;
    shadow_mapping_enabled = p_enabled;
    emit_changed();
}
bool GameplayGraphicsOption::get_shadow_mapping_enabled() const { return shadow_mapping_enabled; }

void GameplayGraphicsOption::set_positional_shadow_atlas_size(int p_size) {
    int clamped = godot::Math::clamp(p_size, 0, 16384);
    if (positional_shadow_atlas_size == clamped) return;
    positional_shadow_atlas_size = clamped;
    emit_changed();
}
int GameplayGraphicsOption::get_positional_shadow_atlas_size() const { return positional_shadow_atlas_size; }

void GameplayGraphicsOption::set_gi_type(GraphicsGIType p_type) {
    if (gi_type == p_type) return;
    gi_type = p_type;
    emit_changed();
}
GraphicsGIType GameplayGraphicsOption::get_gi_type() const { return gi_type; }

void GameplayGraphicsOption::set_gi_quality(GraphicsGIQuality p_quality) {
    if (gi_quality == p_quality) return;
    gi_quality = p_quality;
    emit_changed();
}
GraphicsGIQuality GameplayGraphicsOption::get_gi_quality() const { return gi_quality; }

// --- 4. Environment & Post-Processing Setters / Getters ---

void GameplayGraphicsOption::set_ssao_quality(GraphicsEffectQuality p_quality) {
    if (ssao_quality == p_quality) return;
    ssao_quality = p_quality;
    emit_changed();
}
GraphicsEffectQuality GameplayGraphicsOption::get_ssao_quality() const { return ssao_quality; }

void GameplayGraphicsOption::set_ssil_quality(GraphicsEffectQuality p_quality) {
    if (ssil_quality == p_quality) return;
    ssil_quality = p_quality;
    emit_changed();
}
GraphicsEffectQuality GameplayGraphicsOption::get_ssil_quality() const { return ssil_quality; }

void GameplayGraphicsOption::set_ssr_quality(GraphicsEffectQuality p_quality) {
    if (ssr_quality == p_quality) return;
    ssr_quality = p_quality;
    emit_changed();
}
GraphicsEffectQuality GameplayGraphicsOption::get_ssr_quality() const { return ssr_quality; }

void GameplayGraphicsOption::set_bloom_enabled(bool p_enabled) {
    if (bloom_enabled == p_enabled) return;
    bloom_enabled = p_enabled;
    emit_changed();
}
bool GameplayGraphicsOption::get_bloom_enabled() const { return bloom_enabled; }

void GameplayGraphicsOption::set_volumetric_fog_enabled(bool p_enabled) {
    if (volumetric_fog_enabled == p_enabled) return;
    volumetric_fog_enabled = p_enabled;
    emit_changed();
}
bool GameplayGraphicsOption::get_volumetric_fog_enabled() const { return volumetric_fog_enabled; }

// --- 5. LOD & Performance Budgets Setters / Getters ---

void GameplayGraphicsOption::set_mesh_lod_threshold(float p_threshold) {
    float clamped = godot::Math::clamp(p_threshold, 0.0f, 8.0f);
    if (godot::Math::is_equal_approx(mesh_lod_threshold, clamped)) return;
    mesh_lod_threshold = clamped;
    emit_changed();
}
float GameplayGraphicsOption::get_mesh_lod_threshold() const { return mesh_lod_threshold; }

void GameplayGraphicsOption::set_debris_budget_multiplier(float p_multiplier) {
    float clamped = godot::Math::clamp(p_multiplier, 0.0f, 4.0f);
    if (godot::Math::is_equal_approx(debris_budget_multiplier, clamped)) return;
    debris_budget_multiplier = clamped;
    emit_changed();
}
float GameplayGraphicsOption::get_debris_budget_multiplier() const { return debris_budget_multiplier; }

// --- Internal Scene Traversal Helpers ---

void GameplayGraphicsOption::_apply_shadow_and_gi_recursive(godot::Node *p_node) const {
    if (!p_node) {
        return;
    }

    // 1. Non-Destructive Light3D Shadow Toggle (Fixes TPS Demo `settings.gd` FIXME bug)
    if (godot::Light3D *light = godot::Object::cast_to<godot::Light3D>(p_node)) {
        const godot::StringName meta_key("_ideam_orig_shadow_enabled");
        if (!light->has_meta(meta_key)) {
            light->set_meta(meta_key, light->has_shadow());
        }

        if (!shadow_mapping_enabled) {
            light->set_shadow(false);
        } else {
            bool orig_shadow = static_cast<bool>(light->get_meta(meta_key));
            light->set_shadow(orig_shadow);
        }
    }

    // 2. Toggle VoxelGI and LightmapGI Nodes based on active `gi_type` and `gi_quality`
    if (godot::VoxelGI *voxel_gi = godot::Object::cast_to<godot::VoxelGI>(p_node)) {
        bool should_show = (gi_quality != GraphicsGIQuality::GI_QUALITY_DISABLED) &&
                           (gi_type == GraphicsGIType::GI_TYPE_VOXEL_GI);
        voxel_gi->set_visible(should_show);
    } else if (godot::LightmapGI *lightmap_gi = godot::Object::cast_to<godot::LightmapGI>(p_node)) {
        bool should_show = (gi_quality != GraphicsGIQuality::GI_QUALITY_DISABLED) &&
                           (gi_type == GraphicsGIType::GI_TYPE_LIGHTMAP_GI);
        lightmap_gi->set_visible(should_show);
    }

    int child_count = p_node->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        _apply_shadow_and_gi_recursive(p_node->get_child(i));
    }
}

// --- Application, Rule Synthesis & Persistence ---

void GameplayGraphicsOption::apply_graphics_option(const godot::Ref<GameplayGraphicsOption> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }

    GameplayResource::apply_resource(p_other);

    quality_preset = p_other->get_quality_preset();
    display_mode = p_other->get_display_mode();
    vsync_mode = p_other->get_vsync_mode();
    max_fps = p_other->get_max_fps();
    resolution_scale = p_other->get_resolution_scale();
    scale_filter = p_other->get_scale_filter();
    fsr_sharpness = p_other->get_fsr_sharpness();

    taa_enabled = p_other->get_taa_enabled();
    msaa_3d = p_other->get_msaa_3d();
    screen_space_aa = p_other->get_screen_space_aa();

    shadow_mapping_enabled = p_other->get_shadow_mapping_enabled();
    positional_shadow_atlas_size = p_other->get_positional_shadow_atlas_size();
    gi_type = p_other->get_gi_type();
    gi_quality = p_other->get_gi_quality();

    ssao_quality = p_other->get_ssao_quality();
    ssil_quality = p_other->get_ssil_quality();
    ssr_quality = p_other->get_ssr_quality();
    bloom_enabled = p_other->get_bloom_enabled();
    volumetric_fog_enabled = p_other->get_volumetric_fog_enabled();

    mesh_lod_threshold = p_other->get_mesh_lod_threshold();
    debris_budget_multiplier = p_other->get_debris_budget_multiplier();

    emit_changed();
}

void GameplayGraphicsOption::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    godot::Ref<GameplayGraphicsOption> gfx_other = p_other;
    if (gfx_other.is_valid()) {
        apply_graphics_option(gfx_other);
    } else {
        GameplayResource::apply_resource(p_other);
    }
}

void GameplayGraphicsOption::apply_to_scene(godot::Window *p_window, godot::Environment *p_environment, godot::Node *p_scene_root) {
    if (!get_enabled()) {
        return;
    }

    // 1. DisplayServer & Engine Frame Pacing
    if (godot::DisplayServer *ds = godot::DisplayServer::get_singleton()) {
        ds->window_set_vsync_mode(vsync_mode);
    }
    if (godot::Engine *eng = godot::Engine::get_singleton()) {
        eng->set_max_fps(max_fps);
    }

    // 2. Window & Viewport 3D Scaling, Anti-Aliasing, Shadow Atlas, and Mesh LOD
    if (p_window) {
        p_window->set_mode(display_mode);
        p_window->set_scaling_3d_scale(resolution_scale);
        p_window->set_scaling_3d_mode(get_effective_scale_filter());
        p_window->set_fsr_sharpness(fsr_sharpness);

        p_window->set_use_taa(taa_enabled);
        p_window->set_msaa_3d(msaa_3d);
        p_window->set_screen_space_aa(screen_space_aa);

        p_window->set_positional_shadow_atlas_size(shadow_mapping_enabled ? positional_shadow_atlas_size : 0);
        p_window->set_mesh_lod_threshold(mesh_lod_threshold);
    }

    // 3. RenderingServer & Environment Settings (Fixing the inverted SSAO branch in `settings.gd`)
    godot::RenderingServer *rs = godot::RenderingServer::get_singleton();
    if (rs) {
        rs->voxel_gi_set_quality(
            gi_quality == GraphicsGIQuality::GI_QUALITY_HIGH
                ? godot::RenderingServer::VOXEL_GI_QUALITY_HIGH
                : godot::RenderingServer::VOXEL_GI_QUALITY_LOW
        );
    }

    if (p_environment) {
        // Global Illumination (SDFGI)
        bool sdfgi_active = (gi_quality != GraphicsGIQuality::GI_QUALITY_DISABLED) &&
                            (gi_type == GraphicsGIType::GI_TYPE_SDFGI);
        p_environment->set_sdfgi_enabled(sdfgi_active);

        // SSAO (Fixed: MEDIUM maps to ENV_SSAO_QUALITY_MEDIUM, HIGH maps to ENV_SSAO_QUALITY_HIGH)
        if (ssao_quality == GraphicsEffectQuality::EFFECT_DISABLED) {
            p_environment->set_ssao_enabled(false);
        } else {
            p_environment->set_ssao_enabled(true);
            if (rs) {
                auto rs_ssao = static_cast<godot::RenderingServer::EnvironmentSSAOQuality>(static_cast<int>(ssao_quality));
                bool half_size = (ssao_quality <= GraphicsEffectQuality::EFFECT_LOW);
                rs->environment_set_ssao_quality(rs_ssao, half_size, 0.5f, 2, 50.0f, 300.0f);
            }
        }

        // SSIL
        if (ssil_quality == GraphicsEffectQuality::EFFECT_DISABLED) {
            p_environment->set_ssil_enabled(false);
        } else {
            p_environment->set_ssil_enabled(true);
            if (rs) {
                auto rs_ssil = static_cast<godot::RenderingServer::EnvironmentSSILQuality>(static_cast<int>(ssil_quality));
                bool half_size = (ssil_quality <= GraphicsEffectQuality::EFFECT_LOW);
                rs->environment_set_ssil_quality(rs_ssil, half_size, 0.5f, 2, 50.0f, 300.0f);
            }
        }

        // SSR
        if (ssr_quality == GraphicsEffectQuality::EFFECT_DISABLED) {
            p_environment->set_ssr_enabled(false);
        } else {
            p_environment->set_ssr_enabled(true);
            int max_steps = 32;
            if (ssr_quality <= GraphicsEffectQuality::EFFECT_LOW) max_steps = 16;
            else if (ssr_quality == GraphicsEffectQuality::EFFECT_HIGH) max_steps = 64;
            else if (ssr_quality >= GraphicsEffectQuality::EFFECT_ULTRA) max_steps = 128;
            p_environment->set_ssr_max_steps(max_steps);
        }

        // Bloom & Volumetric Fog
        p_environment->set_glow_enabled(bloom_enabled);
        p_environment->set_volumetric_fog_enabled(volumetric_fog_enabled);
    }

    // 4. Non-Destructive Scene Light3D Shadow & GI Node Visibility Traversal
    if (p_scene_root) {
        _apply_shadow_and_gi_recursive(p_scene_root);
    }
}

godot::TypedArray<GameplayRule> GameplayGraphicsOption::collect_rules() const {
    godot::TypedArray<GameplayRule> collected = GameplayResource::collect_rules();
    if (!get_enabled()) {
        return collected;
    }

    // Synthesize a cascading rule for debris/particle budget scaling on GameBoards
    if (!godot::Math::is_equal_approx(debris_budget_multiplier, 1.0f)) {
        godot::Ref<GameplayRule> debris_rule;
        debris_rule.instantiate();
        debris_rule->set_title("Graphics_DebrisBudgetScale");
        debris_rule->set_scope(RuleScope::DESCENDANTS);
        debris_rule->set_target_class(godot::StringName("GameEntity"));
        debris_rule->set_target_property(godot::StringName("DebrisBudget"));
        debris_rule->set_operation(RuleOp::MULTIPLY);
        debris_rule->set_value(debris_budget_multiplier);
        debris_rule->set_priority(50);
        collected.append(debris_rule);
    }

    return collected;
}

bool GameplayGraphicsOption::save_to_tps_config(const godot::Ref<godot::ConfigFile> &p_config) const {
    if (!p_config.is_valid()) {
        return false;
    }

    // `[video]` section matching `settings.gd`
    p_config->set_value("video", "display_mode", static_cast<int>(display_mode));
    p_config->set_value("video", "vsync", static_cast<int>(vsync_mode));
    p_config->set_value("video", "max_fps", max_fps);
    p_config->set_value("video", "resolution_scale", resolution_scale);
    p_config->set_value("video", "scale_filter", static_cast<int>(scale_filter));
    p_config->set_value("video", "fsr_sharpness", fsr_sharpness);

    // `[rendering]` section matching `settings.gd`
    p_config->set_value("rendering", "quality_preset", static_cast<int>(quality_preset));
    p_config->set_value("rendering", "taa", taa_enabled);
    p_config->set_value("rendering", "msaa", static_cast<int>(msaa_3d));
    p_config->set_value("rendering", "screen_space_aa", static_cast<int>(screen_space_aa));
    p_config->set_value("rendering", "shadow_mapping", shadow_mapping_enabled);
    p_config->set_value("rendering", "positional_shadow_atlas_size", positional_shadow_atlas_size);
    p_config->set_value("rendering", "gi_type", static_cast<int>(gi_type));
    p_config->set_value("rendering", "gi_quality", static_cast<int>(gi_quality));
    p_config->set_value("rendering", "ssao_quality", static_cast<int>(ssao_quality));
    p_config->set_value("rendering", "ssil_quality", static_cast<int>(ssil_quality));
    p_config->set_value("rendering", "ssr_quality", static_cast<int>(ssr_quality));
    p_config->set_value("rendering", "bloom", bloom_enabled);
    p_config->set_value("rendering", "volumetric_fog", volumetric_fog_enabled);
    p_config->set_value("rendering", "mesh_lod_threshold", mesh_lod_threshold);
    p_config->set_value("rendering", "debris_budget_multiplier", debris_budget_multiplier);

    return true;
}

bool GameplayGraphicsOption::load_from_tps_config(const godot::Ref<godot::ConfigFile> &p_config) {
    if (!p_config.is_valid()) {
        return false;
    }

    bool loaded_any = false;

    if (p_config->has_section("video")) {
        loaded_any = true;
        if (p_config->has_section_key("video", "display_mode")) {
            display_mode = static_cast<godot::Window::Mode>(static_cast<int>(p_config->get_value("video", "display_mode")));
        }
        if (p_config->has_section_key("video", "vsync")) {
            vsync_mode = static_cast<godot::DisplayServer::VSyncMode>(static_cast<int>(p_config->get_value("video", "vsync")));
        }
        if (p_config->has_section_key("video", "max_fps")) {
            set_max_fps(p_config->get_value("video", "max_fps"));
        }
        if (p_config->has_section_key("video", "resolution_scale")) {
            set_resolution_scale(p_config->get_value("video", "resolution_scale"));
        }
        if (p_config->has_section_key("video", "scale_filter")) {
            scale_filter = static_cast<godot::Viewport::Scaling3DMode>(static_cast<int>(p_config->get_value("video", "scale_filter")));
        }
        if (p_config->has_section_key("video", "fsr_sharpness")) {
            set_fsr_sharpness(p_config->get_value("video", "fsr_sharpness"));
        }
    }

    if (p_config->has_section("rendering")) {
        loaded_any = true;
        if (p_config->has_section_key("rendering", "quality_preset")) {
            quality_preset = static_cast<GraphicsQualityPreset>(static_cast<int>(p_config->get_value("rendering", "quality_preset")));
        }
        if (p_config->has_section_key("rendering", "taa")) {
            taa_enabled = p_config->get_value("rendering", "taa");
        }
        if (p_config->has_section_key("rendering", "msaa")) {
            msaa_3d = static_cast<godot::Viewport::MSAA>(static_cast<int>(p_config->get_value("rendering", "msaa")));
        }
        if (p_config->has_section_key("rendering", "screen_space_aa")) {
            screen_space_aa = static_cast<godot::Viewport::ScreenSpaceAA>(static_cast<int>(p_config->get_value("rendering", "screen_space_aa")));
        }
        if (p_config->has_section_key("rendering", "shadow_mapping")) {
            shadow_mapping_enabled = p_config->get_value("rendering", "shadow_mapping");
        }
        if (p_config->has_section_key("rendering", "positional_shadow_atlas_size")) {
            set_positional_shadow_atlas_size(p_config->get_value("rendering", "positional_shadow_atlas_size"));
        }
        if (p_config->has_section_key("rendering", "gi_type")) {
            gi_type = static_cast<GraphicsGIType>(static_cast<int>(p_config->get_value("rendering", "gi_type")));
        }
        if (p_config->has_section_key("rendering", "gi_quality")) {
            gi_quality = static_cast<GraphicsGIQuality>(static_cast<int>(p_config->get_value("rendering", "gi_quality")));
        }
        if (p_config->has_section_key("rendering", "ssao_quality")) {
            ssao_quality = static_cast<GraphicsEffectQuality>(static_cast<int>(p_config->get_value("rendering", "ssao_quality")));
        }
        if (p_config->has_section_key("rendering", "ssil_quality")) {
            ssil_quality = static_cast<GraphicsEffectQuality>(static_cast<int>(p_config->get_value("rendering", "ssil_quality")));
        }
        if (p_config->has_section_key("rendering", "ssr_quality")) {
            ssr_quality = static_cast<GraphicsEffectQuality>(static_cast<int>(p_config->get_value("rendering", "ssr_quality")));
        }
        if (p_config->has_section_key("rendering", "bloom")) {
            bloom_enabled = p_config->get_value("rendering", "bloom");
        }
        if (p_config->has_section_key("rendering", "volumetric_fog")) {
            volumetric_fog_enabled = p_config->get_value("rendering", "volumetric_fog");
        }
        if (p_config->has_section_key("rendering", "mesh_lod_threshold")) {
            set_mesh_lod_threshold(p_config->get_value("rendering", "mesh_lod_threshold"));
        }
        if (p_config->has_section_key("rendering", "debris_budget_multiplier")) {
            set_debris_budget_multiplier(p_config->get_value("rendering", "debris_budget_multiplier"));
        }
    }

    if (loaded_any) {
        emit_changed();
    }
    return loaded_any;
}

godot::Dictionary GameplayGraphicsOption::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();

    data["quality_preset"] = static_cast<int>(quality_preset);
    data["display_mode"] = static_cast<int>(display_mode);
    data["vsync"] = static_cast<int>(vsync_mode);
    data["max_fps"] = max_fps;
    data["resolution_scale"] = resolution_scale;
    data["scale_filter"] = static_cast<int>(scale_filter);
    data["fsr_sharpness"] = fsr_sharpness;

    data["taa"] = taa_enabled;
    data["msaa"] = static_cast<int>(msaa_3d);
    data["screen_space_aa"] = static_cast<int>(screen_space_aa);

    data["shadow_mapping"] = shadow_mapping_enabled;
    data["positional_shadow_atlas_size"] = positional_shadow_atlas_size;
    data["gi_type"] = static_cast<int>(gi_type);
    data["gi_quality"] = static_cast<int>(gi_quality);

    data["ssao_quality"] = static_cast<int>(ssao_quality);
    data["ssil_quality"] = static_cast<int>(ssil_quality);
    data["ssr_quality"] = static_cast<int>(ssr_quality);
    data["bloom"] = bloom_enabled;
    data["volumetric_fog"] = volumetric_fog_enabled;

    data["mesh_lod_threshold"] = mesh_lod_threshold;
    data["debris_budget_multiplier"] = debris_budget_multiplier;

    return data;
}

void GameplayGraphicsOption::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("quality_preset")) quality_preset = static_cast<GraphicsQualityPreset>(static_cast<int>(p_data["quality_preset"]));
    if (p_data.has("display_mode")) display_mode = static_cast<godot::Window::Mode>(static_cast<int>(p_data["display_mode"]));
    if (p_data.has("vsync")) vsync_mode = static_cast<godot::DisplayServer::VSyncMode>(static_cast<int>(p_data["vsync"]));
    if (p_data.has("max_fps")) set_max_fps(p_data["max_fps"]);
    if (p_data.has("resolution_scale")) set_resolution_scale(p_data["resolution_scale"]);
    if (p_data.has("scale_filter")) scale_filter = static_cast<godot::Viewport::Scaling3DMode>(static_cast<int>(p_data["scale_filter"]));
    if (p_data.has("fsr_sharpness")) set_fsr_sharpness(p_data["fsr_sharpness"]);

    if (p_data.has("taa")) taa_enabled = p_data["taa"];
    if (p_data.has("msaa")) msaa_3d = static_cast<godot::Viewport::MSAA>(static_cast<int>(p_data["msaa"]));
    if (p_data.has("screen_space_aa")) screen_space_aa = static_cast<godot::Viewport::ScreenSpaceAA>(static_cast<int>(p_data["screen_space_aa"]));

    if (p_data.has("shadow_mapping")) shadow_mapping_enabled = p_data["shadow_mapping"];
    if (p_data.has("positional_shadow_atlas_size")) set_positional_shadow_atlas_size(p_data["positional_shadow_atlas_size"]);
    if (p_data.has("gi_type")) gi_type = static_cast<GraphicsGIType>(static_cast<int>(p_data["gi_type"]));
    if (p_data.has("gi_quality")) gi_quality = static_cast<GraphicsGIQuality>(static_cast<int>(p_data["gi_quality"]));

    if (p_data.has("ssao_quality")) ssao_quality = static_cast<GraphicsEffectQuality>(static_cast<int>(p_data["ssao_quality"]));
    if (p_data.has("ssil_quality")) ssil_quality = static_cast<GraphicsEffectQuality>(static_cast<int>(p_data["ssil_quality"]));
    if (p_data.has("ssr_quality")) ssr_quality = static_cast<GraphicsEffectQuality>(static_cast<int>(p_data["ssr_quality"]));
    if (p_data.has("bloom")) bloom_enabled = p_data["bloom"];
    if (p_data.has("volumetric_fog")) volumetric_fog_enabled = p_data["volumetric_fog"];

    if (p_data.has("mesh_lod_threshold")) set_mesh_lod_threshold(p_data["mesh_lod_threshold"]);
    if (p_data.has("debris_budget_multiplier")) set_debris_budget_multiplier(p_data["debris_budget_multiplier"]);

    emit_changed();
}

} // namespace ideam::godot_ext
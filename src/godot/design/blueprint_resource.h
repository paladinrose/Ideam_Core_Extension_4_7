#pragma once

/**
 * blueprint_resource.h
 * `BlueprintResource` and `BlueprintCanvas2D` wrapping `ArchitecturalBlueprintCore`
 * (`src/godot/design/`).
 */

#include "../../core/design/blueprint_core.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

namespace ideam::godot_ext {

class BlueprintResource : public godot::Resource {
    GDCLASS(BlueprintResource, godot::Resource)

public:
    enum LayerIndex {
        LAYER_SPACES                    = 0,
        LAYER_WALLS                     = 1,
        LAYER_FEATURES_AND_APERTURES    = 2,
        LAYER_LABELS_AND_MEASUREMENTS   = 3
    };

    enum FeatureSymbolType {
        SYMBOL_DOOR_SINGLE  = 0,
        SYMBOL_DOOR_DOUBLE  = 1,
        SYMBOL_WINDOW_SASH  = 2,
        SYMBOL_ARCHWAY      = 3,
        SYMBOL_STAIR_RUN    = 4,
        SYMBOL_FIXTURE_PROP = 5
    };

private:
    float section_cut_height_meters_ = 1.2192f;
    core::ArchitecturalBlueprintCore core_blueprint_;

protected:
    static void _bind_methods();

public:
    BlueprintResource() = default;
    ~BlueprintResource() override = default;

    void set_section_cut_height_meters(float p_height);
    [[nodiscard]] float get_section_cut_height_meters() const noexcept { return section_cut_height_meters_; }

    void commit_set_section_cut_height_meters(
        float p_new_height,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );

    int add_space_polygon(
        const godot::String &p_room_name,
        const godot::PackedVector2Array &p_vertices_2d,
        int p_semantic_role_id = static_cast<int>(core::SEMANTIC_ROLE_ASSEMBLY_ROOM),
        int p_nuclear_seme_mask = 0,
        float p_floor_elevation = 0.0f,
        float p_ceiling_height = 3.0f
    );

    int add_feature_symbol(
        int p_symbol_type,
        const godot::Vector2 &p_position_2d,
        float p_width_meters = 0.90f,
        float p_height_meters = 2.10f,
        float p_sill_elevation = 0.0f,
        const godot::String &p_map_key_label = ""
    );

    void rebuild_reactive_layers();

    [[nodiscard]] int get_space_count() const noexcept;
    [[nodiscard]] int get_wall_count() const noexcept;
    [[nodiscard]] int get_feature_count() const noexcept;
    [[nodiscard]] int get_annotation_count() const noexcept;

    [[nodiscard]] godot::Dictionary get_layer_style_info(int p_layer_index, bool p_is_exterior_wall = true) const;
    [[nodiscard]] godot::Array get_map_key_summary() const;
    [[nodiscard]] godot::Dictionary extrude_to_3d_layout_summary() const;

    [[nodiscard]] const core::ArchitecturalBlueprintCore &get_core_blueprint() const noexcept { return core_blueprint_; }
    [[nodiscard]] core::ArchitecturalBlueprintCore &get_core_blueprint_mut() noexcept { return core_blueprint_; }
};

class BlueprintCanvas2D : public godot::Node2D {
    GDCLASS(BlueprintCanvas2D, godot::Node2D)

private:
    godot::Ref<BlueprintResource> blueprint_;
    float pixels_per_meter_     = 32.0f;
    uint32_t visible_layer_mask_ = 0x0Fu;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    BlueprintCanvas2D() = default;
    ~BlueprintCanvas2D() override = default;

    void set_blueprint(const godot::Ref<BlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<BlueprintResource> get_blueprint() const { return blueprint_; }

    void set_pixels_per_meter(float p_ppm);
    [[nodiscard]] float get_pixels_per_meter() const noexcept { return pixels_per_meter_; }

    void set_visible_layer_mask(int p_mask);
    [[nodiscard]] int get_visible_layer_mask() const noexcept { return static_cast<int>(visible_layer_mask_); }

    void set_layer_visible(int p_layer_index, bool p_visible);
    [[nodiscard]] bool is_layer_visible(int p_layer_index) const noexcept;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::BlueprintResource::LayerIndex);
VARIANT_ENUM_CAST(ideam::godot_ext::BlueprintResource::FeatureSymbolType);

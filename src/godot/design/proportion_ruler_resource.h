#pragma once

/**
 * proportion_ruler_resource.h
 * Godot `Resource` wrapping `ideam::core::ProportionRulerGraph` (`src/godot/design/`).
 *
 * Exposes relational proportion-ruler DAGs across all Andrew Loomis body plans
 * (Biped, Quadruped, Avian, Serpentine) and domain-general Props/Architecture (Chair/Custom),
 * with `IdeamUndoRedo` and `IdeamTutorials` help-topic hooks.
 */

#include "../../core/design/proportion_ruler_graph.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

enum ProportionRulerUnitType : int32_t {
    RULER_UNIT_HEAD_HEIGHT     = 0,
    RULER_UNIT_SHOULDER_HEIGHT = 1,
    RULER_UNIT_KEEL_LENGTH     = 2,
    RULER_UNIT_SKULL_LENGTH    = 3,
    RULER_UNIT_SEAT_HEIGHT     = 4,
    RULER_UNIT_CUSTOM          = 5
};

enum LoomisBipedCanonPreset : int32_t {
    LOOMIS_BIPED_CHILD_5H      = 0,
    LOOMIS_BIPED_NORMAL_7_5H   = 1,
    LOOMIS_BIPED_IDEALISTIC_8H = 2,
    LOOMIS_BIPED_HEROIC_8_5H   = 3,
    LOOMIS_BIPED_STYLIZED_4_5H = 4
};

class ProportionRulerGraphResource : public godot::Resource {
    GDCLASS(ProportionRulerGraphResource, godot::Resource)

private:
    core::ProportionRulerGraph core_graph_;
    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray semantic_tags_;

protected:
    static void _bind_methods();

public:
    ProportionRulerGraphResource();
    ~ProportionRulerGraphResource() override = default;

    [[nodiscard]] const core::ProportionRulerGraph &get_core_graph() const noexcept { return core_graph_; }
    [[nodiscard]] core::ProportionRulerGraph &get_core_graph_mut() noexcept { return core_graph_; }
    void set_core_graph(const core::ProportionRulerGraph &p_graph);

    void set_canon_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_canon_name() const;

    void set_unit_type(ProportionRulerUnitType p_type);
    [[nodiscard]] ProportionRulerUnitType get_unit_type() const noexcept;

    void set_base_unit_meters(real_t p_meters);
    [[nodiscard]] real_t get_base_unit_meters() const noexcept;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) { undo_redo_ = p_ur; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    void set_semantic_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    int add_ruler_node(
        const godot::String &p_name,
        int p_parent_id,
        real_t p_ratio,
        real_t p_bias = 0.0f,
        bool p_is_landmark = false
    );
    bool set_node_ratio(int p_node_id, real_t p_ratio);
    bool evaluate();

    [[nodiscard]] real_t get_value_by_name(const godot::String &p_name, real_t p_fallback = 0.0f) const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_landmarks() const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_all_nodes() const;

    // --- Universal Loomis Canon Presets ---
    void setup_loomis_biped(LoomisBipedCanonPreset p_preset = LOOMIS_BIPED_NORMAL_7_5H, real_t p_head_height_m = 0.23f);
    void setup_loomis_quadruped(real_t p_shoulder_height_m = 0.80f);
    void setup_loomis_avian(real_t p_keel_length_m = 0.22f);
    void setup_loomis_serpentine(real_t p_skull_length_m = 0.14f);
    void setup_prop_chair(real_t p_seat_height_m = 0.45f);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ProportionRulerUnitType);
VARIANT_ENUM_CAST(ideam::godot_ext::LoomisBipedCanonPreset);

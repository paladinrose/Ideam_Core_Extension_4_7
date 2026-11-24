#pragma once

/**
 * character_gallery_bank_resource.h
 * Tier 3 `SmartToolProfile`-backed Reusable Gallery Bank (`CharacterGalleryBankResource`)
 * for `IdeamCharacters` (`src/godot/characters/`).
 *
 * Organizes reusable character-building presets across four gallery banks:
 *   - `BANK_SCAFFOLD`: Layer 1 Loomis Form Scaffolds & Proportion Canons
 *   - `BANK_TOPOLOGY`: Layer 2 Landmarks, Asaro Planar Breaks & Topological Bridge Rules
 *   - `BANK_FEATURE`:  Layer 3 Parametric Features (Eyes, Ears, Noses, Horns, Beaks, Fins)
 *   - `BANK_DETAIL`:   Layer 4 Unconstrained Asymmetric Detail Deltas
 *
 * Pre-populates Andrew Loomis starter presets out of the box and supports live keyword/tag
 * search, 1-click apply onto `BodyPlanResource` / `LoomisCharacter3DResource`, and saving
 * custom studio presets.
 */

#include "body_plan_resource.h"
#include "loomis_character_3d_resource.h"
#include "../tools/smart/smart_tool_profile.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

enum CharacterBankCategory : int32_t {
    BANK_SCAFFOLD = 0,
    BANK_TOPOLOGY = 1,
    BANK_FEATURE  = 2,
    BANK_DETAIL   = 3
};

class CharacterGalleryBankResource : public SmartToolProfile {
    GDCLASS(CharacterGalleryBankResource, SmartToolProfile)

private:
    godot::TypedArray<godot::Dictionary> scaffold_presets_;
    godot::TypedArray<godot::Dictionary> topology_presets_;
    godot::TypedArray<godot::Dictionary> feature_presets_;
    godot::TypedArray<godot::Dictionary> detail_presets_;

    [[nodiscard]] godot::TypedArray<godot::Dictionary> &_bank_ref(CharacterBankCategory p_cat);
    [[nodiscard]] const godot::TypedArray<godot::Dictionary> &_bank_cref(CharacterBankCategory p_cat) const;

protected:
    static void _bind_methods();

public:
    CharacterGalleryBankResource();
    ~CharacterGalleryBankResource() override = default;

    void set_scaffold_presets(const godot::TypedArray<godot::Dictionary> &p_presets);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_scaffold_presets() const { return scaffold_presets_; }

    void set_topology_presets(const godot::TypedArray<godot::Dictionary> &p_presets);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_topology_presets() const { return topology_presets_; }

    void set_feature_presets(const godot::TypedArray<godot::Dictionary> &p_presets);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_feature_presets() const { return feature_presets_; }

    void set_detail_presets(const godot::TypedArray<godot::Dictionary> &p_presets);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_detail_presets() const { return detail_presets_; }

    void populate_loomis_starter_banks();

    godot::String add_custom_preset(
        CharacterBankCategory p_category,
        const godot::String &p_display_name,
        const godot::String &p_description,
        const godot::PackedStringArray &p_tags,
        const godot::Dictionary &p_payload
    );

    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_presets_for_bank(
        CharacterBankCategory p_category,
        const godot::String &p_search_filter = ""
    ) const;

    [[nodiscard]] godot::Dictionary find_preset_by_id(const godot::String &p_preset_id) const;

    bool apply_preset_to_body_plan(
        const godot::String &p_preset_id,
        const godot::Ref<BodyPlanResource> &p_plan
    ) const;

    bool apply_preset_to_character_instance(
        const godot::String &p_preset_id,
        const godot::Ref<LoomisCharacter3DResource> &p_instance
    ) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CharacterBankCategory);

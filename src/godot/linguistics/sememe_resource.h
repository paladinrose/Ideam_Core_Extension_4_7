#pragma once

/**
 * sememe_resource.h
 * `SememeResource` and `SemanticTagSetResource` (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Replaces `old_unity_code/Ideam/Epistemology/Semantics/Sememe.cs` and
 * `SemanticAffinityCondition.cs` with Godot-native `Resource` types that can be
 * attached to any `GameplayResource`, `ProceduralPattern`, `Narreme`, `Style3DResource`,
 * or `CharacterBlueprintResource` for engine-wide semantic tagging and Wizard discovery.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "../../core/linguistics/linguistics_common.h"

namespace ideam::godot_ext {

class SememeResource : public godot::Resource {
    GDCLASS(SememeResource, godot::Resource)

private:
    int unique_id_          = 0;
    int sememic_form_       = 1;
    float emotive_valence_  = 0.0f;
    godot::StringName canonical_label_;
    godot::PackedStringArray nuclear_seme_tags_;
    godot::PackedStringArray peripheral_seme_tags_;

protected:
    static void _bind_methods();

public:
    SememeResource() = default;
    ~SememeResource() override = default;

    void set_unique_id(int p_id) { unique_id_ = p_id; emit_changed(); }
    [[nodiscard]] int get_unique_id() const noexcept { return unique_id_; }

    void set_canonical_label(const godot::StringName &p_label) { canonical_label_ = p_label; emit_changed(); }
    [[nodiscard]] godot::StringName get_canonical_label() const { return canonical_label_; }

    void set_nuclear_seme_tags(const godot::PackedStringArray &p_tags) { nuclear_seme_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_nuclear_seme_tags() const { return nuclear_seme_tags_; }

    void set_peripheral_seme_tags(const godot::PackedStringArray &p_tags) { peripheral_seme_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_peripheral_seme_tags() const { return peripheral_seme_tags_; }
};

class SemanticTagSetResource : public godot::Resource {
    GDCLASS(SemanticTagSetResource, godot::Resource)

private:
    godot::TypedArray<SememeResource> sememes_;

protected:
    static void _bind_methods();

public:
    SemanticTagSetResource() = default;
    ~SemanticTagSetResource() override = default;

    void set_sememes(const godot::TypedArray<SememeResource> &p_sememes) { sememes_ = p_sememes; emit_changed(); }
    [[nodiscard]] godot::TypedArray<SememeResource> get_sememes() const { return sememes_; }
};

} // namespace ideam::godot_ext

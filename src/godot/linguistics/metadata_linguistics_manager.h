#pragma once

/**
 * metadata_linguistics_manager.h
 * `MetadataLinguisticsManager` (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. ZERO-RESOURCE PER-OBJECT LINGUISTIC STORAGE VIA GODOT METADATA:
 *    - Godot's `Object` metadata (`set_meta`, `get_meta`, `has_meta`, `remove_meta`,
 *      `get_meta_list`) allows attaching semantic, phonological, and dialectal data to any
 *      `Object`, `Node`, `Control`, `Resource`, or `RefCounted` without allocating extra
 *      `SememeResource` or `SemanticTagSetResource` instances.
 *    - Stores flat `Variant` value types (`PackedStringArray`, `PackedInt64Array` 192-bit
 *      masks, `float` valence, `int` lect/category, and a 64-byte `PackedByteArray`
 *      `core::LinguisticProfilePOD` cache blob).
 *
 * 2. HARVESTING TRADITIONAL USER-AUTHORED METADATA:
 *    - Beyond managing `__ideam_ling_*` keys, `harvest_user_metadata_semantics()` scans
 *      all non-internal metadata keys on an `Object` (e.g., `meta/faction = "undead"`,
 *      `meta/material = "iron"`, `meta/is_locked = true`), decomposing both key identifiers
 *      and string/boolean values into Nuclear and Peripheral Seme bitmasks.
 *
 * 3. DOD TRACK ALIGNMENT & JUSTIFICATION:
 *    - Why Metadata is a great Godot-side tool while remaining 100% DOD-compatible:
 *      * In DOD (`TaskGraphDOD`), tasks cannot call `Object::get_meta()` during parallel
 *        wave execution. Therefore, metadata is NEVER used as the primary runtime store
 *        inside `TaskGraphDOD`.
 *      * Instead, `MetadataLinguisticsManager` caches a pre-compiled 64-byte
 *        `core::LinguisticProfilePOD` (`META_PROFILE_POD_BLOB`) inside metadata on the Godot
 *        side and exposes `pack_objects_metadata_to_dod_buffer(Array objects)` to copy those
 *        64-byte records contiguously into `MemoryManagerDOD` (`PRIMARY_DESCRIPTOR_POD`) in
 *        a single pass at scene load or authoring commit!
 */

#include "sememe_resource.h"
#include "semantic_prompt_parser.h"
#include "../../core/linguistics/semantic_lexicon_core.h"
#include "../utilities/ideam_undo_redo.h"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class MetadataLinguisticsManager : public godot::RefCounted {
    GDCLASS(MetadataLinguisticsManager, godot::RefCounted)

public:
    // Standardized Godot Object Metadata Keys for IdeamLinguistics
    static constexpr const char *META_NUCLEAR_SEME_TAGS    = "__ideam_ling_nuclear_semes";
    static constexpr const char *META_PERIPHERAL_SEME_TAGS = "__ideam_ling_peripheral_semes";
    static constexpr const char *META_NUCLEAR_BITMASK      = "__ideam_ling_nuclear_mask";
    static constexpr const char *META_PERIPHERAL_BITMASK   = "__ideam_ling_peripheral_mask";
    static constexpr const char *META_PRIMARY_SEMEME_ID    = "__ideam_ling_sememe_id";
    static constexpr const char *META_EMOTIVE_VALENCE      = "__ideam_ling_valence";
    static constexpr const char *META_SEMEMIC_FORM         = "__ideam_ling_sememic_form";
    static constexpr const char *META_DOMINANT_CATEGORY    = "__ideam_ling_lexical_category";
    static constexpr const char *META_LECT_ID              = "__ideam_ling_lect_id";
    static constexpr const char *META_SEMANTIC_ALIASES     = "__ideam_ling_aliases";
    static constexpr const char *META_PROFILE_POD_BLOB     = "__ideam_ling_profile_pod";

private:
    godot::Ref<IdeamUndoRedo> undo_redo_;
    mutable godot::Ref<SemanticPromptParser> prompt_parser_;

    void _ensure_prompt_parser() const;

protected:
    static void _bind_methods();

public:
    MetadataLinguisticsManager() = default;
    ~MetadataLinguisticsManager() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_prompt_parser(const godot::Ref<SemanticPromptParser> &p_parser) { prompt_parser_ = p_parser; }
    [[nodiscard]] godot::Ref<SemanticPromptParser> get_prompt_parser() const;

    // --- Direct Per-Object Metadata Tagging (Zero Extra Resource Allocations) ---
    static bool has_linguistic_metadata(const godot::Object *p_object);
    static void clear_linguistic_metadata(godot::Object *p_object);

    static void set_object_seme_tags(
        godot::Object *p_object,
        const godot::PackedStringArray &p_nuclear_tags,
        const godot::PackedStringArray &p_peripheral_tags = godot::PackedStringArray(),
        float p_emotive_valence = 0.0f,
        int p_lexical_category = static_cast<int>(core::LexicalCategory::NOUN),
        int p_lect_id = 0
    );

    static void add_object_nuclear_seme(godot::Object *p_object, const godot::String &p_tag);
    static void add_object_peripheral_seme(godot::Object *p_object, const godot::String &p_tag);

    [[nodiscard]] static godot::PackedStringArray get_object_nuclear_seme_tags(const godot::Object *p_object);
    [[nodiscard]] static godot::PackedStringArray get_object_peripheral_seme_tags(const godot::Object *p_object);
    [[nodiscard]] static godot::PackedInt64Array get_object_nuclear_bitmask_words(const godot::Object *p_object);
    [[nodiscard]] static godot::PackedInt64Array get_object_peripheral_bitmask_words(const godot::Object *p_object);

    static void set_object_semantic_aliases(godot::Object *p_object, const godot::PackedStringArray &p_aliases);
    [[nodiscard]] static godot::PackedStringArray get_object_semantic_aliases(const godot::Object *p_object);

    static void set_object_lect_id(godot::Object *p_object, int p_lect_id);
    [[nodiscard]] static int get_object_lect_id(const godot::Object *p_object);

    static void set_object_emotive_valence(godot::Object *p_object, float p_valence);
    [[nodiscard]] static float get_object_emotive_valence(const godot::Object *p_object);

    // --- Conversion Between Object Metadata, TagSetResource & 64-Byte POD ---
    static void write_tag_set_to_metadata(
        godot::Object *p_object,
        const godot::Ref<SemanticTagSetResource> &p_tag_set
    );

    [[nodiscard]] static godot::Ref<SemanticTagSetResource> read_tag_set_from_metadata(
        const godot::Object *p_object
    );
    [[nodiscard]] static godot::Ref<SemanticTagSetResource> extract_tag_set_from_metadata(
        const godot::Object *p_object
    ) {
        return read_tag_set_from_metadata(p_object);
    }

    static void write_profile_pod_to_metadata(
        godot::Object *p_object,
        const core::LinguisticProfilePOD &p_pod
    );

    [[nodiscard]] static bool read_profile_pod_from_metadata(
        const godot::Object *p_object,
        core::LinguisticProfilePOD &r_out_pod
    );

    [[nodiscard]] static godot::PackedByteArray get_cached_profile_pod_bytes(
        const godot::Object *p_object
    );

    [[nodiscard]] static float compute_metadata_affinity_between_objects(
        const godot::Object *p_object_a,
        const godot::Object *p_object_b,
        float p_nuclear_weight = 0.75f,
        float p_peripheral_weight = 0.25f
    );

    // --- Harvesting User-Defined Custom Metadata Keys & Values ---
    [[nodiscard]] godot::Ref<SemanticTagSetResource> harvest_user_metadata_semantics(
        const godot::Object *p_object,
        bool p_include_private_keys = false
    ) const;

    // --- Batch DOD Contiguous Buffer Packing ---
    [[nodiscard]] static godot::PackedByteArray pack_objects_metadata_to_dod_buffer(
        const godot::Array &p_objects
    );

    // --- Undo / Redo Snapshot & Commit Helpers ---
    [[nodiscard]] static godot::Dictionary get_object_metadata_snapshot(const godot::Object *p_object);
    static void apply_object_metadata_snapshot(godot::Object *p_object, const godot::Dictionary &p_snapshot);

    void commit_object_seme_tags(
        godot::Object *p_object,
        const godot::PackedStringArray &p_nuclear_tags,
        const godot::PackedStringArray &p_peripheral_tags,
        const godot::String &p_action_name = "Set Object Linguistic Metadata"
    );

    void commit_clear_linguistic_metadata(
        godot::Object *p_object,
        const godot::String &p_action_name = "Clear Object Linguistic Metadata"
    );
};

} // namespace ideam::godot_ext

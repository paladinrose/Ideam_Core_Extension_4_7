#pragma once

/**
 * linguistic_object_profiler.h
 * `LinguisticObjectProfiler` (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. COMPREHENSIVE HARVESTING OF GODOT'S TRADITIONAL SEMANTIC SOURCES:
 *    - Strongly considers every native source of semantic information in the Godot
 *      environment before cross-plugin bridges are even invoked:
 *        a) **Class Names & Inheritance (`LING_SOURCE_CLASS_HIERARCHY`)**:
 *           `Object::get_class()`, `ClassDB::get_parent_class()`, and attached `Script`
 *           `get_global_name()` / script filename stem.
 *        b) **Object/Node/Resource Names & Groups (`LING_SOURCE_OBJECT_AND_NODE_NAME`, `LING_SOURCE_NODE_GROUPS`)**:
 *           `Node::get_name()`, `Node::get_groups()`, `Resource::get_name()`, `Resource::get_path()`,
 *           and domain `title` / `canonical_label` properties.
 *        c) **Traditional UI Controls & Text Fields (`LING_SOURCE_UI_TEXT_CONTROLS`)**:
 *           `Label`, `Label3D`, `RichTextLabel`, `LineEdit` (`text` & `placeholder_text`),
 *           `TextEdit`, `Button`, `CheckBox`, `OptionButton`, `ItemList`, `TabContainer`,
 *           `Tree`, `Window` (`title`), `Control` (`tooltip_text`), and `Node` (`editor_description`),
 *           plus optional sibling/child label association for composite UI rows.
 *        d) **Reflected Member Variables (`LING_SOURCE_REFLECTED_PROPERTIES`)**:
 *           `Object::get_property_list()` member names, boolean state prefixes (`is_`, `has_`, `can_`),
 *           active `PROPERTY_HINT_ENUM` and `PROPERTY_HINT_FLAGS` constant labels, range ratios,
 *           and attached `SememeResource` / `SemanticTagSetResource` properties.
 *        e) **Reflected Methods (`LING_SOURCE_REFLECTED_METHODS`)**:
 *           `Object::get_method_list()` action verbs (affordances/capabilities) and parameter names.
 *        f) **Reflected Signals (`LING_SOURCE_REFLECTED_SIGNALS`)**:
 *           `Object::get_signal_list()` eventive/participial transitions and payload argument names.
 *        g) **Object Metadata (`LING_SOURCE_OBJECT_METADATA`)**:
 *           Both explicit `__ideam_ling_*` metadata and user-defined custom metadata via
 *           `MetadataLinguisticsManager`.
 *        h) **Zero-Instantiation `PackedScene` (`SceneState`) (`LING_SOURCE_SCENE_STATE`)**:
 *           Harvests node types, names, groups, and serialized properties from a `PackedScene`
 *           without instantiating nodes.
 *
 * 2. DOD ARCHITECTURAL ALIGNMENT ("CRUCIAL & DOD-COMPATIBLE"):
 *    - **Why Reflection Profiling Belongs at the Engine Boundary**:
 *      Calling `Object::get_property_list()`, `get_method_list()`, `ClassDB`, or UI `Control`
 *      methods inside a parallel `TaskGraphDOD` wave is impossible in pure DOD (and would
 *      shatter cache locality and thread safety).
 *    - **How `LinguisticObjectProfiler` Bridges to Pure DOD**:
 *      Instead of requiring runtime reflection during DOD execution, `LinguisticObjectProfiler`
 *      acts as an **ingestion compiler**:
 *        1. It harvests all selected Godot channels once (at scene load, editor authoring,
 *           or `ProceduralPatterner` analysis) and compiles the result into a 64-byte
 *           cache-aligned `core::LinguisticProfilePOD` (and 32-byte `core::ReflectionSignaturePOD`).
 *        2. It optionally caches that 64-byte POD in the `Object`'s metadata (`__ideam_ling_profile_pod`)
 *           so subsequent queries on the Godot side are O(1) without re-running reflection.
 *        3. It exposes `profile_objects_to_dod_buffer(Array objects)` to emit a flat, contiguous
 *           `PackedByteArray` of 64-byte `LinguisticProfilePOD` structs that uploads directly into
 *           `MemoryManagerDOD` (`LinguisticsBufferSlot::PRIMARY_DESCRIPTOR_POD`), where
 *           `SemeComponentialAffinityQueryLogic` and `QuantumDarwinism` execute 100% branchless
 *           SIMD bitwise operations!
 */

#include "metadata_linguistics_manager.h"
#include "semantic_prompt_parser.h"
#include "sememe_resource.h"
#include "../../core/linguistics/semantic_lexicon_core.h"
#include "../../core/procedural_generation/procgen_common.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/scene_state.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class LinguisticObjectProfiler : public godot::RefCounted {
    GDCLASS(LinguisticObjectProfiler, godot::RefCounted)

private:
    mutable godot::Ref<SemanticPromptParser> prompt_parser_;
    mutable godot::Ref<MetadataLinguisticsManager> metadata_manager_;

    uint32_t enabled_channels_mask_     = core::LING_SOURCE_ALL;
    int max_inheritance_depth_          = 5;
    int max_ui_items_to_sample_         = 16;
    bool include_engine_base_members_   = false;
    bool include_child_ui_labels_       = true;
    bool auto_cache_pod_in_metadata_    = true;

    void _ensure_helpers() const;

    void _harvest_class_hierarchy(
        const godot::Object *p_object,
        const godot::Ref<SemanticTagSetResource> &r_tag_set,
        godot::PackedStringArray &r_class_tokens
    ) const;

    void _harvest_names_and_groups(
        const godot::Object *p_object,
        const godot::Ref<SemanticTagSetResource> &r_tag_set,
        godot::PackedStringArray &r_name_tokens
    ) const;

    void _harvest_ui_text_controls(
        godot::Object *p_object,
        const godot::Ref<SemanticTagSetResource> &r_tag_set,
        godot::PackedStringArray &r_ui_texts,
        int p_child_depth = 0
    ) const;

    void _harvest_reflected_properties(
        const godot::Object *p_object,
        const godot::Ref<SemanticTagSetResource> &r_tag_set,
        godot::PackedStringArray &r_property_names,
        uint32_t &r_property_trait_mask
    ) const;

    void _harvest_reflected_methods(
        const godot::Object *p_object,
        const godot::Ref<SemanticTagSetResource> &r_tag_set,
        godot::PackedStringArray &r_method_verbs,
        uint32_t &r_method_trait_mask
    ) const;

    void _harvest_reflected_signals(
        const godot::Object *p_object,
        const godot::Ref<SemanticTagSetResource> &r_tag_set,
        godot::PackedStringArray &r_signal_events,
        uint32_t &r_signal_trait_mask
    ) const;

protected:
    static void _bind_methods();

public:
    LinguisticObjectProfiler() = default;
    ~LinguisticObjectProfiler() override = default;

    // --- Configuration Properties ---
    void set_prompt_parser(const godot::Ref<SemanticPromptParser> &p_parser) { prompt_parser_ = p_parser; }
    [[nodiscard]] godot::Ref<SemanticPromptParser> get_prompt_parser() const;

    void set_metadata_manager(const godot::Ref<MetadataLinguisticsManager> &p_mgr) { metadata_manager_ = p_mgr; }
    [[nodiscard]] godot::Ref<MetadataLinguisticsManager> get_metadata_manager() const;

    void set_enabled_channels_mask(int p_mask) { enabled_channels_mask_ = static_cast<uint32_t>(p_mask & 0xFFFF); }
    [[nodiscard]] int get_enabled_channels_mask() const noexcept { return static_cast<int>(enabled_channels_mask_); }

    void set_max_inheritance_depth(int p_depth) { max_inheritance_depth_ = std::clamp(p_depth, 1, 16); }
    [[nodiscard]] int get_max_inheritance_depth() const noexcept { return max_inheritance_depth_; }

    void set_max_ui_items_to_sample(int p_max_items) { max_ui_items_to_sample_ = std::clamp(p_max_items, 1, 128); }
    [[nodiscard]] int get_max_ui_items_to_sample() const noexcept { return max_ui_items_to_sample_; }

    void set_include_engine_base_members(bool p_inc) { include_engine_base_members_ = p_inc; }
    [[nodiscard]] bool get_include_engine_base_members() const noexcept { return include_engine_base_members_; }

    void set_include_child_ui_labels(bool p_inc) { include_child_ui_labels_ = p_inc; }
    [[nodiscard]] bool get_include_child_ui_labels() const noexcept { return include_child_ui_labels_; }

    void set_auto_cache_pod_in_metadata(bool p_cache) { auto_cache_pod_in_metadata_ = p_cache; }
    [[nodiscard]] bool get_auto_cache_pod_in_metadata() const noexcept { return auto_cache_pod_in_metadata_; }

    // --- Core Object & Subtree Profiling API ---
    [[nodiscard]] godot::Ref<SemanticTagSetResource> profile_object(
        godot::Object *p_object,
        bool p_use_cached_metadata_if_present = false
    ) const;

    [[nodiscard]] godot::Dictionary inspect_object_linguistic_breakdown(
        godot::Object *p_object
    ) const;

    [[nodiscard]] godot::Ref<SemanticTagSetResource> profile_node_subtree(
        godot::Node *p_root_node,
        int p_max_depth = 4
    ) const;

    // --- Zero-Instantiation `PackedScene` (`SceneState`) Profiling ---
    [[nodiscard]] godot::Ref<SemanticTagSetResource> profile_packed_scene(
        const godot::Ref<godot::PackedScene> &p_scene
    ) const;

    [[nodiscard]] godot::Ref<SemanticTagSetResource> profile_scene_state_node(
        const godot::Ref<godot::SceneState> &p_state,
        int p_node_idx
    ) const;

    // --- Similarity & Matching Between Live Godot Objects ---
    [[nodiscard]] float compute_semantic_similarity_between_objects(
        godot::Object *p_object_a,
        godot::Object *p_object_b,
        float p_nuclear_weight = 0.75f,
        float p_peripheral_weight = 0.25f
    ) const;

    [[nodiscard]] bool does_object_match_prompt(
        godot::Object *p_object,
        const godot::String &p_natural_language_prompt,
        float p_min_affinity = 0.35f
    ) const;

    // --- DOD & `ProceduralEnvironment` Compilation Bridges ---
    [[nodiscard]] core::LinguisticProfilePOD compile_linguistic_profile_pod(
        godot::Object *p_object,
        bool p_cache_in_metadata = true
    ) const;

    [[nodiscard]] core::ReflectionSignaturePOD compile_reflection_signature_pod(
        const godot::Object *p_object
    ) const;

    [[nodiscard]] godot::Dictionary build_reflection_signature_dictionary(
        const godot::Object *p_object
    ) const;

    [[nodiscard]] godot::PackedByteArray profile_objects_to_dod_buffer(
        const godot::Array &p_objects,
        bool p_cache_in_metadata = true
    ) const;
};

} // namespace ideam::godot_ext

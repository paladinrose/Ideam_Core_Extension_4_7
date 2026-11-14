#include "metadata_linguistics_manager.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <algorithm>
#include <cstring>

namespace ideam::godot_ext {

void MetadataLinguisticsManager::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &MetadataLinguisticsManager::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &MetadataLinguisticsManager::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_prompt_parser", "parser"), &MetadataLinguisticsManager::set_prompt_parser);
    ClassDB::bind_method(D_METHOD("get_prompt_parser"), &MetadataLinguisticsManager::get_prompt_parser);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "prompt_parser", PROPERTY_HINT_RESOURCE_TYPE, "SemanticPromptParser"),
        "set_prompt_parser",
        "get_prompt_parser"
    );

    // Static & Instance Methods
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("has_linguistic_metadata", "object"),
        &MetadataLinguisticsManager::has_linguistic_metadata
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("clear_linguistic_metadata", "object"),
        &MetadataLinguisticsManager::clear_linguistic_metadata
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("set_object_seme_tags", "object", "nuclear_tags", "peripheral_tags", "emotive_valence", "lexical_category", "lect_id"),
        &MetadataLinguisticsManager::set_object_seme_tags,
        DEFVAL(PackedStringArray()),
        DEFVAL(0.0f),
        DEFVAL(static_cast<int>(core::LexicalCategory::NOUN)),
        DEFVAL(0)
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("add_object_nuclear_seme", "object", "tag"),
        &MetadataLinguisticsManager::add_object_nuclear_seme
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("add_object_peripheral_seme", "object", "tag"),
        &MetadataLinguisticsManager::add_object_peripheral_seme
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_nuclear_seme_tags", "object"),
        &MetadataLinguisticsManager::get_object_nuclear_seme_tags
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_peripheral_seme_tags", "object"),
        &MetadataLinguisticsManager::get_object_peripheral_seme_tags
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_nuclear_bitmask_words", "object"),
        &MetadataLinguisticsManager::get_object_nuclear_bitmask_words
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_peripheral_bitmask_words", "object"),
        &MetadataLinguisticsManager::get_object_peripheral_bitmask_words
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("set_object_semantic_aliases", "object", "aliases"),
        &MetadataLinguisticsManager::set_object_semantic_aliases
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_semantic_aliases", "object"),
        &MetadataLinguisticsManager::get_object_semantic_aliases
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("set_object_lect_id", "object", "lect_id"),
        &MetadataLinguisticsManager::set_object_lect_id
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_lect_id", "object"),
        &MetadataLinguisticsManager::get_object_lect_id
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("set_object_emotive_valence", "object", "valence"),
        &MetadataLinguisticsManager::set_object_emotive_valence
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_emotive_valence", "object"),
        &MetadataLinguisticsManager::get_object_emotive_valence
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("write_tag_set_to_metadata", "object", "tag_set"),
        &MetadataLinguisticsManager::write_tag_set_to_metadata
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("read_tag_set_from_metadata", "object"),
        &MetadataLinguisticsManager::read_tag_set_from_metadata
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_cached_profile_pod_bytes", "object"),
        &MetadataLinguisticsManager::get_cached_profile_pod_bytes
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("compute_metadata_affinity_between_objects", "object_a", "object_b", "nuclear_weight", "peripheral_weight"),
        &MetadataLinguisticsManager::compute_metadata_affinity_between_objects,
        DEFVAL(0.75f),
        DEFVAL(0.25f)
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("pack_objects_metadata_to_dod_buffer", "objects"),
        &MetadataLinguisticsManager::pack_objects_metadata_to_dod_buffer
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("get_object_metadata_snapshot", "object"),
        &MetadataLinguisticsManager::get_object_metadata_snapshot
    );
    ClassDB::bind_static_method(
        "MetadataLinguisticsManager",
        D_METHOD("apply_object_metadata_snapshot", "object", "snapshot"),
        &MetadataLinguisticsManager::apply_object_metadata_snapshot
    );

    ClassDB::bind_method(
        D_METHOD("harvest_user_metadata_semantics", "object", "include_private_keys"),
        &MetadataLinguisticsManager::harvest_user_metadata_semantics,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("commit_object_seme_tags", "object", "nuclear_tags", "peripheral_tags", "action_name"),
        &MetadataLinguisticsManager::commit_object_seme_tags,
        DEFVAL("Set Object Linguistic Metadata")
    );
    ClassDB::bind_method(
        D_METHOD("commit_clear_linguistic_metadata", "object", "action_name"),
        &MetadataLinguisticsManager::commit_clear_linguistic_metadata,
        DEFVAL("Clear Object Linguistic Metadata")
    );
}

void MetadataLinguisticsManager::_ensure_prompt_parser() const {
    if (prompt_parser_.is_null()) {
        prompt_parser_.instantiate();
    }
}

godot::Ref<SemanticPromptParser> MetadataLinguisticsManager::get_prompt_parser() const {
    _ensure_prompt_parser();
    return prompt_parser_;
}

bool MetadataLinguisticsManager::has_linguistic_metadata(const godot::Object *p_object) {
    if (!p_object) return false;
    return p_object->has_meta(META_PROFILE_POD_BLOB) ||
           p_object->has_meta(META_NUCLEAR_SEME_TAGS) ||
           p_object->has_meta(META_PERIPHERAL_SEME_TAGS) ||
           p_object->has_meta(META_NUCLEAR_BITMASK) ||
           p_object->has_meta(META_PERIPHERAL_BITMASK);
}

void MetadataLinguisticsManager::clear_linguistic_metadata(godot::Object *p_object) {
    if (!p_object) return;
    static constexpr const char *KEYS[] = {
        META_NUCLEAR_SEME_TAGS,
        META_PERIPHERAL_SEME_TAGS,
        META_NUCLEAR_BITMASK,
        META_PERIPHERAL_BITMASK,
        META_PRIMARY_SEMEME_ID,
        META_EMOTIVE_VALENCE,
        META_SEMEMIC_FORM,
        META_DOMINANT_CATEGORY,
        META_LECT_ID,
        META_SEMANTIC_ALIASES,
        META_PROFILE_POD_BLOB
    };
    for (const char *k : KEYS) {
        if (p_object->has_meta(k)) {
            p_object->remove_meta(k);
        }
    }
}

void MetadataLinguisticsManager::set_object_seme_tags(
    godot::Object *p_object,
    const godot::PackedStringArray &p_nuclear_tags,
    const godot::PackedStringArray &p_peripheral_tags,
    float p_emotive_valence,
    int p_lexical_category,
    int p_lect_id
) {
    if (!p_object) return;

    core::LinguisticProfilePOD pod{};
    pod.emotive_valence     = std::clamp(p_emotive_valence, -1.0f, 1.0f);
    pod.dominant_category   = static_cast<uint8_t>(std::clamp(p_lexical_category, 0, 9));
    pod.source_channel_mask = core::LING_SOURCE_OBJECT_METADATA;

    if (const auto *node = godot::Object::cast_to<godot::Node>(p_object)) {
        pod.object_name_hash = core::hash_linguistic_token(godot::String(node->get_name()).utf8().get_data());
    } else if (const auto *res = godot::Object::cast_to<godot::Resource>(p_object)) {
        pod.object_name_hash = core::hash_linguistic_token(res->get_name().utf8().get_data());
    }

    core::SememeDescriptorPOD desc{};
    for (int i = 0; i < p_nuclear_tags.size(); ++i) {
        const godot::CharString cs = p_nuclear_tags[i].utf8();
        if (cs.length() > 0) {
            desc.set_nuclear_bit(core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data()));
        }
    }
    for (int i = 0; i < p_peripheral_tags.size(); ++i) {
        const godot::CharString cs = p_peripheral_tags[i].utf8();
        if (cs.length() > 0) {
            desc.set_peripheral_bit(core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data()));
        }
    }

    pod.nuclear_seme_words    = desc.nuclear_seme_words;
    pod.peripheral_seme_words = desc.peripheral_seme_words;

    godot::PackedInt64Array nuc_words;
    godot::PackedInt64Array per_words;
    nuc_words.resize(3);
    per_words.resize(3);
    for (int i = 0; i < 3; ++i) {
        nuc_words.set(i, static_cast<int64_t>(pod.nuclear_seme_words[static_cast<size_t>(i)]));
        per_words.set(i, static_cast<int64_t>(pod.peripheral_seme_words[static_cast<size_t>(i)]));
    }

    p_object->set_meta(META_NUCLEAR_SEME_TAGS, p_nuclear_tags);
    p_object->set_meta(META_PERIPHERAL_SEME_TAGS, p_peripheral_tags);
    p_object->set_meta(META_NUCLEAR_BITMASK, nuc_words);
    p_object->set_meta(META_PERIPHERAL_BITMASK, per_words);
    p_object->set_meta(META_EMOTIVE_VALENCE, pod.emotive_valence);
    p_object->set_meta(META_DOMINANT_CATEGORY, static_cast<int>(pod.dominant_category));
    p_object->set_meta(META_LECT_ID, std::max(0, p_lect_id));

    write_profile_pod_to_metadata(p_object, pod);
}

void MetadataLinguisticsManager::add_object_nuclear_seme(
    godot::Object *p_object,
    const godot::String &p_tag
) {
    if (!p_object || p_tag.is_empty()) return;
    godot::PackedStringArray nuc = get_object_nuclear_seme_tags(p_object);
    godot::PackedStringArray per = get_object_peripheral_seme_tags(p_object);
    if (!nuc.has(p_tag)) {
        nuc.push_back(p_tag);
    }
    set_object_seme_tags(
        p_object,
        nuc,
        per,
        get_object_emotive_valence(p_object),
        static_cast<int>(core::LexicalCategory::NOUN),
        get_object_lect_id(p_object)
    );
}

void MetadataLinguisticsManager::add_object_peripheral_seme(
    godot::Object *p_object,
    const godot::String &p_tag
) {
    if (!p_object || p_tag.is_empty()) return;
    godot::PackedStringArray nuc = get_object_nuclear_seme_tags(p_object);
    godot::PackedStringArray per = get_object_peripheral_seme_tags(p_object);
    if (!per.has(p_tag)) {
        per.push_back(p_tag);
    }
    set_object_seme_tags(
        p_object,
        nuc,
        per,
        get_object_emotive_valence(p_object),
        static_cast<int>(core::LexicalCategory::NOUN),
        get_object_lect_id(p_object)
    );
}

godot::PackedStringArray MetadataLinguisticsManager::get_object_nuclear_seme_tags(
    const godot::Object *p_object
) {
    if (p_object && p_object->has_meta(META_NUCLEAR_SEME_TAGS)) {
        return p_object->get_meta(META_NUCLEAR_SEME_TAGS);
    }
    return {};
}

godot::PackedStringArray MetadataLinguisticsManager::get_object_peripheral_seme_tags(
    const godot::Object *p_object
) {
    if (p_object && p_object->has_meta(META_PERIPHERAL_SEME_TAGS)) {
        return p_object->get_meta(META_PERIPHERAL_SEME_TAGS);
    }
    return {};
}

godot::PackedInt64Array MetadataLinguisticsManager::get_object_nuclear_bitmask_words(
    const godot::Object *p_object
) {
    if (p_object && p_object->has_meta(META_NUCLEAR_BITMASK)) {
        return p_object->get_meta(META_NUCLEAR_BITMASK);
    }
    core::LinguisticProfilePOD pod{};
    if (read_profile_pod_from_metadata(p_object, pod)) {
        godot::PackedInt64Array arr;
        arr.resize(3);
        for (int i = 0; i < 3; ++i) {
            arr.set(i, static_cast<int64_t>(pod.nuclear_seme_words[static_cast<size_t>(i)]));
        }
        return arr;
    }
    return {};
}

godot::PackedInt64Array MetadataLinguisticsManager::get_object_peripheral_bitmask_words(
    const godot::Object *p_object
) {
    if (p_object && p_object->has_meta(META_PERIPHERAL_BITMASK)) {
        return p_object->get_meta(META_PERIPHERAL_BITMASK);
    }
    core::LinguisticProfilePOD pod{};
    if (read_profile_pod_from_metadata(p_object, pod)) {
        godot::PackedInt64Array arr;
        arr.resize(3);
        for (int i = 0; i < 3; ++i) {
            arr.set(i, static_cast<int64_t>(pod.peripheral_seme_words[static_cast<size_t>(i)]));
        }
        return arr;
    }
    return {};
}

void MetadataLinguisticsManager::set_object_semantic_aliases(
    godot::Object *p_object,
    const godot::PackedStringArray &p_aliases
) {
    if (!p_object) return;
    p_object->set_meta(META_SEMANTIC_ALIASES, p_aliases);
}

godot::PackedStringArray MetadataLinguisticsManager::get_object_semantic_aliases(
    const godot::Object *p_object
) {
    if (p_object && p_object->has_meta(META_SEMANTIC_ALIASES)) {
        return p_object->get_meta(META_SEMANTIC_ALIASES);
    }
    return {};
}

void MetadataLinguisticsManager::set_object_lect_id(godot::Object *p_object, int p_lect_id) {
    if (!p_object) return;
    p_object->set_meta(META_LECT_ID, std::max(0, p_lect_id));
}

int MetadataLinguisticsManager::get_object_lect_id(const godot::Object *p_object) {
    if (p_object && p_object->has_meta(META_LECT_ID)) {
        return static_cast<int>(p_object->get_meta(META_LECT_ID));
    }
    return 0;
}

void MetadataLinguisticsManager::set_object_emotive_valence(godot::Object *p_object, float p_valence) {
    if (!p_object) return;
    p_object->set_meta(META_EMOTIVE_VALENCE, std::clamp(p_valence, -1.0f, 1.0f));
}

float MetadataLinguisticsManager::get_object_emotive_valence(const godot::Object *p_object) {
    if (p_object && p_object->has_meta(META_EMOTIVE_VALENCE)) {
        return static_cast<float>(p_object->get_meta(META_EMOTIVE_VALENCE));
    }
    return 0.0f;
}

void MetadataLinguisticsManager::write_tag_set_to_metadata(
    godot::Object *p_object,
    const godot::Ref<SemanticTagSetResource> &p_tag_set
) {
    if (!p_object || p_tag_set.is_null()) return;
    p_object->set_meta(META_NUCLEAR_SEME_TAGS, p_tag_set->get_all_nuclear_tags());
    p_object->set_meta(META_PERIPHERAL_SEME_TAGS, p_tag_set->get_all_peripheral_tags());
    p_object->set_meta(META_NUCLEAR_BITMASK, p_tag_set->get_nuclear_bitmask_words());
    p_object->set_meta(META_PERIPHERAL_BITMASK, p_tag_set->get_peripheral_bitmask_words());
    p_object->set_meta(META_EMOTIVE_VALENCE, p_tag_set->get_aggregate_valence());

    core::LinguisticProfilePOD pod = p_tag_set->to_linguistic_profile_pod(
        core::LING_SOURCE_OBJECT_METADATA | core::LING_SOURCE_EXPLICIT_RESOURCE
    );
    write_profile_pod_to_metadata(p_object, pod);
}

godot::Ref<SemanticTagSetResource> MetadataLinguisticsManager::read_tag_set_from_metadata(
    const godot::Object *p_object
) {
    godot::Ref<SemanticTagSetResource> tag_set;
    tag_set.instantiate();
    if (!p_object) {
        return tag_set;
    }

    if (p_object->has_meta(META_NUCLEAR_SEME_TAGS)) {
        tag_set->set_explicit_nuclear_tags(p_object->get_meta(META_NUCLEAR_SEME_TAGS));
    }
    if (p_object->has_meta(META_PERIPHERAL_SEME_TAGS)) {
        tag_set->set_explicit_peripheral_tags(p_object->get_meta(META_PERIPHERAL_SEME_TAGS));
    }
    if (p_object->has_meta(META_EMOTIVE_VALENCE)) {
        tag_set->set_aggregate_valence(static_cast<float>(p_object->get_meta(META_EMOTIVE_VALENCE)));
    }
    if (p_object->has_meta(META_PROFILE_POD_BLOB)) {
        tag_set->load_from_profile_pod_bytes(p_object->get_meta(META_PROFILE_POD_BLOB));
    } else if (p_object->has_meta(META_NUCLEAR_BITMASK) || p_object->has_meta(META_PERIPHERAL_BITMASK)) {
        tag_set->set_raw_bitmask_words(
            get_object_nuclear_bitmask_words(p_object),
            get_object_peripheral_bitmask_words(p_object)
        );
    }

    return tag_set;
}

void MetadataLinguisticsManager::write_profile_pod_to_metadata(
    godot::Object *p_object,
    const core::LinguisticProfilePOD &p_pod
) {
    if (!p_object) return;
    godot::PackedByteArray bytes;
    bytes.resize(sizeof(core::LinguisticProfilePOD));
    std::memcpy(bytes.ptrw(), &p_pod, sizeof(core::LinguisticProfilePOD));
    p_object->set_meta(META_PROFILE_POD_BLOB, bytes);
}

bool MetadataLinguisticsManager::read_profile_pod_from_metadata(
    const godot::Object *p_object,
    core::LinguisticProfilePOD &r_out_pod
) {
    if (!p_object || !p_object->has_meta(META_PROFILE_POD_BLOB)) {
        return false;
    }
    const godot::Variant v = p_object->get_meta(META_PROFILE_POD_BLOB);
    if (v.get_type() != godot::Variant::PACKED_BYTE_ARRAY) {
        return false;
    }
    const godot::PackedByteArray bytes = v;
    if (bytes.size() < static_cast<int64_t>(sizeof(core::LinguisticProfilePOD))) {
        return false;
    }
    std::memcpy(&r_out_pod, bytes.ptr(), sizeof(core::LinguisticProfilePOD));
    return true;
}

godot::PackedByteArray MetadataLinguisticsManager::get_cached_profile_pod_bytes(
    const godot::Object *p_object
) {
    if (p_object && p_object->has_meta(META_PROFILE_POD_BLOB)) {
        return p_object->get_meta(META_PROFILE_POD_BLOB);
    }
    return {};
}

float MetadataLinguisticsManager::compute_metadata_affinity_between_objects(
    const godot::Object *p_object_a,
    const godot::Object *p_object_b,
    float p_nuclear_weight,
    float p_peripheral_weight
) {
    if (!p_object_a || !p_object_b) {
        return 0.0f;
    }
    core::LinguisticProfilePOD pod_a{};
    core::LinguisticProfilePOD pod_b{};
    if (read_profile_pod_from_metadata(p_object_a, pod_a) &&
        read_profile_pod_from_metadata(p_object_b, pod_b)) {
        return core::SemanticLexiconCore::compute_descriptor_affinity(
            pod_a.to_sememe_descriptor(),
            pod_b.to_sememe_descriptor(),
            p_nuclear_weight,
            p_peripheral_weight
        );
    }
    godot::Ref<SemanticTagSetResource> set_a = read_tag_set_from_metadata(p_object_a);
    godot::Ref<SemanticTagSetResource> set_b = read_tag_set_from_metadata(p_object_b);
    return set_a->compute_affinity_with(set_b, p_nuclear_weight, p_peripheral_weight);
}

godot::Ref<SemanticTagSetResource> MetadataLinguisticsManager::harvest_user_metadata_semantics(
    const godot::Object *p_object,
    bool p_include_private_keys
) const {
    _ensure_prompt_parser();
    godot::Ref<SemanticTagSetResource> tag_set = read_tag_set_from_metadata(p_object);
    if (!p_object) {
        return tag_set;
    }

    // Merge any explicit semantic aliases stored in metadata
    const godot::PackedStringArray aliases = get_object_semantic_aliases(p_object);
    for (int i = 0; i < aliases.size(); ++i) {
        godot::Ref<SemanticTagSetResource> alias_tags = prompt_parser_->parse_prompt_to_tag_set(aliases[i]);
        tag_set->merge_from(alias_tags);
    }

    // Inspect all user-authored metadata entries on the Object
    const godot::TypedArray<godot::StringName> meta_keys = p_object->get_meta_list();
    for (int i = 0; i < meta_keys.size(); ++i) {
        const godot::String key_str(meta_keys[i]);
        if (key_str.begins_with("__ideam_")) {
            continue;
        }
        if (!p_include_private_keys && key_str.begins_with("_")) {
            continue;
        }

        const godot::Variant val = p_object->get_meta(meta_keys[i]);

        // If boolean metadata (e.g., meta/is_flammable = true or false)
        if (val.get_type() == godot::Variant::BOOL) {
            godot::Ref<SemanticTagSetResource> key_tags =
                prompt_parser_->parse_identifier_to_tag_set(key_str, false);
            if (static_cast<bool>(val)) {
                tag_set->merge_from(key_tags);
            } else {
                const godot::PackedStringArray excluded = key_tags->get_all_nuclear_tags();
                for (int k = 0; k < excluded.size(); ++k) {
                    tag_set->add_excluded_tag(excluded[k]);
                }
            }
            continue;
        }

        // Parse the metadata key name itself as a peripheral/contextual domain cue
        godot::Ref<SemanticTagSetResource> key_tags =
            prompt_parser_->parse_identifier_to_tag_set(key_str, true);
        const godot::PackedStringArray key_nuc = key_tags->get_all_nuclear_tags();
        for (int k = 0; k < key_nuc.size(); ++k) {
            tag_set->add_peripheral_tag(key_nuc[k]);
        }

        // Parse String / StringName / PackedStringArray metadata values as nuclear tags
        if (val.get_type() == godot::Variant::STRING || val.get_type() == godot::Variant::STRING_NAME) {
            godot::Ref<SemanticTagSetResource> val_tags =
                prompt_parser_->parse_prompt_to_tag_set(godot::String(val));
            tag_set->merge_from(val_tags);
        } else if (val.get_type() == godot::Variant::PACKED_STRING_ARRAY) {
            const godot::PackedStringArray arr = val;
            for (int k = 0; k < arr.size(); ++k) {
                godot::Ref<SemanticTagSetResource> elem_tags =
                    prompt_parser_->parse_prompt_to_tag_set(arr[k]);
                tag_set->merge_from(elem_tags);
            }
        }
    }

    return tag_set;
}

godot::PackedByteArray MetadataLinguisticsManager::pack_objects_metadata_to_dod_buffer(
    const godot::Array &p_objects
) {
    godot::PackedByteArray buffer;
    buffer.resize(static_cast<int64_t>(p_objects.size()) * static_cast<int64_t>(sizeof(core::LinguisticProfilePOD)));
    uint8_t *dst = buffer.ptrw();

    for (int i = 0; i < p_objects.size(); ++i) {
        core::LinguisticProfilePOD pod{};
        if (p_objects[i].get_type() == godot::Variant::OBJECT) {
            const godot::Object *obj = p_objects[i];
            if (obj && !read_profile_pod_from_metadata(obj, pod)) {
                godot::Ref<SemanticTagSetResource> set = read_tag_set_from_metadata(obj);
                pod = set->to_linguistic_profile_pod(core::LING_SOURCE_OBJECT_METADATA);
            }
        }
        std::memcpy(
            dst + static_cast<size_t>(i) * sizeof(core::LinguisticProfilePOD),
            &pod,
            sizeof(core::LinguisticProfilePOD)
        );
    }
    return buffer;
}

godot::Dictionary MetadataLinguisticsManager::get_object_metadata_snapshot(
    const godot::Object *p_object
) {
    godot::Dictionary snap;
    if (!p_object) return snap;
    snap["has_meta"]        = has_linguistic_metadata(p_object);
    snap["nuclear_tags"]    = get_object_nuclear_seme_tags(p_object);
    snap["peripheral_tags"] = get_object_peripheral_seme_tags(p_object);
    snap["valence"]         = get_object_emotive_valence(p_object);
    snap["lect_id"]         = get_object_lect_id(p_object);
    snap["aliases"]         = get_object_semantic_aliases(p_object);
    return snap;
}

void MetadataLinguisticsManager::apply_object_metadata_snapshot(
    godot::Object *p_object,
    const godot::Dictionary &p_snapshot
) {
    if (!p_object) return;
    if (!static_cast<bool>(p_snapshot.get("has_meta", false))) {
        clear_linguistic_metadata(p_object);
        return;
    }
    set_object_seme_tags(
        p_object,
        p_snapshot.get("nuclear_tags", godot::PackedStringArray()),
        p_snapshot.get("peripheral_tags", godot::PackedStringArray()),
        static_cast<float>(p_snapshot.get("valence", 0.0f)),
        static_cast<int>(core::LexicalCategory::NOUN),
        static_cast<int>(p_snapshot.get("lect_id", 0))
    );
    if (p_snapshot.has("aliases")) {
        set_object_semantic_aliases(p_object, p_snapshot["aliases"]);
    }
}

void MetadataLinguisticsManager::commit_object_seme_tags(
    godot::Object *p_object,
    const godot::PackedStringArray &p_nuclear_tags,
    const godot::PackedStringArray &p_peripheral_tags,
    const godot::String &p_action_name
) {
    if (!p_object) return;
    const godot::Dictionary before = get_object_metadata_snapshot(p_object);
    set_object_seme_tags(
        p_object,
        p_nuclear_tags,
        p_peripheral_tags,
        get_object_emotive_valence(p_object),
        static_cast<int>(core::LexicalCategory::NOUN),
        get_object_lect_id(p_object)
    );
    if (undo_redo_.is_valid()) {
        const godot::Dictionary after = get_object_metadata_snapshot(p_object);
        undo_redo_->create_action(p_action_name);
        undo_redo_->add_do_method(godot::Callable(this, "apply_object_metadata_snapshot").bind(p_object, after));
        undo_redo_->add_undo_method(godot::Callable(this, "apply_object_metadata_snapshot").bind(p_object, before));
        undo_redo_->commit_action(false);
    }
}

void MetadataLinguisticsManager::commit_clear_linguistic_metadata(
    godot::Object *p_object,
    const godot::String &p_action_name
) {
    if (!p_object) return;
    const godot::Dictionary before = get_object_metadata_snapshot(p_object);
    clear_linguistic_metadata(p_object);
    if (undo_redo_.is_valid()) {
        const godot::Dictionary after = get_object_metadata_snapshot(p_object);
        undo_redo_->create_action(p_action_name);
        undo_redo_->add_do_method(godot::Callable(this, "apply_object_metadata_snapshot").bind(p_object, after));
        undo_redo_->add_undo_method(godot::Callable(this, "apply_object_metadata_snapshot").bind(p_object, before));
        undo_redo_->commit_action(false);
    }
}

} // namespace ideam::godot_ext

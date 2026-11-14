#include "sememe_resource.h"
#include <algorithm>
#include <cstring>
#include <span>

namespace ideam::godot_ext {

// ============================================================================
// SememeResource Implementation
// ============================================================================

void SememeResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &SememeResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &SememeResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_unique_id", "unique_id"), &SememeResource::set_unique_id);
    ClassDB::bind_method(D_METHOD("get_unique_id"), &SememeResource::get_unique_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "unique_id"), "set_unique_id", "get_unique_id");

    ClassDB::bind_method(D_METHOD("set_canonical_label", "label"), &SememeResource::set_canonical_label);
    ClassDB::bind_method(D_METHOD("get_canonical_label"), &SememeResource::get_canonical_label);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "canonical_label"), "set_canonical_label", "get_canonical_label");

    ClassDB::bind_method(D_METHOD("set_wordnet_synset_id", "synset_id"), &SememeResource::set_wordnet_synset_id);
    ClassDB::bind_method(D_METHOD("get_wordnet_synset_id"), &SememeResource::get_wordnet_synset_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "wordnet_synset_id"), "set_wordnet_synset_id", "get_wordnet_synset_id");

    ClassDB::bind_method(D_METHOD("set_sememic_form", "form"), &SememeResource::set_sememic_form);
    ClassDB::bind_method(D_METHOD("get_sememic_form"), &SememeResource::get_sememic_form);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "sememic_form",
            PROPERTY_HINT_ENUM,
            "Not Set:0,Unknown:1,Primary Denotation:2,Secondary Denotation:3,Analogous Connotation:4,Emotive Connotation:5,Evaluative Connotation:6"
        ),
        "set_sememic_form",
        "get_sememic_form"
    );

    ClassDB::bind_method(D_METHOD("set_lexical_category", "category"), &SememeResource::set_lexical_category);
    ClassDB::bind_method(D_METHOD("get_lexical_category"), &SememeResource::get_lexical_category);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "lexical_category",
            PROPERTY_HINT_ENUM,
            "X (Undefined):0,NOUN:1,VERB:2,ADJ:3,ADV:4,ADP:5,PRON:6,DET:7,CCONJ:8,PART:9,PROPN:10,NUM:11,AUX:12,SCONJ:13,INTJ:14,SYM:15,PUNCT:16"
        ),
        "set_lexical_category",
        "get_lexical_category"
    );

    ClassDB::bind_method(D_METHOD("set_upos_tag", "upos"), &SememeResource::set_upos_tag);
    ClassDB::bind_method(D_METHOD("get_upos_tag"), &SememeResource::get_upos_tag);

    ClassDB::bind_method(D_METHOD("set_primary_domain_id", "domain_id"), &SememeResource::set_primary_domain_id);
    ClassDB::bind_method(D_METHOD("get_primary_domain_id"), &SememeResource::get_primary_domain_id);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "primary_domain_id",
            PROPERTY_HINT_ENUM,
            "General:0,Spatial Topology:1,Temporal Era:2,Genre & Tone:3,System Mechanic:4,Entity Ontology:5,Capability & Action:6,Affect & Social:7,UI & Reflection:8"
        ),
        "set_primary_domain_id",
        "get_primary_domain_id"
    );

    ClassDB::bind_method(D_METHOD("set_emotive_valence", "valence"), &SememeResource::set_emotive_valence);
    ClassDB::bind_method(D_METHOD("get_emotive_valence"), &SememeResource::get_emotive_valence);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "emotive_valence", PROPERTY_HINT_RANGE, "-1.0,1.0,0.01"),
        "set_emotive_valence",
        "get_emotive_valence"
    );

    ClassDB::bind_method(D_METHOD("set_gloss_description", "gloss"), &SememeResource::set_gloss_description);
    ClassDB::bind_method(D_METHOD("get_gloss_description"), &SememeResource::get_gloss_description);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "gloss_description", PROPERTY_HINT_MULTILINE_TEXT),
        "set_gloss_description",
        "get_gloss_description"
    );

    ClassDB::bind_method(D_METHOD("set_nuclear_seme_tags", "tags"), &SememeResource::set_nuclear_seme_tags);
    ClassDB::bind_method(D_METHOD("get_nuclear_seme_tags"), &SememeResource::get_nuclear_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "nuclear_seme_tags"), "set_nuclear_seme_tags", "get_nuclear_seme_tags");

    ClassDB::bind_method(D_METHOD("set_peripheral_seme_tags", "tags"), &SememeResource::set_peripheral_seme_tags);
    ClassDB::bind_method(D_METHOD("get_peripheral_seme_tags"), &SememeResource::get_peripheral_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "peripheral_seme_tags"), "set_peripheral_seme_tags", "get_peripheral_seme_tags");

    ClassDB::bind_method(D_METHOD("set_antonym_seme_tags", "tags"), &SememeResource::set_antonym_seme_tags);
    ClassDB::bind_method(D_METHOD("get_antonym_seme_tags"), &SememeResource::get_antonym_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "antonym_seme_tags"), "set_antonym_seme_tags", "get_antonym_seme_tags");

    ClassDB::bind_method(D_METHOD("set_hypernym_labels", "labels"), &SememeResource::set_hypernym_labels);
    ClassDB::bind_method(D_METHOD("get_hypernym_labels"), &SememeResource::get_hypernym_labels);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "hypernym_labels"), "set_hypernym_labels", "get_hypernym_labels");

    ClassDB::bind_method(D_METHOD("set_holonym_labels", "labels"), &SememeResource::set_holonym_labels);
    ClassDB::bind_method(D_METHOD("get_holonym_labels"), &SememeResource::get_holonym_labels);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "holonym_labels"), "set_holonym_labels", "get_holonym_labels");

    ClassDB::bind_method(D_METHOD("set_mrl_float_embedding", "embedding"), &SememeResource::set_mrl_float_embedding);
    ClassDB::bind_method(D_METHOD("get_mrl_float_embedding"), &SememeResource::get_mrl_float_embedding);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "mrl_float_embedding"), "set_mrl_float_embedding", "get_mrl_float_embedding");

    ClassDB::bind_method(D_METHOD("get_mrl_binary_embedding_words"), &SememeResource::get_mrl_binary_embedding_words);
    ClassDB::bind_method(
        D_METHOD("compute_mrl_hamming_similarity_with", "other"),
        &SememeResource::compute_mrl_hamming_similarity_with
    );

    ClassDB::bind_method(D_METHOD("add_nuclear_seme_tag", "tag"), &SememeResource::add_nuclear_seme_tag);
    ClassDB::bind_method(D_METHOD("remove_nuclear_seme_tag", "tag"), &SememeResource::remove_nuclear_seme_tag);
    ClassDB::bind_method(D_METHOD("add_peripheral_seme_tag", "tag"), &SememeResource::add_peripheral_seme_tag);
    ClassDB::bind_method(D_METHOD("remove_peripheral_seme_tag", "tag"), &SememeResource::remove_peripheral_seme_tag);

    ClassDB::bind_method(D_METHOD("has_nuclear_seme", "tag"), &SememeResource::has_nuclear_seme);
    ClassDB::bind_method(D_METHOD("has_peripheral_seme", "tag"), &SememeResource::has_peripheral_seme);
    ClassDB::bind_method(D_METHOD("has_any_seme", "tag"), &SememeResource::has_any_seme);

    ClassDB::bind_method(D_METHOD("get_nuclear_bitmask_words"), &SememeResource::get_nuclear_bitmask_words);
    ClassDB::bind_method(D_METHOD("get_peripheral_bitmask_words"), &SememeResource::get_peripheral_bitmask_words);
    ClassDB::bind_method(D_METHOD("get_nuclear_popcount"), &SememeResource::get_nuclear_popcount);
    ClassDB::bind_method(D_METHOD("get_peripheral_popcount"), &SememeResource::get_peripheral_popcount);
    ClassDB::bind_method(D_METHOD("get_coherence_trait_mask32"), &SememeResource::get_coherence_trait_mask32);

    ClassDB::bind_method(
        D_METHOD("compute_affinity_with", "other", "nuclear_weight", "peripheral_weight"),
        &SememeResource::compute_affinity_with,
        DEFVAL(0.75f),
        DEFVAL(0.25f)
    );
    ClassDB::bind_method(
        D_METHOD("compute_tversky_affinity_with", "other", "alpha", "beta"),
        &SememeResource::compute_tversky_affinity_with,
        DEFVAL(0.75f),
        DEFVAL(0.25f)
    );

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &SememeResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &SememeResource::apply_snapshot_state);
    ClassDB::bind_method(
        D_METHOD("commit_seme_tags", "nuclear_tags", "peripheral_tags", "action_name"),
        &SememeResource::commit_seme_tags,
        DEFVAL("Modify Sememe Tags")
    );
}

void SememeResource::_rebuild_descriptor_if_dirty() const {
    if (!descriptor_dirty_) {
        return;
    }
    cached_descriptor_ = core::SememeDescriptorPOD{};
    cached_descriptor_.sememe_id = static_cast<uint32_t>(std::max(0, unique_id_));

    if (!wordnet_synset_id_.is_empty()) {
        cached_descriptor_.canonical_name_hash =
            core::hash_linguistic_token(wordnet_synset_id_.utf8().get_data());
    } else {
        cached_descriptor_.canonical_name_hash =
            core::hash_linguistic_token(godot::String(canonical_label_).utf8().get_data());
    }

    cached_descriptor_.emotive_valence   = emotive_valence_;
    cached_descriptor_.sememic_form      = static_cast<uint8_t>(std::clamp(sememic_form_, 0, 255));
    cached_descriptor_.taxeme_category   = static_cast<uint8_t>(std::clamp(lexical_category_, 0, 255));
    cached_descriptor_.primary_domain_id = static_cast<uint16_t>(std::clamp(primary_domain_id_, 0, 65535));

    for (int i = 0; i < nuclear_seme_tags_.size(); ++i) {
        const godot::CharString cs = nuclear_seme_tags_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_descriptor_.set_nuclear_bit(bit);
        }
    }
    for (int i = 0; i < hypernym_labels_.size(); ++i) {
        const godot::CharString cs = hypernym_labels_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_descriptor_.set_nuclear_bit(bit);
        }
    }
    for (int i = 0; i < peripheral_seme_tags_.size(); ++i) {
        const godot::CharString cs = peripheral_seme_tags_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_descriptor_.set_peripheral_bit(bit);
        }
    }
    for (int i = 0; i < holonym_labels_.size(); ++i) {
        const godot::CharString cs = holonym_labels_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_descriptor_.set_peripheral_bit(bit);
        }
    }

    // If a Matryoshka (MRL) float embedding vector is attached, binary-quantize it into words[1..2] + peripheral tail
    if (!mrl_float_embedding_.is_empty()) {
        std::span<const float> float_span(
            mrl_float_embedding_.ptr(),
            static_cast<size_t>(mrl_float_embedding_.size())
        );
        core::SemanticLexiconCore::quantize_mrl_float_embedding(float_span, cached_descriptor_, true);
    }

    for (int i = 0; i < antonym_seme_tags_.size(); ++i) {
        const godot::CharString cs = antonym_seme_tags_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_descriptor_.clear_nuclear_bit(bit);
            cached_descriptor_.clear_peripheral_bit(bit);
        }
    }

    descriptor_dirty_ = false;
}

void SememeResource::set_unique_id(int p_id) {
    if (unique_id_ == p_id) return;
    unique_id_ = p_id;
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::set_canonical_label(const godot::StringName &p_label) {
    if (canonical_label_ == p_label) return;
    canonical_label_ = p_label;
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::set_wordnet_synset_id(const godot::String &p_synset_id) {
    if (wordnet_synset_id_ == p_synset_id) return;
    wordnet_synset_id_ = p_synset_id;
    descriptor_dirty_  = true;
    emit_changed();
}

void SememeResource::set_sememic_form(int p_form) {
    int clamped = std::clamp(p_form, 0, 6);
    if (sememic_form_ == clamped) return;
    sememic_form_ = clamped;
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::set_lexical_category(int p_category) {
    int clamped = std::clamp(p_category, 0, 16);
    if (lexical_category_ == clamped) return;
    lexical_category_ = clamped;
    descriptor_dirty_ = true;
    emit_changed();
}

godot::String SememeResource::get_upos_tag() const {
    const std::string_view sv = core::upos_to_string(static_cast<core::UniversalPOS>(lexical_category_));
    return godot::String::utf8(sv.data(), static_cast<int>(sv.size()));
}

void SememeResource::set_upos_tag(const godot::String &p_upos) {
    set_lexical_category(static_cast<int>(core::parse_upos_tag(p_upos.utf8().get_data())));
}

void SememeResource::set_primary_domain_id(int p_domain) {
    int clamped = std::clamp(p_domain, 0, 65535);
    if (primary_domain_id_ == clamped) return;
    primary_domain_id_ = clamped;
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::set_emotive_valence(float p_valence) {
    float clamped = std::clamp(p_valence, -1.0f, 1.0f);
    if (emotive_valence_ == clamped) return;
    emotive_valence_ = clamped;
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::set_gloss_description(const godot::String &p_gloss) {
    if (gloss_description_ == p_gloss) return;
    gloss_description_ = p_gloss;
    emit_changed();
}

void SememeResource::set_nuclear_seme_tags(const godot::PackedStringArray &p_tags) {
    nuclear_seme_tags_ = p_tags;
    descriptor_dirty_  = true;
    emit_changed();
}

void SememeResource::set_peripheral_seme_tags(const godot::PackedStringArray &p_tags) {
    peripheral_seme_tags_ = p_tags;
    descriptor_dirty_     = true;
    emit_changed();
}

void SememeResource::set_antonym_seme_tags(const godot::PackedStringArray &p_tags) {
    antonym_seme_tags_ = p_tags;
    descriptor_dirty_  = true;
    emit_changed();
}

void SememeResource::set_hypernym_labels(const godot::PackedStringArray &p_labels) {
    hypernym_labels_  = p_labels;
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::set_holonym_labels(const godot::PackedStringArray &p_labels) {
    holonym_labels_   = p_labels;
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::set_mrl_float_embedding(const godot::PackedFloat32Array &p_embedding) {
    mrl_float_embedding_ = p_embedding;
    descriptor_dirty_    = true;
    emit_changed();
}

godot::PackedInt64Array SememeResource::get_mrl_binary_embedding_words() const {
    _rebuild_descriptor_if_dirty();
    godot::PackedInt64Array arr;
    arr.resize(2);
    arr.set(0, static_cast<int64_t>(cached_descriptor_.nuclear_seme_words[1]));
    arr.set(1, static_cast<int64_t>(cached_descriptor_.nuclear_seme_words[2]));
    return arr;
}

float SememeResource::compute_mrl_hamming_similarity_with(
    const godot::Ref<SememeResource> &p_other
) const {
    if (p_other.is_null()) return 0.0f;
    return get_compiled_descriptor().mrl_hamming_similarity_128(p_other->get_compiled_descriptor());
}

void SememeResource::add_nuclear_seme_tag(const godot::String &p_tag) {
    if (p_tag.is_empty() || nuclear_seme_tags_.has(p_tag)) return;
    nuclear_seme_tags_.push_back(p_tag);
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::remove_nuclear_seme_tag(const godot::String &p_tag) {
    int64_t idx = nuclear_seme_tags_.find(p_tag);
    if (idx >= 0) {
        nuclear_seme_tags_.remove_at(idx);
        descriptor_dirty_ = true;
        emit_changed();
    }
}

void SememeResource::add_peripheral_seme_tag(const godot::String &p_tag) {
    if (p_tag.is_empty() || peripheral_seme_tags_.has(p_tag)) return;
    peripheral_seme_tags_.push_back(p_tag);
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::remove_peripheral_seme_tag(const godot::String &p_tag) {
    int64_t idx = peripheral_seme_tags_.find(p_tag);
    if (idx >= 0) {
        peripheral_seme_tags_.remove_at(idx);
        descriptor_dirty_ = true;
        emit_changed();
    }
}

bool SememeResource::has_nuclear_seme(const godot::StringName &p_tag) const {
    godot::String tag_str(p_tag);
    if (nuclear_seme_tags_.has(tag_str) || hypernym_labels_.has(tag_str)) {
        return true;
    }
    _rebuild_descriptor_if_dirty();
    const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(tag_str.utf8().get_data());
    return cached_descriptor_.test_nuclear_bit(bit);
}

bool SememeResource::has_peripheral_seme(const godot::StringName &p_tag) const {
    godot::String tag_str(p_tag);
    if (peripheral_seme_tags_.has(tag_str) || holonym_labels_.has(tag_str)) {
        return true;
    }
    _rebuild_descriptor_if_dirty();
    const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(tag_str.utf8().get_data());
    return cached_descriptor_.test_peripheral_bit(bit);
}

bool SememeResource::has_any_seme(const godot::StringName &p_tag) const {
    return has_nuclear_seme(p_tag) || has_peripheral_seme(p_tag);
}

const core::SememeDescriptorPOD &SememeResource::get_compiled_descriptor() const {
    _rebuild_descriptor_if_dirty();
    return cached_descriptor_;
}

godot::PackedInt64Array SememeResource::get_nuclear_bitmask_words() const {
    _rebuild_descriptor_if_dirty();
    godot::PackedInt64Array arr;
    arr.resize(3);
    for (int i = 0; i < 3; ++i) {
        arr.set(i, static_cast<int64_t>(cached_descriptor_.nuclear_seme_words[static_cast<size_t>(i)]));
    }
    return arr;
}

godot::PackedInt64Array SememeResource::get_peripheral_bitmask_words() const {
    _rebuild_descriptor_if_dirty();
    godot::PackedInt64Array arr;
    arr.resize(3);
    for (int i = 0; i < 3; ++i) {
        arr.set(i, static_cast<int64_t>(cached_descriptor_.peripheral_seme_words[static_cast<size_t>(i)]));
    }
    return arr;
}

int SememeResource::get_nuclear_popcount() const {
    _rebuild_descriptor_if_dirty();
    return static_cast<int>(cached_descriptor_.nuclear_popcount());
}

int SememeResource::get_peripheral_popcount() const {
    _rebuild_descriptor_if_dirty();
    return static_cast<int>(cached_descriptor_.peripheral_popcount());
}

int64_t SememeResource::get_coherence_trait_mask32() const {
    _rebuild_descriptor_if_dirty();
    return static_cast<int64_t>(cached_descriptor_.compute_coherence_trait_mask32());
}

float SememeResource::compute_affinity_with(
    const godot::Ref<SememeResource> &p_other,
    float p_nuclear_weight,
    float p_peripheral_weight
) const {
    if (p_other.is_null()) {
        return 0.0f;
    }
    return core::SemanticLexiconCore::compute_descriptor_affinity(
        get_compiled_descriptor(),
        p_other->get_compiled_descriptor(),
        p_nuclear_weight,
        p_peripheral_weight
    );
}

float SememeResource::compute_tversky_affinity_with(
    const godot::Ref<SememeResource> &p_other,
    float p_alpha,
    float p_beta
) const {
    if (p_other.is_null()) {
        return 0.0f;
    }
    return core::SemanticLexiconCore::compute_descriptor_tversky_affinity(
        get_compiled_descriptor(),
        p_other->get_compiled_descriptor(),
        p_alpha,
        p_beta
    );
}

godot::Dictionary SememeResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["unique_id"]            = unique_id_;
    d["canonical_label"]      = canonical_label_;
    d["wordnet_synset_id"]    = wordnet_synset_id_;
    d["sememic_form"]         = sememic_form_;
    d["lexical_category"]     = lexical_category_;
    d["primary_domain_id"]    = primary_domain_id_;
    d["emotive_valence"]      = emotive_valence_;
    d["gloss_description"]    = gloss_description_;
    d["nuclear_seme_tags"]    = nuclear_seme_tags_.duplicate();
    d["peripheral_seme_tags"] = peripheral_seme_tags_.duplicate();
    d["antonym_seme_tags"]    = antonym_seme_tags_.duplicate();
    d["hypernym_labels"]      = hypernym_labels_.duplicate();
    d["holonym_labels"]       = holonym_labels_.duplicate();
    d["mrl_float_embedding"]  = mrl_float_embedding_.duplicate();
    return d;
}

void SememeResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("unique_id"))            unique_id_            = static_cast<int>(p_state["unique_id"]);
    if (p_state.has("canonical_label"))      canonical_label_      = p_state["canonical_label"];
    if (p_state.has("wordnet_synset_id"))    wordnet_synset_id_    = p_state["wordnet_synset_id"];
    if (p_state.has("sememic_form"))         sememic_form_         = static_cast<int>(p_state["sememic_form"]);
    if (p_state.has("lexical_category"))     lexical_category_     = static_cast<int>(p_state["lexical_category"]);
    if (p_state.has("primary_domain_id"))    primary_domain_id_    = static_cast<int>(p_state["primary_domain_id"]);
    if (p_state.has("emotive_valence"))      emotive_valence_      = static_cast<float>(p_state["emotive_valence"]);
    if (p_state.has("gloss_description"))    gloss_description_    = p_state["gloss_description"];
    if (p_state.has("nuclear_seme_tags"))    nuclear_seme_tags_    = p_state["nuclear_seme_tags"];
    if (p_state.has("peripheral_seme_tags")) peripheral_seme_tags_ = p_state["peripheral_seme_tags"];
    if (p_state.has("antonym_seme_tags"))    antonym_seme_tags_    = p_state["antonym_seme_tags"];
    if (p_state.has("hypernym_labels"))      hypernym_labels_      = p_state["hypernym_labels"];
    if (p_state.has("holonym_labels"))       holonym_labels_       = p_state["holonym_labels"];
    if (p_state.has("mrl_float_embedding"))  mrl_float_embedding_  = p_state["mrl_float_embedding"];
    descriptor_dirty_ = true;
    emit_changed();
}

void SememeResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) {
        return;
    }
    const godot::Dictionary after_state = get_snapshot_state();
    undo_redo_->create_action(p_action_name);
    undo_redo_->add_do_method(godot::Callable(this, "apply_snapshot_state").bind(after_state));
    undo_redo_->add_undo_method(godot::Callable(this, "apply_snapshot_state").bind(p_before_state));
    undo_redo_->commit_action(false);
}

void SememeResource::commit_seme_tags(
    const godot::PackedStringArray &p_nuclear_tags,
    const godot::PackedStringArray &p_peripheral_tags,
    const godot::String &p_action_name
) {
    const godot::Dictionary before = get_snapshot_state();
    nuclear_seme_tags_    = p_nuclear_tags;
    peripheral_seme_tags_ = p_peripheral_tags;
    descriptor_dirty_     = true;
    emit_changed();
    commit_snapshot_mutation(p_action_name, before);
}

// ============================================================================
// SemanticTagSetResource Implementation
// ============================================================================

void SemanticTagSetResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &SemanticTagSetResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &SemanticTagSetResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_sememes", "sememes"), &SemanticTagSetResource::set_sememes);
    ClassDB::bind_method(D_METHOD("get_sememes"), &SemanticTagSetResource::get_sememes);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "sememes", PROPERTY_HINT_ARRAY_TYPE, "SememeResource"),
        "set_sememes",
        "get_sememes"
    );

    ClassDB::bind_method(D_METHOD("set_explicit_nuclear_tags", "tags"), &SemanticTagSetResource::set_explicit_nuclear_tags);
    ClassDB::bind_method(D_METHOD("get_explicit_nuclear_tags"), &SemanticTagSetResource::get_explicit_nuclear_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_STRING_ARRAY, "explicit_nuclear_tags"),
        "set_explicit_nuclear_tags",
        "get_explicit_nuclear_tags"
    );

    ClassDB::bind_method(D_METHOD("set_explicit_peripheral_tags", "tags"), &SemanticTagSetResource::set_explicit_peripheral_tags);
    ClassDB::bind_method(D_METHOD("get_explicit_peripheral_tags"), &SemanticTagSetResource::get_explicit_peripheral_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_STRING_ARRAY, "explicit_peripheral_tags"),
        "set_explicit_peripheral_tags",
        "get_explicit_peripheral_tags"
    );

    ClassDB::bind_method(D_METHOD("set_excluded_seme_tags", "tags"), &SemanticTagSetResource::set_excluded_seme_tags);
    ClassDB::bind_method(D_METHOD("get_excluded_seme_tags"), &SemanticTagSetResource::get_excluded_seme_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_STRING_ARRAY, "excluded_seme_tags"),
        "set_excluded_seme_tags",
        "get_excluded_seme_tags"
    );

    ClassDB::bind_method(D_METHOD("set_mrl_float_embedding", "embedding"), &SemanticTagSetResource::set_mrl_float_embedding);
    ClassDB::bind_method(D_METHOD("get_mrl_float_embedding"), &SemanticTagSetResource::get_mrl_float_embedding);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "mrl_float_embedding"),
        "set_mrl_float_embedding",
        "get_mrl_float_embedding"
    );

    ClassDB::bind_method(D_METHOD("set_source_prompt", "prompt"), &SemanticTagSetResource::set_source_prompt);
    ClassDB::bind_method(D_METHOD("get_source_prompt"), &SemanticTagSetResource::get_source_prompt);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_prompt"), "set_source_prompt", "get_source_prompt");

    ClassDB::bind_method(D_METHOD("set_aggregate_valence", "valence"), &SemanticTagSetResource::set_aggregate_valence);
    ClassDB::bind_method(D_METHOD("get_aggregate_valence"), &SemanticTagSetResource::get_aggregate_valence);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "aggregate_valence", PROPERTY_HINT_RANGE, "-1.0,1.0,0.01"),
        "set_aggregate_valence",
        "get_aggregate_valence"
    );

    ClassDB::bind_method(D_METHOD("add_sememe", "sememe"), &SemanticTagSetResource::add_sememe);
    ClassDB::bind_method(D_METHOD("remove_sememe", "sememe"), &SemanticTagSetResource::remove_sememe);
    ClassDB::bind_method(D_METHOD("clear_sememes"), &SemanticTagSetResource::clear_sememes);

    ClassDB::bind_method(D_METHOD("add_nuclear_tag", "tag"), &SemanticTagSetResource::add_nuclear_tag);
    ClassDB::bind_method(D_METHOD("add_peripheral_tag", "tag"), &SemanticTagSetResource::add_peripheral_tag);
    ClassDB::bind_method(D_METHOD("add_excluded_tag", "tag"), &SemanticTagSetResource::add_excluded_tag);

    ClassDB::bind_method(D_METHOD("has_nuclear_tag", "tag"), &SemanticTagSetResource::has_nuclear_tag);
    ClassDB::bind_method(D_METHOD("has_peripheral_tag", "tag"), &SemanticTagSetResource::has_peripheral_tag);
    ClassDB::bind_method(D_METHOD("has_any_tag", "tag"), &SemanticTagSetResource::has_any_tag);

    ClassDB::bind_method(D_METHOD("get_all_nuclear_tags"), &SemanticTagSetResource::get_all_nuclear_tags);
    ClassDB::bind_method(D_METHOD("get_all_peripheral_tags"), &SemanticTagSetResource::get_all_peripheral_tags);

    ClassDB::bind_method(D_METHOD("merge_from", "other"), &SemanticTagSetResource::merge_from);

    ClassDB::bind_method(D_METHOD("get_nuclear_bitmask_words"), &SemanticTagSetResource::get_nuclear_bitmask_words);
    ClassDB::bind_method(D_METHOD("get_peripheral_bitmask_words"), &SemanticTagSetResource::get_peripheral_bitmask_words);
    ClassDB::bind_method(
        D_METHOD("set_raw_bitmask_words", "nuclear_words", "peripheral_words"),
        &SemanticTagSetResource::set_raw_bitmask_words
    );
    ClassDB::bind_method(D_METHOD("get_nuclear_popcount"), &SemanticTagSetResource::get_nuclear_popcount);
    ClassDB::bind_method(D_METHOD("get_peripheral_popcount"), &SemanticTagSetResource::get_peripheral_popcount);
    ClassDB::bind_method(D_METHOD("get_coherence_trait_mask32"), &SemanticTagSetResource::get_coherence_trait_mask32);

    ClassDB::bind_method(
        D_METHOD("to_profile_pod_bytes", "source_channel_mask"),
        &SemanticTagSetResource::to_profile_pod_bytes,
        DEFVAL(static_cast<int>(core::LING_SOURCE_EXPLICIT_RESOURCE))
    );
    ClassDB::bind_method(
        D_METHOD("load_from_profile_pod_bytes", "bytes"),
        &SemanticTagSetResource::load_from_profile_pod_bytes
    );

    ClassDB::bind_method(
        D_METHOD("compute_affinity_with", "other", "nuclear_weight", "peripheral_weight"),
        &SemanticTagSetResource::compute_affinity_with,
        DEFVAL(0.75f),
        DEFVAL(0.25f)
    );
    ClassDB::bind_method(
        D_METHOD("compute_tversky_affinity_with", "candidate", "alpha", "beta"),
        &SemanticTagSetResource::compute_tversky_affinity_with,
        DEFVAL(0.75f),
        DEFVAL(0.25f)
    );
    ClassDB::bind_method(
        D_METHOD("matches_required_nuclear_tags", "required"),
        &SemanticTagSetResource::matches_required_nuclear_tags
    );

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &SemanticTagSetResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &SemanticTagSetResource::apply_snapshot_state);
}

void SemanticTagSetResource::_rebuild_union_descriptor_if_dirty() const {
    if (!union_dirty_) {
        return;
    }
    cached_union_descriptor_.clear_bits();
    cached_union_descriptor_.emotive_valence = aggregate_valence_;

    float valence_accum = 0.0f;
    int valence_count   = 0;

    for (int i = 0; i < sememes_.size(); ++i) {
        godot::Ref<SememeResource> sem = sememes_[i];
        if (sem.is_valid()) {
            const auto &desc = sem->get_compiled_descriptor();
            cached_union_descriptor_.union_with(desc);
            if (i == 0) {
                cached_union_descriptor_.sememe_id         = desc.sememe_id;
                cached_union_descriptor_.taxeme_category   = desc.taxeme_category;
                cached_union_descriptor_.primary_domain_id = desc.primary_domain_id;
            }
            valence_accum += sem->get_emotive_valence();
            ++valence_count;
        }
    }

    if (valence_count > 0 && std::abs(aggregate_valence_) < 1e-5f) {
        cached_union_descriptor_.emotive_valence =
            std::clamp(valence_accum / static_cast<float>(valence_count), -1.0f, 1.0f);
    }

    for (int i = 0; i < explicit_nuclear_tags_.size(); ++i) {
        const godot::CharString cs = explicit_nuclear_tags_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_union_descriptor_.set_nuclear_bit(bit);
        }
    }

    for (int i = 0; i < explicit_peripheral_tags_.size(); ++i) {
        const godot::CharString cs = explicit_peripheral_tags_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_union_descriptor_.set_peripheral_bit(bit);
        }
    }

    if (!mrl_float_embedding_.is_empty()) {
        std::span<const float> float_span(
            mrl_float_embedding_.ptr(),
            static_cast<size_t>(mrl_float_embedding_.size())
        );
        core::SemanticLexiconCore::quantize_mrl_float_embedding(float_span, cached_union_descriptor_, true);
    }

    for (int i = 0; i < excluded_seme_tags_.size(); ++i) {
        const godot::CharString cs = excluded_seme_tags_[i].utf8();
        if (cs.length() > 0) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(cs.get_data());
            cached_union_descriptor_.clear_nuclear_bit(bit);
            cached_union_descriptor_.clear_peripheral_bit(bit);
        }
    }

    union_dirty_ = false;
}

void SemanticTagSetResource::set_sememes(const godot::TypedArray<SememeResource> &p_sememes) {
    sememes_     = p_sememes;
    union_dirty_ = true;
    emit_changed();
}

void SemanticTagSetResource::set_explicit_nuclear_tags(const godot::PackedStringArray &p_tags) {
    explicit_nuclear_tags_ = p_tags;
    union_dirty_           = true;
    emit_changed();
}

void SemanticTagSetResource::set_explicit_peripheral_tags(const godot::PackedStringArray &p_tags) {
    explicit_peripheral_tags_ = p_tags;
    union_dirty_              = true;
    emit_changed();
}

void SemanticTagSetResource::set_excluded_seme_tags(const godot::PackedStringArray &p_tags) {
    excluded_seme_tags_ = p_tags;
    union_dirty_        = true;
    emit_changed();
}

void SemanticTagSetResource::set_mrl_float_embedding(const godot::PackedFloat32Array &p_embedding) {
    mrl_float_embedding_ = p_embedding;
    union_dirty_         = true;
    emit_changed();
}

void SemanticTagSetResource::set_source_prompt(const godot::String &p_prompt) {
    if (source_prompt_ == p_prompt) return;
    source_prompt_ = p_prompt;
    emit_changed();
}

void SemanticTagSetResource::set_aggregate_valence(float p_valence) {
    float clamped = std::clamp(p_valence, -1.0f, 1.0f);
    if (aggregate_valence_ == clamped) return;
    aggregate_valence_ = clamped;
    union_dirty_       = true;
    emit_changed();
}

void SemanticTagSetResource::add_sememe(const godot::Ref<SememeResource> &p_sememe) {
    if (p_sememe.is_null()) return;
    if (!sememes_.has(p_sememe)) {
        sememes_.push_back(p_sememe);
        union_dirty_ = true;
        emit_changed();
    }
}

void SemanticTagSetResource::remove_sememe(const godot::Ref<SememeResource> &p_sememe) {
    int64_t idx = sememes_.find(p_sememe);
    if (idx >= 0) {
        sememes_.remove_at(idx);
        union_dirty_ = true;
        emit_changed();
    }
}

void SemanticTagSetResource::clear_sememes() {
    sememes_.clear();
    explicit_nuclear_tags_.clear();
    explicit_peripheral_tags_.clear();
    excluded_seme_tags_.clear();
    mrl_float_embedding_.clear();
    union_dirty_ = true;
    emit_changed();
}

void SemanticTagSetResource::add_nuclear_tag(const godot::String &p_tag) {
    if (p_tag.is_empty() || explicit_nuclear_tags_.has(p_tag)) return;
    explicit_nuclear_tags_.push_back(p_tag);
    union_dirty_ = true;
    emit_changed();
}

void SemanticTagSetResource::add_peripheral_tag(const godot::String &p_tag) {
    if (p_tag.is_empty() || explicit_peripheral_tags_.has(p_tag)) return;
    explicit_peripheral_tags_.push_back(p_tag);
    union_dirty_ = true;
    emit_changed();
}

void SemanticTagSetResource::add_excluded_tag(const godot::String &p_tag) {
    if (p_tag.is_empty() || excluded_seme_tags_.has(p_tag)) return;
    excluded_seme_tags_.push_back(p_tag);
    union_dirty_ = true;
    emit_changed();
}

bool SemanticTagSetResource::has_nuclear_tag(const godot::StringName &p_tag) const {
    godot::String tag_str(p_tag);
    if (explicit_nuclear_tags_.has(tag_str)) return true;
    _rebuild_union_descriptor_if_dirty();
    const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(tag_str.utf8().get_data());
    return cached_union_descriptor_.test_nuclear_bit(bit);
}

bool SemanticTagSetResource::has_peripheral_tag(const godot::StringName &p_tag) const {
    godot::String tag_str(p_tag);
    if (explicit_peripheral_tags_.has(tag_str)) return true;
    _rebuild_union_descriptor_if_dirty();
    const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(tag_str.utf8().get_data());
    return cached_union_descriptor_.test_peripheral_bit(bit);
}

bool SemanticTagSetResource::has_any_tag(const godot::StringName &p_tag) const {
    return has_nuclear_tag(p_tag) || has_peripheral_tag(p_tag);
}

godot::PackedStringArray SemanticTagSetResource::get_all_nuclear_tags() const {
    godot::PackedStringArray out = explicit_nuclear_tags_.duplicate();
    for (int i = 0; i < sememes_.size(); ++i) {
        godot::Ref<SememeResource> sem = sememes_[i];
        if (sem.is_valid()) {
            const godot::PackedStringArray sub = sem->get_nuclear_seme_tags();
            for (int j = 0; j < sub.size(); ++j) {
                if (!out.has(sub[j]) && !excluded_seme_tags_.has(sub[j])) {
                    out.push_back(sub[j]);
                }
            }
        }
    }
    return out;
}

godot::PackedStringArray SemanticTagSetResource::get_all_peripheral_tags() const {
    godot::PackedStringArray out = explicit_peripheral_tags_.duplicate();
    for (int i = 0; i < sememes_.size(); ++i) {
        godot::Ref<SememeResource> sem = sememes_[i];
        if (sem.is_valid()) {
            const godot::PackedStringArray sub = sem->get_peripheral_seme_tags();
            for (int j = 0; j < sub.size(); ++j) {
                if (!out.has(sub[j]) && !excluded_seme_tags_.has(sub[j])) {
                    out.push_back(sub[j]);
                }
            }
        }
    }
    return out;
}

void SemanticTagSetResource::merge_from(const godot::Ref<SemanticTagSetResource> &p_other) {
    if (p_other.is_null() || p_other.ptr() == this) {
        return;
    }
    const godot::TypedArray<SememeResource> other_sems = p_other->get_sememes();
    for (int i = 0; i < other_sems.size(); ++i) {
        add_sememe(other_sems[i]);
    }
    const godot::PackedStringArray nuc = p_other->get_explicit_nuclear_tags();
    for (int i = 0; i < nuc.size(); ++i) {
        add_nuclear_tag(nuc[i]);
    }
    const godot::PackedStringArray per = p_other->get_explicit_peripheral_tags();
    for (int i = 0; i < per.size(); ++i) {
        add_peripheral_tag(per[i]);
    }
    const godot::PackedStringArray exc = p_other->get_excluded_seme_tags();
    for (int i = 0; i < exc.size(); ++i) {
        add_excluded_tag(exc[i]);
    }
    union_dirty_ = true;
    emit_changed();
}

const core::SememeDescriptorPOD &SemanticTagSetResource::get_compiled_descriptor() const {
    _rebuild_union_descriptor_if_dirty();
    return cached_union_descriptor_;
}

godot::PackedInt64Array SemanticTagSetResource::get_nuclear_bitmask_words() const {
    _rebuild_union_descriptor_if_dirty();
    godot::PackedInt64Array arr;
    arr.resize(3);
    for (int i = 0; i < 3; ++i) {
        arr.set(i, static_cast<int64_t>(cached_union_descriptor_.nuclear_seme_words[static_cast<size_t>(i)]));
    }
    return arr;
}

godot::PackedInt64Array SemanticTagSetResource::get_peripheral_bitmask_words() const {
    _rebuild_union_descriptor_if_dirty();
    godot::PackedInt64Array arr;
    arr.resize(3);
    for (int i = 0; i < 3; ++i) {
        arr.set(i, static_cast<int64_t>(cached_union_descriptor_.peripheral_seme_words[static_cast<size_t>(i)]));
    }
    return arr;
}

void SemanticTagSetResource::set_raw_bitmask_words(
    const godot::PackedInt64Array &p_nuclear_words,
    const godot::PackedInt64Array &p_peripheral_words
) {
    _rebuild_union_descriptor_if_dirty();
    for (int i = 0; i < 3 && i < p_nuclear_words.size(); ++i) {
        cached_union_descriptor_.nuclear_seme_words[static_cast<size_t>(i)] =
            static_cast<uint64_t>(p_nuclear_words[i]);
    }
    for (int i = 0; i < 3 && i < p_peripheral_words.size(); ++i) {
        cached_union_descriptor_.peripheral_seme_words[static_cast<size_t>(i)] =
            static_cast<uint64_t>(p_peripheral_words[i]);
    }
    union_dirty_ = false;
    emit_changed();
}

int SemanticTagSetResource::get_nuclear_popcount() const {
    _rebuild_union_descriptor_if_dirty();
    return static_cast<int>(cached_union_descriptor_.nuclear_popcount());
}

int SemanticTagSetResource::get_peripheral_popcount() const {
    _rebuild_union_descriptor_if_dirty();
    return static_cast<int>(cached_union_descriptor_.peripheral_popcount());
}

int64_t SemanticTagSetResource::get_coherence_trait_mask32() const {
    _rebuild_union_descriptor_if_dirty();
    return static_cast<int64_t>(cached_union_descriptor_.compute_coherence_trait_mask32());
}

core::LinguisticProfilePOD SemanticTagSetResource::to_linguistic_profile_pod(
    uint16_t p_source_channel_mask
) const {
    _rebuild_union_descriptor_if_dirty();
    core::LinguisticProfilePOD pod{};
    pod.nuclear_seme_words    = cached_union_descriptor_.nuclear_seme_words;
    pod.peripheral_seme_words = cached_union_descriptor_.peripheral_seme_words;
    pod.primary_sememe_id     = cached_union_descriptor_.sememe_id;
    pod.object_name_hash      = cached_union_descriptor_.canonical_name_hash;
    pod.emotive_valence       = cached_union_descriptor_.emotive_valence;
    pod.source_channel_mask   = p_source_channel_mask;
    pod.dominant_category     = cached_union_descriptor_.taxeme_category;
    pod.sememic_form          = cached_union_descriptor_.sememic_form;
    return pod;
}

godot::PackedByteArray SemanticTagSetResource::to_profile_pod_bytes(int p_source_channel_mask) const {
    const core::LinguisticProfilePOD pod = to_linguistic_profile_pod(
        static_cast<uint16_t>(p_source_channel_mask & 0xFFFF)
    );
    godot::PackedByteArray bytes;
    bytes.resize(sizeof(core::LinguisticProfilePOD));
    std::memcpy(bytes.ptrw(), &pod, sizeof(core::LinguisticProfilePOD));
    return bytes;
}

void SemanticTagSetResource::load_from_profile_pod_bytes(const godot::PackedByteArray &p_bytes) {
    if (p_bytes.size() < static_cast<int64_t>(sizeof(core::LinguisticProfilePOD))) {
        return;
    }
    core::LinguisticProfilePOD pod{};
    std::memcpy(&pod, p_bytes.ptr(), sizeof(core::LinguisticProfilePOD));
    cached_union_descriptor_ = pod.to_sememe_descriptor();
    aggregate_valence_       = pod.emotive_valence;
    union_dirty_             = false;
    emit_changed();
}

float SemanticTagSetResource::compute_affinity_with(
    const godot::Ref<SemanticTagSetResource> &p_other,
    float p_nuclear_weight,
    float p_peripheral_weight
) const {
    if (p_other.is_null()) {
        return 0.0f;
    }
    return core::SemanticLexiconCore::compute_descriptor_affinity(
        get_compiled_descriptor(),
        p_other->get_compiled_descriptor(),
        p_nuclear_weight,
        p_peripheral_weight
    );
}

float SemanticTagSetResource::compute_tversky_affinity_with(
    const godot::Ref<SemanticTagSetResource> &p_candidate,
    float p_alpha,
    float p_beta
) const {
    if (p_candidate.is_null()) {
        return 0.0f;
    }
    return core::SemanticLexiconCore::compute_descriptor_tversky_affinity(
        get_compiled_descriptor(),
        p_candidate->get_compiled_descriptor(),
        p_alpha,
        p_beta
    );
}

bool SemanticTagSetResource::matches_required_nuclear_tags(
    const godot::Ref<SemanticTagSetResource> &p_required
) const {
    if (p_required.is_null()) {
        return true;
    }
    return get_compiled_descriptor().contains_all_nuclear(p_required->get_compiled_descriptor());
}

godot::Dictionary SemanticTagSetResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["sememes"]                  = sememes_.duplicate();
    d["explicit_nuclear_tags"]    = explicit_nuclear_tags_.duplicate();
    d["explicit_peripheral_tags"] = explicit_peripheral_tags_.duplicate();
    d["excluded_seme_tags"]       = excluded_seme_tags_.duplicate();
    d["mrl_float_embedding"]      = mrl_float_embedding_.duplicate();
    d["source_prompt"]            = source_prompt_;
    d["aggregate_valence"]        = aggregate_valence_;
    return d;
}

void SemanticTagSetResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("sememes"))                  sememes_                  = p_state["sememes"];
    if (p_state.has("explicit_nuclear_tags"))    explicit_nuclear_tags_    = p_state["explicit_nuclear_tags"];
    if (p_state.has("explicit_peripheral_tags")) explicit_peripheral_tags_ = p_state["explicit_peripheral_tags"];
    if (p_state.has("excluded_seme_tags"))       excluded_seme_tags_       = p_state["excluded_seme_tags"];
    if (p_state.has("mrl_float_embedding"))      mrl_float_embedding_      = p_state["mrl_float_embedding"];
    if (p_state.has("source_prompt"))            source_prompt_            = p_state["source_prompt"];
    if (p_state.has("aggregate_valence"))        aggregate_valence_        = static_cast<float>(p_state["aggregate_valence"]);
    union_dirty_ = true;
    emit_changed();
}

void SemanticTagSetResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) {
        return;
    }
    const godot::Dictionary after_state = get_snapshot_state();
    undo_redo_->create_action(p_action_name);
    undo_redo_->add_do_method(godot::Callable(this, "apply_snapshot_state").bind(after_state));
    undo_redo_->add_undo_method(godot::Callable(this, "apply_snapshot_state").bind(p_before_state));
    undo_redo_->commit_action(false);
}

// ============================================================================
// SemanticFrameResource Implementation
// ============================================================================

void SemanticFrameResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &SemanticFrameResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &SemanticFrameResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_frame_name", "frame_name"), &SemanticFrameResource::set_frame_name);
    ClassDB::bind_method(D_METHOD("get_frame_name"), &SemanticFrameResource::get_frame_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "frame_name"), "set_frame_name", "get_frame_name");

    ClassDB::bind_method(D_METHOD("set_parent_frame_name", "parent_frame_name"), &SemanticFrameResource::set_parent_frame_name);
    ClassDB::bind_method(D_METHOD("get_parent_frame_name"), &SemanticFrameResource::get_parent_frame_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "parent_frame_name"), "set_parent_frame_name", "get_parent_frame_name");

    ClassDB::bind_method(D_METHOD("set_evoking_lexical_units", "units"), &SemanticFrameResource::set_evoking_lexical_units);
    ClassDB::bind_method(D_METHOD("get_evoking_lexical_units"), &SemanticFrameResource::get_evoking_lexical_units);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "evoking_lexical_units"), "set_evoking_lexical_units", "get_evoking_lexical_units");

    ClassDB::bind_method(D_METHOD("set_required_agent_seme_tags", "tags"), &SemanticFrameResource::set_required_agent_seme_tags);
    ClassDB::bind_method(D_METHOD("get_required_agent_seme_tags"), &SemanticFrameResource::get_required_agent_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "required_agent_seme_tags"), "set_required_agent_seme_tags", "get_required_agent_seme_tags");

    ClassDB::bind_method(D_METHOD("set_required_patient_seme_tags", "tags"), &SemanticFrameResource::set_required_patient_seme_tags);
    ClassDB::bind_method(D_METHOD("get_required_patient_seme_tags"), &SemanticFrameResource::get_required_patient_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "required_patient_seme_tags"), "set_required_patient_seme_tags", "get_required_patient_seme_tags");

    ClassDB::bind_method(D_METHOD("set_required_instrument_seme_tags", "tags"), &SemanticFrameResource::set_required_instrument_seme_tags);
    ClassDB::bind_method(D_METHOD("get_required_instrument_seme_tags"), &SemanticFrameResource::get_required_instrument_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "required_instrument_seme_tags"), "set_required_instrument_seme_tags", "get_required_instrument_seme_tags");

    ClassDB::bind_method(D_METHOD("set_consequent_state_seme_tags", "tags"), &SemanticFrameResource::set_consequent_state_seme_tags);
    ClassDB::bind_method(D_METHOD("get_consequent_state_seme_tags"), &SemanticFrameResource::get_consequent_state_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "consequent_state_seme_tags"), "set_consequent_state_seme_tags", "get_consequent_state_seme_tags");

    ClassDB::bind_method(D_METHOD("set_valence_polarity", "valence"), &SemanticFrameResource::set_valence_polarity);
    ClassDB::bind_method(D_METHOD("get_valence_polarity"), &SemanticFrameResource::get_valence_polarity);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "valence_polarity", PROPERTY_HINT_RANGE, "-1.0,1.0,0.01"),
        "set_valence_polarity",
        "get_valence_polarity"
    );

    ClassDB::bind_method(
        D_METHOD("evaluate_role_fit", "agent_tags", "patient_tags", "instrument_tags"),
        &SemanticFrameResource::evaluate_role_fit,
        DEFVAL(Ref<SemanticTagSetResource>())
    );
}

core::SemanticFramePOD SemanticFrameResource::get_compiled_frame_pod() const {
    core::SemanticFramePOD pod{};
    pod.frame_name_hash = core::hash_linguistic_token(godot::String(frame_name_).utf8().get_data());
    auto build_mask32 = [](const godot::PackedStringArray &p_tags) -> uint32_t {
        uint32_t m = 0u;
        for (int i = 0; i < p_tags.size(); ++i) {
            const uint32_t bit = core::SemanticLexiconCore::resolve_canonical_seme_bit(p_tags[i].utf8().get_data());
            m |= (1u << (bit & 31u));
        }
        return m;
    };
    pod.required_agent_seme_mask32   = build_mask32(required_agent_seme_tags_);
    pod.required_patient_seme_mask32 = build_mask32(required_patient_seme_tags_);
    pod.required_instrument_mask32   = build_mask32(required_instrument_seme_tags_);
    pod.consequent_state_seme_mask32 = build_mask32(consequent_state_seme_tags_);
    pod.valence_polarity             = static_cast<uint8_t>(std::clamp((valence_polarity_ + 1.0f) * 127.5f, 0.0f, 255.0f));
    return pod;
}

float SemanticFrameResource::evaluate_role_fit(
    const godot::Ref<SemanticTagSetResource> &p_agent_tags,
    const godot::Ref<SemanticTagSetResource> &p_patient_tags,
    const godot::Ref<SemanticTagSetResource> &p_instrument_tags
) const {
    core::SemanticLexiconCore temp_core;
    const uint32_t fid = temp_core.register_semantic_frame(
        godot::String(frame_name_).utf8().get_data(),
        get_compiled_frame_pod()
    );
    const core::SememeDescriptorPOD ag_desc = p_agent_tags.is_valid() ? p_agent_tags->get_compiled_descriptor() : core::SememeDescriptorPOD{};
    const core::SememeDescriptorPOD pt_desc = p_patient_tags.is_valid() ? p_patient_tags->get_compiled_descriptor() : core::SememeDescriptorPOD{};
    const core::SememeDescriptorPOD in_desc = p_instrument_tags.is_valid() ? p_instrument_tags->get_compiled_descriptor() : core::SememeDescriptorPOD{};
    return temp_core.evaluate_semantic_frame_role_fit(fid, ag_desc, pt_desc, in_desc);
}

} // namespace ideam::godot_ext

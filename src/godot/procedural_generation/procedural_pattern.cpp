#include "procedural_pattern.h"

namespace ideam::godot_ext {

void ProceduralPattern::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_generation_rules", "rules"), &ProceduralPattern::set_generation_rules);
    ClassDB::bind_method(D_METHOD("get_generation_rules"), &ProceduralPattern::get_generation_rules);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "generation_rules", PROPERTY_HINT_ARRAY_TYPE, "GenerationRule"),
        "set_generation_rules",
        "get_generation_rules"
    );

    ClassDB::bind_method(D_METHOD("set_discovered_tokens", "tokens"), &ProceduralPattern::set_discovered_tokens);
    ClassDB::bind_method(D_METHOD("get_discovered_tokens"), &ProceduralPattern::get_discovered_tokens);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "discovered_tokens"), "set_discovered_tokens", "get_discovered_tokens");

    ClassDB::bind_method(D_METHOD("add_generation_rule", "rule"), &ProceduralPattern::add_generation_rule);
    ClassDB::bind_method(D_METHOD("clear_generation_rules"), &ProceduralPattern::clear_generation_rules);
    ClassDB::bind_method(D_METHOD("get_generation_rule_count"), &ProceduralPattern::get_generation_rule_count);
    ClassDB::bind_method(D_METHOD("add_discovered_token", "token"), &ProceduralPattern::add_discovered_token);
    ClassDB::bind_method(D_METHOD("find_token_index", "token"), &ProceduralPattern::find_token_index);

    ClassDB::bind_method(
        D_METHOD("commit_add_generation_rule", "rule", "action_name"),
        &ProceduralPattern::commit_add_generation_rule,
        DEFVAL("Add Generation Rule to ProceduralPattern")
    );
    ClassDB::bind_method(
        D_METHOD("commit_clear_generation_rules", "action_name"),
        &ProceduralPattern::commit_clear_generation_rules,
        DEFVAL("Clear ProceduralPattern Generation Rules")
    );
}

void ProceduralPattern::set_generation_rules(const godot::TypedArray<GenerationRule> &p_rules) {
    generation_rules_ = p_rules;
    emit_changed();
}

void ProceduralPattern::add_generation_rule(const godot::Ref<GenerationRule> &p_rule) {
    if (p_rule.is_valid()) {
        generation_rules_.push_back(p_rule);
        emit_changed();
    }
}

void ProceduralPattern::clear_generation_rules() {
    generation_rules_.clear();
    emit_changed();
}

void ProceduralPattern::add_discovered_token(const godot::String &p_token) {
    if (p_token.is_empty()) return;
    if (find_token_index(p_token) < 0) {
        discovered_tokens_.push_back(p_token);
        emit_changed();
    }
}

int ProceduralPattern::find_token_index(const godot::String &p_token) const {
    for (int i = 0; i < discovered_tokens_.size(); ++i) {
        if (discovered_tokens_[i] == p_token) return i;
    }
    return -1;
}

void ProceduralPattern::compile_to_environment(core::ProceduralEnvironment &r_env) const {
    // 1. Register discovered tokens
    uint32_t pat_seme_mask = 0u;
    if (get_semantic_tags().is_valid()) {
        pat_seme_mask = static_cast<uint32_t>(
            get_semantic_tags()->get_core_descriptor().nuclear_seme_words[0] & 0xFFFFFFFFULL
        );
    }

    for (int i = 0; i < discovered_tokens_.size(); ++i) {
        const std::string utf8 = discovered_tokens_[i].utf8().get_data();
        core::TokenDescriptorPOD desc{};
        desc.signature.object_name_hash = core::hash_reflection_symbol(utf8);
        desc.nuclear_seme_mask_lo32     = pat_seme_mask;
        desc.coherence_trait_mask       = pat_seme_mask | (1u << (static_cast<uint32_t>(i) & 31u));
        r_env.register_token(utf8, desc);
    }

    // 2. Flatten and register generation rules + allowed_relatives bitmasks
    auto register_rule_recursive = [&](auto &self, const godot::Ref<GenerationRule> &p_rule, uint32_t p_parent_id) -> void {
        if (p_rule.is_null()) return;
        const godot::String subj(p_rule->get_subject_token_name());
        if (!subj.is_empty()) {
            const std::string subj_utf8 = subj.utf8().get_data();
            if (r_env.find_token_by_name(subj_utf8) < 0) {
                core::TokenDescriptorPOD desc{};
                desc.signature.object_name_hash = core::hash_reflection_symbol(subj_utf8);
                r_env.register_token(subj_utf8, desc);
            }
        }

        core::GenerationRulePOD pod = p_rule->to_pod(static_cast<uint32_t>(r_env.active_rules().size()), p_parent_id);

        const godot::String req_sib(p_rule->get_required_sibling());
        if (!req_sib.is_empty()) {
            const int32_t sib_id = r_env.find_token_by_name(req_sib.utf8().get_data());
            if (sib_id >= 0) pod.required_sibling_token = static_cast<uint32_t>(sib_id);
        }
        const godot::String forb_sib(p_rule->get_forbidden_sibling());
        if (!forb_sib.is_empty()) {
            const int32_t forb_id = r_env.find_token_by_name(forb_sib.utf8().get_data());
            if (forb_id >= 0) pod.forbidden_sibling_token = static_cast<uint32_t>(forb_id);
        }

        core::BitmaskDomain256POD rel_mask{};
        const godot::PackedStringArray rels = p_rule->get_allowed_relatives();
        for (int k = 0; k < rels.size(); ++k) {
            const std::string rel_utf8 = rels[k].utf8().get_data();
            int32_t tok_id = r_env.find_token_by_name(rel_utf8);
            if (tok_id < 0) {
                core::TokenDescriptorPOD desc{};
                desc.signature.object_name_hash = core::hash_reflection_symbol(rel_utf8);
                tok_id = static_cast<int32_t>(r_env.register_token(rel_utf8, desc));
            }
            rel_mask.set_bit(static_cast<uint32_t>(tok_id));
        }

        const uint32_t added_id = r_env.add_generation_rule(pod, rel_mask);
        const godot::TypedArray<GenerationRule> children = p_rule->get_child_rules();
        for (int c = 0; c < children.size(); ++c) {
            const godot::Ref<GenerationRule> child = children[c];
            self(self, child, added_id);
        }
    };

    for (int i = 0; i < generation_rules_.size(); ++i) {
        const godot::Ref<GenerationRule> rule = generation_rules_[i];
        register_rule_recursive(register_rule_recursive, rule, 0xFFFFFFFFu);
    }

    r_env.compile_cascade_and_initialize_domains();
}

void ProceduralPattern::import_from_environment(const core::ProceduralEnvironment &p_env) {
    discovered_tokens_.clear();
    generation_rules_.clear();

    for (const std::string &name : p_env.token_names()) {
        discovered_tokens_.push_back(godot::String::utf8(name.c_str()));
    }

    for (const core::GenerationRulePOD &pod : p_env.active_rules()) {
        godot::Ref<GenerationRule> rule;
        rule.instantiate();
        rule->set_operation(static_cast<int>(pod.operation));
        rule->set_scope(static_cast<int>(pod.scope));
        rule->set_cascade_layer(static_cast<int>(pod.layer));
        rule->set_priority(pod.priority);
        rule->set_is_important(pod.is_important != 0u);
        rule->set_min_occurrences(pod.min_occurrences);
        rule->set_max_occurrences(pod.max_occurrences);

        // Resolve subject token name by matching object_name_hash
        for (size_t t = 0; t < p_env.tokens().size(); ++t) {
            if (p_env.tokens()[t].signature.object_name_hash == pod.selector.object_name_hash &&
                pod.selector.object_name_hash != 0u) {
                rule->set_subject_token_name(godot::StringName(p_env.get_token_name(static_cast<uint32_t>(t)).data()));
                break;
            }
        }

        if (pod.required_sibling_token < p_env.tokens().size()) {
            rule->set_required_sibling(godot::StringName(p_env.get_token_name(pod.required_sibling_token).data()));
        }
        if (pod.forbidden_sibling_token < p_env.tokens().size()) {
            rule->set_forbidden_sibling(godot::StringName(p_env.get_token_name(pod.forbidden_sibling_token).data()));
        }

        if (pod.target_token_mask_index < p_env.rule_token_masks().size()) {
            const core::BitmaskDomain256POD &mask = p_env.rule_token_masks()[pod.target_token_mask_index];
            godot::PackedStringArray rels;
            for (uint32_t t = 0; t < p_env.tokens().size(); ++t) {
                if (mask.test_bit(t)) {
                    rels.push_back(godot::String::utf8(p_env.get_token_name(t).data()));
                }
            }
            rule->set_allowed_relatives(rels);
        }

        generation_rules_.push_back(rule);
    }
    emit_changed();
}

godot::Dictionary ProceduralPattern::get_snapshot_state() const {
    godot::Dictionary d = ProceduralGeneratorResource::get_snapshot_state();
    d["discovered_tokens"] = discovered_tokens_;
    d["generation_rules"]  = generation_rules_.duplicate(false);
    return d;
}

void ProceduralPattern::apply_snapshot_state(const godot::Dictionary &p_state) {
    ProceduralGeneratorResource::apply_snapshot_state(p_state);
    if (p_state.has("discovered_tokens")) discovered_tokens_ = p_state["discovered_tokens"];
    if (p_state.has("generation_rules"))  generation_rules_  = p_state["generation_rules"];
    emit_changed();
}

void ProceduralPattern::commit_add_generation_rule(
    const godot::Ref<GenerationRule> &p_rule,
    const godot::String &p_action_name
) {
    if (p_rule.is_null()) return;
    const godot::Dictionary before = get_snapshot_state();
    generation_rules_.push_back(p_rule);
    emit_changed();
    commit_snapshot_mutation(p_action_name, before);
}

void ProceduralPattern::commit_clear_generation_rules(const godot::String &p_action_name) {
    const godot::Dictionary before = get_snapshot_state();
    generation_rules_.clear();
    emit_changed();
    commit_snapshot_mutation(p_action_name, before);
}

} // namespace ideam::godot_ext

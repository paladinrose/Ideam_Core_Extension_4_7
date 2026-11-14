#include "procedural_environment.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace ideam::core {

void ProceduralEnvironment::clear() noexcept {
    tokens_.clear();
    token_names_.clear();
    slots_.clear();
    child_slot_csr_.clear();
    spatial_neighbor_csr_.clear();
    active_rules_.clear();
    rule_token_masks_.clear();
}

void ProceduralEnvironment::configure_slots(uint32_t p_slot_count, uint32_t p_token_count) {
    const uint32_t capped_tokens = std::min<uint32_t>(p_token_count, 256u);
    slots_.resize(p_slot_count);
    for (uint32_t i = 0; i < p_slot_count; ++i) {
        slots_[i]         = EnvironmentSlotPOD{};
        slots_[i].slot_id = i;
        slots_[i].domain.fill_first_n(capped_tokens);
        const uint32_t pop = slots_[i].domain.popcount();
        slots_[i].collapsed_token_id = (pop == 1u) ? slots_[i].domain.first_set_state() : -1;
        slots_[i].entropy_bucket     = static_cast<uint8_t>(std::min<uint32_t>(15u, pop));
    }
}

uint32_t ProceduralEnvironment::register_token(
    std::string_view p_token_name,
    const TokenDescriptorPOD &p_descriptor
) {
    const int32_t existing = find_token_by_name(p_token_name);
    if (existing >= 0) {
        tokens_[static_cast<size_t>(existing)] = p_descriptor;
        tokens_[static_cast<size_t>(existing)].token_id = static_cast<uint32_t>(existing);
        return static_cast<uint32_t>(existing);
    }
    if (tokens_.size() >= 256u) {
        return 255u;
    }
    const uint32_t id = static_cast<uint32_t>(tokens_.size());
    TokenDescriptorPOD copy = p_descriptor;
    copy.token_id = id;
    if (copy.signature.object_name_hash == 0u) {
        copy.signature.object_name_hash = hash_reflection_symbol(p_token_name);
    }
    tokens_.push_back(copy);
    token_names_.emplace_back(p_token_name);
    return id;
}

int32_t ProceduralEnvironment::find_token_by_name(std::string_view p_token_name) const noexcept {
    for (size_t i = 0; i < token_names_.size(); ++i) {
        if (token_names_[i] == p_token_name) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

std::string_view ProceduralEnvironment::get_token_name(uint32_t p_token_id) const noexcept {
    if (p_token_id < token_names_.size()) {
        return token_names_[p_token_id];
    }
    return {};
}

void ProceduralEnvironment::set_slot_children(
    uint32_t p_parent_slot_id,
    std::span<const uint32_t> p_child_slot_ids
) {
    if (p_parent_slot_id >= slots_.size()) return;
    EnvironmentSlotPOD &parent = slots_[p_parent_slot_id];
    parent.children_csr_offset = static_cast<uint32_t>(child_slot_csr_.size());
    parent.children_count      = static_cast<uint32_t>(p_child_slot_ids.size());
    for (uint32_t c_id : p_child_slot_ids) {
        child_slot_csr_.push_back(c_id);
        if (c_id < slots_.size()) {
            slots_[c_id].parent_slot_id = p_parent_slot_id;
        }
    }
}

void ProceduralEnvironment::set_slot_spatial_neighbors(
    uint32_t p_slot_id,
    std::span<const SpatialNeighborEdgePOD> p_neighbors
) {
    if (p_slot_id >= slots_.size()) return;
    EnvironmentSlotPOD &slot = slots_[p_slot_id];
    slot.spatial_csr_offset     = static_cast<uint16_t>(spatial_neighbor_csr_.size());
    slot.spatial_neighbor_count = static_cast<uint16_t>(p_neighbors.size());
    for (const SpatialNeighborEdgePOD &edge : p_neighbors) {
        spatial_neighbor_csr_.push_back(edge);
    }
}

uint32_t ProceduralEnvironment::add_generation_rule(
    const GenerationRulePOD &p_rule,
    const BitmaskDomain256POD &p_target_token_mask
) {
    const uint32_t mask_idx = static_cast<uint32_t>(rule_token_masks_.size());
    rule_token_masks_.push_back(p_target_token_mask);

    GenerationRulePOD copy       = p_rule;
    copy.rule_id                 = static_cast<uint32_t>(active_rules_.size());
    copy.target_token_mask_index = mask_idx;
    if (copy.specificity == 0u) {
        copy.specificity = CascadeRuleEngine::compute_generation_specificity(copy);
    }
    active_rules_.push_back(copy);
    return copy.rule_id;
}

void ProceduralEnvironment::synthesize_pattern_from_harvested_nodes(
    std::span<const HarvestedSceneNodeDescriptor> p_nodes,
    std::span<const AnalysisRulePOD> p_analysis_rules,
    std::string_view p_required_class_filter,
    std::string_view p_required_suffix_filter
) {
    if (p_nodes.empty()) return;

    // 1. Determine which harvested nodes pass the cascading AnalysisRule filter
    std::vector<int32_t> node_to_token_id(p_nodes.size(), -1);
    const uint32_t req_class_hash  = hash_reflection_symbol(p_required_class_filter);
    const uint32_t req_suffix_hash = hash_reflection_symbol(p_required_suffix_filter);

    for (size_t i = 0; i < p_nodes.size(); ++i) {
        const HarvestedSceneNodeDescriptor &nd = p_nodes[i];
        if (!p_required_class_filter.empty() && nd.class_name != p_required_class_filter) {
            continue;
        }
        if (!p_required_suffix_filter.empty() && !nd.node_name.ends_with(p_required_suffix_filter)) {
            continue;
        }

        const ReflectionSignaturePOD cand_sig = CascadeRuleEngine::build_candidate_signature(
            nd.class_name,
            nd.node_name,
            {},
            p_required_suffix_filter,
            nd.group_or_tag_mask,
            nd.property_trait_mask,
            nd.method_trait_mask,
            nd.signal_trait_mask
        );

        bool admitted = true;
        for (const AnalysisRulePOD &ar : p_analysis_rules) {
            const bool matches = ar.selector.matches_candidate(cand_sig);
            if (!matches && ar.ignore_unmatched != 0u) {
                admitted = false;
                break;
            }
        }
        if (!admitted) continue;

        const std::string_view canonical_name =
            CascadeRuleEngine::strip_suffix_if_present(nd.node_name, p_required_suffix_filter);

        TokenDescriptorPOD tok_desc{};
        tok_desc.signature               = cand_sig;
        tok_desc.signature.object_name_hash = hash_reflection_symbol(canonical_name);
        tok_desc.nuclear_seme_mask_lo32    = nd.nuclear_seme_mask;
        tok_desc.peripheral_seme_mask_lo32 = nd.peripheral_seme_mask;
        tok_desc.coherence_trait_mask      = nd.group_or_tag_mask | nd.nuclear_seme_mask | (1u << (static_cast<uint32_t>(tokens_.size()) & 31u));

        const uint32_t tok_id = register_token(canonical_name, tok_desc);
        node_to_token_id[i]   = static_cast<int32_t>(tok_id);
    }

    if (tokens_.empty()) return;

    // 2. Emit Root ADMIT_ARCHETYPE GenerationRule
    {
        GenerationRulePOD root_rule{};
        root_rule.selector.class_name_hash  = req_class_hash;
        root_rule.selector.name_suffix_hash = req_suffix_hash;
        root_rule.layer                     = PatternCascadeLayer::LAYER_DOMAIN;
        root_rule.scope                     = PatternRuleScope::ALL;
        root_rule.operation                 = GenerationRuleOp::ADMIT_ARCHETYPE;

        BitmaskDomain256POD all_admitted{};
        all_admitted.fill_first_n(static_cast<uint32_t>(tokens_.size()));
        add_generation_rule(root_rule, all_admitted);
    }

    // 3. Collect Parent Acceptance & Child Cardinality / Sibling Co-Occurrence Statistics
    std::map<uint32_t, BitmaskDomain256POD> observed_parents_for_child;
    // parent_token_id -> list of child-token-count maps per observed instance of that parent
    std::map<uint32_t, std::vector<std::map<uint32_t, uint16_t>>> parent_instances_children;

    for (size_t i = 0; i < p_nodes.size(); ++i) {
        const int32_t tok_i = node_to_token_id[i];
        if (tok_i < 0) continue;

        // Walk up to nearest admitted ancestor
        int32_t p_idx = p_nodes[i].parent_index;
        while (p_idx >= 0 && p_idx < static_cast<int32_t>(p_nodes.size()) && node_to_token_id[static_cast<size_t>(p_idx)] < 0) {
            p_idx = p_nodes[static_cast<size_t>(p_idx)].parent_index;
        }
        if (p_idx >= 0 && p_idx < static_cast<int32_t>(p_nodes.size())) {
            const uint32_t parent_tok = static_cast<uint32_t>(node_to_token_id[static_cast<size_t>(p_idx)]);
            observed_parents_for_child[static_cast<uint32_t>(tok_i)].set_bit(parent_tok);
        }
    }

    // Group direct admitted children per parent node instance
    for (size_t p = 0; p < p_nodes.size(); ++p) {
        const int32_t parent_tok = node_to_token_id[p];
        if (parent_tok < 0) continue;

        std::map<uint32_t, uint16_t> child_counts;
        for (size_t c = 0; c < p_nodes.size(); ++c) {
            if (c == p || node_to_token_id[c] < 0) continue;
            int32_t anc = p_nodes[c].parent_index;
            while (anc >= 0 && anc < static_cast<int32_t>(p_nodes.size()) && node_to_token_id[static_cast<size_t>(anc)] < 0) {
                anc = p_nodes[static_cast<size_t>(anc)].parent_index;
            }
            if (anc == static_cast<int32_t>(p)) {
                ++child_counts[static_cast<uint32_t>(node_to_token_id[c])];
            }
        }
        if (!child_counts.empty()) {
            parent_instances_children[static_cast<uint32_t>(parent_tok)].push_back(child_counts);
        }
    }

    // 4. Emit ACCEPT_PARENTS rules (e.g., CastleTower accepts parents CastleGrounds | CastleRuins)
    for (const auto &[child_tok, parent_mask] : observed_parents_for_child) {
        if (parent_mask.empty()) continue;
        GenerationRulePOD parent_rule{};
        parent_rule.selector  = tokens_[child_tok].signature;
        parent_rule.layer     = PatternCascadeLayer::LAYER_HIERARCHY_PARENT;
        parent_rule.scope     = PatternRuleScope::SELF;
        parent_rule.operation = GenerationRuleOp::ACCEPT_PARENTS;
        add_generation_rule(parent_rule, parent_mask);
    }

    // 5. Emit ACCEPT_CHILDREN_CARDINALITY & REQUIRE_SIBLING rules per parent archetype
    for (const auto &[parent_tok, instances] : parent_instances_children) {
        std::set<uint32_t> all_child_types;
        for (const auto &inst_map : instances) {
            for (const auto &[c_tok, cnt] : inst_map) {
                if (cnt > 0u) all_child_types.insert(c_tok);
            }
        }

        for (uint32_t c_tok : all_child_types) {
            uint16_t min_c = 65535u;
            uint16_t max_c = 0u;
            for (const auto &inst_map : instances) {
                auto it = inst_map.find(c_tok);
                const uint16_t cnt = (it != inst_map.end()) ? it->second : 0u;
                min_c = std::min(min_c, cnt);
                max_c = std::max(max_c, cnt);
            }

            // Detect required sibling: if whenever c_tok > 0 (and min_c == 0 or c_tok is conditional),
            // another sibling s_tok is always present across all instances where c_tok appears
            uint32_t req_sibling = 0xFFFFFFFFu;
            for (uint32_t cand_sib : all_child_types) {
                if (cand_sib == c_tok) continue;
                bool always_present_with_c = true;
                bool cand_is_optional_overall = false;
                for (const auto &inst_map : instances) {
                    const uint16_t c_cnt   = inst_map.count(c_tok) ? inst_map.at(c_tok) : 0u;
                    const uint16_t sib_cnt = inst_map.count(cand_sib) ? inst_map.at(cand_sib) : 0u;
                    if (sib_cnt == 0u) cand_is_optional_overall = true;
                    if (c_cnt > 0u && sib_cnt == 0u) {
                        always_present_with_c = false;
                        break;
                    }
                }
                if (always_present_with_c && cand_is_optional_overall) {
                    req_sibling = cand_sib;
                    break;
                }
            }

            GenerationRulePOD card_rule{};
            card_rule.selector               = tokens_[parent_tok].signature;
            card_rule.layer                  = PatternCascadeLayer::LAYER_CHILD_CARDINALITY;
            card_rule.scope                  = PatternRuleScope::CHILDREN;
            card_rule.operation              = GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY;
            card_rule.min_occurrences        = (min_c == 65535u) ? 0u : min_c;
            card_rule.max_occurrences        = std::max<uint16_t>(1u, max_c);
            card_rule.required_sibling_token = req_sibling;

            BitmaskDomain256POD child_mask{};
            child_mask.set_bit(c_tok);
            add_generation_rule(card_rule, child_mask);

            if (req_sibling != 0xFFFFFFFFu) {
                GenerationRulePOD sib_rule{};
                sib_rule.selector               = tokens_[c_tok].signature;
                sib_rule.layer                  = PatternCascadeLayer::LAYER_SIBLING_DEPENDENCY;
                sib_rule.scope                  = PatternRuleScope::SIBLINGS;
                sib_rule.operation              = GenerationRuleOp::REQUIRE_SIBLING;
                sib_rule.required_sibling_token = req_sibling;
                add_generation_rule(sib_rule, child_mask);
            }
        }
    }

    CascadeRuleEngine::sort_generation_rules_by_cascade(active_rules_);
}

void ProceduralEnvironment::compile_cascade_and_initialize_domains() {
    CascadeRuleEngine::sort_generation_rules_by_cascade(active_rules_);

    // 1. Update TokenDescriptorPOD coherence_trait_mask from matching rules
    for (TokenDescriptorPOD &tok : tokens_) {
        tok.coherence_trait_mask |= tok.nuclear_seme_mask_lo32 | tok.signature.group_or_tag_mask;
        for (const GenerationRulePOD &rule : active_rules_) {
            if (rule.selector.matches_candidate(tok.signature)) {
                tok.coherence_trait_mask |= (1u << static_cast<uint32_t>(rule.layer));
            }
        }
    }

    // 2. Build union of ADMIT_ARCHETYPE masks (or all tokens if no ADMIT_ARCHETYPE rule exists)
    BitmaskDomain256POD admitted_mask{};
    bool has_admit_rule = false;
    for (const GenerationRulePOD &rule : active_rules_) {
        if (rule.operation == GenerationRuleOp::ADMIT_ARCHETYPE) {
            has_admit_rule = true;
            if (rule.target_token_mask_index < rule_token_masks_.size() &&
                !rule_token_masks_[rule.target_token_mask_index].empty()) {
                admitted_mask.union_with(rule_token_masks_[rule.target_token_mask_index]);
            } else {
                for (const TokenDescriptorPOD &tok : tokens_) {
                    if (rule.selector.matches_candidate(tok.signature)) {
                        admitted_mask.set_bit(tok.token_id);
                    }
                }
            }
        }
    }
    if (!has_admit_rule) {
        admitted_mask.fill_first_n(static_cast<uint32_t>(tokens_.size()));
    }

    for (EnvironmentSlotPOD &slot : slots_) {
        if (slot.collapsed_token_id >= 0 && slot.collapsed_token_id < static_cast<int32_t>(tokens_.size())) {
            slot.domain.clear();
            slot.domain.set_bit(static_cast<uint32_t>(slot.collapsed_token_id));
        } else {
            slot.domain = admitted_mask;
        }
        const uint32_t pop = slot.domain.popcount();
        slot.collapsed_token_id = (pop == 1u) ? slot.domain.first_set_state() : -1;
        slot.has_conflict       = (pop == 0u) ? 1u : 0u;
        slot.entropy_bucket     = static_cast<uint8_t>(std::min<uint32_t>(15u, pop));
    }
}

bool ProceduralEnvironment::propagate_hierarchy_and_cardinality_constraints() {
    if (slots_.empty() || tokens_.empty()) return true;

    bool any_conflict = false;
    bool changed      = true;
    uint32_t guard    = 0u;

    while (changed && guard++ < 32u) {
        changed = false;

        for (size_t s = 0; s < slots_.size(); ++s) {
            EnvironmentSlotPOD &slot = slots_[s];
            if (slot.domain.empty()) {
                slot.has_conflict = 1u;
                any_conflict      = true;
                continue;
            }

            // A. Enforce ACCEPT_PARENTS rules between child slot `s` and `slot.parent_slot_id`
            if (slot.parent_slot_id < slots_.size()) {
                EnvironmentSlotPOD &parent_slot = slots_[slot.parent_slot_id];
                for (const GenerationRulePOD &rule : active_rules_) {
                    if (rule.operation != GenerationRuleOp::ACCEPT_PARENTS) continue;
                    if (rule.target_token_mask_index >= rule_token_masks_.size()) continue;
                    const BitmaskDomain256POD &allowed_parents = rule_token_masks_[rule.target_token_mask_index];

                    // Find which tokens in `slot.domain` match this rule's child selector
                    for (uint32_t t = 0; t < tokens_.size(); ++t) {
                        if (!slot.domain.test_bit(t)) continue;
                        if (rule.selector.matches_candidate(tokens_[t].signature)) {
                            // If parent_slot has zero overlap with allowed_parents, token `t` is invalid in `slot`
                            if (!parent_slot.domain.overlaps(allowed_parents)) {
                                slot.domain.clear_bit(t);
                                changed = true;
                            }
                            // If `slot` is already collapsed to `t`, restrict `parent_slot.domain` to `allowed_parents`
                            if (slot.domain.popcount() == 1u && slot.domain.test_bit(t)) {
                                if (parent_slot.domain.intersect_with(allowed_parents)) {
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }

            // B. Enforce ACCEPT_CHILDREN_CARDINALITY & REQUIRE_SIBLING / FORBID_SIBLING on children of `slot`
            if (slot.children_count > 0u && slot.children_csr_offset + slot.children_count <= child_slot_csr_.size()) {
                const int32_t parent_collapsed = (slot.domain.popcount() == 1u)
                    ? slot.domain.first_set_state()
                    : slot.collapsed_token_id;

                if (parent_collapsed >= 0 && static_cast<size_t>(parent_collapsed) < tokens_.size()) {
                    const TokenDescriptorPOD &parent_tok = tokens_[static_cast<size_t>(parent_collapsed)];

                    // Gather union of all allowed child tokens for this parent if any ACCEPT_CHILDREN_CARDINALITY rules apply
                    BitmaskDomain256POD union_allowed_children{};
                    bool has_child_rules_for_parent = false;

                    for (const GenerationRulePOD &rule : active_rules_) {
                        if (rule.operation != GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY) continue;
                        if (!rule.selector.matches_candidate(parent_tok.signature)) continue;
                        if (rule.target_token_mask_index >= rule_token_masks_.size()) continue;

                        const BitmaskDomain256POD &child_mask = rule_token_masks_[rule.target_token_mask_index];
                        const int32_t target_child_tok = child_mask.first_set_state();
                        if (target_child_tok < 0) continue;

                        has_child_rules_for_parent = true;
                        union_allowed_children.union_with(child_mask);

                        const uint32_t c_tok = static_cast<uint32_t>(target_child_tok);
                        uint32_t definite_count = 0u;
                        uint32_t possible_count = 0u;

                        for (uint32_t k = 0; k < slot.children_count; ++k) {
                            const uint32_t child_slot_id = child_slot_csr_[slot.children_csr_offset + k];
                            if (child_slot_id >= slots_.size()) continue;
                            const EnvironmentSlotPOD &c_slot = slots_[child_slot_id];
                            if (c_slot.domain.test_bit(c_tok)) {
                                ++possible_count;
                                if (c_slot.domain.popcount() == 1u) {
                                    ++definite_count;
                                }
                            }
                        }

                        // 1. Check cardinality contradiction
                        if (definite_count > rule.max_occurrences || possible_count < rule.min_occurrences) {
                            slot.has_conflict = 1u;
                            any_conflict      = true;
                        }

                        // 2. If `definite_count == rule.max_occurrences`, clear `c_tok` from all other uncollapsed sibling slots
                        if (definite_count == rule.max_occurrences) {
                            for (uint32_t k = 0; k < slot.children_count; ++k) {
                                const uint32_t child_slot_id = child_slot_csr_[slot.children_csr_offset + k];
                                if (child_slot_id >= slots_.size()) continue;
                                EnvironmentSlotPOD &c_slot = slots_[child_slot_id];
                                if (c_slot.domain.popcount() > 1u && c_slot.domain.test_bit(c_tok)) {
                                    c_slot.domain.clear_bit(c_tok);
                                    changed = true;
                                }
                            }
                        }

                        // 3. If `possible_count == rule.min_occurrences` and `rule.min_occurrences > 0`,
                        // force every child slot that can still hold `c_tok` to collapse to `c_tok`!
                        if (rule.min_occurrences > 0u && possible_count == rule.min_occurrences && definite_count < rule.min_occurrences) {
                            for (uint32_t k = 0; k < slot.children_count; ++k) {
                                const uint32_t child_slot_id = child_slot_csr_[slot.children_csr_offset + k];
                                if (child_slot_id >= slots_.size()) continue;
                                EnvironmentSlotPOD &c_slot = slots_[child_slot_id];
                                if (c_slot.domain.test_bit(c_tok) && c_slot.domain.popcount() > 1u) {
                                    c_slot.domain.clear();
                                    c_slot.domain.set_bit(c_tok);
                                    changed = true;
                                }
                            }
                        }

                        // 4. Enforce sibling co-occurrence (`required_sibling_token`, e.g., `TowerLanding` requires `TowerStairs`)
                        if (rule.required_sibling_token < tokens_.size()) {
                            const uint32_t req_sib = rule.required_sibling_token;
                            bool any_sib_possible = false;
                            for (uint32_t k = 0; k < slot.children_count; ++k) {
                                const uint32_t child_slot_id = child_slot_csr_[slot.children_csr_offset + k];
                                if (child_slot_id < slots_.size() && slots_[child_slot_id].domain.test_bit(req_sib)) {
                                    any_sib_possible = true;
                                    break;
                                }
                            }
                            if (!any_sib_possible) {
                                // `req_sib` cannot exist under this parent, so `c_tok` must be pruned from all children!
                                for (uint32_t k = 0; k < slot.children_count; ++k) {
                                    const uint32_t child_slot_id = child_slot_csr_[slot.children_csr_offset + k];
                                    if (child_slot_id < slots_.size() && slots_[child_slot_id].domain.test_bit(c_tok)) {
                                        slots_[child_slot_id].domain.clear_bit(c_tok);
                                        changed = true;
                                    }
                                }
                            } else if (definite_count > 0u) {
                                // `c_tok` is definitely present; if only one sibling slot can hold `req_sib`, force it!
                                uint32_t sib_possible_slots = 0u;
                                uint32_t last_sib_slot      = 0xFFFFFFFFu;
                                for (uint32_t k = 0; k < slot.children_count; ++k) {
                                    const uint32_t child_slot_id = child_slot_csr_[slot.children_csr_offset + k];
                                    if (child_slot_id < slots_.size() && slots_[child_slot_id].domain.test_bit(req_sib)) {
                                        ++sib_possible_slots;
                                        last_sib_slot = child_slot_id;
                                    }
                                }
                                if (sib_possible_slots == 1u && last_sib_slot < slots_.size() &&
                                    slots_[last_sib_slot].domain.popcount() > 1u) {
                                    slots_[last_sib_slot].domain.clear();
                                    slots_[last_sib_slot].domain.set_bit(req_sib);
                                    changed = true;
                                }
                            }
                        }
                    }

                    // Restrict children to the union of allowed child tokens for this parent
                    if (has_child_rules_for_parent && !union_allowed_children.empty()) {
                        for (uint32_t k = 0; k < slot.children_count; ++k) {
                            const uint32_t child_slot_id = child_slot_csr_[slot.children_csr_offset + k];
                            if (child_slot_id < slots_.size()) {
                                if (slots_[child_slot_id].domain.intersect_with(union_allowed_children)) {
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }

            const uint32_t pop = slot.domain.popcount();
            slot.collapsed_token_id = (pop == 1u) ? slot.domain.first_set_state() : -1;
            slot.entropy_bucket     = static_cast<uint8_t>(std::min<uint32_t>(15u, pop));
            if (pop == 0u) {
                slot.has_conflict = 1u;
                any_conflict      = true;
            }
        }
    }

    return !any_conflict;
}

void ProceduralEnvironment::compute_einselection_redundancy_scores(float p_pointer_threshold) noexcept {
    for (size_t i = 0; i < slots_.size(); ++i) {
        EnvironmentSlotPOD &slot = slots_[i];
        if (slot.has_conflict != 0u || slot.domain.empty()) {
            slot.einselection_score = 0.0f;
            slot.is_pointer_state   = 0u;
            continue;
        }

        const uint32_t pop = slot.domain.popcount();
        if (pop > 1u) {
            // Superposition state: low einselection redundancy inversely proportional to log2(pop)
            slot.einselection_score = std::clamp(0.35f / static_cast<float>(pop), 0.02f, 0.35f);
            slot.is_pointer_state   = 0u;
            continue;
        }

        const int32_t tok_id = slot.domain.first_set_state();
        slot.collapsed_token_id = tok_id;

        // 1. Evaluate highest CSS GenerationRule specificity satisfied by this collapsed token
        uint32_t max_spec = 0u;
        PatternCascadeLayer best_layer = PatternCascadeLayer::LAYER_DOMAIN;
        if (tok_id >= 0 && static_cast<size_t>(tok_id) < tokens_.size()) {
            const TokenDescriptorPOD &tok = tokens_[static_cast<size_t>(tok_id)];
            for (const GenerationRulePOD &rule : active_rules_) {
                if (rule.selector.matches_candidate(tok.signature)) {
                    max_spec = std::max<uint32_t>(max_spec, rule.specificity);
                    if (static_cast<uint8_t>(rule.layer) > static_cast<uint8_t>(best_layer)) {
                        best_layer = rule.layer;
                    }
                }
            }
        }

        // 2. Evaluate witness agreement across parent, children, and spatial neighbors
        uint32_t total_witnesses  = 0u;
        uint32_t agreed_witnesses = 0u;

        if (slot.parent_slot_id < slots_.size()) {
            ++total_witnesses;
            if (slots_[slot.parent_slot_id].has_conflict == 0u &&
                slots_[slot.parent_slot_id].domain.popcount() == 1u) {
                ++agreed_witnesses;
            }
        }
        for (uint32_t k = 0; k < slot.children_count; ++k) {
            const uint32_t c_id = child_slot_csr_[slot.children_csr_offset + k];
            if (c_id < slots_.size()) {
                ++total_witnesses;
                if (slots_[c_id].has_conflict == 0u && slots_[c_id].domain.popcount() == 1u) {
                    ++agreed_witnesses;
                }
            }
        }
        for (uint16_t k = 0; k < slot.spatial_neighbor_count; ++k) {
            const uint32_t n_id = spatial_neighbor_csr_[slot.spatial_csr_offset + k].neighbor_slot_id;
            if (n_id < slots_.size()) {
                ++total_witnesses;
                if (slots_[n_id].has_conflict == 0u && slots_[n_id].domain.popcount() == 1u) {
                    ++agreed_witnesses;
                }
            }
        }

        const float witness_ratio = (total_witnesses > 0u)
            ? static_cast<float>(agreed_witnesses) / static_cast<float>(total_witnesses)
            : 0.75f;
        const float spec_norm = std::clamp(static_cast<float>(max_spec) / 1500.0f, 0.1f, 1.0f);
        const float pinned_bonus = (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) ? 0.30f : 0.0f;

        slot.einselection_score = std::clamp(0.55f * witness_ratio + 0.45f * spec_norm + pinned_bonus, 0.0f, 1.0f);
        slot.is_pointer_state   = (slot.einselection_score >= p_pointer_threshold) ? 1u : 0u;
    }
}

} // namespace ideam::core

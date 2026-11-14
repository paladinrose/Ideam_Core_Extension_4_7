#include "wfc_solver_core.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace ideam::core {

uint32_t WFCSolverCore::compute_slot_hierarchy_depth(uint32_t p_slot_id) const noexcept {
    if (!bound_environment_ || p_slot_id >= bound_environment_->slots().size()) {
        return 0u;
    }
    const auto &slots = bound_environment_->slots();
    uint32_t depth = 0u;
    uint32_t curr  = slots[p_slot_id].parent_slot_id;
    while (curr != 0xFFFFFFFFu && curr < slots.size() && depth < 64u) {
        ++depth;
        curr = slots[curr].parent_slot_id;
    }
    return depth;
}

void WFCSolverCore::set_token_weights(std::span<const float> p_weights) {
    token_weights_.assign(p_weights.begin(), p_weights.end());
    for (float &w : token_weights_) {
        if (!(w > 0.0f) || std::isnan(w)) {
            w = 1.0f;
        }
    }
}

void WFCSolverCore::clear_rules_and_edges() noexcept {
    rule_tensor_.clear();
    adjacency_edges_.clear();
    contradiction_reports_.clear();
    collapse_history_.clear();
}

void WFCSolverCore::add_rule(
    uint32_t p_port_id,
    uint32_t p_source_state,
    const BitmaskDomain256POD &p_allowed_mask,
    WFCConstraintAxis p_axis,
    float p_weight
) {
    // Merge if an identical (port_id, source_state, axis) rule already exists
    for (auto &existing : rule_tensor_) {
        if (existing.port_id == p_port_id &&
            existing.source_state == p_source_state &&
            existing.axis == p_axis) {
            existing.allowed_mask.union_with(p_allowed_mask);
            existing.rule_weight = std::max(existing.rule_weight, p_weight);
            return;
        }
    }
    WFCRuleEntryPOD entry{};
    entry.port_id      = p_port_id;
    entry.source_state = p_source_state;
    entry.allowed_mask = p_allowed_mask;
    entry.rule_weight  = p_weight;
    entry.axis         = p_axis;
    rule_tensor_.push_back(entry);
}

void WFCSolverCore::add_directed_edge(
    uint32_t p_from_slot,
    uint32_t p_to_slot,
    uint32_t p_port_id,
    WFCConstraintAxis p_axis,
    bool p_bidirectional
) {
    WFCDirectedEdgePOD edge{};
    edge.from_slot        = p_from_slot;
    edge.to_slot          = p_to_slot;
    edge.port_id          = p_port_id;
    edge.axis             = p_axis;
    edge.is_bidirectional = p_bidirectional ? 1u : 0u;
    adjacency_edges_.push_back(edge);
}

void WFCSolverCore::compile_rules_from_observed_co_occurrences(
    std::span<const WFCObservedCoOccurrencePod> p_observed
) {
    for (const auto &obs : p_observed) {
        if (obs.parent_or_anchor_state >= 256u || obs.child_or_neighbor_state >= 256u) {
            continue;
        }
        BitmaskDomain256POD mask{};
        mask.set_bit(obs.child_or_neighbor_state);
        add_rule(obs.port_or_socket_tag, obs.parent_or_anchor_state, mask, obs.axis, obs.observed_frequency);

        // If bound to a ProceduralEnvironment and the observed rule expresses hierarchy,
        // cardinality, or sibling co-occurrence, synthesize the cascading GenerationRulePODs.
        if (bound_environment_ && obs.child_or_neighbor_state < bound_environment_->tokens().size() &&
            obs.parent_or_anchor_state < bound_environment_->tokens().size()) {
            const auto &tokens = bound_environment_->tokens();
            if (obs.axis == WFCConstraintAxis::AXIS_HIERARCHY_CHILD_TO_PARENT ||
                obs.axis == WFCConstraintAxis::AXIS_HIERARCHY_PARENT_TO_CHILD) {
                GenerationRulePOD parent_rule{};
                parent_rule.selector  = tokens[obs.child_or_neighbor_state].signature;
                parent_rule.operation = GenerationRuleOp::ACCEPT_PARENTS;
                parent_rule.scope     = PatternRuleScope::SELF;
                parent_rule.layer     = PatternCascadeLayer::LAYER_HIERARCHY_PARENT;
                parent_rule.specificity = CascadeRuleEngine::compute_generation_specificity(parent_rule);

                BitmaskDomain256POD allowed_parents{};
                allowed_parents.set_bit(obs.parent_or_anchor_state);
                bound_environment_->add_generation_rule(parent_rule, allowed_parents);

                GenerationRulePOD card_rule{};
                card_rule.selector              = tokens[obs.parent_or_anchor_state].signature;
                card_rule.operation             = GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY;
                card_rule.scope                 = PatternRuleScope::CHILDREN;
                card_rule.layer                 = PatternCascadeLayer::LAYER_CHILD_CARDINALITY;
                card_rule.min_occurrences       = static_cast<uint16_t>(std::min<uint32_t>(65535u, obs.min_occurrences));
                card_rule.max_occurrences       = static_cast<uint16_t>(std::min<uint32_t>(65535u, std::max(obs.min_occurrences, obs.max_occurrences)));
                card_rule.required_sibling_token  = obs.required_sibling_state;
                card_rule.forbidden_sibling_token = obs.forbidden_sibling_state;
                card_rule.specificity             = CascadeRuleEngine::compute_generation_specificity(card_rule);

                BitmaskDomain256POD child_mask{};
                child_mask.set_bit(obs.child_or_neighbor_state);
                bound_environment_->add_generation_rule(card_rule, child_mask);
            }
        }
    }
}

void WFCSolverCore::initialize_from_bound_environment(uint64_t p_seed) {
    rng_    = ProcGenSeedStreamPOD(p_seed);
    status_ = ProcGenSolverStatus::UNSOLVED;
    sync_from_bound_environment();
}

void WFCSolverCore::sync_from_bound_environment() {
    if (!bound_environment_) {
        return;
    }
    const auto &env_slots  = bound_environment_->slots();
    const auto &env_tokens = bound_environment_->tokens();

    state_count_ = std::min<uint32_t>(256u, static_cast<uint32_t>(env_tokens.size()));
    if (state_count_ == 0u) {
        state_count_ = 1u;
    }

    slot_domains_.resize(env_slots.size());
    for (size_t i = 0; i < env_slots.size(); ++i) {
        slot_domains_[i] = env_slots[i].domain;
    }

    token_weights_.resize(state_count_, 1.0f);
    for (uint32_t t = 0; t < state_count_ && t < env_tokens.size(); ++t) {
        const float w = env_tokens[t].base_frequency_weight;
        token_weights_[t] = (w > 0.0f && !std::isnan(w)) ? w : 1.0f;
    }

    // Import spatial neighbor CSR links from ProceduralEnvironment if not already added
    const auto &spatial_csr = bound_environment_->spatial_neighbor_csr();
    if (!spatial_csr.empty()) {
        for (size_t u = 0; u < env_slots.size(); ++u) {
            const uint32_t off = env_slots[u].spatial_csr_offset;
            const uint32_t cnt = env_slots[u].spatial_neighbor_count;
            for (uint32_t k = 0; k < cnt && (off + k) < spatial_csr.size(); ++k) {
                const auto &s_edge = spatial_csr[off + k];
                if (s_edge.neighbor_slot_id < env_slots.size()) {
                    bool found = false;
                    for (const auto &e : adjacency_edges_) {
                        if (e.from_slot == static_cast<uint32_t>(u) &&
                            e.to_slot == s_edge.neighbor_slot_id &&
                            e.port_id == s_edge.port_or_direction_id) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        add_directed_edge(
                            static_cast<uint32_t>(u),
                            s_edge.neighbor_slot_id,
                            s_edge.port_or_direction_id,
                            WFCConstraintAxis::AXIS_SPATIAL_PORT,
                            false
                        );
                    }
                }
            }
        }
    }

    // Import ALLOW_SPATIAL_NEIGHBOR rules from ProceduralEnvironment::active_rules()
    const auto &rules = bound_environment_->active_rules();
    const auto &masks = bound_environment_->rule_token_masks();
    for (const auto &rule : rules) {
        if (rule.operation == GenerationRuleOp::ALLOW_SPATIAL_NEIGHBOR &&
            rule.target_token_mask_index < masks.size()) {
            const BitmaskDomain256POD &allowed = masks[rule.target_token_mask_index];
            for (uint32_t t = 0; t < state_count_ && t < env_tokens.size(); ++t) {
                if (rule.selector.matches_candidate(env_tokens[t].signature)) {
                    add_rule(
                        static_cast<uint32_t>(std::max<int16_t>(0, rule.priority)),
                        t,
                        allowed,
                        WFCConstraintAxis::AXIS_SPATIAL_PORT,
                        1.0f
                    );
                }
            }
        }
    }

    partition_entropy_buckets();
}

void WFCSolverCore::sync_to_bound_environment() {
    if (!bound_environment_) {
        return;
    }
    auto &env_slots = bound_environment_->slots();
    const size_t n  = std::min(env_slots.size(), slot_domains_.size());
    for (size_t i = 0; i < n; ++i) {
        env_slots[i].domain             = slot_domains_[i];
        const uint32_t pop              = slot_domains_[i].popcount();
        env_slots[i].collapsed_token_id = (pop == 1u) ? slot_domains_[i].first_set_state() : -1;
        if (pop == 0u) {
            env_slots[i].has_conflict = 1u;
        }
        if (i < slot_entropy_buckets_.size()) {
            env_slots[i].entropy_bucket = slot_entropy_buckets_[i];
        }
    }
    for (const auto &rep : contradiction_reports_) {
        if (rep.slot_id < env_slots.size()) {
            env_slots[rep.slot_id].has_conflict = 1u;
        }
    }
    bound_environment_->compute_einselection_redundancy_scores();
}

float WFCSolverCore::compute_slot_entropy(uint32_t p_slot_id) const noexcept {
    if (p_slot_id >= slot_domains_.size()) {
        return 0.0f;
    }
    const BitmaskDomain256POD &dom = slot_domains_[p_slot_id];
    const uint32_t pop = dom.popcount();
    if (pop <= 1u) {
        return 0.0f;
    }

    if (entropy_heuristic_ == WFCEntropyHeuristic::HEURISTIC_POPCOUNT_MRV) {
        return static_cast<float>(pop);
    }

    double sum_w       = 0.0;
    double sum_w_log_w = 0.0;
    const uint32_t lim = std::min<uint32_t>(256u, state_count_ > 0u ? state_count_ : 256u);
    for (uint32_t s = 0; s < lim; ++s) {
        if (!dom.test_bit(s)) {
            continue;
        }
        const double w = (s < token_weights_.size() && token_weights_[s] > 0.0f)
            ? static_cast<double>(token_weights_[s])
            : 1.0;
        sum_w += w;
        sum_w_log_w += w * std::log2(w);
    }
    if (sum_w <= 0.0) {
        return 0.0f;
    }
    const double shannon = std::max(0.0, std::log2(sum_w) - (sum_w_log_w / sum_w));
    return static_cast<float>(shannon);
}

void WFCSolverCore::partition_entropy_buckets() noexcept {
    const size_t n = slot_domains_.size();
    slot_entropies_.resize(n, 0.0f);
    slot_entropy_buckets_.resize(n, 0u);

    const float max_possible_entropy = (state_count_ > 1u)
        ? std::log2(static_cast<float>(state_count_))
        : 1.0f;
    const float inv_max = (max_possible_entropy > 1e-5f) ? (1.0f / max_possible_entropy) : 1.0f;

    for (size_t i = 0; i < n; ++i) {
        const uint32_t pop = slot_domains_[i].popcount();
        const float ent    = compute_slot_entropy(static_cast<uint32_t>(i));
        slot_entropies_[i] = ent;

        if (pop == 0u) {
            slot_entropy_buckets_[i] = 15u; // Reserved bucket 15 for contradicted/empty slots
        } else if (pop == 1u) {
            slot_entropy_buckets_[i] = 0u;  // Bucket 0 for settled/collapsed slots
        } else {
            const float norm = std::clamp(ent * inv_max, 0.0f, 1.0f);
            const uint8_t b  = static_cast<uint8_t>(1u + static_cast<uint32_t>(std::min(13.0f, std::floor(norm * 13.99f))));
            slot_entropy_buckets_[i] = b;   // Buckets 1..14 for uncollapsed superpositions
        }

        if (bound_environment_ && i < bound_environment_->slots().size()) {
            bound_environment_->slots()[i].entropy_bucket = slot_entropy_buckets_[i];
        }
    }
}

int32_t WFCSolverCore::select_next_slot_to_collapse() noexcept {
    partition_entropy_buckets();

    int32_t best_slot  = -1;
    float best_score   = std::numeric_limits<float>::max();

    for (uint32_t i = 0; i < static_cast<uint32_t>(slot_domains_.size()); ++i) {
        const uint32_t pop = slot_domains_[i].popcount();
        if (pop == 0u) {
            status_ = ProcGenSolverStatus::CONTRADICTION;
            return -1;
        }
        if (pop == 1u) {
            continue;
        }

        float score = slot_entropies_[i];
        if (entropy_heuristic_ == WFCEntropyHeuristic::HEURISTIC_HIERARCHY_TOP_DOWN_SHANNON) {
            // Prioritize root and parent game-configuration slots before deep leaf slots
            const uint32_t depth = compute_slot_hierarchy_depth(i);
            score += static_cast<float>(depth) * 16.0f;
        } else if (entropy_heuristic_ == WFCEntropyHeuristic::HEURISTIC_CASCADE_LAYER_PRIORITY &&
                   bound_environment_ && i < bound_environment_->slots().size()) {
            const uint8_t layer_val = static_cast<uint8_t>(bound_environment_->slots()[i].locked_layer);
            score += static_cast<float>(6u - std::min<uint8_t>(6u, layer_val)) * 4.0f;
        }

        // Small deterministic SplitMix64 tie-breaking jitter
        const float jitter = rng_.next_unit_float() * 1e-4f;
        const float candidate_score = score + jitter;
        if (candidate_score < best_score) {
            best_score = candidate_score;
            best_slot  = static_cast<int32_t>(i);
        }
    }
    return best_slot;
}

int32_t WFCSolverCore::sample_state_for_slot(uint32_t p_slot_id) noexcept {
    if (p_slot_id >= slot_domains_.size()) {
        return -1;
    }
    const BitmaskDomain256POD &dom = slot_domains_[p_slot_id];
    if (dom.empty()) {
        return -1;
    }
    const uint32_t lim = std::min<uint32_t>(256u, state_count_ > 0u ? state_count_ : 256u);

    float total_weight = 0.0f;
    for (uint32_t s = 0; s < lim; ++s) {
        if (dom.test_bit(s)) {
            const float w = (s < token_weights_.size() && token_weights_[s] > 0.0f)
                ? token_weights_[s]
                : 1.0f;
            total_weight += w;
        }
    }

    if (total_weight <= 0.0f) {
        return dom.first_set_state();
    }

    const float pick = rng_.next_unit_float() * total_weight;
    float accum      = 0.0f;
    int32_t fallback = -1;
    for (uint32_t s = 0; s < lim; ++s) {
        if (!dom.test_bit(s)) {
            continue;
        }
        fallback = static_cast<int32_t>(s);
        const float w = (s < token_weights_.size() && token_weights_[s] > 0.0f)
            ? token_weights_[s]
            : 1.0f;
        accum += w;
        if (pick <= accum) {
            return static_cast<int32_t>(s);
        }
    }
    return fallback;
}

bool WFCSolverCore::audit_contradictions() {
    contradiction_reports_.clear();
    bool any_conflict = false;

    for (uint32_t i = 0; i < static_cast<uint32_t>(slot_domains_.size()); ++i) {
        if (slot_domains_[i].empty()) {
            any_conflict = true;
            WFCContradictionReportPOD rep{};
            rep.slot_id                    = i;
            rep.parent_slot_id             = (bound_environment_ && i < bound_environment_->slots().size())
                ? bound_environment_->slots()[i].parent_slot_id
                : 0xFFFFFFFFu;
            rep.contradiction_flags        = WFC_CONTRADICTION_EMPTY_DOMAIN;
            rep.recommended_horizon_radius = recommended_horizon_radius_;
            rep.local_einselection_score   = (bound_environment_ && i < bound_environment_->slots().size())
                ? bound_environment_->slots()[i].einselection_score
                : 0.0f;
            contradiction_reports_.push_back(rep);
        }
    }

    if (bound_environment_ && enable_hierarchy_and_cardinality_rules_) {
        const auto &env_slots  = bound_environment_->slots();
        const auto &env_tokens = bound_environment_->tokens();
        const auto &env_rules  = bound_environment_->active_rules();
        const auto &env_masks  = bound_environment_->rule_token_masks();
        const auto &child_csr  = bound_environment_->child_slot_csr();

        for (size_t i = 0; i < env_slots.size() && i < slot_domains_.size(); ++i) {
            if (env_slots[i].has_conflict != 0u && !slot_domains_[i].empty()) {
                any_conflict = true;
                WFCContradictionReportPOD rep{};
                rep.slot_id                    = static_cast<uint32_t>(i);
                rep.parent_slot_id             = env_slots[i].parent_slot_id;
                rep.contradiction_flags        = WFC_CONTRADICTION_CARDINALITY_MIN_DEFICIT;
                rep.recommended_horizon_radius = recommended_horizon_radius_;
                rep.local_einselection_score   = env_slots[i].einselection_score;
                rep.failed_cascade_layer       = env_slots[i].locked_layer;
                contradiction_reports_.push_back(rep);
            }

            // Audit explicit child cardinality & sibling co-occurrence rules on collapsed parent slots
            const int32_t p_tok = slot_domains_[i].popcount() == 1u ? slot_domains_[i].first_set_state() : -1;
            if (p_tok >= 0 && static_cast<size_t>(p_tok) < env_tokens.size() && env_slots[i].children_count > 0u) {
                const uint32_t c_off = env_slots[i].children_csr_offset;
                const uint32_t c_cnt = env_slots[i].children_count;

                for (const auto &rule : env_rules) {
                    if (rule.operation != GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY &&
                        rule.operation != GenerationRuleOp::REQUIRE_SIBLING &&
                        rule.operation != GenerationRuleOp::FORBID_SIBLING) {
                        continue;
                    }
                    if (!rule.selector.matches_candidate(env_tokens[static_cast<size_t>(p_tok)].signature)) {
                        continue;
                    }
                    if (rule.target_token_mask_index >= env_masks.size()) {
                        continue;
                    }
                    const BitmaskDomain256POD &target_mask = env_masks[rule.target_token_mask_index];
                    const int32_t target_tok = target_mask.first_set_state();
                    if (target_tok < 0) {
                        continue;
                    }

                    uint32_t guaranteed = 0u;
                    uint32_t possible   = 0u;
                    for (uint32_t k = 0; k < c_cnt && (c_off + k) < child_csr.size(); ++k) {
                        const uint32_t c_slot = child_csr[c_off + k];
                        if (c_slot >= slot_domains_.size()) continue;
                        const auto &c_dom = slot_domains_[c_slot];
                        if (c_dom.test_bit(static_cast<uint32_t>(target_tok))) {
                            ++possible;
                            if (c_dom.popcount() == 1u) {
                                ++guaranteed;
                            }
                        }
                    }

                    uint32_t flags = WFC_CONTRADICTION_NONE;
                    if (rule.operation == GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY) {
                        if (possible < rule.min_occurrences) {
                            flags |= WFC_CONTRADICTION_CARDINALITY_MIN_DEFICIT;
                        }
                        if (guaranteed > rule.max_occurrences) {
                            flags |= WFC_CONTRADICTION_CARDINALITY_MAX_EXCEEDED;
                        }
                    }

                    if (rule.required_sibling_token != 0xFFFFFFFFu && guaranteed > 0u) {
                        bool req_possible = false;
                        for (uint32_t k = 0; k < c_cnt && (c_off + k) < child_csr.size(); ++k) {
                            const uint32_t c_slot = child_csr[c_off + k];
                            if (c_slot < slot_domains_.size() &&
                                slot_domains_[c_slot].test_bit(rule.required_sibling_token)) {
                                req_possible = true;
                                break;
                            }
                        }
                        if (!req_possible) {
                            flags |= WFC_CONTRADICTION_MISSING_REQUIRED_SIBLING;
                        }
                    }

                    if (flags != WFC_CONTRADICTION_NONE) {
                        any_conflict = true;
                        WFCContradictionReportPOD rep{};
                        rep.slot_id                    = static_cast<uint32_t>(i);
                        rep.parent_slot_id             = env_slots[i].parent_slot_id;
                        rep.violated_rule_id           = rule.rule_id;
                        rep.contradiction_flags        = flags;
                        rep.deficit_or_excess_token_id = static_cast<uint32_t>(target_tok);
                        rep.recommended_horizon_radius = recommended_horizon_radius_;
                        rep.local_einselection_score   = env_slots[i].einselection_score;
                        rep.failed_cascade_layer       = rule.layer;
                        contradiction_reports_.push_back(rep);
                    }
                }
            }
        }
    }

    return !any_conflict;
}

bool WFCSolverCore::propagate_arc_consistency() {
    contradiction_reports_.clear();

    bool global_changed = true;
    uint32_t outer_pass = 0u;

    while (global_changed && outer_pass < 32u) {
        global_changed = false;
        ++outer_pass;

        // 1. Directional Port / Reflection / Socket Hypergraph AC-3 Wave
        std::queue<uint32_t> worklist;
        for (uint32_t i = 0; i < static_cast<uint32_t>(slot_domains_.size()); ++i) {
            if (slot_domains_[i].empty()) {
                status_ = ProcGenSolverStatus::CONTRADICTION;
                audit_contradictions();
                sync_to_bound_environment();
                return false;
            }
            worklist.push(i);
        }

        while (!worklist.empty()) {
            const uint32_t u = worklist.front();
            worklist.pop();

            for (const auto &edge : adjacency_edges_) {
                // Forward direction: u -> edge.to_slot
                if (edge.from_slot == u && edge.to_slot < slot_domains_.size()) {
                    BitmaskDomain256POD union_allowed{};
                    bool has_port_rules = false;
                    for (const auto &rule : rule_tensor_) {
                        if (rule.port_id == edge.port_id &&
                            rule.axis == edge.axis &&
                            slot_domains_[u].test_bit(rule.source_state)) {
                            has_port_rules = true;
                            union_allowed.union_with(rule.allowed_mask);
                        }
                    }
                    if (has_port_rules) {
                        auto &target_dom = slot_domains_[edge.to_slot];
                        if (target_dom.intersect_with(union_allowed)) {
                            global_changed = true;
                            if (target_dom.empty()) {
                                status_ = ProcGenSolverStatus::CONTRADICTION;
                                WFCContradictionReportPOD rep{};
                                rep.slot_id                    = edge.to_slot;
                                rep.violated_rule_id           = edge.port_id;
                                rep.contradiction_flags        = WFC_CONTRADICTION_EMPTY_DOMAIN | WFC_CONTRADICTION_PORT_ADJACENCY_EMPTY;
                                rep.recommended_horizon_radius = recommended_horizon_radius_;
                                contradiction_reports_.push_back(rep);
                                sync_to_bound_environment();
                                return false;
                            }
                            worklist.push(edge.to_slot);
                        }
                    }
                }

                // Reverse direction on bidirectional edges: u == edge.to_slot -> prune edge.from_slot
                if (edge.is_bidirectional != 0u && edge.to_slot == u && edge.from_slot < slot_domains_.size()) {
                    BitmaskDomain256POD reverse_allowed{};
                    bool has_reverse_rules = false;
                    for (const auto &rule : rule_tensor_) {
                        if (rule.port_id == edge.port_id && rule.axis == edge.axis) {
                            has_reverse_rules = true;
                            if (rule.allowed_mask.overlaps(slot_domains_[u])) {
                                reverse_allowed.set_bit(rule.source_state);
                            }
                        }
                    }
                    if (has_reverse_rules) {
                        auto &src_dom = slot_domains_[edge.from_slot];
                        if (src_dom.intersect_with(reverse_allowed)) {
                            global_changed = true;
                            if (src_dom.empty()) {
                                status_ = ProcGenSolverStatus::CONTRADICTION;
                                WFCContradictionReportPOD rep{};
                                rep.slot_id                    = edge.from_slot;
                                rep.violated_rule_id           = edge.port_id;
                                rep.contradiction_flags        = WFC_CONTRADICTION_EMPTY_DOMAIN | WFC_CONTRADICTION_PORT_ADJACENCY_EMPTY;
                                rep.recommended_horizon_radius = recommended_horizon_radius_;
                                contradiction_reports_.push_back(rep);
                                sync_to_bound_environment();
                                return false;
                            }
                            worklist.push(edge.from_slot);
                        }
                    }
                }
            }
        }

        // 2. Cascading ProceduralPattern Hierarchy, Child Cardinality [min..max], & Sibling Dependency Wave
        if (bound_environment_ && enable_hierarchy_and_cardinality_rules_) {
            auto &env_slots = bound_environment_->slots();
            const size_t n  = std::min(env_slots.size(), slot_domains_.size());
            for (size_t i = 0; i < n; ++i) {
                env_slots[i].domain             = slot_domains_[i];
                env_slots[i].collapsed_token_id = (slot_domains_[i].popcount() == 1u)
                    ? slot_domains_[i].first_set_state()
                    : -1;
            }

            const bool env_ok = bound_environment_->propagate_hierarchy_and_cardinality_constraints();
            for (size_t i = 0; i < n; ++i) {
                if (!(slot_domains_[i] == env_slots[i].domain)) {
                    slot_domains_[i] = env_slots[i].domain;
                    global_changed   = true;
                }
            }

            if (!env_ok) {
                status_ = ProcGenSolverStatus::CONTRADICTION;
                audit_contradictions();
                sync_to_bound_environment();
                return false;
            }
        }
    }

    partition_entropy_buckets();
    sync_to_bound_environment();
    return audit_contradictions();
}

ProcGenSolverStatus WFCSolverCore::step_collapse() {
    if (status_ == ProcGenSolverStatus::UNSOLVED) {
        status_ = ProcGenSolverStatus::IN_PROGRESS;
        if (!apply_pinned_landmarks() || !propagate_arc_consistency()) {
            status_ = ProcGenSolverStatus::CONTRADICTION;
            return status_;
        }
    }

    const int32_t slot = select_next_slot_to_collapse();
    if (status_ == ProcGenSolverStatus::CONTRADICTION) {
        return status_;
    }
    if (slot < 0) {
        status_ = ProcGenSolverStatus::SOLVED;
        sync_to_bound_environment();
        return status_;
    }

    const uint32_t u_slot       = static_cast<uint32_t>(slot);
    const uint32_t pre_popcount = slot_domains_[u_slot].popcount();
    const float pre_entropy     = (u_slot < slot_entropies_.size())
        ? slot_entropies_[u_slot]
        : compute_slot_entropy(u_slot);
    const uint8_t pre_bucket    = (u_slot < slot_entropy_buckets_.size())
        ? slot_entropy_buckets_[u_slot]
        : 1u;

    uint32_t total_bits_before = 0u;
    for (const auto &d : slot_domains_) {
        total_bits_before += d.popcount();
    }

    const int32_t chosen_state = sample_state_for_slot(u_slot);
    if (chosen_state < 0) {
        status_ = ProcGenSolverStatus::CONTRADICTION;
        audit_contradictions();
        sync_to_bound_environment();
        return status_;
    }

    slot_domains_[u_slot].clear();
    slot_domains_[u_slot].set_bit(static_cast<uint32_t>(chosen_state));

    const bool ok = propagate_arc_consistency();

    uint32_t total_bits_after = 0u;
    for (const auto &d : slot_domains_) {
        total_bits_after += d.popcount();
    }

    WFCCollapseStepRecordPOD step_rec{};
    step_rec.step_index            = static_cast<uint32_t>(collapse_history_.size());
    step_rec.slot_id               = u_slot;
    step_rec.chosen_token_id       = chosen_state;
    step_rec.pre_collapse_popcount = pre_popcount;
    step_rec.pre_collapse_entropy  = pre_entropy;
    step_rec.propagated_prunes     = (total_bits_before > total_bits_after)
        ? (total_bits_before - total_bits_after)
        : 0u;
    step_rec.active_layer          = (bound_environment_ && u_slot < bound_environment_->slots().size())
        ? bound_environment_->slots()[u_slot].locked_layer
        : PatternCascadeLayer::LAYER_DOMAIN;
    step_rec.entropy_bucket        = pre_bucket;
    step_rec.caused_contradiction  = ok ? 0u : 1u;
    collapse_history_.push_back(step_rec);

    if (!ok) {
        status_ = ProcGenSolverStatus::CONTRADICTION;
        return status_;
    }

    // Check if all slots are now collapsed
    bool all_collapsed = true;
    for (const auto &d : slot_domains_) {
        if (d.popcount() != 1u) {
            all_collapsed = false;
            break;
        }
    }
    if (all_collapsed) {
        status_ = ProcGenSolverStatus::SOLVED;
    }
    return status_;
}

ProcGenSolverStatus WFCSolverCore::solve_all(uint32_t p_max_steps) {
    collapse_history_.clear();
    contradiction_reports_.clear();
    status_ = ProcGenSolverStatus::IN_PROGRESS;

    if (bound_environment_ && slot_domains_.empty()) {
        sync_from_bound_environment();
    }

    if (!apply_pinned_landmarks() || !propagate_arc_consistency()) {
        status_ = ProcGenSolverStatus::CONTRADICTION;
        return status_;
    }

    for (uint32_t step = 0; step < p_max_steps; ++step) {
        const ProcGenSolverStatus st = step_collapse();
        if (st == ProcGenSolverStatus::SOLVED || st == ProcGenSolverStatus::CONTRADICTION) {
            return st;
        }
    }
    return status_;
}

} // namespace ideam::core

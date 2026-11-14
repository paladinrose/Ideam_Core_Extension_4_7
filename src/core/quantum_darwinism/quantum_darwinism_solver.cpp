#include "quantum_darwinism_solver.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace ideam::core {

bool QuantumDarwinismSolver::isolate_horizon_box(
    const ProceduralEnvironment &p_environment,
    std::span<const uint32_t> p_conflict_slot_ids
) {
    horizon_partitions_.clear();
    horizon_slot_indices_.clear();
    pointer_states_.clear();
    bridge_slots_.clear();

    const auto &slots       = p_environment.slots();
    const auto &child_csr   = p_environment.child_slot_csr();
    const auto &spatial_csr = p_environment.spatial_neighbor_csr();
    const size_t n_slots    = slots.size();

    if (n_slots == 0u) {
        einselection_status_ = EinselectionStatus::IDLE;
        return false;
    }

    horizon_partitions_.resize(n_slots);
    for (size_t i = 0; i < n_slots; ++i) {
        horizon_partitions_[i].slot_id                 = static_cast<uint32_t>(i);
        horizon_partitions_[i].nearest_conflict_slot   = 0xFFFFFFFFu;
        horizon_partitions_[i].predecessor_toward_conf = 0xFFFFFFFFu;
        horizon_partitions_[i].hop_distance            = 0xFFFFu;
        horizon_partitions_[i].role_flags              = HORIZON_ROLE_OUTSIDE;
        horizon_partitions_[i].locked_cascade_floor    = static_cast<uint8_t>(config_.min_locked_layer);
    }

    std::queue<uint32_t> bfs_queue;
    const uint32_t max_radius = std::max<uint32_t>(1u, config_.max_graph_hop_radius);
    const uint32_t max_box    = std::max<uint32_t>(1u, config_.max_horizon_slots);

    auto seed_conflict = [&](uint32_t c_id) {
        if (c_id >= n_slots) return;
        if (horizon_partitions_[c_id].hop_distance == 0u) return;
        horizon_partitions_[c_id].nearest_conflict_slot   = c_id;
        horizon_partitions_[c_id].predecessor_toward_conf = c_id;
        horizon_partitions_[c_id].hop_distance            = 0u;
        horizon_partitions_[c_id].role_flags              = HORIZON_ROLE_CONFLICT_CENTER;
        horizon_slot_indices_.push_back(c_id);
        bfs_queue.push(c_id);
    };

    for (uint32_t c_id : p_conflict_slot_ids) {
        seed_conflict(c_id);
    }
    // Also pick up any slot already flagged in `p_environment` if none were explicitly passed
    if (bfs_queue.empty()) {
        for (size_t i = 0; i < n_slots; ++i) {
            if (slots[i].has_conflict != 0u || slots[i].domain.empty()) {
                seed_conflict(static_cast<uint32_t>(i));
            }
        }
    }

    if (bfs_queue.empty()) {
        einselection_status_ = EinselectionStatus::IDLE;
        return false;
    }

    while (!bfs_queue.empty() && horizon_slot_indices_.size() < max_box) {
        const uint32_t u    = bfs_queue.front();
        bfs_queue.pop();
        const uint16_t dist = horizon_partitions_[u].hop_distance;
        if (dist >= max_radius) {
            continue;
        }

        auto try_visit = [&](uint32_t v) {
            if (v >= n_slots || horizon_slot_indices_.size() >= max_box) return;
            if (horizon_partitions_[v].hop_distance != 0xFFFFu) return;

            const uint16_t next_dist = static_cast<uint16_t>(dist + 1u);
            horizon_partitions_[v].hop_distance            = next_dist;
            horizon_partitions_[v].nearest_conflict_slot   = horizon_partitions_[u].nearest_conflict_slot;
            horizon_partitions_[v].predecessor_toward_conf = u;
            horizon_partitions_[v].role_flags              = (next_dist == max_radius)
                ? HORIZON_ROLE_PERIMETER
                : HORIZON_ROLE_INTERIOR;
            horizon_slot_indices_.push_back(v);
            if (next_dist < max_radius) {
                bfs_queue.push(v);
            }
        };

        const EnvironmentSlotPOD &u_slot = slots[u];

        // 1. Hierarchy Parent hop
        if (u_slot.parent_slot_id < n_slots) {
            const uint32_t p_id = u_slot.parent_slot_id;
            try_visit(p_id);

            // 2. Sibling hops under the same parent
            const EnvironmentSlotPOD &p_slot = slots[p_id];
            if (p_slot.children_count > 0u &&
                (p_slot.children_csr_offset + p_slot.children_count) <= child_csr.size()) {
                for (uint32_t k = 0; k < p_slot.children_count; ++k) {
                    try_visit(child_csr[p_slot.children_csr_offset + k]);
                }
            }
        }

        // 3. Hierarchy Children hops
        if (u_slot.children_count > 0u &&
            (u_slot.children_csr_offset + u_slot.children_count) <= child_csr.size()) {
            for (uint32_t k = 0; k < u_slot.children_count; ++k) {
                try_visit(child_csr[u_slot.children_csr_offset + k]);
            }
        }

        // 4. Spatial / Port Neighbor CSR hops
        if (u_slot.spatial_neighbor_count > 0u &&
            static_cast<size_t>(u_slot.spatial_csr_offset + u_slot.spatial_neighbor_count) <= spatial_csr.size()) {
            for (uint16_t k = 0; k < u_slot.spatial_neighbor_count; ++k) {
                try_visit(spatial_csr[u_slot.spatial_csr_offset + k].neighbor_slot_id);
            }
        }
    }

    einselection_status_ = EinselectionStatus::HORIZON_ISOLATED;
    return !horizon_slot_indices_.empty();
}

bool QuantumDarwinismSolver::establish_pointer_states(ProceduralEnvironment &r_environment) {
    pointer_states_.clear();
    r_environment.compute_einselection_redundancy_scores(config_.pointer_redundancy_thresh);

    auto &slots        = r_environment.slots();
    const auto &tokens = r_environment.tokens();
    const auto &rules  = r_environment.active_rules();

    for (uint32_t slot_id : horizon_slot_indices_) {
        if (slot_id >= slots.size()) continue;
        EnvironmentSlotPOD &slot        = slots[slot_id];
        HorizonSlotPartitionPOD &h_part = horizon_partitions_[slot_id];

        if (slot.has_conflict != 0u || slot.domain.popcount() != 1u) {
            slot.is_pointer_state = 0u;
            continue;
        }

        const int32_t settled_tok = slot.domain.first_set_state();
        if (settled_tok < 0 || static_cast<size_t>(settled_tok) >= tokens.size()) {
            continue;
        }

        const bool is_pinned    = (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE);
        const bool is_perimeter = ((h_part.role_flags & HORIZON_ROLE_PERIMETER) != 0u) && (config_.promote_perimeter_anchors != 0u);
        const bool meets_thresh = (slot.einselection_score >= config_.pointer_redundancy_thresh);

        if (is_pinned || is_perimeter || meets_thresh) {
            slot.is_pointer_state = 1u;
            h_part.role_flags    |= HORIZON_ROLE_POINTER_STATE;

            const TokenDescriptorPOD &tok = tokens[static_cast<size_t>(settled_tok)];
            uint32_t max_spec = 0u;
            for (const GenerationRulePOD &rule : rules) {
                if (rule.selector.matches_candidate(tok.signature)) {
                    max_spec = std::max<uint32_t>(max_spec, rule.specificity);
                }
            }

            PointerStateRecordPOD rec{};
            rec.slot_id                  = slot_id;
            rec.settled_token_id         = static_cast<uint32_t>(settled_tok);
            rec.einselection_redundancy  = slot.einselection_score;
            rec.highest_satisfied_spec   = max_spec;
            rec.preserved_trait_mask     = tok.coherence_trait_mask | tok.nuclear_seme_mask_lo32;
            rec.hop_distance_to_conflict = h_part.hop_distance;
            pointer_states_.push_back(rec);
        } else {
            slot.is_pointer_state = 0u;
        }
    }

    // Sort pointer states by highest redundancy R_delta, then highest CSS specificity
    std::sort(
        pointer_states_.begin(),
        pointer_states_.end(),
        [](const PointerStateRecordPOD &a, const PointerStateRecordPOD &b) {
            if (std::abs(a.einselection_redundancy - b.einselection_redundancy) > 1e-4f) {
                return a.einselection_redundancy > b.einselection_redundancy;
            }
            return a.highest_satisfied_spec > b.highest_satisfied_spec;
        }
    );

    einselection_status_ = EinselectionStatus::POINTERS_ESTABLISHED;
    return !pointer_states_.empty();
}

bool QuantumDarwinismSolver::evaluate_coherence_bridge_paths(const ProceduralEnvironment &p_environment) {
    bridge_slots_.clear();
    const auto &slots  = p_environment.slots();
    const auto &tokens = p_environment.tokens();

    std::vector<uint8_t> added_bridge(slots.size(), 0u);

    // 1. Trace inward paths from every Pointer State to its nearest conflict slot
    for (const PointerStateRecordPOD &ptr_rec : pointer_states_) {
        uint32_t curr      = ptr_rec.slot_id;
        uint32_t hops_from = 0u;

        while (curr < horizon_partitions_.size() && hops_from <= config_.max_graph_hop_radius) {
            const uint32_t next = horizon_partitions_[curr].predecessor_toward_conf;
            if (next >= slots.size() || next == curr) {
                break;
            }
            ++hops_from;
            const HorizonSlotPartitionPOD &next_part = horizon_partitions_[next];

            if (added_bridge[next] == 0u && (next_part.role_flags & HORIZON_ROLE_POINTER_STATE) == 0u) {
                added_bridge[next] = 1u;

                uint32_t mutable_mask = 0xFFFFFFFFu;
                const int32_t tok_id  = slots[next].collapsed_token_id;
                if (tok_id >= 0 && static_cast<size_t>(tok_id) < tokens.size()) {
                    const TokenDescriptorPOD &tok = tokens[static_cast<size_t>(tok_id)];
                    // Peripheral seme bits and variant traits are mutable; nuclear semes stay locked
                    mutable_mask = tok.peripheral_seme_mask_lo32 | (~tok.nuclear_seme_mask_lo32);
                }

                CoherenceBridgeSlotPOD bridge{};
                bridge.slot_id                 = next;
                bridge.source_pointer_slot_id  = ptr_rec.slot_id;
                bridge.target_conflict_slot_id = next_part.nearest_conflict_slot;
                bridge.hops_from_pointer       = hops_from;
                bridge.hops_to_conflict        = next_part.hop_distance;
                bridge.mutable_trait_mask      = mutable_mask;
                bridge.relax_down_to           = (next_part.hop_distance <= 1u)
                    ? PatternCascadeLayer::LAYER_SIBLING_DEPENDENCY
                    : PatternCascadeLayer::LAYER_CHILD_CARDINALITY;
                bridge_slots_.push_back(bridge);
            }
            if (next_part.hop_distance == 0u) {
                break;
            }
            curr = next;
        }
    }

    // 2. Also include any non-pointer interior or conflict slots inside E_box that were not on a direct pointer chain
    for (uint32_t slot_id : horizon_slot_indices_) {
        if (slot_id >= slots.size() || added_bridge[slot_id] != 0u) continue;
        const HorizonSlotPartitionPOD &h_part = horizon_partitions_[slot_id];
        if ((h_part.role_flags & HORIZON_ROLE_POINTER_STATE) != 0u) continue;

        added_bridge[slot_id] = 1u;
        CoherenceBridgeSlotPOD bridge{};
        bridge.slot_id                 = slot_id;
        bridge.source_pointer_slot_id  = pointer_states_.empty() ? slot_id : pointer_states_.front().slot_id;
        bridge.target_conflict_slot_id = h_part.nearest_conflict_slot;
        bridge.hops_from_pointer       = 1u;
        bridge.hops_to_conflict        = h_part.hop_distance;
        bridge.mutable_trait_mask      = 0xFFFFFFFFu;
        bridge.relax_down_to           = config_.min_locked_layer;
        bridge_slots_.push_back(bridge);
    }

    return !bridge_slots_.empty();
}

EinselectionStatus QuantumDarwinismSolver::propagate_pointer_coherence_to_conflict(
    ProceduralEnvironment &r_environment,
    WFCSolverCore *p_optional_wfc_solver
) {
    auto &slots           = r_environment.slots();
    const auto &tokens    = r_environment.tokens();
    const auto &rules     = r_environment.active_rules();
    const auto &masks     = r_environment.rule_token_masks();
    const auto &child_csr = r_environment.child_slot_csr();
    const uint32_t n_tok  = static_cast<uint32_t>(std::min<size_t>(256u, tokens.size()));

    if (slots.empty() || n_tok == 0u) {
        einselection_status_ = EinselectionStatus::UNRESOLVABLE_HORIZON;
        return einselection_status_;
    }

    // Compute union of admitted tokens from LAYER_DOMAIN (`ADMIT_ARCHETYPE`)
    BitmaskDomain256POD domain_admitted{};
    for (const GenerationRulePOD &rule : rules) {
        if (rule.operation == GenerationRuleOp::ADMIT_ARCHETYPE &&
            rule.target_token_mask_index < masks.size()) {
            domain_admitted.union_with(masks[rule.target_token_mask_index]);
        }
    }
    if (domain_admitted.empty()) {
        domain_admitted.fill_first_n(n_tok);
    }

    // Helper: compute valid domain for `slot_id` respecting locked high-precedence layers
    // (`LAYER_DOMAIN` + `LAYER_HIERARCHY_PARENT` + Pointer State nuclear seme coherence)
    auto build_coherent_relaxed_domain = [&](uint32_t p_slot_id, bool p_preserve_self_nuclear_identity) -> BitmaskDomain256POD {
        const EnvironmentSlotPOD &slot = slots[p_slot_id];
        if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && !slot.domain.empty()) {
            return slot.domain;
        }

        BitmaskDomain256POD relaxed = domain_admitted;

        // If this slot is a settled parent with children (e.g., CastleTower under CastleGrounds),
        // preserve its nuclear seme identity if requested so macro-hierarchy never collapses!
        if (p_preserve_self_nuclear_identity &&
            slot.collapsed_token_id >= 0 &&
            static_cast<size_t>(slot.collapsed_token_id) < tokens.size() &&
            slot.children_count > 0u) {
            const uint32_t self_nuc = tokens[static_cast<size_t>(slot.collapsed_token_id)].nuclear_seme_mask_lo32;
            BitmaskDomain256POD nuc_matches{};
            for (uint32_t t = 0; t < n_tok; ++t) {
                if ((tokens[t].nuclear_seme_mask_lo32 & self_nuc) == self_nuc &&
                    tokens[t].signature.object_name_hash == tokens[static_cast<size_t>(slot.collapsed_token_id)].signature.object_name_hash) {
                    nuc_matches.set_bit(t);
                }
            }
            if (!nuc_matches.empty()) {
                return nuc_matches;
            }
        }

        // Enforce upward ACCEPT_PARENTS compatibility against parent slot
        if (slot.parent_slot_id < slots.size()) {
            const EnvironmentSlotPOD &parent_slot = slots[slot.parent_slot_id];
            for (const GenerationRulePOD &rule : rules) {
                if (rule.operation != GenerationRuleOp::ACCEPT_PARENTS) continue;
                if (rule.target_token_mask_index >= masks.size()) continue;
                const BitmaskDomain256POD &allowed_parents = masks[rule.target_token_mask_index];
                for (uint32_t t = 0; t < n_tok; ++t) {
                    if (!relaxed.test_bit(t)) continue;
                    if (rule.selector.matches_candidate(tokens[t].signature) &&
                        !parent_slot.domain.overlaps(allowed_parents)) {
                        relaxed.clear_bit(t);
                    }
                }
            }

            // If parent is collapsed, restrict child domain to union of parent's ACCEPT_CHILDREN_CARDINALITY tokens
            const int32_t p_tok = (parent_slot.domain.popcount() == 1u)
                ? parent_slot.domain.first_set_state()
                : parent_slot.collapsed_token_id;
            if (p_tok >= 0 && static_cast<size_t>(p_tok) < tokens.size()) {
                BitmaskDomain256POD allowed_by_parent{};
                bool has_parent_child_rules = false;
                for (const GenerationRulePOD &rule : rules) {
                    if (rule.operation == GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY &&
                        rule.selector.matches_candidate(tokens[static_cast<size_t>(p_tok)].signature) &&
                        rule.target_token_mask_index < masks.size()) {
                        has_parent_child_rules = true;
                        allowed_by_parent.union_with(masks[rule.target_token_mask_index]);
                    }
                }
                if (has_parent_child_rules && !allowed_by_parent.empty()) {
                    relaxed.intersect_with(allowed_by_parent);
                }
            }
        }

        return relaxed.empty() ? domain_admitted : relaxed;
    };

    // 1. Relax bridge slots and conflicted slots according to CSS cascade precedence:
    //    - Pointer states stay 100% locked.
    //    - High-precedence structural parents (`CastleTower` with children) keep their parent identity,
    //      while their non-pinned child/sibling slots relax their `LAYER_CHILD_CARDINALITY` &
    //      `LAYER_SIBLING_DEPENDENCY` assignments so sibling budgets (`[min..max]` & `REQUIRE_SIBLING`)
    //      can rebalance cleanly!
    for (CoherenceBridgeSlotPOD &bridge : bridge_slots_) {
        if (bridge.slot_id >= slots.size()) continue;
        EnvironmentSlotPOD &slot = slots[bridge.slot_id];
        if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && !slot.domain.empty()) {
            continue;
        }

        const bool is_structural_parent = (slot.children_count > 0u && !slot.domain.empty());
        slot.domain            = build_coherent_relaxed_domain(bridge.slot_id, is_structural_parent);
        const uint32_t pop     = slot.domain.popcount();
        slot.collapsed_token_id = (pop == 1u) ? slot.domain.first_set_state() : -1;
        slot.has_conflict      = 0u;
        if (bridge.slot_id < horizon_partitions_.size()) {
            horizon_partitions_[bridge.slot_id].role_flags |= HORIZON_ROLE_BRIDGE_RELAXED;
        }

        // If this slot is a child under a parent, or is a parent whose child cardinality/sibling rules conflicted,
        // relax non-pinned children of that parent so sibling counts can rebalance
        const uint32_t rebalance_parent_id = (slot.children_count > 0u) ? bridge.slot_id : slot.parent_slot_id;
        if (config_.allow_sibling_rebalancing != 0u &&
            rebalance_parent_id < slots.size() &&
            slots[rebalance_parent_id].children_count > 0u &&
            (slots[rebalance_parent_id].children_csr_offset + slots[rebalance_parent_id].children_count) <= child_csr.size()) {
            const EnvironmentSlotPOD &reb_parent = slots[rebalance_parent_id];
            for (uint32_t k = 0; k < reb_parent.children_count; ++k) {
                const uint32_t c_id = child_csr[reb_parent.children_csr_offset + k];
                if (c_id >= slots.size()) continue;
                EnvironmentSlotPOD &c_slot = slots[c_id];
                if (c_slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && !c_slot.domain.empty()) {
                    continue;
                }
                c_slot.domain             = build_coherent_relaxed_domain(c_id, false);
                const uint32_t c_pop      = c_slot.domain.popcount();
                c_slot.collapsed_token_id = (c_pop == 1u) ? c_slot.domain.first_set_state() : -1;
                c_slot.has_conflict       = 0u;
                c_slot.is_pointer_state   = 0u;
            }
        }
    }

    // 2. Propagate hierarchy and cardinality constraints across the relaxed horizon box
    r_environment.propagate_hierarchy_and_cardinality_constraints();

    // 3. Deterministically collapse any re-opened slots inside E_box while respecting:
    //    - Unmet `min_occurrences` child deficits first (e.g., `TowerGate [1..2]`)
    //    - Required sibling tokens (`TowerStairs` when `TowerLanding` is present)
    //    - Pointer State nuclear seme affinity
    for (uint32_t pass = 0u; pass < static_cast<uint32_t>(horizon_slot_indices_.size()) + 4u; ++pass) {
        bool any_uncollapsed = false;
        for (uint32_t slot_id : horizon_slot_indices_) {
            if (slot_id >= slots.size()) continue;
            EnvironmentSlotPOD &slot = slots[slot_id];
            if (slot.domain.popcount() <= 1u) continue;
            any_uncollapsed = true;

            int32_t preferred_token = -1;

            // Check if parent needs a specific required sibling or min_occurrences child token
            if (slot.parent_slot_id < slots.size()) {
                const EnvironmentSlotPOD &parent_slot = slots[slot.parent_slot_id];
                const int32_t p_tok = (parent_slot.domain.popcount() == 1u)
                    ? parent_slot.domain.first_set_state()
                    : parent_slot.collapsed_token_id;
                if (p_tok >= 0 && static_cast<size_t>(p_tok) < tokens.size() &&
                    (parent_slot.children_csr_offset + parent_slot.children_count) <= child_csr.size()) {
                    for (const GenerationRulePOD &rule : rules) {
                        if (rule.operation != GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY) continue;
                        if (!rule.selector.matches_candidate(tokens[static_cast<size_t>(p_tok)].signature)) continue;
                        if (rule.target_token_mask_index >= masks.size()) continue;
                        const int32_t c_tok = masks[rule.target_token_mask_index].first_set_state();
                        if (c_tok < 0) continue;
                        const uint32_t u_c_tok = static_cast<uint32_t>(c_tok);

                        uint32_t definite = 0u;
                        for (uint32_t k = 0; k < parent_slot.children_count; ++k) {
                            const uint32_t sib_id = child_csr[parent_slot.children_csr_offset + k];
                            if (sib_id < slots.size() &&
                                slots[sib_id].domain.popcount() == 1u &&
                                slots[sib_id].domain.test_bit(u_c_tok)) {
                                ++definite;
                            }
                        }

                        // Priority A: If `u_c_tok` is already present and requires `rule.required_sibling_token`,
                        // check if `req_sib` is still missing among siblings!
                        if (definite > 0u && rule.required_sibling_token < n_tok &&
                            slot.domain.test_bit(rule.required_sibling_token)) {
                            uint32_t req_definite = 0u;
                            for (uint32_t k = 0; k < parent_slot.children_count; ++k) {
                                const uint32_t sib_id = child_csr[parent_slot.children_csr_offset + k];
                                if (sib_id < slots.size() &&
                                    slots[sib_id].domain.popcount() == 1u &&
                                    slots[sib_id].domain.test_bit(rule.required_sibling_token)) {
                                    ++req_definite;
                                }
                            }
                            if (req_definite == 0u) {
                                preferred_token = static_cast<int32_t>(rule.required_sibling_token);
                                break;
                            }
                        }

                        // Priority B: If `definite < rule.min_occurrences` and this slot can hold `u_c_tok`, pick it!
                        if (definite < rule.min_occurrences && slot.domain.test_bit(u_c_tok)) {
                            preferred_token = c_tok;
                            break;
                        }
                    }
                }
            }

            if (preferred_token < 0) {
                // Score remaining candidate tokens by Pointer State nuclear seme overlap
                uint32_t union_pointer_traits = 0u;
                for (const PointerStateRecordPOD &ptr_rec : pointer_states_) {
                    union_pointer_traits |= ptr_rec.preserved_trait_mask;
                }
                int32_t best_t   = slot.domain.first_set_state();
                uint32_t best_sc = 0u;
                for (uint32_t t = 0; t < n_tok; ++t) {
                    if (!slot.domain.test_bit(t)) continue;
                    const uint32_t overlap = static_cast<uint32_t>(
                        std::popcount((tokens[t].coherence_trait_mask | tokens[t].nuclear_seme_mask_lo32) & union_pointer_traits)
                    );
                    if (overlap > best_sc) {
                        best_sc = overlap;
                        best_t  = static_cast<int32_t>(t);
                    }
                }
                preferred_token = best_t;
            }

            if (preferred_token >= 0) {
                slot.domain.clear();
                slot.domain.set_bit(static_cast<uint32_t>(preferred_token));
                slot.collapsed_token_id = preferred_token;
                r_environment.propagate_hierarchy_and_cardinality_constraints();
                break;
            }
        }
        if (!any_uncollapsed) {
            break;
        }
    }

    // 4. Final constraint verification & Einselection Redundancy score update
    for (EnvironmentSlotPOD &slot : slots) {
        slot.has_conflict = slot.domain.empty() ? 1u : 0u;
    }
    const bool final_ok = r_environment.propagate_hierarchy_and_cardinality_constraints();
    r_environment.compute_einselection_redundancy_scores(config_.pointer_redundancy_thresh);

    if (p_optional_wfc_solver) {
        p_optional_wfc_solver->sync_from_bound_environment();
        p_optional_wfc_solver->propagate_arc_consistency();
    }

    einselection_status_ = final_ok
        ? EinselectionStatus::COHERENCE_RESTORED
        : EinselectionStatus::UNRESOLVABLE_HORIZON;
    return einselection_status_;
}

EinselectionStatus QuantumDarwinismSolver::stabilize_conflict_in_environment(
    ProceduralEnvironment &r_environment,
    uint32_t p_conflict_slot_id
) {
    bind_environment(&r_environment);
    const uint32_t conflict_ids[1] = { p_conflict_slot_id };
    if (!isolate_horizon_box(r_environment, conflict_ids)) {
        einselection_status_ = EinselectionStatus::UNRESOLVABLE_HORIZON;
        return einselection_status_;
    }
    establish_pointer_states(r_environment);
    evaluate_coherence_bridge_paths(r_environment);
    return propagate_pointer_coherence_to_conflict(r_environment, nullptr);
}

EinselectionStatus QuantumDarwinismSolver::stabilize_all_conflicts_in_environment(
    ProceduralEnvironment &r_environment,
    WFCSolverCore *p_optional_wfc_solver
) {
    bind_environment(&r_environment);
    std::vector<uint32_t> conflict_ids;
    for (size_t i = 0; i < r_environment.slots().size(); ++i) {
        if (r_environment.slots()[i].has_conflict != 0u || r_environment.slots()[i].domain.empty()) {
            conflict_ids.push_back(static_cast<uint32_t>(i));
        }
    }
    if (conflict_ids.empty()) {
        einselection_status_ = EinselectionStatus::COHERENCE_RESTORED;
        return einselection_status_;
    }
    if (!isolate_horizon_box(r_environment, conflict_ids)) {
        einselection_status_ = EinselectionStatus::UNRESOLVABLE_HORIZON;
        return einselection_status_;
    }
    establish_pointer_states(r_environment);
    evaluate_coherence_bridge_paths(r_environment);
    return propagate_pointer_coherence_to_conflict(r_environment, p_optional_wfc_solver);
}

} // namespace ideam::core

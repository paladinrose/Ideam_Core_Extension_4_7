#include "quantum_darwinism_solver.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <queue>

namespace ideam::core {

namespace {

[[nodiscard]] inline uint32_t splitmix_hash_u32(uint64_t p_seed, uint32_t p_a, uint32_t p_b, uint32_t p_c) noexcept {
    uint64_t x = p_seed ^ (static_cast<uint64_t>(p_a) * 0x9E3779B97F4A7C15ULL)
                        ^ (static_cast<uint64_t>(p_b) * 0xBF58476D1CE4E5B9ULL)
                        ^ (static_cast<uint64_t>(p_c) * 0x94D049BB133111EBULL);
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return static_cast<uint32_t>((x ^ (x >> 31)) & 0xFFFFFFFFULL);
}

[[nodiscard]] inline float hash_to_signed_unit(uint64_t p_seed, uint32_t p_a, uint32_t p_b, uint32_t p_c) noexcept {
    const uint32_t h = splitmix_hash_u32(p_seed, p_a, p_b, p_c);
    const float u01  = static_cast<float>(h >> 8) * (1.0f / 16777216.0f);
    return (u01 * 2.0f) - 1.0f;
}

template <typename Fn>
void for_each_topology_neighbor(
    const ProceduralEnvironment &p_env,
    uint32_t p_slot_id,
    uint8_t p_topology_mask,
    Fn &&p_callback
) {
    const auto &slots       = p_env.slots();
    const auto &child_csr   = p_env.child_slot_csr();
    const auto &spatial_csr = p_env.spatial_neighbor_csr();
    const uint32_t n_slots  = static_cast<uint32_t>(slots.size());
    if (p_slot_id >= n_slots) return;

    const EnvironmentSlotPOD &u_slot = slots[p_slot_id];

    // 1. Parent -> Child (Hierarchy Down)
    if ((p_topology_mask & TOPOLOGY_HIERARCHY_DOWN) != 0u &&
        u_slot.children_count > 0u &&
        (u_slot.children_csr_offset + u_slot.children_count) <= child_csr.size()) {
        for (uint32_t k = 0u; k < u_slot.children_count; ++k) {
            const uint32_t c_id = child_csr[u_slot.children_csr_offset + k];
            if (c_id < n_slots && c_id != p_slot_id) {
                p_callback(c_id, 1.0f, TOPOLOGY_HIERARCHY_DOWN);
            }
        }
    }

    // 2. Child -> Parent (Hierarchy Up)
    if ((p_topology_mask & TOPOLOGY_HIERARCHY_UP) != 0u &&
        u_slot.parent_slot_id < n_slots &&
        u_slot.parent_slot_id != p_slot_id) {
        p_callback(u_slot.parent_slot_id, 1.0f, TOPOLOGY_HIERARCHY_UP);
    }

    // 3. Siblings (Lateral under same parent)
    if ((p_topology_mask & TOPOLOGY_SIBLINGS) != 0u &&
        u_slot.parent_slot_id < n_slots) {
        const EnvironmentSlotPOD &p_slot = slots[u_slot.parent_slot_id];
        if (p_slot.children_count > 0u &&
            (p_slot.children_csr_offset + p_slot.children_count) <= child_csr.size()) {
            for (uint32_t k = 0u; k < p_slot.children_count; ++k) {
                const uint32_t sib_id = child_csr[p_slot.children_csr_offset + k];
                if (sib_id < n_slots && sib_id != p_slot_id) {
                    p_callback(sib_id, 1.0f, TOPOLOGY_SIBLINGS);
                }
            }
        }
    }

    // 4. Directed / Port / Reference / Spatial Edges
    if ((p_topology_mask & TOPOLOGY_GRAPH_OR_SPATIAL) != 0u &&
        u_slot.spatial_neighbor_count > 0u &&
        static_cast<size_t>(u_slot.spatial_csr_offset + u_slot.spatial_neighbor_count) <= spatial_csr.size()) {
        for (uint16_t k = 0u; k < u_slot.spatial_neighbor_count; ++k) {
            const SpatialNeighborEdgePOD &edge = spatial_csr[u_slot.spatial_csr_offset + k];
            if (edge.neighbor_slot_id < n_slots && edge.neighbor_slot_id != p_slot_id) {
                const float w = std::max(0.1f, edge.distance_weight);
                p_callback(edge.neighbor_slot_id, w, TOPOLOGY_GRAPH_OR_SPATIAL);
            }
        }
    }
}

[[nodiscard]] inline uint32_t full_mask_for_state_count(uint32_t p_count) noexcept {
    if (p_count == 0u || p_count >= 32u) return 0xFFFFFFFFu;
    return (1u << p_count) - 1u;
}

} // namespace

void QuantumDarwinismSolver::clear_channels() noexcept {
    channel_specs_.clear();
    slot_channel_states_.clear();
    nucleation_events_.clear();
    current_wave_index_ = 0u;
}

uint32_t QuantumDarwinismSolver::add_channel_spec(const PropertyChannelSpecPOD &p_spec) {
    PropertyChannelSpecPOD spec = p_spec;
    spec.channel_id = static_cast<uint32_t>(channel_specs_.size());
    channel_specs_.push_back(spec);
    return spec.channel_id;
}

void QuantumDarwinismSolver::clear_coupling_rules() noexcept {
    coupling_rules_.clear();
}

uint32_t QuantumDarwinismSolver::add_coupling_rule(const ChannelCouplingRulePOD &p_rule) {
    ChannelCouplingRulePOD rule = p_rule;
    rule.rule_id = static_cast<uint32_t>(coupling_rules_.size());
    coupling_rules_.push_back(rule);
    return rule.rule_id;
}

void QuantumDarwinismSolver::set_external_perturbation_field(std::span<const float> p_field) {
    external_perturbation_field_.assign(p_field.begin(), p_field.end());
}

float QuantumDarwinismSolver::sample_perturbation(uint32_t p_slot_id, uint32_t p_channel_id, uint32_t p_salt) const noexcept {
    const float amp = gen_config_.perturbation_strength;
    if (amp <= 0.0f) return 0.0f;

    float base = hash_to_signed_unit(gen_config_.master_seed, p_slot_id, p_channel_id, p_salt);
    if (!external_perturbation_field_.empty()) {
        const size_t idx = static_cast<size_t>(p_slot_id) % external_perturbation_field_.size();
        const float field_val = std::clamp(external_perturbation_field_[idx], -1.0f, 1.0f);
        base = 0.65f * field_val + 0.35f * base;
    }
    return base * amp;
}

bool QuantumDarwinismSolver::initialize_channel_states_from_environment(const ProceduralEnvironment &p_environment) {
    const auto &slots  = p_environment.slots();
    const auto &tokens = p_environment.tokens();
    const uint32_t n_slots = static_cast<uint32_t>(slots.size());
    if (n_slots == 0u) return false;

    // Ensure at least one default Archetype/Nuclear-Seme channel exists if the caller didn't author custom channels
    if (channel_specs_.empty()) {
        PropertyChannelSpecPOD default_ch{};
        default_ch.channel_id                 = 0u;
        default_ch.symbol_hash                = hash_reflection_symbol("archetype");
        default_ch.default_threshold_tau      = config_.pointer_redundancy_thresh;
        default_ch.default_permeability_gamma = 0.80f;
        default_ch.decay_per_hop              = 0.14f;
        default_ch.state_count                = static_cast<uint32_t>(std::clamp<size_t>(tokens.size(), 1u, 32u));
        default_ch.aspect                     = QDChannelAspect::ASPECT_CLASS_ARCHETYPE;
        default_ch.topology_mask              = TOPOLOGY_ALL;
        default_ch.is_discrete_mask           = 1u;
        channel_specs_.push_back(default_ch);
    }

    const uint32_t n_channels = static_cast<uint32_t>(channel_specs_.size());
    slot_channel_states_.assign(static_cast<size_t>(n_slots) * static_cast<size_t>(n_channels), SlotChannelStatePOD{});
    nucleation_events_.clear();
    current_wave_index_ = 0u;

    for (uint32_t s = 0u; s < n_slots; ++s) {
        const EnvironmentSlotPOD &slot = slots[s];
        const bool slot_pinned_or_settled =
            (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && slot.domain.popcount() == 1u) ||
            (slot.is_pointer_state != 0u && slot.domain.popcount() == 1u);
        const int32_t settled_tok = slot_pinned_or_settled ? slot.domain.first_set_state() : -1;

        for (uint32_t c = 0u; c < n_channels; ++c) {
            const PropertyChannelSpecPOD &spec = channel_specs_[c];
            SlotChannelStatePOD &st = slot_channel_states_[static_cast<size_t>(s) * n_channels + c];
            st.slot_id                 = s;
            st.channel_id              = c;
            st.candidate_state_mask    = spec.is_discrete_mask ? full_mask_for_state_count(spec.state_count) : 0u;
            st.value                   = spec.default_value;
            st.redundancy_r_delta      = 0.0f;
            st.effective_threshold_tau = std::clamp(spec.default_threshold_tau + sample_perturbation(s, c, 1u), 0.05f, 0.99f);
            st.effective_permeability  = std::clamp(spec.default_permeability_gamma, 0.05f, 1.0f);
            st.is_locked               = 0u;
            st.is_pinned               = 0u;
            st.is_nucleated            = 0u;
            st.locked_wave_index       = 0u;

            if (settled_tok >= 0 && static_cast<size_t>(settled_tok) < tokens.size()) {
                const TokenDescriptorPOD &tok = tokens[static_cast<size_t>(settled_tok)];
                st.redundancy_r_delta = 1.0f;
                st.is_locked          = 1u;
                st.is_pinned          = (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) ? 1u : 0u;

                if (spec.is_discrete_mask != 0u) {
                    uint32_t derived_mask = 0u;
                    switch (spec.aspect) {
                        case QDChannelAspect::ASPECT_GROUP_OR_TAG:
                            derived_mask = tok.signature.group_or_tag_mask;
                            break;
                        case QDChannelAspect::ASPECT_NUCLEAR_SEME:
                            derived_mask = tok.nuclear_seme_mask_lo32;
                            break;
                        case QDChannelAspect::ASPECT_PERIPHERAL_SEME:
                            derived_mask = tok.peripheral_seme_mask_lo32;
                            break;
                        case QDChannelAspect::ASPECT_METHOD_TRAIT:
                            derived_mask = tok.signature.method_trait_mask;
                            break;
                        case QDChannelAspect::ASPECT_SIGNAL_TRAIT:
                            derived_mask = tok.signature.signal_trait_mask;
                            break;
                        case QDChannelAspect::ASPECT_REFLECTED_PROPERTY:
                            derived_mask = tok.signature.property_trait_mask;
                            break;
                        default:
                            derived_mask = (static_cast<uint32_t>(settled_tok) < 32u)
                                ? (1u << static_cast<uint32_t>(settled_tok))
                                : 1u;
                            break;
                    }
                    if (derived_mask == 0u) {
                        derived_mask = 1u << (static_cast<uint32_t>(settled_tok) % std::max<uint32_t>(1u, std::min<uint32_t>(32u, spec.state_count)));
                    }
                    st.candidate_state_mask = derived_mask;
                    st.value = static_cast<float>(std::countr_zero(derived_mask));
                } else {
                    const float denom = static_cast<float>(std::max<size_t>(1u, tokens.size() - 1u));
                    const float t01   = static_cast<float>(settled_tok) / denom;
                    st.value = spec.value_min + t01 * (spec.value_max - spec.value_min);
                }
            }
        }
    }

    // Trigger initial couplings from any pinned/settled landmarks
    if (gen_config_.enable_cross_channel_coupling != 0u) {
        for (uint32_t s = 0u; s < n_slots; ++s) {
            for (uint32_t c = 0u; c < n_channels; ++c) {
                if (slot_channel_states_[static_cast<size_t>(s) * n_channels + c].is_locked != 0u) {
                    apply_couplings_from_locked_channel(p_environment, s, c);
                }
            }
        }
    }

    einselection_status_ = EinselectionStatus::WAVEFRONT_PROPAGATING;
    return true;
}

bool QuantumDarwinismSolver::pin_slot_channel(
    uint32_t p_slot_id,
    uint32_t p_channel_id,
    float p_value,
    uint32_t p_state_mask
) {
    const uint32_t n_channels = static_cast<uint32_t>(channel_specs_.size());
    if (n_channels == 0u || p_channel_id >= n_channels) return false;
    const size_t idx = static_cast<size_t>(p_slot_id) * n_channels + p_channel_id;
    if (idx >= slot_channel_states_.size()) return false;

    const PropertyChannelSpecPOD &spec = channel_specs_[p_channel_id];
    SlotChannelStatePOD &st = slot_channel_states_[idx];
    st.value                = std::clamp(p_value, spec.value_min, spec.value_max);
    if (spec.is_discrete_mask != 0u) {
        if (p_state_mask != 0u) {
            st.candidate_state_mask = p_state_mask;
        } else {
            const uint32_t bit = static_cast<uint32_t>(std::max(0, static_cast<int>(std::round(p_value)))) %
                                 std::max<uint32_t>(1u, std::min<uint32_t>(32u, spec.state_count));
            st.candidate_state_mask = (1u << bit);
        }
    }
    st.redundancy_r_delta = 1.0f;
    st.is_locked          = 1u;
    st.is_pinned          = 1u;
    st.locked_wave_index  = static_cast<uint8_t>(std::min<uint32_t>(255u, current_wave_index_));

    if (bound_environment_ && gen_config_.enable_cross_channel_coupling != 0u) {
        apply_couplings_from_locked_channel(*bound_environment_, p_slot_id, p_channel_id);
    }
    return true;
}

void QuantumDarwinismSolver::apply_couplings_from_locked_channel(
    const ProceduralEnvironment &p_environment,
    uint32_t p_source_slot_id,
    uint32_t p_source_channel_id
) {
    const uint32_t n_channels = static_cast<uint32_t>(channel_specs_.size());
    const uint32_t n_slots    = static_cast<uint32_t>(p_environment.slots().size());
    if (n_channels == 0u || p_source_slot_id >= n_slots || p_source_channel_id >= n_channels) return;

    const SlotChannelStatePOD &src_state =
        slot_channel_states_[static_cast<size_t>(p_source_slot_id) * n_channels + p_source_channel_id];
    if (src_state.is_locked == 0u) return;

    const PropertyChannelSpecPOD &src_spec = channel_specs_[p_source_channel_id];

    auto apply_to_target = [&](uint32_t p_target_slot_id, const ChannelCouplingRulePOD &p_rule, float p_atten) {
        if (p_target_slot_id >= n_slots || p_rule.target_channel_id >= n_channels) return;
        SlotChannelStatePOD &dst =
            slot_channel_states_[static_cast<size_t>(p_target_slot_id) * n_channels + p_rule.target_channel_id];
        if (dst.is_locked != 0u) return;

        const PropertyChannelSpecPOD &dst_spec = channel_specs_[p_rule.target_channel_id];
        dst.effective_threshold_tau = std::clamp(
            dst.effective_threshold_tau + p_rule.delta_threshold_tau * p_atten,
            0.05f,
            0.99f
        );
        dst.effective_permeability = std::clamp(
            dst.effective_permeability + p_rule.delta_permeability_gamma * p_atten,
            0.05f,
            1.0f
        );
        if (dst_spec.is_discrete_mask != 0u) {
            if (p_rule.target_forbid_state_mask != 0u) {
                const uint32_t narrowed = dst.candidate_state_mask & ~p_rule.target_forbid_state_mask;
                if (narrowed != 0u) {
                    dst.candidate_state_mask = narrowed;
                }
            }
            if (p_rule.target_state_bias_mask != 0u) {
                const uint32_t intersected = dst.candidate_state_mask & p_rule.target_state_bias_mask;
                if (intersected != 0u) {
                    dst.candidate_state_mask = intersected;
                }
            }
        } else if (p_rule.value_bias_weight > 0.0f) {
            const float w = std::clamp(p_rule.value_bias_weight * p_atten, 0.0f, 1.0f);
            dst.value = std::clamp(dst.value * (1.0f - w) + p_rule.target_value_bias * w, dst_spec.value_min, dst_spec.value_max);
        }
    };

    for (const ChannelCouplingRulePOD &rule : coupling_rules_) {
        if (rule.source_channel_id != p_source_channel_id) continue;
        if (rule.target_channel_id >= n_channels) continue;

        // Check trigger condition
        if (src_spec.is_discrete_mask != 0u) {
            if (rule.required_source_state_mask != 0u &&
                (src_state.candidate_state_mask & rule.required_source_state_mask) == 0u) {
                continue;
            }
        } else {
            if (src_state.value < rule.source_value_min || src_state.value > rule.source_value_max) {
                continue;
            }
        }

        // Apply to SELF when scope is SELF or ALL
        if (rule.scope == PatternRuleScope::SELF || rule.scope == PatternRuleScope::ALL) {
            apply_to_target(p_source_slot_id, rule, 1.0f);
        }

        if (rule.scope == PatternRuleScope::SELF || rule.max_coupling_hops == 0u) {
            continue;
        }

        // Determine effective topology mask for the rule's scope
        uint8_t scope_topo = rule.topology_mask;
        if (rule.scope == PatternRuleScope::CHILDREN || rule.scope == PatternRuleScope::DESCENDANTS) {
            scope_topo &= TOPOLOGY_HIERARCHY_DOWN;
        } else if (rule.scope == PatternRuleScope::SIBLINGS) {
            scope_topo &= TOPOLOGY_SIBLINGS;
        }
        if (scope_topo == TOPOLOGY_NONE) {
            scope_topo = rule.topology_mask;
        }

        // Bounded BFS up to rule.max_coupling_hops
        std::vector<uint8_t> visited(n_slots, 0u);
        std::queue<std::pair<uint32_t, uint8_t>> q;
        visited[p_source_slot_id] = 1u;
        q.push({ p_source_slot_id, 0u });

        while (!q.empty()) {
            const auto [u, hop] = q.front();
            q.pop();
            if (hop >= rule.max_coupling_hops) continue;

            for_each_topology_neighbor(p_environment, u, scope_topo, [&](uint32_t v, float /*w*/, uint8_t /*t*/) {
                if (visited[v] != 0u) return;
                visited[v] = 1u;
                const uint8_t next_hop = static_cast<uint8_t>(hop + 1u);
                const float atten = 1.0f / static_cast<float>(next_hop);
                apply_to_target(v, rule, atten);
                if (next_hop < rule.max_coupling_hops) {
                    q.push({ v, next_hop });
                }
            });
        }
    }
}

bool QuantumDarwinismSolver::nucleate_frontier_on_stall(const ProceduralEnvironment &p_environment) {
    if (gen_config_.enable_spontaneous_nucleation == 0u) return false;
    const uint32_t n_channels = static_cast<uint32_t>(channel_specs_.size());
    const uint32_t n_slots    = static_cast<uint32_t>(p_environment.slots().size());
    if (n_channels == 0u || n_slots == 0u) return false;

    int32_t best_slot    = -1;
    int32_t best_channel = -1;
    float   best_score   = -std::numeric_limits<float>::infinity();

    for (uint32_t s = 0u; s < n_slots; ++s) {
        for (uint32_t c = 0u; c < n_channels; ++c) {
            const SlotChannelStatePOD &st = slot_channel_states_[static_cast<size_t>(s) * n_channels + c];
            if (st.is_locked != 0u) continue;

            // Prefer frontier cells with highest redundancy R_delta relative to their threshold tau_k,
            // with deterministic stochastic tie-breaking controlled by nucleation_temperature
            const float jitter = hash_to_signed_unit(gen_config_.master_seed, s, c, current_wave_index_ + 77u) *
                                 gen_config_.nucleation_temperature;
            const float frontier_bonus = (st.redundancy_r_delta >= gen_config_.min_frontier_redundancy) ? 0.5f : 0.0f;
            const float score = (st.redundancy_r_delta - 0.25f * st.effective_threshold_tau) + frontier_bonus + jitter;

            if (score > best_score) {
                best_score   = score;
                best_slot    = static_cast<int32_t>(s);
                best_channel = static_cast<int32_t>(c);
            }
        }
    }

    if (best_slot < 0 || best_channel < 0) return false;

    const uint32_t u_s = static_cast<uint32_t>(best_slot);
    const uint32_t u_c = static_cast<uint32_t>(best_channel);
    const PropertyChannelSpecPOD &spec = channel_specs_[u_c];
    SlotChannelStatePOD &st = slot_channel_states_[static_cast<size_t>(u_s) * n_channels + u_c];

    const float pre_r = st.redundancy_r_delta;
    if (spec.is_discrete_mask != 0u) {
        uint32_t mask = st.candidate_state_mask;
        if (mask == 0u) mask = full_mask_for_state_count(spec.state_count);
        const uint32_t pop = static_cast<uint32_t>(std::popcount(mask));
        const uint32_t pick_ord = splitmix_hash_u32(gen_config_.master_seed, u_s, u_c, current_wave_index_ + 19u) %
                                  std::max<uint32_t>(1u, pop);
        uint32_t chosen_bit = 0u;
        uint32_t seen       = 0u;
        for (uint32_t b = 0u; b < 32u; ++b) {
            if ((mask & (1u << b)) != 0u) {
                if (seen == pick_ord) {
                    chosen_bit = b;
                    break;
                }
                ++seen;
            }
        }
        st.candidate_state_mask = (1u << chosen_bit);
        st.value = static_cast<float>(chosen_bit);
    } else {
        const float jitter = hash_to_signed_unit(gen_config_.master_seed, u_s, u_c, current_wave_index_ + 31u);
        const float span   = spec.value_max - spec.value_min;
        st.value = std::clamp(st.value + jitter * 0.25f * span, spec.value_min, spec.value_max);
    }

    st.redundancy_r_delta = std::max(st.redundancy_r_delta, st.effective_threshold_tau);
    st.is_locked          = 1u;
    st.is_nucleated       = 1u;
    st.locked_wave_index  = static_cast<uint8_t>(std::min<uint32_t>(255u, current_wave_index_));

    WavefrontNucleationEventPOD ev{};
    ev.slot_id                   = u_s;
    ev.channel_id                = u_c;
    ev.wave_index                = current_wave_index_;
    ev.pre_nucleation_redundancy = pre_r;
    nucleation_events_.push_back(ev);

    if (gen_config_.enable_cross_channel_coupling != 0u) {
        apply_couplings_from_locked_channel(p_environment, u_s, u_c);
    }
    return true;
}

bool QuantumDarwinismSolver::step_outward_wavefront(ProceduralEnvironment &r_environment) {
    const uint32_t n_slots    = static_cast<uint32_t>(r_environment.slots().size());
    const uint32_t n_channels = static_cast<uint32_t>(channel_specs_.size());
    if (n_slots == 0u || n_channels == 0u) return false;
    if (slot_channel_states_.size() != static_cast<size_t>(n_slots) * n_channels) {
        if (!initialize_channel_states_from_environment(r_environment)) return false;
    }

    ++current_wave_index_;
    std::vector<SlotChannelStatePOD> next_states = slot_channel_states_;
    std::vector<std::pair<uint32_t, uint32_t>> newly_locked;

    uint32_t unsettled_count  = 0u;
    uint32_t progressed_count = 0u;

    for (uint32_t s = 0u; s < n_slots; ++s) {
        for (uint32_t c = 0u; c < n_channels; ++c) {
            const size_t idx = static_cast<size_t>(s) * n_channels + c;
            const SlotChannelStatePOD &curr = slot_channel_states_[idx];
            if (curr.is_locked != 0u) continue;
            ++unsettled_count;

            const PropertyChannelSpecPOD &spec = channel_specs_[c];
            float    accum_r        = 0.0f;
            float    max_incoming_r = 0.0f;
            float    weighted_val   = 0.0f;
            float    weight_sum     = 0.0f;
            uint32_t witness_votes[32] = {};
            uint32_t total_witnesses   = 0u;

            for_each_topology_neighbor(r_environment, s, spec.topology_mask, [&](uint32_t nb_id, float dist_w, uint8_t /*t*/) {
                const SlotChannelStatePOD &nb = slot_channel_states_[static_cast<size_t>(nb_id) * n_channels + c];
                if (nb.redundancy_r_delta <= 1e-4f) return;

                const float edge_perturb = 1.0f + sample_perturbation(s, c, nb_id + 13u) * 0.5f;
                const float eff_dist     = std::max(0.25f, dist_w * edge_perturb);
                const float atten        = std::max(0.0f, (1.0f - spec.decay_per_hop * eff_dist)) *
                                           curr.effective_permeability * nb.effective_permeability;
                const float contrib      = nb.redundancy_r_delta * atten;
                if (contrib <= 1e-4f) return;

                ++total_witnesses;
                max_incoming_r = std::max(max_incoming_r, contrib);
                accum_r += contrib;
                weighted_val += nb.value * contrib;
                weight_sum   += contrib;

                if (spec.is_discrete_mask != 0u) {
                    const uint32_t nb_mask = nb.candidate_state_mask & curr.candidate_state_mask;
                    const uint32_t use_mask = (nb_mask != 0u) ? nb_mask : nb.candidate_state_mask;
                    for (uint32_t b = 0u; b < 32u; ++b) {
                        if ((use_mask & (1u << b)) != 0u) {
                            witness_votes[b] += static_cast<uint32_t>(std::max(1.0f, contrib * 100.0f));
                        }
                    }
                }
            });

            if (total_witnesses == 0u) continue;

            // Redundancy grows from strongest witness + multi-witness mutual-information reinforcement
            const float multi_witness_boost = (total_witnesses > 1u)
                ? std::min(0.30f, 0.12f * static_cast<float>(total_witnesses - 1u))
                : 0.0f;
            const float new_r = std::clamp(
                std::max(curr.redundancy_r_delta, max_incoming_r + multi_witness_boost * (accum_r / static_cast<float>(total_witnesses))),
                0.0f,
                1.0f
            );

            SlotChannelStatePOD &dst = next_states[idx];
            if (new_r > curr.redundancy_r_delta + 1e-4f) {
                dst.redundancy_r_delta = new_r;
                ++progressed_count;
            }

            if (spec.is_discrete_mask != 0u) {
                uint32_t best_b = 0u;
                uint32_t best_v = 0u;
                for (uint32_t b = 0u; b < 32u; ++b) {
                    if (witness_votes[b] > best_v) {
                        best_v = witness_votes[b];
                        best_b = b;
                    }
                }
                if (best_v > 0u) {
                    dst.value = static_cast<float>(best_b);
                    if (new_r >= dst.effective_threshold_tau) {
                        dst.candidate_state_mask = (1u << best_b);
                    }
                }
            } else if (weight_sum > 1e-5f) {
                dst.value = std::clamp(weighted_val / weight_sum, spec.value_min, spec.value_max);
            }

            if (dst.redundancy_r_delta >= dst.effective_threshold_tau) {
                dst.is_locked         = 1u;
                dst.locked_wave_index = static_cast<uint8_t>(std::min<uint32_t>(255u, current_wave_index_));
                newly_locked.emplace_back(s, c);
            }
        }
    }

    slot_channel_states_.swap(next_states);

    if (gen_config_.enable_cross_channel_coupling != 0u) {
        for (const auto &[s, c] : newly_locked) {
            apply_couplings_from_locked_channel(r_environment, s, c);
        }
    }

    if (unsettled_count == 0u) {
        einselection_status_ = EinselectionStatus::GENERATION_CONVERGED;
        return false;
    }

    // If the wavefront stalled with unsettled channels remaining, spontaneously nucleate one frontier cell!
    if (progressed_count == 0u && newly_locked.empty()) {
        if (nucleate_frontier_on_stall(r_environment)) {
            einselection_status_ = EinselectionStatus::WAVEFRONT_PROPAGATING;
            return true;
        }
        einselection_status_ = EinselectionStatus::GENERATION_CONVERGED;
        return false;
    }

    einselection_status_ = EinselectionStatus::WAVEFRONT_PROPAGATING;
    return true;
}

bool QuantumDarwinismSolver::project_channels_to_environment(ProceduralEnvironment &r_environment) {
    auto &slots        = r_environment.slots();
    const auto &tokens = r_environment.tokens();
    const uint32_t n_slots    = static_cast<uint32_t>(slots.size());
    const uint32_t n_tokens   = static_cast<uint32_t>(std::min<size_t>(256u, tokens.size()));
    const uint32_t n_channels = static_cast<uint32_t>(channel_specs_.size());

    if (n_slots == 0u || n_channels == 0u) return false;

    pointer_states_.clear();

    for (uint32_t s = 0u; s < n_slots; ++s) {
        EnvironmentSlotPOD &slot = slots[s];

        // Compute mean redundancy R_delta across all channels on this slot
        float mean_r = 0.0f;
        uint32_t locked_count = 0u;
        for (uint32_t c = 0u; c < n_channels; ++c) {
            const SlotChannelStatePOD &st = slot_channel_states_[static_cast<size_t>(s) * n_channels + c];
            mean_r += st.redundancy_r_delta;
            if (st.is_locked != 0u) ++locked_count;
        }
        mean_r /= static_cast<float>(n_channels);
        slot.einselection_score = std::clamp(mean_r, 0.0f, 1.0f);
        slot.is_pointer_state   = (slot.einselection_score >= config_.pointer_redundancy_thresh) ? 1u : 0u;

        // Do not overwrite designer-pinned tokens
        if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && slot.domain.popcount() == 1u) {
            slot.collapsed_token_id = slot.domain.first_set_state();
            continue;
        }

        if (n_tokens > 0u) {
            // Score every candidate token currently admissible in `slot.domain` (or all tokens if domain empty)
            BitmaskDomain256POD cand_mask = slot.domain;
            if (cand_mask.empty()) {
                cand_mask.fill_first_n(n_tokens);
            }

            int32_t best_tok   = cand_mask.first_set_state();
            float   best_score = -std::numeric_limits<float>::infinity();

            for (uint32_t t = 0u; t < n_tokens; ++t) {
                if (!cand_mask.test_bit(t)) continue;
                const TokenDescriptorPOD &tok = tokens[t];
                float score = std::max(0.01f, tok.base_frequency_weight) * 0.1f;

                for (uint32_t c = 0u; c < n_channels; ++c) {
                    const PropertyChannelSpecPOD &spec = channel_specs_[c];
                    const SlotChannelStatePOD &st = slot_channel_states_[static_cast<size_t>(s) * n_channels + c];
                    if (st.redundancy_r_delta <= 1e-4f) continue;

                    const float w = st.redundancy_r_delta * (st.is_locked ? 1.5f : 0.75f);
                    if (spec.is_discrete_mask != 0u) {
                        uint32_t tok_bits = 0u;
                        switch (spec.aspect) {
                            case QDChannelAspect::ASPECT_GROUP_OR_TAG:
                                tok_bits = tok.signature.group_or_tag_mask;
                                break;
                            case QDChannelAspect::ASPECT_NUCLEAR_SEME:
                                tok_bits = tok.nuclear_seme_mask_lo32;
                                break;
                            case QDChannelAspect::ASPECT_PERIPHERAL_SEME:
                                tok_bits = tok.peripheral_seme_mask_lo32;
                                break;
                            case QDChannelAspect::ASPECT_METHOD_TRAIT:
                                tok_bits = tok.signature.method_trait_mask;
                                break;
                            case QDChannelAspect::ASPECT_SIGNAL_TRAIT:
                                tok_bits = tok.signature.signal_trait_mask;
                                break;
                            case QDChannelAspect::ASPECT_REFLECTED_PROPERTY:
                                tok_bits = tok.signature.property_trait_mask;
                                break;
                            default:
                                tok_bits = (t < 32u) ? (1u << t) : 0u;
                                break;
                        }
                        if ((tok_bits & st.candidate_state_mask) != 0u) {
                            score += w * static_cast<float>(std::popcount(tok_bits & st.candidate_state_mask));
                        }
                    } else {
                        const float denom = static_cast<float>(std::max<uint32_t>(1u, n_tokens - 1u));
                        const float tok_norm = static_cast<float>(t) / denom;
                        const float ch_span  = std::max(1e-5f, spec.value_max - spec.value_min);
                        const float ch_norm  = (st.value - spec.value_min) / ch_span;
                        score += w * (1.0f - std::abs(tok_norm - ch_norm));
                    }
                }

                // Deterministic tie-breaker
                score += hash_to_signed_unit(gen_config_.master_seed, s, t, 91u) * 1e-3f;
                if (score > best_score) {
                    best_score = score;
                    best_tok   = static_cast<int32_t>(t);
                }
            }

            if (best_tok >= 0) {
                slot.domain.clear();
                slot.domain.set_bit(static_cast<uint32_t>(best_tok));
                slot.collapsed_token_id = best_tok;
                slot.has_conflict       = 0u;
                slot.entropy_bucket     = 0u;
            }
        }

        if (slot.is_pointer_state != 0u && slot.collapsed_token_id >= 0 &&
            static_cast<size_t>(slot.collapsed_token_id) < tokens.size()) {
            PointerStateRecordPOD rec{};
            rec.slot_id                 = s;
            rec.settled_token_id        = static_cast<uint32_t>(slot.collapsed_token_id);
            rec.einselection_redundancy = slot.einselection_score;
            rec.preserved_trait_mask    = tokens[static_cast<size_t>(slot.collapsed_token_id)].coherence_trait_mask |
                                          tokens[static_cast<size_t>(slot.collapsed_token_id)].nuclear_seme_mask_lo32;
            pointer_states_.push_back(rec);
        }
    }

    const bool ok = r_environment.propagate_hierarchy_and_cardinality_constraints();
    return ok;
}

EinselectionStatus QuantumDarwinismSolver::run_generative_einselection(
    ProceduralEnvironment &r_environment,
    WFCSolverCore *p_optional_wfc_solver
) {
    bind_environment(&r_environment);
    if (r_environment.slots().empty()) {
        einselection_status_ = EinselectionStatus::IDLE;
        return einselection_status_;
    }

    if (slot_channel_states_.size() != r_environment.slots().size() * std::max<size_t>(1u, channel_specs_.size())) {
        if (!initialize_channel_states_from_environment(r_environment)) {
            einselection_status_ = EinselectionStatus::UNRESOLVABLE_HORIZON;
            return einselection_status_;
        }
    }

    const uint32_t max_iters = std::max<uint32_t>(1u, gen_config_.max_wavefront_iterations);
    for (uint32_t iter = 0u; iter < max_iters; ++iter) {
        if (!step_outward_wavefront(r_environment)) {
            break;
        }
    }

    const bool projected_clean = project_channels_to_environment(r_environment);

    if (p_optional_wfc_solver) {
        p_optional_wfc_solver->sync_from_bound_environment();
    }

    if (!projected_clean &&
        (gen_config_.auto_stabilize_after_projection != 0u ||
         gen_config_.mode == QuantumDarwinismMode::MODE_HYBRID_GENERATE_AND_STABILIZE)) {
        return stabilize_all_conflicts_in_environment(r_environment, p_optional_wfc_solver);
    }

    einselection_status_ = projected_clean
        ? EinselectionStatus::GENERATION_CONVERGED
        : EinselectionStatus::UNRESOLVABLE_HORIZON;
    return einselection_status_;
}

} // namespace ideam::core

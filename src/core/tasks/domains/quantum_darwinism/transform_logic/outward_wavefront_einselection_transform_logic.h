#pragma once

/**
 * outward_wavefront_einselection_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`QD4`) for `IdeamQuantumDarwinism`.
 *
 * Performs outward multi-channel wavefront einselection directly on DOD SoA buffers:
 *   - Propagates per-channel redundancy R_delta_k outward across parent/child/sibling/graph edges
 *   - Applies cross-channel threshold (tau_m) & permeability (gamma_m) couplings
 *   - Breaks symmetry via deterministic slot/channel hashing + optional external perturbation field
 *   - Performs spontaneous frontier nucleation when a wavefront stalls with unsettled slots remaining
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../quantum_darwinism/quantum_darwinism_common.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) OutwardWavefrontEinselectionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "QD Outward Multi-Channel Wavefront Einselection";
    static constexpr std::string_view ui_category  = "QuantumDarwinism";

    uint32_t target_buffer_id              = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD`
    uint32_t channel_spec_buffer_id        = INVALID_ID; ///< `PROPERTY_CHANNEL_SPEC_POD`
    uint32_t slot_channel_buffer_id        = INVALID_ID; ///< `SLOT_CHANNEL_STATE_POD`
    uint32_t coupling_rule_buffer_id       = INVALID_ID; ///< `CHANNEL_COUPLING_RULE_POD`
    uint32_t child_csr_buffer_id           = INVALID_ID; ///< `CHILD_SLOT_CSR`
    uint32_t spatial_csr_buffer_id         = INVALID_ID; ///< `SPATIAL_NEIGHBOR_CSR`
    uint32_t perturbation_buffer_id        = INVALID_ID; ///< `PERTURBATION_FIELD_FLOATS` (optional)
    uint32_t pointer_conflict_buffer_id    = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK`
    uint64_t master_seed                   = 1337ULL;
    uint32_t max_wavefront_iterations      = 32;
    float    perturbation_strength         = 0.12f;
    float    nucleation_temperature        = 0.15f;
    float    min_frontier_redundancy       = 0.05f;
    uint8_t  enable_spontaneous_nucleation = 1;
    uint8_t  enable_cross_channel_coupling = 1;
    uint8_t  reserved_padding[6]           = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",              godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("channel_spec_buffer_id",        godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("slot_channel_buffer_id",        godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("coupling_rule_buffer_id",       godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("child_csr_buffer_id",           godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("spatial_csr_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("perturbation_buffer_id",        godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_conflict_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("master_seed",                   godot::Variant::INT,   static_cast<int64_t>(1337));
        add_prop("max_wavefront_iterations",      godot::Variant::INT,   static_cast<int64_t>(32));
        add_prop("perturbation_strength",         godot::Variant::FLOAT, 0.12);
        add_prop("nucleation_temperature",        godot::Variant::FLOAT, 0.15);
        add_prop("enable_spontaneous_nucleation", godot::Variant::BOOL,  true);
        add_prop("enable_cross_channel_coupling", godot::Variant::BOOL,  true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))              target_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("channel_spec_buffer_id"))        channel_spec_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["channel_spec_buffer_id"]));
        if (p_props.has("slot_channel_buffer_id"))        slot_channel_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["slot_channel_buffer_id"]));
        if (p_props.has("coupling_rule_buffer_id"))       coupling_rule_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["coupling_rule_buffer_id"]));
        if (p_props.has("child_csr_buffer_id"))           child_csr_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_csr_buffer_id"]));
        if (p_props.has("spatial_csr_buffer_id"))         spatial_csr_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["spatial_csr_buffer_id"]));
        if (p_props.has("perturbation_buffer_id"))        perturbation_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["perturbation_buffer_id"]));
        if (p_props.has("pointer_conflict_buffer_id"))    pointer_conflict_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_conflict_buffer_id"]));
        if (p_props.has("master_seed"))                   master_seed                   = static_cast<uint64_t>(static_cast<int64_t>(p_props["master_seed"]));
        if (p_props.has("max_wavefront_iterations"))      max_wavefront_iterations      = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_wavefront_iterations"]));
        if (p_props.has("perturbation_strength"))         perturbation_strength         = static_cast<float>(static_cast<double>(p_props["perturbation_strength"]));
        if (p_props.has("nucleation_temperature"))        nucleation_temperature        = static_cast<float>(static_cast<double>(p_props["nucleation_temperature"]));
        if (p_props.has("enable_spontaneous_nucleation")) enable_spontaneous_nucleation = static_cast<bool>(p_props["enable_spontaneous_nucleation"]) ? 1u : 0u;
        if (p_props.has("enable_cross_channel_coupling")) enable_cross_channel_coupling = static_cast<bool>(p_props["enable_cross_channel_coupling"]) ? 1u : 0u;
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_Elem>
    [[nodiscard]] static T_Elem *_resolve_ptr(const TaskContextPOD &p_ctx, uint32_t p_buf_id, size_t &r_cap) noexcept {
        r_cap = 0u;
        if (p_buf_id == INVALID_ID) return nullptr;
        if (const GrantPartPOD *part = p_ctx.get_grant_part(p_buf_id)) {
            if (part->raw_base_ptr) {
                const size_t cap = part->capacity_bytes / sizeof(T_Elem);
                r_cap = (part->selection.element_count > 0)
                    ? std::min<size_t>(static_cast<size_t>(part->selection.element_count), cap)
                    : cap;
                return reinterpret_cast<T_Elem *>(part->raw_base_ptr);
            }
        }
        if (p_ctx.manager) {
            if (MemoryBufferPOD *buf = p_ctx.manager->get_buffer(p_buf_id)) {
                if (buf->master_block_ptr) {
                    const size_t cap = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
                    r_cap = (buf->current_count > 0)
                        ? std::min<size_t>(static_cast<size_t>(buf->current_count), cap)
                        : cap;
                    return reinterpret_cast<T_Elem *>(buf->master_block_ptr + buf->memory_offset);
                }
            }
        }
        return nullptr;
    }

    [[nodiscard]] static float _signed_hash(uint64_t p_seed, uint32_t p_a, uint32_t p_b, uint32_t p_c) noexcept {
        uint64_t x = p_seed ^ (static_cast<uint64_t>(p_a) * 0x9E3779B97F4A7C15ULL)
                            ^ (static_cast<uint64_t>(p_b) * 0xBF58476D1CE4E5B9ULL)
                            ^ (static_cast<uint64_t>(p_c) * 0x94D049BB133111EBULL);
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        const uint32_t h = static_cast<uint32_t>((x ^ (x >> 31)) & 0xFFFFFFFFULL);
        return (static_cast<float>(h >> 8) * (2.0f / 16777216.0f)) - 1.0f;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t slot_cap = 0, spec_cap = 0, state_cap = 0, rule_cap = 0, child_cap = 0, spatial_cap = 0, pert_cap = 0, status_cap = 0;
        const auto *slots       = _resolve_ptr<const EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *specs       = _resolve_ptr<const PropertyChannelSpecPOD>(p_ctx, channel_spec_buffer_id, spec_cap);
        auto *states            = _resolve_ptr<SlotChannelStatePOD>(p_ctx, slot_channel_buffer_id, state_cap);
        const auto *couplings   = _resolve_ptr<const ChannelCouplingRulePOD>(p_ctx, coupling_rule_buffer_id, rule_cap);
        const auto *child_csr   = _resolve_ptr<const uint32_t>(p_ctx, child_csr_buffer_id, child_cap);
        const auto *spatial_csr = _resolve_ptr<const SpatialNeighborEdgePOD>(p_ctx, spatial_csr_buffer_id, spatial_cap);
        const auto *pert_field  = _resolve_ptr<const float>(p_ctx, perturbation_buffer_id, pert_cap);
        auto *status_masks      = _resolve_ptr<uint32_t>(p_ctx, pointer_conflict_buffer_id, status_cap);

        if (!slots || !specs || !states || slot_cap == 0u || spec_cap == 0u) return;

        const uint32_t n_slots    = static_cast<uint32_t>(slot_cap);
        const uint32_t n_channels = static_cast<uint32_t>(spec_cap);
        const uint32_t n_rules    = static_cast<uint32_t>(rule_cap);
        const uint32_t n_child    = static_cast<uint32_t>(child_cap);
        const uint32_t n_spatial  = static_cast<uint32_t>(spatial_cap);

        if (state_cap < static_cast<size_t>(n_slots) * n_channels) return;

        auto sample_pert = [&](uint32_t s, uint32_t c, uint32_t salt) noexcept -> float {
            if (perturbation_strength <= 0.0f) return 0.0f;
            float base = _signed_hash(master_seed, s, c, salt);
            if (pert_field && pert_cap > 0u) {
                const float fv = std::clamp(pert_field[s % pert_cap], -1.0f, 1.0f);
                base = 0.65f * fv + 0.35f * base;
            }
            return base * perturbation_strength;
        };

        auto apply_coupling_to = [&](uint32_t dst_slot, const ChannelCouplingRulePOD &rule, float atten) noexcept {
            if (dst_slot >= n_slots || rule.target_channel_id >= n_channels) return;
            SlotChannelStatePOD &dst = states[static_cast<size_t>(dst_slot) * n_channels + rule.target_channel_id];
            if (dst.is_locked != 0u) return;
            const PropertyChannelSpecPOD &dspec = specs[rule.target_channel_id];
            dst.effective_threshold_tau = std::clamp(dst.effective_threshold_tau + rule.delta_threshold_tau * atten, 0.05f, 0.99f);
            dst.effective_permeability  = std::clamp(dst.effective_permeability + rule.delta_permeability_gamma * atten, 0.05f, 1.0f);
            if (dspec.is_discrete_mask != 0u) {
                if (rule.target_forbid_state_mask != 0u) {
                    const uint32_t n = dst.candidate_state_mask & ~rule.target_forbid_state_mask;
                    if (n != 0u) dst.candidate_state_mask = n;
                }
                if (rule.target_state_bias_mask != 0u) {
                    const uint32_t n = dst.candidate_state_mask & rule.target_state_bias_mask;
                    if (n != 0u) dst.candidate_state_mask = n;
                }
            } else if (rule.value_bias_weight > 0.0f) {
                const float w = std::clamp(rule.value_bias_weight * atten, 0.0f, 1.0f);
                dst.value = std::clamp(dst.value * (1.0f - w) + rule.target_value_bias * w, dspec.value_min, dspec.value_max);
            }
        };

        auto fire_couplings = [&](uint32_t src_slot, uint32_t src_ch) noexcept {
            if (enable_cross_channel_coupling == 0u || !couplings || n_rules == 0u) return;
            const SlotChannelStatePOD &src = states[static_cast<size_t>(src_slot) * n_channels + src_ch];
            const PropertyChannelSpecPOD &sspec = specs[src_ch];
            for (uint32_t r = 0u; r < n_rules; ++r) {
                const ChannelCouplingRulePOD &rule = couplings[r];
                if (rule.source_channel_id != src_ch || rule.target_channel_id >= n_channels) continue;
                if (sspec.is_discrete_mask != 0u) {
                    if (rule.required_source_state_mask != 0u &&
                        (src.candidate_state_mask & rule.required_source_state_mask) == 0u) continue;
                } else {
                    if (src.value < rule.source_value_min || src.value > rule.source_value_max) continue;
                }
                if (rule.scope == PatternRuleScope::SELF || rule.scope == PatternRuleScope::ALL) {
                    apply_coupling_to(src_slot, rule, 1.0f);
                }
                if (rule.scope == PatternRuleScope::SELF || rule.max_coupling_hops == 0u) continue;
                const EnvironmentSlotPOD &u_slot = slots[src_slot];
                if ((rule.topology_mask & TOPOLOGY_HIERARCHY_DOWN) != 0u && child_csr &&
                    u_slot.children_count > 0u && (u_slot.children_csr_offset + u_slot.children_count) <= n_child) {
                    for (uint32_t k = 0u; k < u_slot.children_count; ++k) {
                        apply_coupling_to(child_csr[u_slot.children_csr_offset + k], rule, 1.0f);
                    }
                }
                if ((rule.topology_mask & TOPOLOGY_HIERARCHY_UP) != 0u && u_slot.parent_slot_id < n_slots) {
                    apply_coupling_to(u_slot.parent_slot_id, rule, 1.0f);
                }
                if ((rule.topology_mask & TOPOLOGY_SIBLINGS) != 0u && child_csr && u_slot.parent_slot_id < n_slots) {
                    const EnvironmentSlotPOD &p_slot = slots[u_slot.parent_slot_id];
                    if (p_slot.children_count > 0u && (p_slot.children_csr_offset + p_slot.children_count) <= n_child) {
                        for (uint32_t k = 0u; k < p_slot.children_count; ++k) {
                            const uint32_t sib = child_csr[p_slot.children_csr_offset + k];
                            if (sib != src_slot) apply_coupling_to(sib, rule, 1.0f);
                        }
                    }
                }
                if ((rule.topology_mask & TOPOLOGY_GRAPH_OR_SPATIAL) != 0u && spatial_csr &&
                    u_slot.spatial_neighbor_count > 0u &&
                    static_cast<uint32_t>(u_slot.spatial_csr_offset + u_slot.spatial_neighbor_count) <= n_spatial) {
                    for (uint16_t k = 0u; k < u_slot.spatial_neighbor_count; ++k) {
                        apply_coupling_to(spatial_csr[u_slot.spatial_csr_offset + k].neighbor_slot_id, rule, 1.0f);
                    }
                }
            }
        };

        const uint32_t max_iters = std::max<uint32_t>(1u, max_wavefront_iterations);
        for (uint32_t wave = 1u; wave <= max_iters; ++wave) {
            uint32_t unsettled = 0u;
            uint32_t progressed = 0u;

            for (uint32_t s = 0u; s < n_slots; ++s) {
                const EnvironmentSlotPOD &u_slot = slots[s];
                for (uint32_t c = 0u; c < n_channels; ++c) {
                    SlotChannelStatePOD &st = states[static_cast<size_t>(s) * n_channels + c];
                    if (st.is_locked != 0u) continue;
                    ++unsettled;

                    const PropertyChannelSpecPOD &spec = specs[c];
                    float max_in = 0.0f, accum_r = 0.0f, w_val = 0.0f, w_sum = 0.0f;
                    uint32_t witnesses = 0u;
                    uint32_t votes[32] = {};

                    auto visit_nb = [&](uint32_t nb_id, float dist_w) noexcept {
                        if (nb_id >= n_slots || nb_id == s) return;
                        const SlotChannelStatePOD &nb = states[static_cast<size_t>(nb_id) * n_channels + c];
                        if (nb.redundancy_r_delta <= 1e-4f) return;
                        const float eff_d = std::max(0.25f, dist_w * (1.0f + sample_pert(s, c, nb_id + 13u) * 0.5f));
                        const float contrib = nb.redundancy_r_delta * std::max(0.0f, 1.0f - spec.decay_per_hop * eff_d) *
                                              st.effective_permeability * nb.effective_permeability;
                        if (contrib <= 1e-4f) return;
                        ++witnesses;
                        max_in  = std::max(max_in, contrib);
                        accum_r += contrib;
                        w_val   += nb.value * contrib;
                        w_sum   += contrib;
                        if (spec.is_discrete_mask != 0u) {
                            const uint32_t m = (nb.candidate_state_mask & st.candidate_state_mask) != 0u
                                ? (nb.candidate_state_mask & st.candidate_state_mask)
                                : nb.candidate_state_mask;
                            for (uint32_t b = 0u; b < 32u; ++b) {
                                if ((m & (1u << b)) != 0u) votes[b] += static_cast<uint32_t>(std::max(1.0f, contrib * 100.0f));
                            }
                        }
                    };

                    if ((spec.topology_mask & TOPOLOGY_HIERARCHY_DOWN) != 0u && child_csr &&
                        u_slot.children_count > 0u && (u_slot.children_csr_offset + u_slot.children_count) <= n_child) {
                        for (uint32_t k = 0u; k < u_slot.children_count; ++k) visit_nb(child_csr[u_slot.children_csr_offset + k], 1.0f);
                    }
                    if ((spec.topology_mask & TOPOLOGY_HIERARCHY_UP) != 0u && u_slot.parent_slot_id < n_slots) {
                        visit_nb(u_slot.parent_slot_id, 1.0f);
                    }
                    if ((spec.topology_mask & TOPOLOGY_SIBLINGS) != 0u && child_csr && u_slot.parent_slot_id < n_slots) {
                        const EnvironmentSlotPOD &p_slot = slots[u_slot.parent_slot_id];
                        if (p_slot.children_count > 0u && (p_slot.children_csr_offset + p_slot.children_count) <= n_child) {
                            for (uint32_t k = 0u; k < p_slot.children_count; ++k) visit_nb(child_csr[p_slot.children_csr_offset + k], 1.0f);
                        }
                    }
                    if ((spec.topology_mask & TOPOLOGY_GRAPH_OR_SPATIAL) != 0u && spatial_csr &&
                        u_slot.spatial_neighbor_count > 0u &&
                        static_cast<uint32_t>(u_slot.spatial_csr_offset + u_slot.spatial_neighbor_count) <= n_spatial) {
                        for (uint16_t k = 0u; k < u_slot.spatial_neighbor_count; ++k) {
                            const auto &e = spatial_csr[u_slot.spatial_csr_offset + k];
                            visit_nb(e.neighbor_slot_id, std::max(0.1f, e.distance_weight));
                        }
                    }

                    if (witnesses == 0u) continue;
                    const float boost = (witnesses > 1u) ? std::min(0.30f, 0.12f * static_cast<float>(witnesses - 1u)) : 0.0f;
                    const float new_r = std::clamp(std::max(st.redundancy_r_delta, max_in + boost * (accum_r / static_cast<float>(witnesses))), 0.0f, 1.0f);
                    if (new_r > st.redundancy_r_delta + 1e-4f) {
                        st.redundancy_r_delta = new_r;
                        ++progressed;
                    }
                    if (spec.is_discrete_mask != 0u) {
                        uint32_t best_b = 0u, best_v = 0u;
                        for (uint32_t b = 0u; b < 32u; ++b) {
                            if (votes[b] > best_v) { best_v = votes[b]; best_b = b; }
                        }
                        if (best_v > 0u) {
                            st.value = static_cast<float>(best_b);
                            if (st.redundancy_r_delta >= st.effective_threshold_tau) st.candidate_state_mask = (1u << best_b);
                        }
                    } else if (w_sum > 1e-5f) {
                        st.value = std::clamp(w_val / w_sum, spec.value_min, spec.value_max);
                    }
                    if (st.redundancy_r_delta >= st.effective_threshold_tau) {
                        st.is_locked         = 1u;
                        st.locked_wave_index = static_cast<uint8_t>(std::min<uint32_t>(255u, wave));
                        fire_couplings(s, c);
                        ++progressed;
                    }
                }
            }

            if (unsettled == 0u) break;

            if (progressed == 0u) {
                if (enable_spontaneous_nucleation == 0u) break;
                int32_t best_s = -1, best_c = -1;
                float best_sc = -1e30f;
                for (uint32_t s = 0u; s < n_slots; ++s) {
                    for (uint32_t c = 0u; c < n_channels; ++c) {
                        const SlotChannelStatePOD &st = states[static_cast<size_t>(s) * n_channels + c];
                        if (st.is_locked != 0u) continue;
                        const float f_bonus = (st.redundancy_r_delta >= min_frontier_redundancy) ? 0.5f : 0.0f;
                        const float sc = (st.redundancy_r_delta - 0.25f * st.effective_threshold_tau) + f_bonus +
                                         _signed_hash(master_seed, s, c, wave + 77u) * nucleation_temperature;
                        if (sc > best_sc) { best_sc = sc; best_s = static_cast<int32_t>(s); best_c = static_cast<int32_t>(c); }
                    }
                }
                if (best_s < 0) break;
                const uint32_t us = static_cast<uint32_t>(best_s);
                const uint32_t uc = static_cast<uint32_t>(best_c);
                SlotChannelStatePOD &st = states[static_cast<size_t>(us) * n_channels + uc];
                const PropertyChannelSpecPOD &spec = specs[uc];
                if (spec.is_discrete_mask != 0u) {
                    uint32_t mask = st.candidate_state_mask ? st.candidate_state_mask : 1u;
                    const uint32_t bit = static_cast<uint32_t>(std::countr_zero(mask));
                    st.candidate_state_mask = (1u << bit);
                    st.value = static_cast<float>(bit);
                } else {
                    st.value = std::clamp(st.value + _signed_hash(master_seed, us, uc, wave + 31u) * 0.25f * (spec.value_max - spec.value_min),
                                          spec.value_min, spec.value_max);
                }
                st.redundancy_r_delta = std::max(st.redundancy_r_delta, st.effective_threshold_tau);
                st.is_locked          = 1u;
                st.is_nucleated       = 1u;
                st.locked_wave_index  = static_cast<uint8_t>(std::min<uint32_t>(255u, wave));
                fire_couplings(us, uc);
            }
        }

        if (status_masks) {
            for (uint32_t s = 0u; s < n_slots && s < status_cap; ++s) {
                uint32_t locked = 0u;
                for (uint32_t c = 0u; c < n_channels; ++c) {
                    if (states[static_cast<size_t>(s) * n_channels + c].is_locked != 0u) ++locked;
                }
                if (locked == n_channels) {
                    status_masks[s] |= SLOT_STATUS_POINTER_STATE;
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(OutwardWavefrontEinselectionTransformLogic<>) == 64, "OutwardWavefrontEinselectionTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<OutwardWavefrontEinselectionTransformLogic<>>, "OutwardWavefrontEinselectionTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core

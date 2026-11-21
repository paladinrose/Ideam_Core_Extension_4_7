#include "quantum_darwinism_dod.h"
#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

template <typename T_Elem>
bool copy_span_to_buffer(MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::span<const T_Elem> p_src) {
    if (!p_mem || p_buf_id == INVALID_ID) return false;
    MemoryBufferPOD *buf = p_mem->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr) return false;
    const size_t max_elems = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
    const size_t count     = std::min<size_t>(p_src.size(), max_elems);
    if (count > 0) {
        std::memcpy(buf->master_block_ptr + buf->memory_offset, p_src.data(), count * sizeof(T_Elem));
    }
    buf->current_count = static_cast<int64_t>(count);
    return true;
}

template <typename T_Elem>
void read_buffer_to_vector(const MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::vector<T_Elem> &r_out) {
    r_out.clear();
    if (!p_mem || p_buf_id == INVALID_ID) return;
    const MemoryBufferPOD *buf = const_cast<MemoryManagerDOD *>(p_mem)->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr || buf->current_count <= 0) return;
    const size_t count = static_cast<size_t>(std::min<int64_t>(buf->current_count, buf->max_elements));
    r_out.resize(count);
    std::memcpy(r_out.data(), buf->master_block_ptr + buf->memory_offset, count * sizeof(T_Elem));
}

} // namespace

bool QuantumDarwinismDOD::initialize_quantum_darwinism_memory(MemoryManagerDOD *p_mem) {
    if (!p_mem) return false;
    memory_manager_ = p_mem;
    manifest_.reset();

    for (size_t i = 0; i < QD_BUFFER_SPECS.size(); ++i) {
        const QuantumDarwinismBufferSpec &spec = QD_BUFFER_SPECS[i];
        const int64_t cap = capacities_.resolve(spec.capacity_kind);
        const size_t safe_cap = static_cast<size_t>(std::max<int64_t>(1, cap));
        const size_t total_bytes = safe_cap * static_cast<size_t>(spec.element_bytes);
        const uint32_t buf_id = p_mem->create_buffer(BufferLayoutType::FLAT, total_bytes, 64u);
        if (MemoryBufferPOD *buf = p_mem->get_buffer(buf_id)) {
            buf->max_elements   = static_cast<int64_t>(safe_cap);
            buf->current_count  = 0;
            buf->element_stride = static_cast<uint32_t>(spec.element_bytes);
        }
        manifest_.set(spec.slot, buf_id);
    }
    return manifest_.has(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD);
}

bool QuantumDarwinismDOD::stage_environment_to_dod(const ProceduralEnvironment &p_env) {
    if (!memory_manager_) return false;
    if (!manifest_.has(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD)) {
        if (!initialize_quantum_darwinism_memory(memory_manager_)) return false;
    }

    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::TOKEN_DESCRIPTOR_POD),
        std::span<const TokenDescriptorPOD>(p_env.tokens())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD),
        std::span<const EnvironmentSlotPOD>(p_env.slots())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::CASCADE_RULE_POD),
        std::span<const GenerationRulePOD>(p_env.active_rules())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::CHILD_SLOT_CSR),
        std::span<const uint32_t>(p_env.child_slot_csr())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::RULE_TOKEN_DOMAIN_MASKS),
        std::span<const BitmaskDomain256POD>(p_env.rule_token_masks())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::SPATIAL_NEIGHBOR_CSR),
        std::span<const SpatialNeighborEdgePOD>(p_env.spatial_neighbor_csr())
    );

    const int64_t slot_cnt = static_cast<int64_t>(p_env.slots().size());
    for (QuantumDarwinismBufferSlot slot : {
            QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK,
            QuantumDarwinismBufferSlot::SLOT_ENTROPY_FLOATS,
            QuantumDarwinismBufferSlot::HORIZON_PARTITION_POD,
            QuantumDarwinismBufferSlot::POINTER_STATE_RECORD_POD,
            QuantumDarwinismBufferSlot::COHERENCE_BRIDGE_SLOT_POD,
            QuantumDarwinismBufferSlot::WAVEFRONT_FRONTIER_QUEUE }) {
        if (MemoryBufferPOD *buf = memory_manager_->get_buffer(get_buffer_id(slot))) {
            buf->current_count = std::min<int64_t>(slot_cnt, buf->max_elements);
        }
    }
    return true;
}

bool QuantumDarwinismDOD::stage_channels_and_couplings_to_dod() {
    if (!memory_manager_) return false;
    if (!manifest_.has(QuantumDarwinismBufferSlot::PROPERTY_CHANNEL_SPEC_POD)) {
        if (!initialize_quantum_darwinism_memory(memory_manager_)) return false;
    }

    if (solver_.slot_channel_states().size() != environment_.slots().size() * std::max<size_t>(1u, solver_.channel_specs().size())) {
        solver_.initialize_channel_states_from_environment(environment_);
    }

    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::PROPERTY_CHANNEL_SPEC_POD),
        std::span<const PropertyChannelSpecPOD>(solver_.channel_specs())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::SLOT_CHANNEL_STATE_POD),
        std::span<const SlotChannelStatePOD>(solver_.slot_channel_states())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::CHANNEL_COUPLING_RULE_POD),
        std::span<const ChannelCouplingRulePOD>(solver_.coupling_rules())
    );
    return true;
}

bool QuantumDarwinismDOD::stage_perturbation_field_to_dod(std::span<const float> p_field) {
    if (!memory_manager_) return false;
    if (!manifest_.has(QuantumDarwinismBufferSlot::PERTURBATION_FIELD_FLOATS)) {
        if (!initialize_quantum_darwinism_memory(memory_manager_)) return false;
    }
    solver_.set_external_perturbation_field(p_field);
    return copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(QuantumDarwinismBufferSlot::PERTURBATION_FIELD_FLOATS),
        p_field
    );
}

bool QuantumDarwinismDOD::execute_full_pipeline_direct(uint32_t p_stage_mask) {
    if (!memory_manager_) return false;

    const HorizonBoxConfigPOD &cfg            = solver_.horizon_config();
    const GenerativeWavefrontConfigPOD &g_cfg = solver_.generative_config();
    TaskContextPOD ctx{};
    ctx.manager = memory_manager_;
    SingleElementView<uint32_t, FlatStrategy> dummy_view{};

    // Wave 4 (runs first in Generative/Hybrid pipelines): QD4 OutwardWavefrontEinselectionTransformLogic
    if ((p_stage_mask & QD_STAGE_WAVE_4_OUTWARD_WAVEFRONT_EINSELECTION) != 0u) {
        OutwardWavefrontEinselectionTransformLogic<> qd4{};
        qd4.target_buffer_id              = get_buffer_id(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD);
        qd4.channel_spec_buffer_id        = get_buffer_id(QuantumDarwinismBufferSlot::PROPERTY_CHANNEL_SPEC_POD);
        qd4.slot_channel_buffer_id        = get_buffer_id(QuantumDarwinismBufferSlot::SLOT_CHANNEL_STATE_POD);
        qd4.coupling_rule_buffer_id       = get_buffer_id(QuantumDarwinismBufferSlot::CHANNEL_COUPLING_RULE_POD);
        qd4.child_csr_buffer_id           = get_buffer_id(QuantumDarwinismBufferSlot::CHILD_SLOT_CSR);
        qd4.spatial_csr_buffer_id         = get_buffer_id(QuantumDarwinismBufferSlot::SPATIAL_NEIGHBOR_CSR);
        qd4.perturbation_buffer_id        = get_buffer_id(QuantumDarwinismBufferSlot::PERTURBATION_FIELD_FLOATS);
        qd4.pointer_conflict_buffer_id    = get_buffer_id(QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK);
        qd4.master_seed                   = g_cfg.master_seed;
        qd4.max_wavefront_iterations      = g_cfg.max_wavefront_iterations;
        qd4.perturbation_strength         = g_cfg.perturbation_strength;
        qd4.nucleation_temperature        = g_cfg.nucleation_temperature;
        qd4.min_frontier_redundancy       = g_cfg.min_frontier_redundancy;
        qd4.enable_spontaneous_nucleation = g_cfg.enable_spontaneous_nucleation;
        qd4.enable_cross_channel_coupling = g_cfg.enable_cross_channel_coupling;
        qd4.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 5: QD5 ChannelToTokenProjectionTransformLogic
    if ((p_stage_mask & QD_STAGE_WAVE_5_CHANNEL_TO_TOKEN_PROJECTION) != 0u) {
        ChannelToTokenProjectionTransformLogic<> qd5{};
        qd5.target_buffer_id           = get_buffer_id(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD);
        qd5.token_buffer_id            = get_buffer_id(QuantumDarwinismBufferSlot::TOKEN_DESCRIPTOR_POD);
        qd5.channel_spec_buffer_id     = get_buffer_id(QuantumDarwinismBufferSlot::PROPERTY_CHANNEL_SPEC_POD);
        qd5.slot_channel_buffer_id     = get_buffer_id(QuantumDarwinismBufferSlot::SLOT_CHANNEL_STATE_POD);
        qd5.pointer_conflict_buffer_id = get_buffer_id(QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK);
        qd5.pointer_redundancy_thresh  = cfg.pointer_redundancy_thresh;
        qd5.collapse_to_best_token     = 1u;
        qd5.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 1: QD1 ConflictHorizonBoxPartitionMetadataLogic
    if ((p_stage_mask & QD_STAGE_WAVE_1_CONFLICT_HORIZON_BOX_PARTITION) != 0u) {
        ConflictHorizonBoxPartitionMetadataLogic<> qd1{};
        qd1.target_buffer_id           = get_buffer_id(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD);
        qd1.metadata_buffer_id         = get_buffer_id(QuantumDarwinismBufferSlot::HORIZON_PARTITION_POD);
        qd1.child_csr_buffer_id        = get_buffer_id(QuantumDarwinismBufferSlot::CHILD_SLOT_CSR);
        qd1.spatial_csr_buffer_id      = get_buffer_id(QuantumDarwinismBufferSlot::SPATIAL_NEIGHBOR_CSR);
        qd1.pointer_conflict_buffer_id = get_buffer_id(QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK);
        qd1.frontier_queue_buffer_id   = get_buffer_id(QuantumDarwinismBufferSlot::WAVEFRONT_FRONTIER_QUEUE);
        qd1.max_horizon_radius         = cfg.max_graph_hop_radius;
        qd1.max_horizon_slots          = cfg.max_horizon_slots;
        qd1.min_locked_layer           = cfg.min_locked_layer;
        qd1.seed_from_pinned_landmarks = (g_cfg.mode == QuantumDarwinismMode::MODE_OUTWARD_GENERATIVE) ? 1u : 0u;
        qd1.execute_metadata<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 2: QD2 PointerStateEinselectionQueryLogic
    if ((p_stage_mask & QD_STAGE_WAVE_2_POINTER_STATE_EINSELECTION) != 0u) {
        PointerStateEinselectionQueryLogic<> qd2{};
        qd2.target_buffer_id           = get_buffer_id(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD);
        qd2.token_buffer_id            = get_buffer_id(QuantumDarwinismBufferSlot::TOKEN_DESCRIPTOR_POD);
        qd2.rule_buffer_id             = get_buffer_id(QuantumDarwinismBufferSlot::CASCADE_RULE_POD);
        qd2.child_csr_buffer_id        = get_buffer_id(QuantumDarwinismBufferSlot::CHILD_SLOT_CSR);
        qd2.horizon_partition_buf_id   = get_buffer_id(QuantumDarwinismBufferSlot::HORIZON_PARTITION_POD);
        qd2.pointer_record_buffer_id   = get_buffer_id(QuantumDarwinismBufferSlot::POINTER_STATE_RECORD_POD);
        qd2.pointer_conflict_buffer_id = get_buffer_id(QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK);
        qd2.threshold                  = cfg.pointer_redundancy_thresh;
        qd2.promote_perimeter_anchors  = cfg.promote_perimeter_anchors;
        qd2.execute_query<QueryOp::CULL, SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 3: QD3 CoherenceCascadeRelaxAndPropagateTransformLogic
    if ((p_stage_mask & QD_STAGE_WAVE_3_COHERENCE_CASCADE_RELAX_AND_PROPAGATE) != 0u) {
        CoherenceCascadeRelaxAndPropagateTransformLogic<> qd3{};
        qd3.target_buffer_id          = get_buffer_id(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD);
        qd3.token_buffer_id           = get_buffer_id(QuantumDarwinismBufferSlot::TOKEN_DESCRIPTOR_POD);
        qd3.rule_buffer_id            = get_buffer_id(QuantumDarwinismBufferSlot::CASCADE_RULE_POD);
        qd3.rule_mask_buffer_id       = get_buffer_id(QuantumDarwinismBufferSlot::RULE_TOKEN_DOMAIN_MASKS);
        qd3.child_csr_buffer_id       = get_buffer_id(QuantumDarwinismBufferSlot::CHILD_SLOT_CSR);
        qd3.horizon_partition_buf_id  = get_buffer_id(QuantumDarwinismBufferSlot::HORIZON_PARTITION_POD);
        qd3.pointer_record_buffer_id  = get_buffer_id(QuantumDarwinismBufferSlot::POINTER_STATE_RECORD_POD);
        qd3.bridge_slot_buffer_id     = get_buffer_id(QuantumDarwinismBufferSlot::COHERENCE_BRIDGE_SLOT_POD);
        qd3.pointer_buffer_id         = get_buffer_id(QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK);
        qd3.pointer_redundancy_thresh = cfg.pointer_redundancy_thresh;
        qd3.allow_sibling_rebalancing = cfg.allow_sibling_rebalancing;
        qd3.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    sync_dod_slots_back_to_environment();
    return true;
}

bool QuantumDarwinismDOD::execute_generative_pipeline_direct() {
    if (!stage_channels_and_couplings_to_dod()) return false;
    return execute_full_pipeline_direct(QD_STAGE_GENERATIVE_PIPELINE_MASK);
}

void QuantumDarwinismDOD::read_environment_slots(std::vector<EnvironmentSlotPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD), r_out);
}

void QuantumDarwinismDOD::read_horizon_partitions(std::vector<HorizonSlotPartitionPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(QuantumDarwinismBufferSlot::HORIZON_PARTITION_POD), r_out);
}

void QuantumDarwinismDOD::read_pointer_state_records(std::vector<PointerStateRecordPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(QuantumDarwinismBufferSlot::POINTER_STATE_RECORD_POD), r_out);
}

void QuantumDarwinismDOD::read_coherence_bridge_slots(std::vector<CoherenceBridgeSlotPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(QuantumDarwinismBufferSlot::COHERENCE_BRIDGE_SLOT_POD), r_out);
}

void QuantumDarwinismDOD::read_slot_channel_states(std::vector<SlotChannelStatePOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(QuantumDarwinismBufferSlot::SLOT_CHANNEL_STATE_POD), r_out);
}

void QuantumDarwinismDOD::read_pointer_or_conflict_masks(std::vector<uint32_t> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK), r_out);
}

void QuantumDarwinismDOD::sync_dod_slots_back_to_environment() {
    std::vector<EnvironmentSlotPOD> updated;
    read_environment_slots(updated);
    if (!updated.empty()) {
        environment_.slots() = std::move(updated);
    }
    std::vector<SlotChannelStatePOD> updated_ch;
    read_slot_channel_states(updated_ch);
    if (!updated_ch.empty()) {
        solver_.slot_channel_states_mut() = std::move(updated_ch);
    }
}

} // namespace ideam::core

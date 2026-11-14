#include "wfc_dod.h"
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

bool WaveFunctionCollapseDOD::initialize_wfc_memory(MemoryManagerDOD *p_mem) {
    if (!p_mem) return false;
    memory_manager_ = p_mem;
    manifest_.reset();

    for (size_t i = 0; i < WFC_BUFFER_SPECS.size(); ++i) {
        const WaveFunctionCollapseBufferSpec &spec = WFC_BUFFER_SPECS[i];
        const int64_t cap = capacities_.resolve(spec.capacity_kind);
        const uint32_t buf_id = p_mem->create_buffer(
            godot::String(spec.buffer_name.data()),
            cap,
            spec.data_type,
            BufferLayoutType::FLAT,
            BufferAlignmentMode::TIGHT,
            static_cast<int64_t>(spec.element_bytes)
        );
        manifest_.set(spec.slot, buf_id);
    }
    return manifest_.has(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD);
}

bool WaveFunctionCollapseDOD::stage_environment_and_rules_to_dod(
    const ProceduralEnvironment &p_env,
    std::span<const WFCRuleEntryPOD> p_wfc_rules
) {
    if (!memory_manager_) return false;
    if (!manifest_.has(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD)) {
        if (!initialize_wfc_memory(memory_manager_)) return false;
    }

    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(WaveFunctionCollapseBufferSlot::TOKEN_DESCRIPTOR_POD),
        std::span<const TokenDescriptorPOD>(p_env.tokens())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD),
        std::span<const EnvironmentSlotPOD>(p_env.slots())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(WaveFunctionCollapseBufferSlot::CASCADE_RULE_POD),
        std::span<const GenerationRulePOD>(p_env.active_rules())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(WaveFunctionCollapseBufferSlot::CHILD_SLOT_CSR),
        std::span<const uint32_t>(p_env.child_slot_csr())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(WaveFunctionCollapseBufferSlot::RULE_TOKEN_DOMAIN_MASKS),
        std::span<const BitmaskDomain256POD>(p_env.rule_token_masks())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(WaveFunctionCollapseBufferSlot::SPATIAL_NEIGHBOR_CSR),
        std::span<const SpatialNeighborEdgePOD>(p_env.spatial_neighbor_csr())
    );
    copy_span_to_buffer(
        memory_manager_,
        get_buffer_id(WaveFunctionCollapseBufferSlot::WFC_RULE_TENSOR_POD),
        p_wfc_rules
    );

    const int64_t slot_cnt = static_cast<int64_t>(p_env.slots().size());
    if (MemoryBufferPOD *ptr_buf = memory_manager_->get_buffer(get_buffer_id(WaveFunctionCollapseBufferSlot::POINTER_OR_CONFLICT_MASK))) {
        ptr_buf->current_count = slot_cnt;
    }
    if (MemoryBufferPOD *ent_buf = memory_manager_->get_buffer(get_buffer_id(WaveFunctionCollapseBufferSlot::SLOT_ENTROPY_FLOATS))) {
        ent_buf->current_count = slot_cnt;
    }
    if (MemoryBufferPOD *rep_buf = memory_manager_->get_buffer(get_buffer_id(WaveFunctionCollapseBufferSlot::WFC_CONTRADICTION_REPORT_POD))) {
        rep_buf->current_count = slot_cnt;
    }
    return true;
}

bool WaveFunctionCollapseDOD::execute_full_pipeline_direct(
    uint32_t p_stage_mask,
    SelectionGrantPOD *r_conflict_selection
) {
    (void)r_conflict_selection;
    if (!memory_manager_) return false;

    TaskContextPOD ctx{};
    ctx.manager = memory_manager_;
    SingleElementView<uint32_t, FlatStrategy> dummy_view{};

    // Wave 1: WFC1 WFCEntropyPartitionMetadataLogic
    if ((p_stage_mask & WFC_STAGE_WAVE_1_ENTROPY_PARTITION) != 0u) {
        WFCEntropyPartitionMetadataLogic<> wfc1{};
        wfc1.target_buffer_id           = get_buffer_id(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD);
        wfc1.metadata_buffer_id         = get_buffer_id(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD);
        wfc1.token_buffer_id            = get_buffer_id(WaveFunctionCollapseBufferSlot::TOKEN_DESCRIPTOR_POD);
        wfc1.slot_entropy_buffer_id     = get_buffer_id(WaveFunctionCollapseBufferSlot::SLOT_ENTROPY_FLOATS);
        wfc1.pointer_conflict_buffer_id = get_buffer_id(WaveFunctionCollapseBufferSlot::POINTER_OR_CONFLICT_MASK);
        wfc1.max_horizon_radius         = horizon_radius_;
        wfc1.execute_metadata<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 2: WFC2 WFCForwardCollapseAndPropagateTransformLogic
    if ((p_stage_mask & WFC_STAGE_WAVE_2_FORWARD_COLLAPSE_AND_PROPAGATE) != 0u) {
        WFCForwardCollapseAndPropagateTransformLogic<> wfc2{};
        wfc2.target_buffer_id          = get_buffer_id(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD);
        wfc2.token_buffer_id           = get_buffer_id(WaveFunctionCollapseBufferSlot::TOKEN_DESCRIPTOR_POD);
        wfc2.rule_buffer_id            = get_buffer_id(WaveFunctionCollapseBufferSlot::CASCADE_RULE_POD);
        wfc2.rule_mask_buffer_id       = get_buffer_id(WaveFunctionCollapseBufferSlot::RULE_TOKEN_DOMAIN_MASKS);
        wfc2.child_csr_buffer_id       = get_buffer_id(WaveFunctionCollapseBufferSlot::CHILD_SLOT_CSR);
        wfc2.spatial_csr_buffer_id     = get_buffer_id(WaveFunctionCollapseBufferSlot::SPATIAL_NEIGHBOR_CSR);
        wfc2.wfc_rule_tensor_buffer_id = get_buffer_id(WaveFunctionCollapseBufferSlot::WFC_RULE_TENSOR_POD);
        wfc2.pointer_buffer_id         = get_buffer_id(WaveFunctionCollapseBufferSlot::POINTER_OR_CONFLICT_MASK);
        wfc2.slot_entropy_buffer_id    = get_buffer_id(WaveFunctionCollapseBufferSlot::SLOT_ENTROPY_FLOATS);
        wfc2.max_collapse_steps        = max_collapse_steps_;
        wfc2.master_seed_lo            = master_seed_lo_;
        wfc2.pointer_redundancy_thresh = pointer_redundancy_threshold_;
        wfc2.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 3: WFC3 WFCContradictionDetectQueryLogic
    if ((p_stage_mask & WFC_STAGE_WAVE_3_CONTRADICTION_DETECT) != 0u) {
        WFCContradictionDetectQueryLogic<> wfc3{};
        wfc3.target_buffer_id               = get_buffer_id(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD);
        wfc3.token_buffer_id                = get_buffer_id(WaveFunctionCollapseBufferSlot::TOKEN_DESCRIPTOR_POD);
        wfc3.rule_buffer_id                 = get_buffer_id(WaveFunctionCollapseBufferSlot::CASCADE_RULE_POD);
        wfc3.rule_mask_buffer_id            = get_buffer_id(WaveFunctionCollapseBufferSlot::RULE_TOKEN_DOMAIN_MASKS);
        wfc3.child_csr_buffer_id            = get_buffer_id(WaveFunctionCollapseBufferSlot::CHILD_SLOT_CSR);
        wfc3.pointer_conflict_buffer_id     = get_buffer_id(WaveFunctionCollapseBufferSlot::POINTER_OR_CONFLICT_MASK);
        wfc3.contradiction_report_buffer_id = get_buffer_id(WaveFunctionCollapseBufferSlot::WFC_CONTRADICTION_REPORT_POD);
        wfc3.recommended_horizon_radius     = horizon_radius_;
        wfc3.threshold                      = pointer_redundancy_threshold_;
        wfc3.execute_query<QueryOp::CULL, SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    sync_dod_slots_back_to_environment();
    return true;
}

void WaveFunctionCollapseDOD::read_environment_slots(std::vector<EnvironmentSlotPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD), r_out);
}

void WaveFunctionCollapseDOD::read_pointer_or_conflict_masks(std::vector<uint32_t> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(WaveFunctionCollapseBufferSlot::POINTER_OR_CONFLICT_MASK), r_out);
}

void WaveFunctionCollapseDOD::read_contradiction_reports(std::vector<WFCContradictionReportPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(WaveFunctionCollapseBufferSlot::WFC_CONTRADICTION_REPORT_POD), r_out);
}

void WaveFunctionCollapseDOD::sync_dod_slots_back_to_environment() {
    std::vector<EnvironmentSlotPOD> updated;
    read_environment_slots(updated);
    if (!updated.empty()) {
        environment_.slots() = std::move(updated);
    }
}

} // namespace ideam::core

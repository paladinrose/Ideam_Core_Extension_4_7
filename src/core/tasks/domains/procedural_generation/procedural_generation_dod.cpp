#include "procedural_generation_dod.h"
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

bool ProceduralGenerationDOD::initialize_procedural_generation_memory(MemoryManagerDOD *p_mem) {
    if (!p_mem) return false;
    memory_manager_ = p_mem;
    manifest_.reset();

    for (size_t i = 0; i < PROCGEN_BUFFER_SPECS.size(); ++i) {
        const ProceduralGenerationBufferSpec &spec = PROCGEN_BUFFER_SPECS[i];
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
    return manifest_.has(ProceduralGenerationBufferSlot::ENVIRONMENT_SLOT_POD);
}

bool ProceduralGenerationDOD::stage_environment_to_dod(const ProceduralEnvironment &p_env) {
    if (!memory_manager_) return false;
    if (!manifest_.has(ProceduralGenerationBufferSlot::ENVIRONMENT_SLOT_POD)) {
        if (!initialize_procedural_generation_memory(memory_manager_)) return false;
    }

    copy_span_to_buffer(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::TOKEN_DESCRIPTOR_POD), std::span<const TokenDescriptorPOD>(p_env.tokens()));
    copy_span_to_buffer(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::ENVIRONMENT_SLOT_POD), std::span<const EnvironmentSlotPOD>(p_env.slots()));
    copy_span_to_buffer(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::CASCADE_RULE_POD), std::span<const GenerationRulePOD>(p_env.active_rules()));
    copy_span_to_buffer(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::CHILD_SLOT_CSR), std::span<const uint32_t>(p_env.child_slot_csr()));
    copy_span_to_buffer(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::RULE_TOKEN_DOMAIN_MASKS), std::span<const BitmaskDomain256POD>(p_env.rule_token_masks()));
    copy_span_to_buffer(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::SPATIAL_NEIGHBOR_CSR), std::span<const SpatialNeighborEdgePOD>(p_env.spatial_neighbor_csr()));

    const int64_t slot_cnt = static_cast<int64_t>(p_env.slots().size());
    if (MemoryBufferPOD *ptr_buf = memory_manager_->get_buffer(get_buffer_id(ProceduralGenerationBufferSlot::POINTER_OR_CONFLICT_MASK))) {
        ptr_buf->current_count = slot_cnt;
    }
    if (MemoryBufferPOD *ent_buf = memory_manager_->get_buffer(get_buffer_id(ProceduralGenerationBufferSlot::SLOT_ENTROPY_FLOATS))) {
        ent_buf->current_count = slot_cnt;
    }
    if (MemoryBufferPOD *coh_buf = memory_manager_->get_buffer(get_buffer_id(ProceduralGenerationBufferSlot::TOKEN_COHERENCE_MASKS))) {
        coh_buf->current_count = static_cast<int64_t>(p_env.tokens().size());
    }
    return true;
}

bool ProceduralGenerationDOD::execute_full_pipeline_direct(uint32_t p_stage_mask) {
    if (!memory_manager_) return false;

    TaskContextPOD ctx{};
    ctx.manager = memory_manager_;
    SingleElementView<uint32_t, FlatStrategy> dummy_view{};

    // Wave 1: PCG1 CascadeRuleCompileMetadataLogic
    if ((p_stage_mask & PROCGEN_STAGE_WAVE_1_CASCADE_RULE_COMPILE) != 0u) {
        CascadeRuleCompileMetadataLogic<> pcg1{};
        pcg1.target_buffer_id           = get_buffer_id(ProceduralGenerationBufferSlot::CASCADE_RULE_POD);
        pcg1.metadata_buffer_id         = get_buffer_id(ProceduralGenerationBufferSlot::CASCADE_RULE_POD);
        pcg1.token_descriptor_buffer_id = get_buffer_id(ProceduralGenerationBufferSlot::TOKEN_DESCRIPTOR_POD);
        pcg1.token_coherence_buffer_id  = get_buffer_id(ProceduralGenerationBufferSlot::TOKEN_COHERENCE_MASKS);
        pcg1.execute_metadata<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 2: PCG2 ReflectionSelectorMatchQueryLogic
    if ((p_stage_mask & PROCGEN_STAGE_WAVE_2_REFLECTION_SELECTOR_MATCH) != 0u) {
        ReflectionSelectorMatchQueryLogic<> pcg2{};
        pcg2.target_buffer_id            = get_buffer_id(ProceduralGenerationBufferSlot::TOKEN_DESCRIPTOR_POD);
        pcg2.rule_buffer_id              = get_buffer_id(ProceduralGenerationBufferSlot::CASCADE_RULE_POD);
        pcg2.rule_domain_masks_buffer_id = get_buffer_id(ProceduralGenerationBufferSlot::RULE_TOKEN_DOMAIN_MASKS);
        pcg2.execute_query<QueryOp::CULL, SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 3: PCG3 PatternDomainInitializeTransformLogic
    if ((p_stage_mask & PROCGEN_STAGE_WAVE_3_PATTERN_DOMAIN_INITIALIZE) != 0u) {
        PatternDomainInitializeTransformLogic<> pcg3{};
        pcg3.target_buffer_id            = get_buffer_id(ProceduralGenerationBufferSlot::ENVIRONMENT_SLOT_POD);
        pcg3.rule_buffer_id              = get_buffer_id(ProceduralGenerationBufferSlot::CASCADE_RULE_POD);
        pcg3.pointer_buffer_id           = get_buffer_id(ProceduralGenerationBufferSlot::POINTER_OR_CONFLICT_MASK);
        pcg3.token_descriptor_buffer_id  = get_buffer_id(ProceduralGenerationBufferSlot::TOKEN_DESCRIPTOR_POD);
        pcg3.rule_domain_masks_buffer_id = get_buffer_id(ProceduralGenerationBufferSlot::RULE_TOKEN_DOMAIN_MASKS);
        pcg3.slot_entropy_buffer_id      = get_buffer_id(ProceduralGenerationBufferSlot::SLOT_ENTROPY_FLOATS);
        pcg3.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 4: PCG4 HierarchyCardinalityPropagateTransformLogic
    if ((p_stage_mask & PROCGEN_STAGE_WAVE_4_HIERARCHY_CARDINALITY_PROPAGATE) != 0u) {
        HierarchyCardinalityPropagateTransformLogic<> pcg4{};
        pcg4.target_buffer_id             = get_buffer_id(ProceduralGenerationBufferSlot::ENVIRONMENT_SLOT_POD);
        pcg4.token_descriptor_buffer_id   = get_buffer_id(ProceduralGenerationBufferSlot::TOKEN_DESCRIPTOR_POD);
        pcg4.rule_buffer_id               = get_buffer_id(ProceduralGenerationBufferSlot::CASCADE_RULE_POD);
        pcg4.rule_domain_masks_buffer_id  = get_buffer_id(ProceduralGenerationBufferSlot::RULE_TOKEN_DOMAIN_MASKS);
        pcg4.child_slot_csr_buffer_id     = get_buffer_id(ProceduralGenerationBufferSlot::CHILD_SLOT_CSR);
        pcg4.pointer_or_conflict_buf_id   = get_buffer_id(ProceduralGenerationBufferSlot::POINTER_OR_CONFLICT_MASK);
        pcg4.slot_entropy_buffer_id       = get_buffer_id(ProceduralGenerationBufferSlot::SLOT_ENTROPY_FLOATS);
        pcg4.pointer_redundancy_threshold = pointer_redundancy_threshold_;
        pcg4.max_propagation_sweeps       = max_propagation_sweeps_;
        pcg4.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    sync_dod_slots_back_to_environment();
    return true;
}

void ProceduralGenerationDOD::read_environment_slots(std::vector<EnvironmentSlotPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::ENVIRONMENT_SLOT_POD), r_out);
}

void ProceduralGenerationDOD::read_pointer_or_conflict_masks(std::vector<uint32_t> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::POINTER_OR_CONFLICT_MASK), r_out);
}

void ProceduralGenerationDOD::read_slot_entropies(std::vector<float> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(ProceduralGenerationBufferSlot::SLOT_ENTROPY_FLOATS), r_out);
}

void ProceduralGenerationDOD::sync_dod_slots_back_to_environment() {
    std::vector<EnvironmentSlotPOD> updated;
    read_environment_slots(updated);
    if (!updated.empty()) {
        environment_.slots() = std::move(updated);
    }
}

} // namespace ideam::core

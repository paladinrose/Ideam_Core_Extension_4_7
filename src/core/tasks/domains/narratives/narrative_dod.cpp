#include "narrative_dod.h"

#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

constexpr size_t DEFAULT_NARRATIVES_MASTER_BLOCK_BYTES    = 16ULL * 1024ULL * 1024ULL; // 16 MB
constexpr size_t DEFAULT_NARRATIVES_TRANSIENT_BLOCK_BYTES = 4ULL * 1024ULL * 1024ULL;  // 4 MB

uint64_t fnv1a_hash64(std::string_view p_str) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : p_str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Bidirectional column reader/writer eliminating duplicate get_*_row / set_*_row bodies
template <bool Write, typename T_Field>
void io_col(const NarrativeDOD& p_dod, NarrativeBufferSlot p_slot, uint32_t p_idx, T_Field& r_field) noexcept {
    if constexpr (Write) {
        if (auto* col = p_dod.get_column_ptr<std::remove_const_t<T_Field>>(p_slot)) col[p_idx] = r_field;
    } else {
        if (const auto* col = p_dod.get_column_ptr<std::remove_const_t<T_Field>>(p_slot)) r_field = col[p_idx];
    }
}

// Generic contiguous slice appender shared across Character goals, Relationships, Causal links, Incidents, and Plots
template <typename T_Elem>
bool append_contiguous_slice(
    const NarrativeDOD& p_dod,
    NarrativeBufferSlot p_offset_slot,
    NarrativeBufferSlot p_count_slot,
    NarrativeBufferSlot p_data_slot,
    uint32_t p_owner_row,
    uint32_t& r_cursor,
    uint32_t p_max_capacity,
    std::span<const T_Elem> p_items,
    uint32_t* r_out_offset = nullptr,
    uint16_t* r_out_count = nullptr
) noexcept {
    if (p_items.empty() || r_cursor + p_items.size() > p_max_capacity) return false;
    const uint32_t start = r_cursor;
    const auto count     = static_cast<uint16_t>(p_items.size());

    if (p_offset_slot != NarrativeBufferSlot::COUNT) {
        if (auto* off = p_dod.get_column_ptr<uint32_t>(p_offset_slot)) off[p_owner_row] = start;
    }
    if (p_count_slot != NarrativeBufferSlot::COUNT) {
        if (auto* cnt = p_dod.get_column_ptr<uint16_t>(p_count_slot)) cnt[p_owner_row] = count;
    }
    if (auto* data = p_dod.get_column_ptr<T_Elem>(p_data_slot)) {
        std::copy(p_items.begin(), p_items.end(), data + start);
    }

    r_cursor += count;
    if (r_out_offset) *r_out_offset = start;
    if (r_out_count)  *r_out_count  = count;
    return true;
}

struct SlotAccessSpec {
    NarrativeBufferSlot slot = NarrativeBufferSlot::COUNT;
    BufferAccessMode access  = BufferAccessMode::READ;
};

template <typename T_Logic>
struct NarrativeTaskDescriptor;

#define DEFINE_NARRATIVE_TASK_DESCRIPTOR(LogicStruct, EnumVal, S0, A0, S1, A1, S2, A2, S3, A3) \
    template <typename T>                                                                      \
    struct NarrativeTaskDescriptor<LogicStruct<T>> {                                           \
        static constexpr auto logic_id = EnumVal;                                              \
        static constexpr std::array<SlotAccessSpec, 4> default_grants = {{                     \
            {NarrativeBufferSlot::S0, BufferAccessMode::A0},                                   \
            {NarrativeBufferSlot::S1, BufferAccessMode::A1},                                   \
            {NarrativeBufferSlot::S2, BufferAccessMode::A2},                                   \
            {NarrativeBufferSlot::S3, BufferAccessMode::A3}                                    \
        }};                                                                                    \
    }

// --- Compile-Time Task Grant & LogicID Table (N1 - N16) ---
DEFINE_NARRATIVE_TASK_DESCRIPTOR(NarremeSymbolicSimilarityTransformLogic, NarrativesTransformLogicID::NarremeSymbolicSimilarity,
    NARREME_CANDIDATE_SCORE, READ_WRITE, NARREME_TRAIT_BITMASK_LO, READ, NARREME_TRAIT_BITMASK_HI, READ, NARREME_AFFINITY_VECTOR, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(RoleBestFitAssignmentTransformLogic, NarrativesTransformLogicID::RoleBestFitAssignment,
    SYMBOLIC_TO_LITERAL, READ_WRITE, LITERAL_TO_SYMBOLIC, READ_WRITE, ASSIGNMENT_FIT_SCORE, READ_WRITE, NARREME_CANDIDATE_SCORE, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(SymbolicReferenceResolveTransformLogic, NarrativesTransformLogicID::SymbolicReferenceResolve,
    CONDITION_RESOLVED_SUBJECT_ID, READ_WRITE, SYMBOLIC_TO_LITERAL, READ, CONDITION_SUBJECT_ID, READ, CONDITION_RESOLVED_OBJECT_ID, READ_WRITE);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(GameplayConditionBridgeTransformLogic, NarrativesTransformLogicID::GameplayConditionBridge,
    CONDITION_STATUS, READ, CONDITION_SUBJECT_ID, READ, BRIDGED_PROP_MODDED_VALUE, READ, GAMEPLAY_CONDITION_EVENT_RING, READ_WRITE);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(NarrativeConditionEvaluateTransformLogic, NarrativesTransformLogicID::NarrativeConditionEvaluate,
    CONDITION_STATUS, READ, CONDITION_RESOLVED_SUBJECT_ID, READ, CONDITION_RESOLVED_OBJECT_ID, READ, NARREME_LOCATION_ID, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(CausalConditionWavePropagateTransformLogic, NarrativesTransformLogicID::CausalConditionWavePropagate,
    CONDITION_STATUS, READ, CAUSAL_SLICE_OFFSET, READ, CAUSAL_SLICE_COUNT, READ, CAUSAL_LINK_POD, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(IncidentStatusResolveTransformLogic, NarrativesTransformLogicID::IncidentStatusResolve,
    INCIDENT_STATUS, READ_WRITE, CONDITION_STATUS, READ, INCIDENT_COND_SLICE_OFFSET, READ, INCIDENT_COND_SLICE_COUNT, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(PlotStatusEvaluateTransformLogic, NarrativesTransformLogicID::PlotStatusEvaluate,
    PLOT_STATUS, READ_WRITE, INCIDENT_STATUS, READ, PLOT_EVENT_POD, READ, PLOT_EVENT_SLICE_OFFSET, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(CharacterGoalPlotSyncTransformLogic, NarrativesTransformLogicID::CharacterGoalPlotSync,
    CHARACTER_GOAL_STATUS, READ_WRITE, PLOT_STATUS, READ, CHARACTER_PRIMARY_GOAL_PLOT_ID, READ, CHARACTER_GOAL_EVENT_RING, READ_WRITE);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(NarremeLocationTransferTransformLogic, NarrativesTransformLogicID::NarremeLocationTransfer,
    NARREME_LOCATION_ID, READ_WRITE, NARREME_POSSESSOR_ID, READ_WRITE, SPATIAL_TRANSFER_COMMAND_RING, READ_WRITE, SPATIAL_TRANSFER_RING_CURSOR, READ_WRITE);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(NarrativeResourceCommitTransformLogic, NarrativesTransformLogicID::NarrativeResourceCommit,
    NARREME_TELEMETRY_SNAPSHOT, READ_WRITE, NARRATIVE_SIGNAL_COMMAND_RING, READ_WRITE, NARRATIVE_SIGNAL_RING_CURSOR, READ_WRITE, INCIDENT_STATUS, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(LocationOccupancyQueryLogic, NarrativesQueryLogicID::LocationOccupancy,
    NARREME_LOCATION_ID, READ, NARREME_POSSESSOR_ID, READ, NARREME_TYPE, READ, LOCATION_SELECTION, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(NarremeConditionBitmaskQueryLogic, NarrativesQueryLogicID::NarremeConditionBitmask,
    CONDITION_STATUS, READ, CONDITION_ROW_POD, READ, INCIDENT_COND_SLICE_OFFSET, READ, INCIDENT_COND_SLICE_COUNT, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(RelationshipGraphTraversalQueryLogic, NarrativesQueryLogicID::RelationshipGraphTraversal,
    RELATIONSHIP_ROW_OFFSETS, READ, RELATIONSHIP_EDGE_POD, READ, RELATIONSHIP_SOURCE_SELECTION, READ, NARREME_TYPE, READ);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(CausalDependencyDepthMetadataLogic, NarrativesMetadataLogicID::CausalDependencyDepth,
    INCIDENT_CAUSAL_COND_OFFSET, READ, INCIDENT_CAUSAL_COND_COUNT, READ, CONDITION_ROW_POD, READ, INCIDENT_CAUSAL_DEPTH, READ_WRITE);
DEFINE_NARRATIVE_TASK_DESCRIPTOR(StoryArcIslandClusterMetadataLogic, NarrativesMetadataLogicID::StoryArcIslandCluster,
    PLOT_EVENT_SLICE_OFFSET, READ, PLOT_EVENT_SLICE_COUNT, READ, PLOT_EVENT_POD, READ, PLOT_CLUSTER_ID, READ_WRITE);

#undef DEFINE_NARRATIVE_TASK_DESCRIPTOR

template <bool Write, typename T_Row>
void sync_narreme_columns(const NarrativeDOD& d, uint32_t s, T_Row& r) noexcept {
    io_col<Write>(d, NarrativeBufferSlot::NARREME_TRAIT_BITMASK_LO, s, r.trait_bitmask_lo);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_TRAIT_BITMASK_HI, s, r.trait_bitmask_hi);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_AFFINITY_VECTOR, s, r.affinity);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_LOCATION_ID, s, r.location_id);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_POSSESSOR_ID, s, r.possessor_id);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_STRUCTURAL_SIG, s, r.structural_sig);
    io_col<Write>(d, NarrativeBufferSlot::CHARACTER_GOAL_MASK, s, r.character_goal_mask);
    io_col<Write>(d, NarrativeBufferSlot::CHARACTER_PRIMARY_GOAL_PLOT_ID, s, r.primary_goal_plot_id);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_OBJECT_ID, s, r.object_id);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_TYPE, s, r.narreme_type);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_IS_SYMBOLIC, s, r.is_symbolic);
    io_col<Write>(d, NarrativeBufferSlot::NARREME_CANDIDATE_MASK, s, r.candidate_mask);
}

template <bool Write, typename T_Row>
void sync_symbolic_columns(const NarrativeDOD& d, uint32_t s, T_Row& r) noexcept {
    io_col<Write>(d, NarrativeBufferSlot::SYMBOLIC_ASSIGNMENT_POD, s, r);
    io_col<Write>(d, NarrativeBufferSlot::SYMBOLIC_TO_LITERAL, s, r.literal_id);
    io_col<Write>(d, NarrativeBufferSlot::ASSIGNMENT_FIT_SCORE, s, r.fit_score);
    io_col<Write>(d, NarrativeBufferSlot::ASSIGNMENT_FIT_MODE, s, r.fit_mode);
    io_col<Write>(d, NarrativeBufferSlot::ASSIGNMENT_REPLACE_POLICY, s, r.replacement_policy);
    io_col<Write>(d, NarrativeBufferSlot::ASSIGNMENT_DIRTY_FLAGS, s, r.dirty_flags);
}

template <bool Write, typename T_Row>
void sync_condition_columns(const NarrativeDOD& d, uint32_t s, T_Row& r) noexcept {
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_ROW_POD, s, r);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_SUBJECT_ID, s, r.subject_id);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_OBJECT_ID, s, r.object_id);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_RESOLVED_SUBJECT_ID, s, r.resolved_subject_id);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_RESOLVED_OBJECT_ID, s, r.resolved_object_id);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_PREDICATE_OP, s, r.predicate_op);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_TYPE, s, r.condition_type);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_SUBJECT_IS_SYMBOLIC, s, r.subject_is_symbolic);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_OBJECT_IS_SYMBOLIC, s, r.object_is_symbolic);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_EXPECTED_STATUS, s, r.expected_status);
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_STATUS, s, r.current_status);
    if constexpr (Write) {
        io_col<true>(d, NarrativeBufferSlot::CONDITION_READ_STATUS, s, r.current_status);
    }
    io_col<Write>(d, NarrativeBufferSlot::CONDITION_TRANSITION_FLAGS, s, r.status_dirty_flag);
}

template <bool Write, typename T_Row>
void sync_incident_columns(const NarrativeDOD& d, uint32_t s, T_Row& r) noexcept {
    io_col<Write>(d, NarrativeBufferSlot::INCIDENT_COND_SLICE_OFFSET, s, r.cond_slice_offset);
    io_col<Write>(d, NarrativeBufferSlot::INCIDENT_COND_SLICE_COUNT, s, r.cond_slice_count);
    io_col<Write>(d, NarrativeBufferSlot::INCIDENT_CAUSAL_COND_OFFSET, s, r.causal_cond_offset);
    io_col<Write>(d, NarrativeBufferSlot::INCIDENT_CAUSAL_COND_COUNT, s, r.causal_cond_count);
    io_col<Write>(d, NarrativeBufferSlot::INCIDENT_CAUSAL_DEPTH, s, r.causal_depth);
    io_col<Write>(d, NarrativeBufferSlot::UNRESOLVED_SYMBOLIC_COUNT, s, r.unresolved_symbolic_count);
    io_col<Write>(d, NarrativeBufferSlot::INCIDENT_STATUS, s, r.status);
    io_col<Write>(d, NarrativeBufferSlot::INCIDENT_TRANSITION_FLAGS, s, r.transition_flags);
}

template <bool Write, typename T_Row>
void sync_plot_columns(const NarrativeDOD& d, uint32_t s, T_Row& r) noexcept {
    io_col<Write>(d, NarrativeBufferSlot::PLOT_EVENT_SLICE_OFFSET, s, r.event_slice_offset);
    io_col<Write>(d, NarrativeBufferSlot::PLOT_EVENT_SLICE_COUNT, s, r.event_slice_count);
    io_col<Write>(d, NarrativeBufferSlot::PLOT_PARTICIPANT_MASK, s, r.participant_mask);
    io_col<Write>(d, NarrativeBufferSlot::PLOT_PRIMARY_LOCATION_ID, s, r.primary_location_id);
    io_col<Write>(d, NarrativeBufferSlot::PLOT_GROUP_MASK, s, r.group_mask);
    io_col<Write>(d, NarrativeBufferSlot::PLOT_CLUSTER_ID, s, r.cluster_id);
    io_col<Write>(d, NarrativeBufferSlot::PLOT_STATUS, s, r.status);
    io_col<Write>(d, NarrativeBufferSlot::PLOT_TRANSITION_FLAGS, s, r.transition_flags);
}

} // anonymous namespace

// ============================================================================
// Lifecycle & Loop-Driven Buffer Manifest Management
// ============================================================================

NarrativeDOD::NarrativeDOD(
    std::shared_ptr<MemoryManagerDOD> p_memory_manager,
    const NarrativesBufferCapacities& p_capacities
)
    : memory_manager_(std::move(p_memory_manager)),
      capacities_(p_capacities) {
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_NARRATIVES_MASTER_BLOCK_BYTES,
            DEFAULT_NARRATIVES_TRANSIENT_BLOCK_BYTES
        );
    }
    default_task_graph_ = std::make_shared<TaskGraphDOD>(memory_manager_.get());
    default_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
    initialize_buffers(capacities_);
}

void NarrativeDOD::initialize_buffers(const NarrativesBufferCapacities& p_capacities) {
    capacities_ = p_capacities;
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_NARRATIVES_MASTER_BLOCK_BYTES,
            DEFAULT_NARRATIVES_TRANSIENT_BLOCK_BYTES
        );
    }

    narreme_allocator_.initialize(capacities_.max_narremes);
    symbolic_slot_allocator_.initialize(capacities_.max_symbolic_slots);
    condition_allocator_.initialize(capacities_.max_conditions);
    incident_allocator_.initialize(capacities_.max_incidents);
    plot_allocator_.initialize(capacities_.max_plots);

    for (size_t i = 0; i < NARRATIVE_BUFFER_SLOT_COUNT; ++i) {
        const NarrativeBufferSpec& spec = NARRATIVE_BUFFER_SPECS[i];
        const size_t max_elems = std::max<size_t>(1u, capacities_.get_capacity(spec.capacity_kind));
        const size_t bytes     = max_elems * spec.element_stride;
        const uint32_t buf_id  = memory_manager_->create_shadowed_buffer(
            spec.layout, bytes, static_cast<int64_t>(max_elems), spec.selection_mode
        );
        if (MemoryBufferPOD* buf = memory_manager_->get_buffer(buf_id)) {
            buf->element_stride = spec.element_stride;
            buf->current_count  = 0;
            if (buf->master_block_ptr) {
                std::memset(buf->master_block_ptr + buf->memory_offset, 0, bytes);
            }
        }
        manifest_.buffer_ids[i] = buf_id;
    }

    reset_all_slots();
}

uint32_t NarrativeDOD::_active_count_for_group(NarrativeTableGroup p_group) const noexcept {
    switch (p_group) {
        case NarrativeTableGroup::NARREME_IDENTITY:        return narreme_allocator_.get_high_water_mark();
        case NarrativeTableGroup::SYMBOLIC_ASSIGNMENT:     return symbolic_slot_allocator_.get_high_water_mark();
        case NarrativeTableGroup::RELATIONSHIP_CSR_ROW:    return narreme_allocator_.get_high_water_mark() ? (narreme_allocator_.get_high_water_mark() + 1u) : 0u;
        case NarrativeTableGroup::RELATIONSHIP_CSR_EDGE:   return relationship_edge_cursor_;
        case NarrativeTableGroup::CONDITION_TABLE:         return condition_allocator_.get_high_water_mark();
        case NarrativeTableGroup::CAUSAL_LINK_SLICE:       return causal_link_cursor_;
        case NarrativeTableGroup::INCIDENT_STATE:          return incident_allocator_.get_high_water_mark();
        case NarrativeTableGroup::INCIDENT_COND_SLICE:     return incident_cond_ref_cursor_;
        case NarrativeTableGroup::PLOT_STATE:              return plot_allocator_.get_high_water_mark();
        case NarrativeTableGroup::PLOT_EVENT_SLICE:        return plot_event_cursor_;
        case NarrativeTableGroup::CHARACTER_GOAL_SLICE:    return character_goal_cursor_;
        case NarrativeTableGroup::LOCATION_NEIGHBOR_SLICE: return location_neighbor_cursor_;
        case NarrativeTableGroup::BRIDGED_GAMEPLAY_PROPERTY: return bridged_property_count_;
        case NarrativeTableGroup::EVENT_RING_OR_COUNTER:
        default:                                           return 0u;
    }
}

void NarrativeDOD::sync_selection_ranges() noexcept {
    if (!memory_manager_) return;
    for (size_t i = 0; i < NARRATIVE_BUFFER_SLOT_COUNT; ++i) {
        MemoryBufferPOD* buf = memory_manager_->get_buffer(manifest_.buffer_ids[i]);
        if (!buf) continue;
        const NarrativeBufferSpec& spec = NARRATIVE_BUFFER_SPECS[i];
        buf->current_count = (spec.table_group == NarrativeTableGroup::EVENT_RING_OR_COUNTER)
            ? buf->max_elements
            : std::min<int64_t>(static_cast<int64_t>(_active_count_for_group(spec.table_group)), buf->max_elements);
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void NarrativeDOD::reset_all_slots() noexcept {
    narreme_allocator_.clear();
    symbolic_slot_allocator_.clear();
    condition_allocator_.clear();
    incident_allocator_.clear();
    plot_allocator_.clear();

    relationship_edge_cursor_ = 0u;
    causal_link_cursor_       = 0u;
    incident_cond_ref_cursor_ = 0u;
    plot_event_cursor_        = 0u;
    character_goal_cursor_    = 0u;
    location_neighbor_cursor_ = 0u;

    for (NarrativeBufferSlot s : {
        NarrativeBufferSlot::NARREME_LOCATION_ID, NarrativeBufferSlot::NARREME_POSSESSOR_ID,
        NarrativeBufferSlot::NARREME_PENDING_LOCATION_ID, NarrativeBufferSlot::NARREME_PENDING_POSSESSOR_ID,
        NarrativeBufferSlot::NARREME_PREVIOUS_LOCATION_ID, NarrativeBufferSlot::NARREME_PREVIOUS_POSSESSOR_ID,
        NarrativeBufferSlot::LOCATION_PARENT_ID, NarrativeBufferSlot::CHARACTER_PRIMARY_GOAL_PLOT_ID,
        NarrativeBufferSlot::CONDITION_SUBJECT_ID, NarrativeBufferSlot::CONDITION_OBJECT_ID,
        NarrativeBufferSlot::CONDITION_RESOLVED_SUBJECT_ID, NarrativeBufferSlot::CONDITION_RESOLVED_OBJECT_ID,
        NarrativeBufferSlot::PLOT_PRIMARY_LOCATION_ID
    }) {
        auto span = get_column_span<uint32_t>(s);
        std::fill(span.begin(), span.end(), INVALID_ID);
    }

    for (NarrativeBufferSlot s : {
        NarrativeBufferSlot::SYMBOLIC_TO_LITERAL, NarrativeBufferSlot::LITERAL_TO_SYMBOLIC,
        NarrativeBufferSlot::PREVIOUS_LITERAL_ID, NarrativeBufferSlot::PLOT_CLUSTER_ID
    }) {
        auto span = get_column_span<int32_t>(s);
        std::fill(span.begin(), span.end(), -1);
    }

    for (NarrativeBufferSlot s : {
        NarrativeBufferSlot::SPATIAL_TRANSFER_RING_CURSOR, NarrativeBufferSlot::GAMEPLAY_EVENT_RING_CURSOR,
        NarrativeBufferSlot::CHARACTER_GOAL_RING_CURSOR, NarrativeBufferSlot::NARRATIVE_SIGNAL_RING_CURSOR
    }) {
        auto span = get_column_span<uint32_t>(s);
        std::fill(span.begin(), span.end(), 0u);
    }

    sync_selection_ranges();
}

void NarrativeDOD::populate_task_buffer_dictionary(godot::Dictionary& r_props, bool p_overwrite_existing) const {
    auto set_key = [&](std::string_view p_key, uint32_t p_val) {
        if (p_key.empty() || p_val == INVALID_ID) return;
        const godot::String key_str(std::string(p_key).c_str());
        if (p_overwrite_existing || !r_props.has(key_str)) {
            r_props[key_str] = static_cast<int64_t>(p_val);
        }
    };
    for (size_t i = 0; i < NARRATIVE_BUFFER_SLOT_COUNT; ++i) {
        const uint32_t buf_id = manifest_.buffer_ids[i];
        set_key(NARRATIVE_BUFFER_SPECS[i].canonical_name, buf_id);
        for (std::string_view alias : NARRATIVE_BUFFER_SPECS[i].aliases) {
            set_key(alias, buf_id);
        }
    }
}

GrantPartPOD NarrativeDOD::make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept {
    if (memory_manager_ && p_buffer_id != INVALID_ID) {
        if (const MemoryBufferPOD* buf = memory_manager_->get_buffer(p_buffer_id)) {
            return make_grant_requirement(p_buffer_id, p_access, buf->element_stride, buf->max_elements);
        }
    }
    return make_grant_requirement(p_buffer_id, p_access, sizeof(uint32_t), 0);
}

// ============================================================================
// Token, 128-Bit Trait & 64-Bit Group Bitmask Interning
// ============================================================================

uint32_t NarrativeDOD::intern_name_token(std::string_view p_name) {
    if (p_name.empty()) return 0u;
    std::string key(p_name);
    if (auto it = interned_tokens_.find(key); it != interned_tokens_.end()) return it->second;
    const uint32_t id = next_token_id_++;
    interned_tokens_.emplace(std::move(key), id);
    return id;
}

uint8_t NarrativeDOD::get_or_register_trait_bit(std::string_view p_trait_name) {
    if (p_trait_name.empty()) return 0u;
    std::string key(p_trait_name);
    if (auto it = trait_bit_map_.find(key); it != trait_bit_map_.end()) return it->second;
    if (next_trait_bit_ >= 128u) return static_cast<uint8_t>(fnv1a_hash64(key) & 127ULL);
    const uint8_t bit = next_trait_bit_++;
    trait_bit_map_.emplace(std::move(key), bit);
    return bit;
}

void NarrativeDOD::compute_trait_bitmask_128(std::span<const std::string_view> p_traits, uint64_t& r_lo, uint64_t& r_hi) {
    r_lo = 0ULL;
    r_hi = 0ULL;
    for (std::string_view t : p_traits) {
        if (t.empty()) continue;
        const uint8_t bit = get_or_register_trait_bit(t);
        if (bit < 64u) r_lo |= (1ULL << bit);
        else           r_hi |= (1ULL << (bit - 64u));
    }
}

uint64_t NarrativeDOD::get_or_register_group_bitmask(std::string_view p_group_name) {
    if (p_group_name.empty()) return 0ULL;
    std::string key(p_group_name);
    if (auto it = group_bit_map_.find(key); it != group_bit_map_.end()) return (1ULL << it->second);
    if (next_group_bit_ >= 64u) return (1ULL << (fnv1a_hash64(key) & 63ULL));
    const uint8_t bit = next_group_bit_++;
    group_bit_map_.emplace(std::move(key), bit);
    return (1ULL << bit);
}

// ============================================================================
// Table 1 - 5 Row Allocators, Getters/Setters & CSR Builders
// ============================================================================

uint32_t NarrativeDOD::allocate_narreme(const NarremeIdentityRowPOD& p_row, std::span<const uint32_t> p_goal_plot_ids) {
    const uint32_t slot = narreme_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    set_narreme_row(slot, p_row);
    append_contiguous_slice(
        *this, NarrativeBufferSlot::CHARACTER_GOAL_SLICE_OFFSET, NarrativeBufferSlot::CHARACTER_GOAL_SLICE_COUNT,
        NarrativeBufferSlot::CHARACTER_GOAL_PLOT_ID, slot, character_goal_cursor_, capacities_.max_character_goals, p_goal_plot_ids
    );
    return slot;
}

bool NarrativeDOD::free_narreme(uint32_t p_narreme_slot) {
    if (!narreme_allocator_.is_valid(p_narreme_slot)) return false;
    set_narreme_row(p_narreme_slot, NarremeIdentityRowPOD{});
    return narreme_allocator_.release(p_narreme_slot);
}

NarremeIdentityRowPOD NarrativeDOD::get_narreme_row(uint32_t p_slot) const noexcept {
    NarremeIdentityRowPOD row{};
    if (p_slot < capacities_.max_narremes) sync_narreme_columns<false>(*this, p_slot, row);
    return row;
}

void NarrativeDOD::set_narreme_row(uint32_t p_slot, const NarremeIdentityRowPOD& p_row) noexcept {
    if (p_slot < capacities_.max_narremes) sync_narreme_columns<true>(*this, p_slot, p_row);
}

NarrativeTelemetrySnapshotPOD NarrativeDOD::get_telemetry_snapshot(uint32_t p_slot) const noexcept {
    if (const auto* ptr = get_telemetry_snapshot_ptr(p_slot)) return *ptr;
    return NarrativeTelemetrySnapshotPOD{};
}

const NarrativeTelemetrySnapshotPOD* NarrativeDOD::get_telemetry_snapshot_ptr(uint32_t p_slot) const noexcept {
    if (p_slot >= capacities_.max_narremes) return nullptr;
    const auto* p = get_column_ptr<NarrativeTelemetrySnapshotPOD>(NarrativeBufferSlot::NARREME_TELEMETRY_SNAPSHOT);
    return p ? &p[p_slot] : nullptr;
}

uint32_t NarrativeDOD::allocate_symbolic_slot(const SymbolicAssignmentRecordPOD& p_record, uint32_t p_title_token) {
    const uint32_t slot = symbolic_slot_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    SymbolicAssignmentRecordPOD rec = p_record;
    rec.symbolic_id = slot;
    set_symbolic_assignment(slot, rec, p_title_token);
    return slot;
}

bool NarrativeDOD::free_symbolic_slot(uint32_t p_symbolic_slot) {
    if (!symbolic_slot_allocator_.is_valid(p_symbolic_slot)) return false;
    SymbolicAssignmentRecordPOD cleared{};
    cleared.symbolic_id = p_symbolic_slot;
    cleared.literal_id  = -1;
    set_symbolic_assignment(p_symbolic_slot, cleared, 0u);
    return symbolic_slot_allocator_.release(p_symbolic_slot);
}

SymbolicAssignmentRecordPOD NarrativeDOD::get_symbolic_assignment(uint32_t p_symbolic_slot) const noexcept {
    SymbolicAssignmentRecordPOD rec{};
    if (p_symbolic_slot < capacities_.max_symbolic_slots) sync_symbolic_columns<false>(*this, p_symbolic_slot, rec);
    return rec;
}

void NarrativeDOD::set_symbolic_assignment(uint32_t p_symbolic_slot, const SymbolicAssignmentRecordPOD& p_record, uint32_t p_title_token) noexcept {
    if (p_symbolic_slot >= capacities_.max_symbolic_slots) return;
    sync_symbolic_columns<true>(*this, p_symbolic_slot, p_record);
    if (p_title_token != 0u) io_col<true>(*this, NarrativeBufferSlot::SYMBOLIC_TITLE_TOKEN, p_symbolic_slot, p_title_token);
    if (p_record.literal_id >= 0 && static_cast<uint32_t>(p_record.literal_id) < capacities_.max_narremes) {
        const int32_t sym_idx = static_cast<int32_t>(p_symbolic_slot);
        io_col<true>(*this, NarrativeBufferSlot::LITERAL_TO_SYMBOLIC, static_cast<uint32_t>(p_record.literal_id), sym_idx);
    }
}

bool NarrativeDOD::assign_symbolic_to_literal(uint32_t p_symbolic_slot, int32_t p_literal_narreme_id, int32_t p_fit_score) noexcept {
    if (p_symbolic_slot >= capacities_.max_symbolic_slots) return false;
    SymbolicAssignmentRecordPOD rec = get_symbolic_assignment(p_symbolic_slot);
    io_col<true>(*this, NarrativeBufferSlot::PREVIOUS_LITERAL_ID, p_symbolic_slot, rec.literal_id);
    rec.literal_id   = p_literal_narreme_id;
    rec.fit_score    = p_fit_score;
    rec.dirty_flags |= (p_literal_narreme_id >= 0) ? ASSIGNMENT_DIRTY_FILLED : ASSIGNMENT_DIRTY_VACATED;
    set_symbolic_assignment(p_symbolic_slot, rec);
    return true;
}

bool NarrativeDOD::append_narreme_relationships(uint32_t p_source_narreme_slot, std::span<const RelationshipEdgePOD> p_edges) noexcept {
    if (p_source_narreme_slot >= capacities_.max_narremes) return false;
    if (!append_contiguous_slice(
            *this, NarrativeBufferSlot::RELATIONSHIP_ROW_OFFSETS, NarrativeBufferSlot::RELATIONSHIP_ROW_COUNTS,
            NarrativeBufferSlot::RELATIONSHIP_EDGE_POD, p_source_narreme_slot, relationship_edge_cursor_, capacities_.max_relationships, p_edges)) {
        return false;
    }
    io_col<true>(*this, NarrativeBufferSlot::RELATIONSHIP_ROW_OFFSETS, p_source_narreme_slot + 1u, relationship_edge_cursor_);
    return true;
}

void NarrativeDOD::rebuild_relationship_csr(std::span<const std::vector<RelationshipEdgePOD>> p_per_narreme_edges) noexcept {
    auto* offsets = get_column_ptr<uint32_t>(NarrativeBufferSlot::RELATIONSHIP_ROW_OFFSETS);
    auto* counts  = get_column_ptr<uint16_t>(NarrativeBufferSlot::RELATIONSHIP_ROW_COUNTS);
    auto* edges   = get_column_ptr<RelationshipEdgePOD>(NarrativeBufferSlot::RELATIONSHIP_EDGE_POD);
    if (!offsets || !counts || !edges) return;

    relationship_edge_cursor_ = 0u;
    const size_t row_limit = std::min<size_t>(p_per_narreme_edges.size(), capacities_.max_narremes);
    for (size_t i = 0; i < row_limit; ++i) {
        offsets[i] = relationship_edge_cursor_;
        const size_t avail = (relationship_edge_cursor_ < capacities_.max_relationships) ? (capacities_.max_relationships - relationship_edge_cursor_) : 0u;
        const size_t copy_cnt = std::min(p_per_narreme_edges[i].size(), avail);
        counts[i] = static_cast<uint16_t>(copy_cnt);
        if (copy_cnt > 0u) {
            std::copy_n(p_per_narreme_edges[i].data(), copy_cnt, edges + relationship_edge_cursor_);
            relationship_edge_cursor_ += static_cast<uint32_t>(copy_cnt);
        }
    }
    offsets[row_limit] = relationship_edge_cursor_;
}

void NarrativeDOD::rebuild_location_adjacency_csr(std::span<const std::vector<uint32_t>> p_per_location_neighbors) noexcept {
    auto* offsets   = get_column_ptr<uint32_t>(NarrativeBufferSlot::LOCATION_CSR_OFFSETS);
    auto* neighbors = get_column_ptr<uint32_t>(NarrativeBufferSlot::LOCATION_CSR_NEIGHBORS);
    if (!offsets || !neighbors) return;

    location_neighbor_cursor_ = 0u;
    const size_t row_limit = std::min<size_t>(p_per_location_neighbors.size(), capacities_.max_narremes);
    for (size_t i = 0; i < row_limit; ++i) {
        offsets[i] = location_neighbor_cursor_;
        const size_t avail = (location_neighbor_cursor_ < capacities_.max_location_neighbors) ? (capacities_.max_location_neighbors - location_neighbor_cursor_) : 0u;
        const size_t copy_cnt = std::min(p_per_location_neighbors[i].size(), avail);
        if (copy_cnt > 0u) {
            std::copy_n(p_per_location_neighbors[i].data(), copy_cnt, neighbors + location_neighbor_cursor_);
            location_neighbor_cursor_ += static_cast<uint32_t>(copy_cnt);
        }
    }
    offsets[row_limit] = location_neighbor_cursor_;
}

uint32_t NarrativeDOD::allocate_condition(const ConditionEvaluationRowPOD& p_row, std::span<const CausalLinkPOD> p_causal_links) {
    const uint32_t slot = condition_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    set_condition_row(slot, p_row);
    if (!p_causal_links.empty()) append_causal_links(slot, p_causal_links);
    return slot;
}

bool NarrativeDOD::free_condition(uint32_t p_condition_slot) {
    if (!condition_allocator_.is_valid(p_condition_slot)) return false;
    set_condition_row(p_condition_slot, ConditionEvaluationRowPOD{});
    return condition_allocator_.release(p_condition_slot);
}

ConditionEvaluationRowPOD NarrativeDOD::get_condition_row(uint32_t p_condition_slot) const noexcept {
    ConditionEvaluationRowPOD row{};
    if (p_condition_slot < capacities_.max_conditions) sync_condition_columns<false>(*this, p_condition_slot, row);
    return row;
}

void NarrativeDOD::set_condition_row(uint32_t p_condition_slot, const ConditionEvaluationRowPOD& p_row) noexcept {
    if (p_condition_slot >= capacities_.max_conditions) return;
    ConditionEvaluationRowPOD row = p_row;
    if (row.resolved_subject_id == INVALID_ID) row.resolved_subject_id = row.subject_id;
    if (row.resolved_object_id == INVALID_ID)  row.resolved_object_id  = row.object_id;
    sync_condition_columns<true>(*this, p_condition_slot, row);
}

bool NarrativeDOD::append_causal_links(uint32_t p_condition_slot, std::span<const CausalLinkPOD> p_links) noexcept {
    if (p_condition_slot >= capacities_.max_conditions) return false;
    uint32_t start = 0u;
    if (!append_contiguous_slice(
            *this, NarrativeBufferSlot::CAUSAL_SLICE_OFFSET, NarrativeBufferSlot::CAUSAL_SLICE_COUNT,
            NarrativeBufferSlot::CAUSAL_LINK_POD, p_condition_slot, causal_link_cursor_, capacities_.max_causal_links, p_links, &start)) {
        return false;
    }
    auto* child_col = get_column_ptr<uint32_t>(NarrativeBufferSlot::CAUSAL_CHILD_CONDITION_ID);
    auto* cause_col = get_column_ptr<int8_t>(NarrativeBufferSlot::CAUSAL_EXPECTED_CAUSE_STATUS);
    for (size_t i = 0; i < p_links.size(); ++i) {
        if (child_col) child_col[start + i] = p_links[i].child_condition_id;
        if (cause_col) cause_col[start + i] = p_links[i].required_cause_status;
    }
    return true;
}

void NarrativeDOD::bind_external_gameplay_property_buffers(
    uint32_t p_modded_value_buf_id,
    uint32_t p_modded_min_buf_id,
    uint32_t p_modded_max_buf_id,
    uint32_t p_flags_buf_id,
    uint32_t p_active_property_count
) noexcept {
    if (p_modded_value_buf_id != INVALID_ID) manifest_.set(NarrativeBufferSlot::BRIDGED_PROP_MODDED_VALUE, p_modded_value_buf_id);
    if (p_modded_min_buf_id != INVALID_ID)   manifest_.set(NarrativeBufferSlot::BRIDGED_PROP_MODDED_MIN, p_modded_min_buf_id);
    if (p_modded_max_buf_id != INVALID_ID)   manifest_.set(NarrativeBufferSlot::BRIDGED_PROP_MODDED_MAX, p_modded_max_buf_id);
    if (p_flags_buf_id != INVALID_ID)        manifest_.set(NarrativeBufferSlot::BRIDGED_PROP_FLAGS, p_flags_buf_id);
    bridged_property_count_ = p_active_property_count;
}

uint32_t NarrativeDOD::allocate_incident(
    const IncidentStateRowPOD& p_row,
    std::span<const uint32_t> p_prereq_condition_ids,
    std::span<const uint32_t> p_causal_condition_ids
) {
    const uint32_t slot = incident_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    IncidentStateRowPOD row = p_row;
    append_contiguous_slice(
        *this, NarrativeBufferSlot::COUNT, NarrativeBufferSlot::COUNT, NarrativeBufferSlot::INCIDENT_COND_INDEX_TABLE,
        slot, incident_cond_ref_cursor_, capacities_.max_incident_condition_refs, p_prereq_condition_ids,
        &row.cond_slice_offset, &row.cond_slice_count
    );
    append_contiguous_slice(
        *this, NarrativeBufferSlot::COUNT, NarrativeBufferSlot::COUNT, NarrativeBufferSlot::INCIDENT_COND_INDEX_TABLE,
        slot, incident_cond_ref_cursor_, capacities_.max_incident_condition_refs, p_causal_condition_ids,
        &row.causal_cond_offset, &row.causal_cond_count
    );
    set_incident_row(slot, row);
    return slot;
}

bool NarrativeDOD::free_incident(uint32_t p_incident_slot) {
    if (!incident_allocator_.is_valid(p_incident_slot)) return false;
    set_incident_row(p_incident_slot, IncidentStateRowPOD{});
    return incident_allocator_.release(p_incident_slot);
}

IncidentStateRowPOD NarrativeDOD::get_incident_row(uint32_t p_incident_slot) const noexcept {
    IncidentStateRowPOD row{};
    if (p_incident_slot < capacities_.max_incidents) sync_incident_columns<false>(*this, p_incident_slot, row);
    return row;
}

void NarrativeDOD::set_incident_row(uint32_t p_incident_slot, const IncidentStateRowPOD& p_row) noexcept {
    if (p_incident_slot < capacities_.max_incidents) sync_incident_columns<true>(*this, p_incident_slot, p_row);
}

uint32_t NarrativeDOD::allocate_plot(const PlotProgressionRowPOD& p_row, std::span<const PlotEventPOD> p_events) {
    const uint32_t slot = plot_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    set_plot_row(slot, p_row);
    if (!p_events.empty()) append_plot_events(slot, p_events);
    return slot;
}

bool NarrativeDOD::free_plot(uint32_t p_plot_slot) {
    if (!plot_allocator_.is_valid(p_plot_slot)) return false;
    set_plot_row(p_plot_slot, PlotProgressionRowPOD{});
    return plot_allocator_.release(p_plot_slot);
}

PlotProgressionRowPOD NarrativeDOD::get_plot_row(uint32_t p_plot_slot) const noexcept {
    PlotProgressionRowPOD row{};
    if (p_plot_slot < capacities_.max_plots) sync_plot_columns<false>(*this, p_plot_slot, row);
    return row;
}

void NarrativeDOD::set_plot_row(uint32_t p_plot_slot, const PlotProgressionRowPOD& p_row) noexcept {
    if (p_plot_slot < capacities_.max_plots) sync_plot_columns<true>(*this, p_plot_slot, p_row);
}

bool NarrativeDOD::append_plot_events(uint32_t p_plot_slot, std::span<const PlotEventPOD> p_events) noexcept {
    if (p_plot_slot >= capacities_.max_plots) return false;
    return append_contiguous_slice(
        *this, NarrativeBufferSlot::PLOT_EVENT_SLICE_OFFSET, NarrativeBufferSlot::PLOT_EVENT_SLICE_COUNT,
        NarrativeBufferSlot::PLOT_EVENT_POD, p_plot_slot, plot_event_cursor_, capacities_.max_plot_events, p_events
    );
}

// ============================================================================
// Generic Templated Task Node Builder (N1 - N16) & Stage Injection
// ============================================================================

void NarrativeDOD::_bind_default_logic_buffers(void* r_logic_bytes, size_t p_size, const void* p_default_bytes) const noexcept {
    auto* dst       = static_cast<uint32_t*>(r_logic_bytes);
    const auto* src = static_cast<const uint32_t*>(p_default_bytes);
    for (size_t i = 0, n = p_size / sizeof(uint32_t); i < n; ++i) {
        if (dst[i] == INVALID_ID && src[i] != INVALID_ID) dst[i] = src[i];
    }
}

template <typename T_Logic>
NodeID NarrativeDOD::add_narrative_task_node(
    TaskGraphDOD& r_graph,
    const T_Logic& p_logic_override,
    const NarrativesNodeExecutionConfig& p_config
) const {
    using Descriptor = NarrativeTaskDescriptor<T_Logic>;

    T_Logic logic = p_logic_override;
    T_Logic defaults{};
    godot::Dictionary manifest_props;
    populate_task_buffer_dictionary(manifest_props, true);
    defaults.apply_properties(manifest_props);
    _bind_default_logic_buffers(&logic, sizeof(T_Logic), &defaults);

    if constexpr (std::is_same_v<T_Logic, NarremeLocationTransferTransformLogic<typename T_Logic::ValueType>>) {
        if (p_logic_override.ring_cursor_buffer_id == INVALID_ID) logic.ring_cursor_buffer_id = manifest_.get(NarrativeBufferSlot::SPATIAL_TRANSFER_RING_CURSOR);
    } else if constexpr (std::is_same_v<T_Logic, GameplayConditionBridgeTransformLogic<typename T_Logic::ValueType>>) {
        if (p_logic_override.ring_cursor_buffer_id == INVALID_ID) logic.ring_cursor_buffer_id = manifest_.get(NarrativeBufferSlot::GAMEPLAY_EVENT_RING_CURSOR);
    } else if constexpr (std::is_same_v<T_Logic, CharacterGoalPlotSyncTransformLogic<typename T_Logic::ValueType>>) {
        if (p_logic_override.ring_cursor_buffer_id == INVALID_ID) logic.ring_cursor_buffer_id = manifest_.get(NarrativeBufferSlot::CHARACTER_GOAL_RING_CURSOR);
    } else if constexpr (std::is_same_v<T_Logic, NarrativeResourceCommitTransformLogic<typename T_Logic::ValueType>>) {
        if (p_logic_override.ring_cursor_buffer_id == INVALID_ID) logic.ring_cursor_buffer_id = manifest_.get(NarrativeBufferSlot::NARRATIVE_SIGNAL_RING_CURSOR);
    } else if constexpr (std::is_same_v<T_Logic, NarrativeConditionEvaluateTransformLogic<typename T_Logic::ValueType>>) {
        if (p_logic_override.subject_id_buffer_id == INVALID_ID) logic.subject_id_buffer_id = manifest_.get(NarrativeBufferSlot::CONDITION_RESOLVED_SUBJECT_ID);
        if (p_logic_override.object_id_buffer_id == INVALID_ID)  logic.object_id_buffer_id  = manifest_.get(NarrativeBufferSlot::CONDITION_RESOLVED_OBJECT_ID);
    } else if constexpr (std::is_same_v<T_Logic, NarremeConditionBitmaskQueryLogic<typename T_Logic::ValueType>>) {
        if (p_logic_override.status_buffer_id == INVALID_ID) logic.status_buffer_id = manifest_.get(NarrativeBufferSlot::CONDITION_STATUS);
    }

    const NodeID node_id = r_graph.add_task_node(IsQueryLogic<T_Logic> ? TaskTypeDOD::QUERY_CULLER : TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    if constexpr (requires { T_Logic::transient_workspace_bytes; }) {
        if (T_Logic::transient_workspace_bytes > 0u) {
            r_graph.set_node_transient_requirement(node_id, static_cast<uint32_t>(T_Logic::transient_workspace_bytes));
        }
    }

    std::unique_ptr<INativeTask> native_task;
    if (p_config.use_registry_factory) {
        if constexpr (IsTransformLogic<T_Logic>) {
            native_task = NarrativesTransformTaskRegistry::create(static_cast<uint32_t>(Descriptor::logic_id), static_cast<uint32_t>(p_config.view), static_cast<uint32_t>(p_config.strategy), static_cast<uint32_t>(p_config.element_type));
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = NarrativesQueryTaskRegistry::create(static_cast<uint32_t>(p_config.query_op), static_cast<uint32_t>(Descriptor::logic_id), static_cast<uint32_t>(p_config.view), static_cast<uint32_t>(p_config.strategy), static_cast<uint32_t>(p_config.element_type));
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = NarrativesMetadataTaskRegistry::create(static_cast<uint32_t>(Descriptor::logic_id), static_cast<uint32_t>(p_config.view), static_cast<uint32_t>(p_config.strategy), static_cast<uint32_t>(p_config.element_type));
        }
        if (native_task) {
            manifest_props["target_buffer_id"] = static_cast<int64_t>(logic.get_target_buffer_id());
            native_task->apply_properties(manifest_props);
        }
    }

    if (!native_task) {
        if constexpr (IsTransformLogic<T_Logic>) {
            native_task = std::make_unique<TransformTask<T_Logic>>(logic);
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = (p_config.query_op == QueryOp::ADD)
                ? std::unique_ptr<INativeTask>(std::make_unique<QueryTask<T_Logic, QueryOp::ADD>>(logic))
                : std::unique_ptr<INativeTask>(std::make_unique<QueryTask<T_Logic, QueryOp::CULL>>(logic));
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = std::make_unique<MetadataTask<T_Logic>>(logic);
        }
    }

    r_graph.configure_native_interface(node_id, std::move(native_task));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        std::array<GrantPartPOD, 4> reqs = {{
            make_slot_requirement(Descriptor::default_grants[0].slot, Descriptor::default_grants[0].access),
            make_slot_requirement(Descriptor::default_grants[1].slot, Descriptor::default_grants[1].access),
            make_slot_requirement(Descriptor::default_grants[2].slot, Descriptor::default_grants[2].access),
            make_slot_requirement(Descriptor::default_grants[3].slot, Descriptor::default_grants[3].access)
        }};
        const uint32_t target_id = logic.get_target_buffer_id();
        if (target_id != INVALID_ID && reqs[0].buffer_id != target_id) {
            reqs[0] = make_buffer_requirement(target_id, Descriptor::default_grants[0].access);
        }
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

#define INSTANTIATE_NARRATIVE_TASK_NODE(LogicType) \
    template NodeID NarrativeDOD::add_narrative_task_node<LogicType>(TaskGraphDOD&, const LogicType&, const NarrativesNodeExecutionConfig&) const

INSTANTIATE_NARRATIVE_TASK_NODE(NarremeSymbolicSimilarityTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(RoleBestFitAssignmentTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(SymbolicReferenceResolveTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(GameplayConditionBridgeTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(NarrativeConditionEvaluateTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(CausalConditionWavePropagateTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(IncidentStatusResolveTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(PlotStatusEvaluateTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(CharacterGoalPlotSyncTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(NarremeLocationTransferTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(NarrativeResourceCommitTransformLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(LocationOccupancyQueryLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(NarremeConditionBitmaskQueryLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(RelationshipGraphTraversalQueryLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(CausalDependencyDepthMetadataLogic<>);
INSTANTIATE_NARRATIVE_TASK_NODE(StoryArcIslandClusterMetadataLogic<>);

#undef INSTANTIATE_NARRATIVE_TASK_NODE

NodeID NarrativeDOD::add_transform_node_by_id(TaskGraphDOD& r_graph, NarrativesTransformLogicID p_id, const godot::Dictionary& p_props, const NarrativesNodeExecutionConfig& p_cfg) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_narrative_task_node(r_graph, l, p_cfg);
    };
    switch (p_id) {
        case NarrativesTransformLogicID::CausalConditionWavePropagate: return build.template operator()<CausalConditionWavePropagateTransformLogic<>>();
        case NarrativesTransformLogicID::CharacterGoalPlotSync:        return build.template operator()<CharacterGoalPlotSyncTransformLogic<>>();
        case NarrativesTransformLogicID::GameplayConditionBridge:      return build.template operator()<GameplayConditionBridgeTransformLogic<>>();
        case NarrativesTransformLogicID::IncidentStatusResolve:        return build.template operator()<IncidentStatusResolveTransformLogic<>>();
        case NarrativesTransformLogicID::NarrativeConditionEvaluate:   return build.template operator()<NarrativeConditionEvaluateTransformLogic<>>();
        case NarrativesTransformLogicID::NarrativeResourceCommit:      return build.template operator()<NarrativeResourceCommitTransformLogic<>>();
        case NarrativesTransformLogicID::NarremeLocationTransfer:      return build.template operator()<NarremeLocationTransferTransformLogic<>>();
        case NarrativesTransformLogicID::NarremeSymbolicSimilarity:    return build.template operator()<NarremeSymbolicSimilarityTransformLogic<>>();
        case NarrativesTransformLogicID::PlotStatusEvaluate:           return build.template operator()<PlotStatusEvaluateTransformLogic<>>();
        case NarrativesTransformLogicID::RoleBestFitAssignment:        return build.template operator()<RoleBestFitAssignmentTransformLogic<>>();
        case NarrativesTransformLogicID::SymbolicReferenceResolve:     return build.template operator()<SymbolicReferenceResolveTransformLogic<>>();
        default:                                                       return INVALID_ID;
    }
}

NodeID NarrativeDOD::add_query_node_by_id(TaskGraphDOD& r_graph, NarrativesQueryLogicID p_id, const godot::Dictionary& p_props, const NarrativesNodeExecutionConfig& p_cfg) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_narrative_task_node(r_graph, l, p_cfg);
    };
    switch (p_id) {
        case NarrativesQueryLogicID::LocationOccupancy:          return build.template operator()<LocationOccupancyQueryLogic<>>();
        case NarrativesQueryLogicID::NarremeConditionBitmask:    return build.template operator()<NarremeConditionBitmaskQueryLogic<>>();
        case NarrativesQueryLogicID::RelationshipGraphTraversal: return build.template operator()<RelationshipGraphTraversalQueryLogic<>>();
        default:                                                 return INVALID_ID;
    }
}

NodeID NarrativeDOD::add_metadata_node_by_id(TaskGraphDOD& r_graph, NarrativesMetadataLogicID p_id, const godot::Dictionary& p_props, const NarrativesNodeExecutionConfig& p_cfg) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_props.is_empty()) l.apply_properties(p_props);
        return add_narrative_task_node(r_graph, l, p_cfg);
    };
    switch (p_id) {
        case NarrativesMetadataLogicID::CausalDependencyDepth: return build.template operator()<CausalDependencyDepthMetadataLogic<>>();
        case NarrativesMetadataLogicID::StoryArcIslandCluster: return build.template operator()<StoryArcIslandClusterMetadataLogic<>>();
        default:                                               return INVALID_ID;
    }
}

NarrativesPipelineTopology NarrativeDOD::inject_pipeline_stages(TaskGraphDOD& r_graph, uint32_t p_stage_mask, bool p_auto_connect_stages) {
    NarrativesPipelineTopology topo{};
    bool prev_was_parallel = false;

    auto append_stage = [&](NarrativesPipelineStageFlags p_flag, const std::vector<NodeID>& p_nodes, bool p_chain_internal) {
        if (p_nodes.empty()) return;
        NarrativesStageAnchor anchor{p_flag, p_nodes.front(), p_nodes.back(), p_nodes};

        if (p_chain_internal) {
            for (size_t i = 0; i + 1 < p_nodes.size(); ++i) r_graph.connect_nodes(p_nodes[i], 0, p_nodes[i + 1], 0);
        }
        if (p_auto_connect_stages && !topo.stages.empty()) {
            const NarrativesStageAnchor& prev = topo.stages.back();
            const std::vector<NodeID> up_exits = prev_was_parallel ? prev.nodes : std::vector<NodeID>{prev.exit_node};
            const std::vector<NodeID> down_entries = p_chain_internal ? std::vector<NodeID>{anchor.entry_node} : anchor.nodes;
            for (NodeID u : up_exits) {
                for (NodeID d : down_entries) r_graph.connect_nodes(u, 0, d, 0);
            }
        }
        if (topo.first_node == INVALID_ID) topo.first_node = anchor.entry_node;
        topo.last_node    = anchor.exit_node;
        prev_was_parallel = !p_chain_internal;
        topo.stages.push_back(std::move(anchor));
    };

    if (p_stage_mask & NARRATIVES_STAGE_SPATIAL_TRANSFERS) {
        append_stage(NARRATIVES_STAGE_SPATIAL_TRANSFERS, {add_location_transfer_node(r_graph), add_location_occupancy_query_node(r_graph)}, true);
    }
    if (p_stage_mask & NARRATIVES_STAGE_SYMBOLIC_CASTING) {
        append_stage(NARRATIVES_STAGE_SYMBOLIC_CASTING, {add_symbolic_similarity_node(r_graph), add_role_best_fit_node(r_graph), add_symbolic_reference_resolve_node(r_graph)}, true);
    }
    if (p_stage_mask & NARRATIVES_STAGE_TOPOLOGY_METADATA) {
        append_stage(NARRATIVES_STAGE_TOPOLOGY_METADATA, {add_causal_depth_metadata_node(r_graph), add_story_arc_cluster_metadata_node(r_graph)}, false);
    }
    if (p_stage_mask & NARRATIVES_STAGE_ATOMIC_CONDITIONS) {
        append_stage(NARRATIVES_STAGE_ATOMIC_CONDITIONS, {add_gameplay_condition_bridge_node(r_graph), add_narrative_condition_evaluate_node(r_graph)}, false);
    }
    if (p_stage_mask & NARRATIVES_STAGE_CAUSAL_PROPAGATION) {
        append_stage(NARRATIVES_STAGE_CAUSAL_PROPAGATION, {add_causal_wave_propagate_node(r_graph), add_condition_bitmask_query_node(r_graph)}, true);
    }
    if (p_stage_mask & NARRATIVES_STAGE_INCIDENT_AND_PLOT_RESOLVE) {
        append_stage(NARRATIVES_STAGE_INCIDENT_AND_PLOT_RESOLVE, {add_incident_status_resolve_node(r_graph), add_plot_status_evaluate_node(r_graph), add_character_goal_sync_node(r_graph)}, true);
    }
    if (p_stage_mask & NARRATIVES_STAGE_RESOURCE_COMMIT) {
        append_stage(NARRATIVES_STAGE_RESOURCE_COMMIT, {add_resource_commit_node(r_graph)}, true);
    }

    active_topology_ = topo;
    return topo;
}

bool NarrativeDOD::schedule_node_with_stage(TaskGraphDOD& r_graph, const NarrativesPipelineTopology& p_topology, NarrativesPipelineStageFlags p_target_stage, NodeID p_custom_node, bool p_join_downstream) {
    if (p_custom_node == INVALID_ID) return false;
    for (size_t i = 0; i < p_topology.stages.size(); ++i) {
        if (p_topology.stages[i].stage == p_target_stage) {
            if (i > 0 && p_topology.stages[i - 1].exit_node != INVALID_ID) r_graph.connect_nodes(p_topology.stages[i - 1].exit_node, 0, p_custom_node, 0);
            if (p_join_downstream && (i + 1) < p_topology.stages.size() && p_topology.stages[i + 1].entry_node != INVALID_ID) r_graph.connect_nodes(p_custom_node, 0, p_topology.stages[i + 1].entry_node, 0);
            return true;
        }
    }
    return false;
}

bool NarrativeDOD::schedule_node_between_stages(TaskGraphDOD& r_graph, const NarrativesPipelineTopology& p_topology, NarrativesPipelineStageFlags p_upstream_stage, NarrativesPipelineStageFlags p_downstream_stage, NodeID p_custom_node) {
    if (p_custom_node == INVALID_ID) return false;
    const NarrativesStageAnchor* up   = p_topology.get_stage_anchor(p_upstream_stage);
    const NarrativesStageAnchor* down = p_topology.get_stage_anchor(p_downstream_stage);
    if (!up || !down || !up->is_valid() || !down->is_valid()) return false;
    r_graph.connect_nodes(up->exit_node, 0, p_custom_node, 0);
    r_graph.connect_nodes(p_custom_node, 0, down->entry_node, 0);
    return true;
}

void NarrativeDOD::execute_step(double p_delta, TaskGraphDOD* p_override_graph) {
    sync_selection_ranges();
    if (TaskGraphDOD* target_graph = p_override_graph ? p_override_graph : default_task_graph_.get()) {
        if (target_graph->get_node_count() > 0) {
            MemoryGrantPOD topo_grant{};
            if (target_graph->bake_topology_grant(topo_grant) && memory_manager_) {
                memory_manager_->release_grant(topo_grant);
            }
        }
        target_graph->execute_graph_dod(p_delta);
    }
}

} // namespace ideam::core
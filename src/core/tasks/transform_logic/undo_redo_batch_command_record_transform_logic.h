#pragma once

/**
 * undo_redo_batch_command_record_transform_logic.h
 * Ecosystem Companion Bridge (UT2): Reversible Boundary Snapshot & Delta Recording for `IdeamUndoRedo`.
 *
 * Compares `current_state_buffer_id` against `previous_state_buffer_id`, packages modified
 * elements into compact `UndoRedoDeltaRecordPOD` entries inside `undo_record_ring_buffer_id`,
 * supports live forward/backward simulation scrubbing (`Record Deltas`, `Apply Undo`, `Apply Redo`),
 * and optionally commits a grouped reversible action to a bound `IdeamUndoRedo` wrapper.
 */

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../../godot/utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_UNDO_REDO_BATCH_RECORD_POD_DEFINED
#define IDEAM_UNDO_REDO_BATCH_RECORD_POD_DEFINED

enum UndoRedoBatchOperationMode : uint8_t {
    UNDO_REDO_BATCH_RECORD_DELTAS = 0, // Diff current vs previous, append UndoRedoDeltaRecordPODs, optionally sync previous
    UNDO_REDO_BATCH_APPLY_UNDO    = 1, // Scrub backward: restore before_bits for records matching action_group_id
    UNDO_REDO_BATCH_APPLY_REDO    = 2  // Scrub forward: reapply after_bits for records matching action_group_id
};

/**
 * UndoRedoDeltaRecordPOD (32 Bytes, 16-Byte Aligned)
 * Compact reversible element mutation record for live simulation scrubbing and `IdeamUndoRedo` commits.
 */
struct alignas(16) UndoRedoDeltaRecordPOD {
    uint64_t before_bits    = 0ULL;       // Raw 64-bit snapshot of element value before mutation
    uint64_t after_bits     = 0ULL;       // Raw 64-bit snapshot of element value after mutation
    uint32_t buffer_id      = INVALID_ID; // Target MemoryManagerDOD buffer_id
    uint32_t element_index  = 0u;         // Element index within the buffer
    uint32_t action_group_id= 1u;         // Transaction / frame scrub group identifier
    uint32_t byte_width     = 4u;         // Active element byte width (1, 2, 4, or 8 bytes)
};

static_assert(sizeof(UndoRedoDeltaRecordPOD) == 32, "UndoRedoDeltaRecordPOD must be 32 bytes!");

#endif // IDEAM_UNDO_REDO_BATCH_RECORD_POD_DEFINED

template <typename T = float, typename T_Strategy = FlatStrategy>
struct alignas(64) UndoRedoBatchCommandRecordTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "UndoRedo Batch Command Record";
    static constexpr std::string_view ui_category  = "Utilities/UndoRedo";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint64_t undo_redo_object_id        = 0ULL;                          // 8B: Optional IdeamUndoRedo RefCounted ObjectID
    uint64_t target_context_object_id   = 0ULL;                          // 8B: Optional context ObjectID for IdeamUndoRedo property commit -> 16B

    uint32_t current_state_buffer_id    = INVALID_ID;                    // 4B: Live state buffer (T)
    uint32_t previous_state_buffer_id   = INVALID_ID;                    // 4B: Baseline snapshot state buffer (T)
    uint32_t undo_record_ring_buffer_id = INVALID_ID;                    // 4B: Output/Input UndoRedoDeltaRecordPOD (32B) buffer
    uint32_t ring_cursor_buffer_id      = INVALID_ID;                    // 4B: Optional uint32_t[2] [read_idx, write_idx] cursor buffer
    uint32_t recorded_count_buffer_id   = INVALID_ID;                    // 4B: Optional uint32_t counter of delta records processed
    uint32_t action_group_id            = 1u;                            // 4B: Transaction / scrub frame ID tag
    uint32_t max_records_per_commit     = 0u;                            // 4B: Max delta records per execution (0 = unlimited)
    float change_epsilon                = 1e-5f;                         // 4B: Floating-point delta threshold -> 48B

    uint8_t operation_mode              = UNDO_REDO_BATCH_RECORD_DELTAS; // 1B: UndoRedoBatchOperationMode (0..2)
    uint8_t merge_mode                  = 0u;                            // 1B: godot::UndoRedo::MergeMode (0 = MERGE_DISABLE)
    bool sync_previous_on_record        = true;                          // 1B: Copy current -> previous after recording deltas
    bool commit_to_ideam_undo_redo      = true;                          // 1B: Commit action to IdeamUndoRedo if undo_redo_object_id is valid -> 52B
    uint8_t reserved_padding[12]        = {0};                           // 12B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("current_state_buffer_id");
        add_buf("previous_state_buffer_id");
        add_buf("undo_record_ring_buffer_id");
        add_buf("ring_cursor_buffer_id");
        add_buf("recorded_count_buffer_id");

        godot::Dictionary mode_prop;
        mode_prop["name"]        = "operation_mode";
        mode_prop["type"]        = godot::Variant::INT;
        mode_prop["hint"]        = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Record Deltas:0,Apply Undo (Restore Previous):1,Apply Redo (Reapply After):2";
        props.push_back(mode_prop);

        godot::Dictionary grp_prop;
        grp_prop["name"] = "action_group_id";
        grp_prop["type"] = godot::Variant::INT;
        grp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(grp_prop);

        godot::Dictionary eps_prop;
        eps_prop["name"]        = "change_epsilon";
        eps_prop["type"]        = godot::Variant::FLOAT;
        eps_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        eps_prop["hint_string"] = "0.0,1.0,0.00001";
        props.push_back(eps_prop);

        godot::Dictionary max_prop;
        max_prop["name"]        = "max_records_per_commit";
        max_prop["type"]        = godot::Variant::INT;
        max_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        max_prop["hint_string"] = "0,65536,1";
        props.push_back(max_prop);

        godot::Dictionary ur_prop;
        ur_prop["name"] = "undo_redo_object_id";
        ur_prop["type"] = godot::Variant::INT;
        ur_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ur_prop);

        godot::Dictionary ctx_prop;
        ctx_prop["name"] = "target_context_object_id";
        ctx_prop["type"] = godot::Variant::INT;
        ctx_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ctx_prop);

        godot::Dictionary mrg_prop;
        mrg_prop["name"]        = "merge_mode";
        mrg_prop["type"]        = godot::Variant::INT;
        mrg_prop["hint"]        = godot::PROPERTY_HINT_ENUM;
        mrg_prop["hint_string"] = "Merge Disable:0,Merge Ends:1,Merge All:2";
        props.push_back(mrg_prop);

        godot::Dictionary syn_prop;
        syn_prop["name"] = "sync_previous_on_record";
        syn_prop["type"] = godot::Variant::BOOL;
        syn_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(syn_prop);

        godot::Dictionary cmt_prop;
        cmt_prop["name"] = "commit_to_ideam_undo_redo";
        cmt_prop["type"] = godot::Variant::BOOL;
        cmt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cmt_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (current_state_buffer_id != INVALID_ID)    return current_state_buffer_id;
        if (previous_state_buffer_id != INVALID_ID)   return previous_state_buffer_id;
        return undo_record_ring_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("current_state_buffer_id")) {
            current_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_state_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && current_state_buffer_id == INVALID_ID) {
            current_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("previous_state_buffer_id")) {
            previous_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["previous_state_buffer_id"]));
        }
        if (p_props.has("undo_record_ring_buffer_id")) {
            undo_record_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["undo_record_ring_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("recorded_count_buffer_id")) {
            recorded_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["recorded_count_buffer_id"]));
        }
        if (p_props.has("action_group_id")) {
            action_group_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["action_group_id"]));
        }
        if (p_props.has("max_records_per_commit")) {
            max_records_per_commit = static_cast<uint32_t>(std::max<int64_t>(0, static_cast<int64_t>(p_props["max_records_per_commit"])));
        }
        if (p_props.has("change_epsilon")) {
            change_epsilon = std::max(0.0f, static_cast<float>(p_props["change_epsilon"]));
        }
        if (p_props.has("undo_redo_object_id")) {
            undo_redo_object_id = static_cast<uint64_t>(static_cast<int64_t>(p_props["undo_redo_object_id"]));
        }
        if (p_props.has("target_context_object_id")) {
            target_context_object_id = static_cast<uint64_t>(static_cast<int64_t>(p_props["target_context_object_id"]));
        }
        if (p_props.has("operation_mode")) {
            operation_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["operation_mode"]), 0, 2));
        }
        if (p_props.has("merge_mode")) {
            merge_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["merge_mode"]), 0, 2));
        }
        if (p_props.has("sync_previous_on_record")) {
            sync_previous_on_record = static_cast<bool>(p_props["sync_previous_on_record"]);
        }
        if (p_props.has("commit_to_ideam_undo_redo")) {
            commit_to_ideam_undo_redo = static_cast<bool>(p_props["commit_to_ideam_undo_redo"]);
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        const GrantPartPOD* cur_part  = (current_state_buffer_id != INVALID_ID)    ? p_context.get_grant_part(current_state_buffer_id)    : nullptr;
        const GrantPartPOD* prev_part = (previous_state_buffer_id != INVALID_ID)   ? p_context.get_grant_part(previous_state_buffer_id)   : nullptr;
        const GrantPartPOD* rec_part  = (undo_record_ring_buffer_id != INVALID_ID) ? p_context.get_grant_part(undo_record_ring_buffer_id) : nullptr;
        const GrantPartPOD* rcur_part = (ring_cursor_buffer_id != INVALID_ID)      ? p_context.get_grant_part(ring_cursor_buffer_id)      : nullptr;
        const GrantPartPOD* cnt_part  = (recorded_count_buffer_id != INVALID_ID)   ? p_context.get_grant_part(recorded_count_buffer_id)   : nullptr;

        if (!cur_part || !cur_part->raw_base_ptr) return;

        uint8_t* __restrict cur_raw  = cur_part->raw_base_ptr;
        uint8_t* __restrict prev_raw = (prev_part && prev_part->raw_base_ptr) ? prev_part->raw_base_ptr : nullptr;
        UndoRedoDeltaRecordPOD* __restrict records = (rec_part && rec_part->raw_base_ptr)
            ? reinterpret_cast<UndoRedoDeltaRecordPOD*>(rec_part->raw_base_ptr)
            : nullptr;
        uint32_t* __restrict cursors = (rcur_part && rcur_part->raw_base_ptr && rcur_part->capacity_bytes >= 2 * sizeof(uint32_t))
            ? reinterpret_cast<uint32_t*>(rcur_part->raw_base_ptr)
            : nullptr;
        uint32_t* __restrict out_counter = (cnt_part && cnt_part->raw_base_ptr)
            ? reinterpret_cast<uint32_t*>(cnt_part->raw_base_ptr)
            : nullptr;

        const uint32_t elem_stride = (cur_part->element_stride > 0u) ? cur_part->element_stride : static_cast<uint32_t>(sizeof(T));
        const uint32_t copy_width  = std::min<uint32_t>(elem_stride, static_cast<uint32_t>(sizeof(uint64_t)));
        const int64_t cur_cap      = static_cast<int64_t>(cur_part->capacity_bytes / elem_stride);
        const int64_t prev_cap     = prev_raw ? static_cast<int64_t>(prev_part->capacity_bytes / elem_stride) : 0;
        const uint32_t rec_cap     = records ? static_cast<uint32_t>(rec_part->capacity_bytes / sizeof(UndoRedoDeltaRecordPOD)) : 0u;

        // Mode 1 (Apply Undo) & Mode 2 (Apply Redo): Scrub state directly from UndoRedoDeltaRecordPOD buffer
        if (operation_mode == UNDO_REDO_BATCH_APPLY_UNDO || operation_mode == UNDO_REDO_BATCH_APPLY_REDO) {
            if (!records || rec_cap == 0u) return;
            const uint32_t total_recs = cursors
                ? std::min<uint32_t>(cursors[1], rec_cap)
                : static_cast<uint32_t>(std::clamp<int64_t>(rec_part->selection.element_count, 0, static_cast<int64_t>(rec_cap)));

            uint32_t applied = 0u;
            const bool is_undo = (operation_mode == UNDO_REDO_BATCH_APPLY_UNDO);

            for (uint32_t i = 0u; i < total_recs; ++i) {
                const uint32_t r_idx = is_undo ? (total_recs - 1u - i) : i;
                const UndoRedoDeltaRecordPOD& rec = records[r_idx];
                if (action_group_id != 0u && rec.action_group_id != action_group_id) continue;
                if (rec.buffer_id != INVALID_ID && rec.buffer_id != current_state_buffer_id) continue;

                const int64_t e_idx = static_cast<int64_t>(rec.element_index);
                if (e_idx < 0 || e_idx >= cur_cap) continue;

                const uint64_t bits = is_undo ? rec.before_bits : rec.after_bits;
                const uint32_t w    = std::min<uint32_t>(rec.byte_width, copy_width);
                std::memcpy(cur_raw + static_cast<size_t>(e_idx) * elem_stride, &bits, w);
                if (prev_raw && e_idx < prev_cap) {
                    std::memcpy(prev_raw + static_cast<size_t>(e_idx) * elem_stride, &bits, w);
                }
                ++applied;
            }

            if (out_counter) {
                *out_counter = applied;
            }
            return;
        }

        // Mode 0: Record Deltas between current_state_buffer_id and previous_state_buffer_id
        if (!prev_raw) return;
        const int64_t n_elems = std::min(cur_cap, prev_cap);
        if (n_elems <= 0) return;

        const uint32_t limit = (max_records_per_commit > 0u)
            ? max_records_per_commit
            : (rec_cap > 0u ? rec_cap : static_cast<uint32_t>(n_elems));
        uint32_t recorded = 0u;
        uint32_t write_cursor = (cursors && rec_cap > 0u) ? (cursors[1] % rec_cap) : 0u;

        auto has_changed = [&](const uint8_t* a_ptr, const uint8_t* b_ptr) noexcept -> bool {
            if constexpr (std::is_floating_point_v<T>) {
                const T a = *reinterpret_cast<const T*>(a_ptr);
                const T b = *reinterpret_cast<const T*>(b_ptr);
                return std::abs(static_cast<double>(a) - static_cast<double>(b)) > static_cast<double>(change_epsilon);
            } else {
                return std::memcmp(a_ptr, b_ptr, copy_width) != 0;
            }
        };

        auto eval_elem = [&](int64_t idx) noexcept {
            if (recorded >= limit) return;
            uint8_t* c_ptr = cur_raw + static_cast<size_t>(idx) * elem_stride;
            uint8_t* p_ptr = prev_raw + static_cast<size_t>(idx) * elem_stride;

            if (has_changed(c_ptr, p_ptr)) {
                if (records && rec_cap > 0u) {
                    UndoRedoDeltaRecordPOD rec{};
                    std::memcpy(&rec.before_bits, p_ptr, copy_width);
                    std::memcpy(&rec.after_bits,  c_ptr, copy_width);
                    rec.buffer_id       = current_state_buffer_id;
                    rec.element_index   = static_cast<uint32_t>(idx);
                    rec.action_group_id = action_group_id;
                    rec.byte_width      = copy_width;

                    records[write_cursor] = rec;
                    write_cursor = (write_cursor + 1u) % rec_cap;
                }
                if (sync_previous_on_record) {
                    std::memcpy(p_ptr, c_ptr, elem_stride);
                }
                ++recorded;
            }
        };

        const MemoryBufferSelectionPOD& sel = cur_part->selection;
        if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
            const int64_t cap = std::min<int64_t>(sel.capacity, n_elems);
            for (int64_t i = 0; i < cap; ++i) {
                if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) eval_elem(i);
            }
        } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
            for (int64_t k = 0; k < sel.element_count; ++k) {
                const int64_t idx = sel.data.indices[k];
                if (idx >= 0 && idx < n_elems) eval_elem(idx);
            }
        } else {
            const int64_t start = std::max<int64_t>(0, sel.start_index);
            const int64_t end   = std::min<int64_t>(start + (sel.element_count > 0 ? sel.element_count : n_elems), n_elems);
            for (int64_t i = start; i < end; ++i) eval_elem(i);
        }

        if (records && rec_part) {
            if (cursors) {
                cursors[1] = write_cursor;
            } else {
                const_cast<GrantPartPOD*>(rec_part)->selection.element_count = static_cast<int64_t>(recorded);
            }
        }

        if (out_counter) {
            *out_counter = recorded;
        }

        if (recorded > 0u && commit_to_ideam_undo_redo && undo_redo_object_id != 0ULL) {
            if (auto* ur = godot::Object::cast_to<godot_ext::IdeamUndoRedo>(godot::ObjectDB::get_instance(godot::ObjectID(undo_redo_object_id)))) {
                godot::Object* ctx_obj = (target_context_object_id != 0ULL)
                    ? godot::ObjectDB::get_instance(godot::ObjectID(target_context_object_id))
                    : nullptr;
                const auto m_mode = static_cast<godot::UndoRedo::MergeMode>(std::clamp<uint8_t>(merge_mode, 0u, 2u));
                ur->create_action(
                    godot::String("DOD Simulation Scrub Group #") + godot::String::num_int64(action_group_id),
                    m_mode,
                    ctx_obj
                );
                if (ctx_obj) {
                    ur->add_do_property(ctx_obj, godot::StringName("last_undo_redo_group_id"), static_cast<int64_t>(action_group_id));
                    ur->add_undo_property(ctx_obj, godot::StringName("last_undo_redo_group_id"), static_cast<int64_t>(action_group_id > 0u ? action_group_id - 1u : 0u));
                }
                ur->commit_action(false);
            }
        }
    }
};

static_assert(sizeof(UndoRedoBatchCommandRecordTransformLogic<>) == 64, "UndoRedoBatchCommandRecordTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<UndoRedoBatchCommandRecordTransformLogic<>>, "UndoRedoBatchCommandRecordTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core

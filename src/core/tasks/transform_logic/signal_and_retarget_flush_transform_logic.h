#pragma once

/**
 * signal_and_retarget_flush_transform_logic.h
 * Ecosystem Companion Bridge (UT1): Main-Thread Ring Buffer Flush for `SignalConnector` & `NodeRetargeter`.
 *
 * Drains `SignalRetargetCommandPOD` entries from a `RingView` / `RingStrategy` command buffer at
 * wave end to safely execute `SignalConnector` signal emissions/connections, `NodeRetargeter`
 * property retargeting, and scalar/object property writes on the main thread, while optionally
 * mirroring applied scalar payloads into a DOD target column (`retarget_mirror_buffer_id`).
 */

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/ring_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../../godot/utilities/signal_connector.h"
#include "../../../godot/utilities/node_retargeter.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_SIGNAL_RETARGET_COMMAND_POD_DEFINED
#define IDEAM_SIGNAL_RETARGET_COMMAND_POD_DEFINED

enum SignalRetargetCommandType : uint32_t {
    SIGNAL_RETARGET_CMD_EMIT_INDEXED_SIGNAL = 0, // Emits signal at SignalConnector::destination_signals[slot_index] on target_object_id
    SIGNAL_RETARGET_CMD_SET_PROPERTY_FLOAT  = 1, // Writes payload_float to NodeRetargeter::destination_retarget_properties[slot_index]
    SIGNAL_RETARGET_CMD_SET_PROPERTY_INT    = 2, // Writes payload_int to NodeRetargeter::destination_retarget_properties[slot_index]
    SIGNAL_RETARGET_CMD_SET_PROPERTY_OBJECT = 3, // Writes secondary_object_id Node to destination_retarget_properties[slot_index]
    SIGNAL_RETARGET_CMD_CONNECT_ALL         = 4, // Invokes SignalConnector::connect_signals(source_root, destination_root)
    SIGNAL_RETARGET_CMD_DISCONNECT_ALL      = 5, // Invokes SignalConnector::disconnect_signals(source_root, destination_root)
    SIGNAL_RETARGET_CMD_RETARGET_ALL        = 6, // Invokes NodeRetargeter::retarget(source_root, destination_root)
    SIGNAL_RETARGET_CMD_CLEAR_RETARGETS     = 7  // Invokes NodeRetargeter::clear_set_targets(source_root, destination_root)
};

/**
 * SignalRetargetCommandPOD (32 Bytes, 16-Byte Aligned)
 * Lock-free ring command packet queued by worker waves and flushed by UT1 at wave end.
 */
struct alignas(16) SignalRetargetCommandPOD {
    uint64_t target_object_id    = 0ULL;       // Target Node ObjectID (or 0 to use destination_root_object_id)
    uint64_t secondary_object_id = 0ULL;       // Secondary Node ObjectID (e.g., source_root or assigned Node reference)
    uint32_t command_type        = 0u;         // SignalRetargetCommandType (0..7)
    uint32_t slot_index          = 0u;         // Slot index into SignalConnector / NodeRetargeter arrays or mirror buffer
    float payload_float          = 0.0f;       // Scalar float payload for signal argument / property write
    int32_t payload_int          = 0;          // Scalar int32 payload for signal argument / property write
};

static_assert(sizeof(SignalRetargetCommandPOD) == 32, "SignalRetargetCommandPOD must be 32 bytes!");

#endif // IDEAM_SIGNAL_RETARGET_COMMAND_POD_DEFINED

template <typename T = void*, typename T_Strategy = RingStrategy>
struct alignas(64) SignalAndRetargetFlushTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = RingView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::QUEUE_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::RING;
    static constexpr DataType required_types              = DataType::CUSTOM | DataType::INT32 | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Signal And Retarget Flush";
    static constexpr std::string_view ui_category  = "Utilities/Bridge";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint64_t signal_connector_object_id   = 0ULL;       // 8B: Bound SignalConnector Resource ObjectID
    uint64_t node_retargeter_object_id    = 0ULL;       // 8B: Bound NodeRetargeter Resource ObjectID
    uint64_t source_root_object_id        = 0ULL;       // 8B: Default source_root Node ObjectID
    uint64_t destination_root_object_id   = 0ULL;       // 8B: Default destination_root Node ObjectID -> 32B

    uint32_t command_ring_buffer_id       = INVALID_ID; // 4B: Primary SignalRetargetCommandPOD ring buffer
    uint32_t ring_cursor_buffer_id        = INVALID_ID; // 4B: Optional uint32_t[2] [read_idx, write_idx] cursor buffer
    uint32_t retarget_mirror_buffer_id    = INVALID_ID; // 4B: Optional float/int32 DOD column mirroring retargeted values
    uint32_t flushed_count_buffer_id      = INVALID_ID; // 4B: Optional uint32_t counter recording commands flushed
    uint32_t max_flush_per_frame          = 0u;         // 4B: Max commands to drain per tick (0 = drain all)
    mutable uint32_t fallback_read_index  = 0u;         // 4B: Fallback RingView read cursor
    mutable uint32_t fallback_write_index = 0u;         // 4B: Fallback RingView write cursor -> 60B

    bool flush_signals                    = true;       // 1B: Execute SignalConnector / signal emission commands
    bool flush_retargets                  = true;       // 1B: Execute NodeRetargeter / property write commands
    bool wake_retargeted_indices          = false;      // 1B: Queue slot_index into wave_commands when mirrored
    bool reset_selection_count_on_drain   = true;       // 1B: Reset selection.element_count to 0 after draining -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("command_ring_buffer_id");
        add_buf("ring_cursor_buffer_id");
        add_buf("retarget_mirror_buffer_id");
        add_buf("flushed_count_buffer_id");

        auto add_int = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint"] = godot::PROPERTY_HINT_NONE;
            props.push_back(d);
        };
        add_int("signal_connector_object_id");
        add_int("node_retargeter_object_id");
        add_int("source_root_object_id");
        add_int("destination_root_object_id");

        godot::Dictionary max_prop;
        max_prop["name"]        = "max_flush_per_frame";
        max_prop["type"]        = godot::Variant::INT;
        max_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        max_prop["hint_string"] = "0,4096,1";
        props.push_back(max_prop);

        godot::Dictionary fs_prop;
        fs_prop["name"] = "flush_signals";
        fs_prop["type"] = godot::Variant::BOOL;
        fs_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(fs_prop);

        godot::Dictionary fr_prop;
        fr_prop["name"] = "flush_retargets";
        fr_prop["type"] = godot::Variant::BOOL;
        fr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(fr_prop);

        godot::Dictionary wk_prop;
        wk_prop["name"] = "wake_retargeted_indices";
        wk_prop["type"] = godot::Variant::BOOL;
        wk_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wk_prop);

        godot::Dictionary rst_prop;
        rst_prop["name"] = "reset_selection_count_on_drain";
        rst_prop["type"] = godot::Variant::BOOL;
        rst_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rst_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (command_ring_buffer_id != INVALID_ID) return command_ring_buffer_id;
        return retarget_mirror_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { p_view.read_index_ptr; p_view.write_index_ptr; }) {
            if (ring_cursor_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* rcur = p_context.get_grant_part(ring_cursor_buffer_id)) {
                    if (rcur->raw_base_ptr && rcur->capacity_bytes >= 2 * sizeof(uint32_t)) {
                        uint32_t* cursors      = reinterpret_cast<uint32_t*>(rcur->raw_base_ptr);
                        p_view.read_index_ptr  = &cursors[0];
                        p_view.write_index_ptr = &cursors[1];
                        return;
                    }
                }
            }
            fallback_read_index    = 0u;
            fallback_write_index   = p_part ? static_cast<uint32_t>(std::max<int64_t>(0, p_part->selection.element_count)) : 0u;
            p_view.read_index_ptr  = &fallback_read_index;
            p_view.write_index_ptr = &fallback_write_index;
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("command_ring_buffer_id")) {
            command_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["command_ring_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && command_ring_buffer_id == INVALID_ID) {
            command_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("retarget_mirror_buffer_id")) {
            retarget_mirror_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["retarget_mirror_buffer_id"]));
        }
        if (p_props.has("flushed_count_buffer_id")) {
            flushed_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["flushed_count_buffer_id"]));
        }
        if (p_props.has("signal_connector_object_id")) {
            signal_connector_object_id = static_cast<uint64_t>(static_cast<int64_t>(p_props["signal_connector_object_id"]));
        }
        if (p_props.has("node_retargeter_object_id")) {
            node_retargeter_object_id = static_cast<uint64_t>(static_cast<int64_t>(p_props["node_retargeter_object_id"]));
        }
        if (p_props.has("source_root_object_id")) {
            source_root_object_id = static_cast<uint64_t>(static_cast<int64_t>(p_props["source_root_object_id"]));
        }
        if (p_props.has("destination_root_object_id")) {
            destination_root_object_id = static_cast<uint64_t>(static_cast<int64_t>(p_props["destination_root_object_id"]));
        }
        if (p_props.has("max_flush_per_frame")) {
            max_flush_per_frame = static_cast<uint32_t>(std::max<int64_t>(0, static_cast<int64_t>(p_props["max_flush_per_frame"])));
        }
        if (p_props.has("flush_signals")) {
            flush_signals = static_cast<bool>(p_props["flush_signals"]);
        }
        if (p_props.has("flush_retargets")) {
            flush_retargets = static_cast<bool>(p_props["flush_retargets"]);
        }
        if (p_props.has("wake_retargeted_indices")) {
            wake_retargeted_indices = static_cast<bool>(p_props["wake_retargeted_indices"]);
        }
        if (p_props.has("reset_selection_count_on_drain")) {
            reset_selection_count_on_drain = static_cast<bool>(p_props["reset_selection_count_on_drain"]);
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
        if constexpr (std::is_same_v<T, void*> || std::is_same_v<T, int32_t> || std::is_same_v<T, float>) {
            const uint32_t ring_id = get_target_buffer_id();
            const GrantPartPOD* ring_part = (ring_id != INVALID_ID) ? p_context.get_grant_part(ring_id) : nullptr;
            if (!ring_part || !ring_part->raw_base_ptr) return;

            const GrantPartPOD* rcur_part = (ring_cursor_buffer_id != INVALID_ID)     ? p_context.get_grant_part(ring_cursor_buffer_id)     : nullptr;
            const GrantPartPOD* mir_part  = (retarget_mirror_buffer_id != INVALID_ID) ? p_context.get_grant_part(retarget_mirror_buffer_id) : nullptr;
            const GrantPartPOD* cnt_part  = (flushed_count_buffer_id != INVALID_ID)   ? p_context.get_grant_part(flushed_count_buffer_id)   : nullptr;

            const SignalRetargetCommandPOD* __restrict ring_cmds =
                reinterpret_cast<const SignalRetargetCommandPOD*>(ring_part->raw_base_ptr);
            const uint32_t ring_cap = static_cast<uint32_t>(ring_part->capacity_bytes / sizeof(SignalRetargetCommandPOD));
            if (ring_cap == 0u) return;

            uint8_t* __restrict mir_raw = (mir_part && mir_part->raw_base_ptr) ? mir_part->raw_base_ptr : nullptr;
            const uint32_t mir_stride   = mir_part ? (mir_part->element_stride > 0u ? mir_part->element_stride : 4u) : 0u;
            const int64_t mir_cap       = (mir_part && mir_stride > 0u) ? static_cast<int64_t>(mir_part->capacity_bytes / mir_stride) : 0;

            uint32_t* __restrict flushed_counter = (cnt_part && cnt_part->raw_base_ptr)
                ? reinterpret_cast<uint32_t*>(cnt_part->raw_base_ptr)
                : nullptr;

            uint32_t* __restrict cursors = (rcur_part && rcur_part->raw_base_ptr && rcur_part->capacity_bytes >= 2 * sizeof(uint32_t))
                ? reinterpret_cast<uint32_t*>(rcur_part->raw_base_ptr)
                : nullptr;

            auto* sig_conn = (signal_connector_object_id != 0ULL)
                ? godot::Object::cast_to<godot_ext::SignalConnector>(godot::ObjectDB::get_instance(godot::ObjectID(signal_connector_object_id)))
                : nullptr;
            auto* node_ret = (node_retargeter_object_id != 0ULL)
                ? godot::Object::cast_to<godot_ext::NodeRetargeter>(godot::ObjectDB::get_instance(godot::ObjectID(node_retargeter_object_id)))
                : nullptr;
            auto* default_src_root = (source_root_object_id != 0ULL)
                ? godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(godot::ObjectID(source_root_object_id)))
                : nullptr;
            auto* default_dst_root = (destination_root_object_id != 0ULL)
                ? godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(godot::ObjectID(destination_root_object_id)))
                : nullptr;

            auto apply_command = [&](const SignalRetargetCommandPOD& cmd) noexcept {
                const int64_t sidx = static_cast<int64_t>(cmd.slot_index);

                // Mirror scalar payload into DOD column if bound
                if (mir_raw && sidx >= 0 && sidx < mir_cap) {
                    if (cmd.command_type == SIGNAL_RETARGET_CMD_SET_PROPERTY_FLOAT) {
                        reinterpret_cast<float*>(mir_raw)[sidx] = cmd.payload_float;
                    } else if (cmd.command_type == SIGNAL_RETARGET_CMD_SET_PROPERTY_INT ||
                               cmd.command_type == SIGNAL_RETARGET_CMD_EMIT_INDEXED_SIGNAL) {
                        reinterpret_cast<int32_t*>(mir_raw)[sidx] = cmd.payload_int;
                    }
                    if (wake_retargeted_indices && retarget_mirror_buffer_id != INVALID_ID) {
                        p_context.queue_selection_command(retarget_mirror_buffer_id, sidx);
                    }
                }

                godot::Node* tgt_node = (cmd.target_object_id != 0ULL)
                    ? godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(godot::ObjectID(cmd.target_object_id)))
                    : default_dst_root;
                godot::Node* sec_node = (cmd.secondary_object_id != 0ULL)
                    ? godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(godot::ObjectID(cmd.secondary_object_id)))
                    : default_src_root;

                switch (static_cast<SignalRetargetCommandType>(cmd.command_type)) {
                    case SIGNAL_RETARGET_CMD_EMIT_INDEXED_SIGNAL: {
                        if (!flush_signals || !tgt_node || !sig_conn) break;
                        const godot::TypedArray<godot::String> sigs = sig_conn->get_destination_signals();
                        if (sidx >= 0 && sidx < sigs.size()) {
                            const godot::StringName sig_name(static_cast<godot::String>(sigs[sidx]));
                            if (tgt_node->has_signal(sig_name)) {
                                tgt_node->emit_signal(sig_name, cmd.payload_int, cmd.payload_float);
                            }
                        }
                        break;
                    }
                    case SIGNAL_RETARGET_CMD_SET_PROPERTY_FLOAT:
                    case SIGNAL_RETARGET_CMD_SET_PROPERTY_INT:
                    case SIGNAL_RETARGET_CMD_SET_PROPERTY_OBJECT: {
                        if (!flush_retargets || !tgt_node || !node_ret) break;
                        const godot::TypedArray<godot::String> props = node_ret->get_destination_retarget_properties();
                        if (sidx >= 0 && sidx < props.size()) {
                            const godot::StringName prop_name(static_cast<godot::String>(props[sidx]));
                            if (cmd.command_type == SIGNAL_RETARGET_CMD_SET_PROPERTY_FLOAT) {
                                tgt_node->set(prop_name, cmd.payload_float);
                            } else if (cmd.command_type == SIGNAL_RETARGET_CMD_SET_PROPERTY_INT) {
                                tgt_node->set(prop_name, cmd.payload_int);
                            } else if (sec_node) {
                                tgt_node->set(prop_name, sec_node);
                            }
                        }
                        break;
                    }
                    case SIGNAL_RETARGET_CMD_CONNECT_ALL: {
                        if (flush_signals && sig_conn && sec_node && tgt_node) {
                            sig_conn->connect_signals(sec_node, tgt_node);
                        }
                        break;
                    }
                    case SIGNAL_RETARGET_CMD_DISCONNECT_ALL: {
                        if (flush_signals && sig_conn && sec_node && tgt_node) {
                            sig_conn->disconnect_signals(sec_node, tgt_node);
                        }
                        break;
                    }
                    case SIGNAL_RETARGET_CMD_RETARGET_ALL: {
                        if (flush_retargets && node_ret && sec_node && tgt_node) {
                            node_ret->retarget(sec_node, tgt_node);
                        }
                        break;
                    }
                    case SIGNAL_RETARGET_CMD_CLEAR_RETARGETS: {
                        if (flush_retargets && node_ret && sec_node && tgt_node) {
                            node_ret->clear_set_targets(sec_node, tgt_node);
                        }
                        break;
                    }
                    default:
                        break;
                }
            };

            uint32_t flushed = 0u;
            const uint32_t limit = (max_flush_per_frame > 0u) ? max_flush_per_frame : ring_cap;

            if (cursors) {
                uint32_t r_idx       = cursors[0] % ring_cap;
                const uint32_t w_idx = cursors[1] % ring_cap;
                while (r_idx != w_idx && flushed < limit) {
                    apply_command(ring_cmds[r_idx]);
                    r_idx = (r_idx + 1u) % ring_cap;
                    ++flushed;
                }
                cursors[0] = r_idx;
            } else {
                const uint32_t elem_cnt = static_cast<uint32_t>(std::clamp<int64_t>(
                    ring_part->selection.element_count, 0, static_cast<int64_t>(ring_cap)
                ));
                const uint32_t to_drain = std::min(elem_cnt, limit);
                for (uint32_t i = 0u; i < to_drain; ++i) {
                    apply_command(ring_cmds[i]);
                    ++flushed;
                }
                if (reset_selection_count_on_drain && to_drain == elem_cnt) {
                    const_cast<GrantPartPOD*>(ring_part)->selection.element_count = 0;
                }
                fallback_read_index = flushed;
            }

            if (flushed_counter) {
                *flushed_counter = flushed;
            }
        }
    }
};

static_assert(sizeof(SignalAndRetargetFlushTransformLogic<>) == 64, "SignalAndRetargetFlushTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SignalAndRetargetFlushTransformLogic<>>, "SignalAndRetargetFlushTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core

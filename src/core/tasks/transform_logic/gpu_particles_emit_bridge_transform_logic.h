#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * V8: GPUParticlesEmitBridgeTransformLogic
 * Triggers GPU particle sub-emissions directly on a RenderingServer particles RID
 * via RenderingServer::particles_emit(particles_rid, xform, velocity, color, custom, emit_flags)
 * from active/selected DOD particle or impact buffers.
 */
struct GPUParticlesEmitBridgeTransformLogic {
    static constexpr std::string_view ui_name = "RenderingServer GPUParticles Emit Bridge";
    static constexpr std::string_view ui_category = "Particles/Rendering";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::COLOR
    };

    godot::RID particles_rid;
    uint32_t emit_flags = 1 | 4; // EMISSION_FLAG_POSITION (1) | EMISSION_FLAG_VELOCITY (4)
    godot::Color default_color = godot::Color(1.0f, 1.0f, 1.0f, 1.0f);
    godot::Color default_custom = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    int32_t max_emissions_per_frame = 256;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("particles_rid", godot::Variant::RID, particles_rid);
        add_prop("emit_flags", godot::Variant::INT, static_cast<int64_t>(emit_flags));
        add_prop("default_color", godot::Variant::COLOR, default_color);
        add_prop("default_custom", godot::Variant::COLOR, default_custom);
        add_prop("max_emissions_per_frame", godot::Variant::INT, max_emissions_per_frame);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("particles_rid")) {
            godot::Variant v = p_props["particles_rid"];
            if (v.get_type() == godot::Variant::RID) {
                particles_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&particles_rid)) = raw;
            }
        }
        if (p_props.has("emit_flags")) emit_flags = static_cast<uint32_t>(static_cast<int64_t>(p_props["emit_flags"]));
        if (p_props.has("default_color")) default_color = p_props["default_color"];
        if (p_props.has("default_custom")) default_custom = p_props["default_custom"];
        if (p_props.has("max_emissions_per_frame")) max_emissions_per_frame = std::max<int32_t>(1, static_cast<int32_t>(p_props["max_emissions_per_frame"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !particles_rid.is_valid()) return;

        const GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        if (!pos_part.raw_base_ptr) return;

        godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
        if (!rs) return;

        const GrantPartPOD* vel_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;
        const GrantPartPOD* col_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;
        const GrantPartPOD* cust_part = (p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr)
            ? &p_ctx.grant->parts[3]
            : nullptr;

        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const int64_t max_pos = static_cast<int64_t>(pos_part.capacity_bytes / p_stride);
        const MemoryBufferSelectionPOD& sel = pos_part.selection;

        int32_t emitted = 0;

        auto emit_at = [&](int64_t i) {
            if (i < 0 || i >= max_pos || emitted >= max_emissions_per_frame) return;
            godot::Transform3D xform;
            if (p_stride >= sizeof(godot::Transform3D)) {
                xform = *reinterpret_cast<const godot::Transform3D*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride);
            } else {
                xform.origin = *reinterpret_cast<const godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride);
            }

            godot::Vector3 vel(0.0f, 0.0f, 0.0f);
            if (vel_part && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= vel_part->capacity_bytes) {
                vel = reinterpret_cast<const godot::Vector3*>(vel_part->raw_base_ptr)[i];
            }

            godot::Color col = default_color;
            if (col_part && static_cast<size_t>(i + 1) * sizeof(godot::Color) <= col_part->capacity_bytes) {
                col = reinterpret_cast<const godot::Color*>(col_part->raw_base_ptr)[i];
            }

            godot::Color cust = default_custom;
            if (cust_part && static_cast<size_t>(i + 1) * sizeof(godot::Color) <= cust_part->capacity_bytes) {
                cust = reinterpret_cast<const godot::Color*>(cust_part->raw_base_ptr)[i];
            }

            rs->particles_emit(particles_rid, xform, vel, col, cust, emit_flags);
            ++emitted;
        };

        if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
            for (int64_t k = 0; k < sel.element_count && emitted < max_emissions_per_frame; ++k) {
                emit_at(sel.data.indices[k]);
            }
        } else if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
            const int64_t limit = std::min<int64_t>(max_pos, sel.capacity);
            for (int64_t i = 0; i < limit && emitted < max_emissions_per_frame; ++i) {
                if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) {
                    emit_at(i);
                }
            }
        } else {
            const int64_t count = (sel.element_count > 0) ? std::min<int64_t>(sel.element_count, max_pos) : max_pos;
            for (int64_t i = 0; i < count && emitted < max_emissions_per_frame; ++i) {
                emit_at(i);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core
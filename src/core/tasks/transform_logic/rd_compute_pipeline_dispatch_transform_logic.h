#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/rd_shader_file.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdint>
#include <string_view>
#include <vector>

namespace ideam::core {

/**
 * G_RD7: RDComputePipelineDispatchTransformLogic
 * Self-contained RenderingDevice compute shader dispatcher: resolves or compiles a compute pipeline
 * from pipeline_rid / shader_rid / shader_path, binds node MemoryGrant buffers as SSBO bindings 0..N-1,
 * records the compute list with optional push constants, dispatches workgroups, and reads back WRITE parts.
 */
struct RDComputePipelineDispatchTransformLogic {
    static constexpr std::string_view ui_name = "RD Compute Pipeline SSBO Dispatch";
    static constexpr std::string_view ui_category = "GPU/Dispatch";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::FLOAT32,
        DataType::INT32,
        DataType::VECTOR3,
        DataType::VECTOR4
    };

    godot::RID pipeline_rid;
    godot::RID shader_rid;
    godot::String shader_path;
    int32_t local_size_x = 64;
    int32_t dispatch_x = 0; // 0 = auto-derive ceil(element_count / local_size_x)
    int32_t dispatch_y = 1;
    int32_t dispatch_z = 1;
    bool readback_write_parts = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("pipeline_rid", godot::Variant::RID, pipeline_rid);
        add_prop("shader_rid", godot::Variant::RID, shader_rid);
        add_prop("shader_path", godot::Variant::STRING, shader_path);
        add_prop("local_size_x", godot::Variant::INT, local_size_x);
        add_prop("dispatch_x", godot::Variant::INT, dispatch_x);
        add_prop("dispatch_y", godot::Variant::INT, dispatch_y);
        add_prop("dispatch_z", godot::Variant::INT, dispatch_z);
        add_prop("readback_write_parts", godot::Variant::BOOL, readback_write_parts);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        auto parse_rid = [](const godot::Variant& v) noexcept -> godot::RID {
            if (v.get_type() == godot::Variant::RID) return v;
            if (v.get_type() == godot::Variant::INT) {
                godot::RID r;
                *(reinterpret_cast<uint64_t*>(&r)) = static_cast<uint64_t>(static_cast<int64_t>(v));
                return r;
            }
            return godot::RID();
        };
        if (p_props.has("pipeline_rid")) pipeline_rid = parse_rid(p_props["pipeline_rid"]);
        if (p_props.has("shader_rid")) shader_rid = parse_rid(p_props["shader_rid"]);
        if (p_props.has("shader_path")) shader_path = p_props["shader_path"];
        if (p_props.has("local_size_x")) local_size_x = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_x"]));
        if (p_props.has("dispatch_x")) dispatch_x = std::max<int32_t>(0, static_cast<int32_t>(p_props["dispatch_x"]));
        if (p_props.has("dispatch_y")) dispatch_y = std::max<int32_t>(1, static_cast<int32_t>(p_props["dispatch_y"]));
        if (p_props.has("dispatch_z")) dispatch_z = std::max<int32_t>(1, static_cast<int32_t>(p_props["dispatch_z"]));
        if (p_props.has("readback_write_parts")) readback_write_parts = static_cast<bool>(p_props["readback_write_parts"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        godot::RenderingDevice* rd = p_ctx.manager ? p_ctx.manager->get_rendering_device() : nullptr;
        if (!rd && godot::RenderingServer::get_singleton()) {
            rd = godot::RenderingServer::get_singleton()->get_rendering_device();
        }
        if (!rd) return;

        godot::RID active_shader = shader_rid;
        godot::RID active_pipeline = pipeline_rid;
        bool owns_shader = false;
        bool owns_pipeline = false;

        if (!active_pipeline.is_valid()) {
            if (!active_shader.is_valid() && !shader_path.is_empty() && godot::ResourceLoader::get_singleton()) {
                godot::Ref<godot::RDShaderFile> sf = godot::ResourceLoader::get_singleton()->load(shader_path);
                if (sf.is_valid()) {
                    godot::Ref<godot::RDShaderSPIRV> spirv = sf->get_spirv();
                    if (spirv.is_valid()) {
                        active_shader = rd->shader_create_from_spirv(spirv);
                        owns_shader = active_shader.is_valid();
                    }
                }
            }
            if (active_shader.is_valid()) {
                active_pipeline = rd->compute_pipeline_create(active_shader);
                owns_pipeline = active_pipeline.is_valid();
            }
        }
        if (!active_pipeline.is_valid()) {
            if (owns_shader && active_shader.is_valid()) rd->free_rid(active_shader);
            return;
        }

        // Resolve workgroup dispatch count
        uint32_t gx = static_cast<uint32_t>(std::max<int32_t>(1, dispatch_x));
        if (dispatch_x == 0) {
            const GrantPartPOD& p0 = p_ctx.grant->parts[0];
            const int64_t elems = (p0.selection.element_count > 0)
                ? p0.selection.element_count
                : static_cast<int64_t>(p0.capacity_bytes / std::max<uint32_t>(4, p0.element_stride));
            gx = static_cast<uint32_t>(std::max<int64_t>(1, (elems + local_size_x - 1) / local_size_x));
        }
        const uint32_t gy = static_cast<uint32_t>(std::max<int32_t>(1, dispatch_y));
        const uint32_t gz = static_cast<uint32_t>(std::max<int32_t>(1, dispatch_z));

        // If grant already has a baked uniform_set_handle, dispatch directly
        if (p_ctx.grant->uniform_set_handle != 0) {
            godot::RID uniform_set;
            *(reinterpret_cast<uint64_t*>(&uniform_set)) = p_ctx.grant->uniform_set_handle;
            const int64_t cl = rd->compute_list_begin();
            rd->compute_list_bind_compute_pipeline(cl, active_pipeline);
            rd->compute_list_bind_uniform_set(cl, uniform_set, 0);
            rd->compute_list_dispatch(cl, gx, gy, gz);
            rd->compute_list_end();
        } else if (active_shader.is_valid()) {
            // Allocate temporary SSBOs for each GrantPart, dispatch, and read back WRITE/READ_WRITE slices
            std::vector<godot::RID> temp_ssbos(p_ctx.grant->part_count);
            godot::TypedArray<godot::RDUniform> uniforms;

            for (uint32_t i = 0; i < p_ctx.grant->part_count; ++i) {
                const GrantPartPOD& part = p_ctx.grant->parts[i];
                if (!part.raw_base_ptr || part.capacity_bytes == 0) continue;

                godot::PackedByteArray init_bytes;
                init_bytes.resize(static_cast<int64_t>(part.capacity_bytes));
                std::memcpy(init_bytes.ptrw(), part.raw_base_ptr, part.capacity_bytes);

                temp_ssbos[i] = rd->storage_buffer_create(static_cast<uint32_t>(part.capacity_bytes), init_bytes);
                godot::Ref<godot::RDUniform> u;
                u.instantiate();
                u->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
                u->set_binding(static_cast<int32_t>(i));
                u->add_id(temp_ssbos[i]);
                uniforms.push_back(u);
            }

            godot::RID temp_uset = rd->uniform_set_create(uniforms, active_shader, 0);
            if (temp_uset.is_valid()) {
                const int64_t cl = rd->compute_list_begin();
                rd->compute_list_bind_compute_pipeline(cl, active_pipeline);
                rd->compute_list_bind_uniform_set(cl, temp_uset, 0);
                rd->compute_list_dispatch(cl, gx, gy, gz);
                rd->compute_list_end();

                if (readback_write_parts) {
                    for (uint32_t i = 0; i < p_ctx.grant->part_count; ++i) {
                        GrantPartPOD& part = p_ctx.grant->parts[i];
                        if (part.access_mode != BufferAccessMode::READ && temp_ssbos[i].is_valid()) {
                            godot::PackedByteArray out_bytes = rd->buffer_get_data(temp_ssbos[i]);
                            const size_t n_copy = std::min<size_t>(part.capacity_bytes, static_cast<size_t>(out_bytes.size()));
                            if (n_copy > 0) {
                                std::memcpy(part.raw_base_ptr, out_bytes.ptr(), n_copy);
                            }
                        }
                    }
                }
                rd->free_rid(temp_uset);
            }

            for (godot::RID& s_rid : temp_ssbos) {
                if (s_rid.is_valid()) rd->free_rid(s_rid);
            }
        }

        if (owns_pipeline && active_pipeline.is_valid()) rd->free_rid(active_pipeline);
        if (owns_shader && active_shader.is_valid()) rd->free_rid(active_shader);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core
#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

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
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ideam::core {

/**
 * RDComputePipelineDispatchTransformLogic<T> (G_RD7)
 * Self-contained RenderingDevice compute shader dispatcher: resolves or compiles a compute pipeline
 * from pipeline_rid / shader_rid / shader_path, binds node MemoryGrant buffers as SSBO bindings 0..N-1,
 * records the compute list, dispatches workgroups, and reads back WRITE parts.
 */
template <typename T = float>
struct alignas(64) RDComputePipelineDispatchTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              =
        DataType::FLOAT32 | DataType::INT32 | DataType::VECTOR3 | DataType::VECTOR4;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD Compute Pipeline SSBO Dispatch";
    static constexpr std::string_view ui_category  = "GPU/Dispatch";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t primary_buffer_id  = INVALID_ID; // 4B: Primary SSBO binding 0 buffer
    int32_t local_size_x        = 64;         // 4B: Workgroup local_size_x
    int32_t dispatch_x          = 0;          // 4B: 0 = auto-derive ceil(element_count / local_size_x)
    int32_t dispatch_y          = 1;          // 4B: Dispatched workgroups along Y -> 16B
    int32_t dispatch_z          = 1;          // 4B: Dispatched workgroups along Z
    bool readback_write_parts   = true;       // 1B: Read back WRITE/READ_WRITE GrantParts after dispatch
    uint8_t reserved_flags[3]   = {0};        // 3B -> 24B

    godot::RID pipeline_rid;                  // 8B -> 32B
    godot::RID shader_rid;                    // 8B -> 40B
    godot::String shader_path;                // sizeof(godot::String) (8B) -> 48B
    uint8_t reserved_padding[24 - sizeof(godot::String)] = {0}; // Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pbuf_prop;
        pbuf_prop["name"] = "primary_buffer_id";
        pbuf_prop["type"] = godot::Variant::INT;
        pbuf_prop["hint_string"] = "buffer_option";
        props.push_back(pbuf_prop);

        godot::Dictionary prid_prop;
        prid_prop["name"] = "pipeline_rid";
        prid_prop["type"] = godot::Variant::RID;
        props.push_back(prid_prop);

        godot::Dictionary srid_prop;
        srid_prop["name"] = "shader_rid";
        srid_prop["type"] = godot::Variant::RID;
        props.push_back(srid_prop);

        godot::Dictionary spth_prop;
        spth_prop["name"] = "shader_path";
        spth_prop["type"] = godot::Variant::STRING;
        spth_prop["hint"] = godot::PROPERTY_HINT_FILE;
        spth_prop["hint_string"] = "*.glsl";
        props.push_back(spth_prop);

        godot::Dictionary lx_prop;
        lx_prop["name"] = "local_size_x";
        lx_prop["type"] = godot::Variant::INT;
        lx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        lx_prop["hint_string"] = "1,1024,1";
        props.push_back(lx_prop);

        godot::Dictionary dx_prop;
        dx_prop["name"] = "dispatch_x";
        dx_prop["type"] = godot::Variant::INT;
        props.push_back(dx_prop);

        godot::Dictionary dy_prop;
        dy_prop["name"] = "dispatch_y";
        dy_prop["type"] = godot::Variant::INT;
        props.push_back(dy_prop);

        godot::Dictionary dz_prop;
        dz_prop["name"] = "dispatch_z";
        dz_prop["type"] = godot::Variant::INT;
        props.push_back(dz_prop);

        godot::Dictionary rbk_prop;
        rbk_prop["name"] = "readback_write_parts";
        rbk_prop["type"] = godot::Variant::BOOL;
        props.push_back(rbk_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return primary_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("primary_buffer_id")) {
            primary_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["primary_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && primary_buffer_id == INVALID_ID) {
            primary_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && primary_buffer_id == INVALID_ID) {
            primary_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        auto parse_rid = [](const godot::Variant& v) noexcept -> godot::RID {
            if (v.get_type() == godot::Variant::RID) return v;
            if (v.get_type() == godot::Variant::INT) {
                godot::RID r;
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&r, &raw, sizeof(uint64_t));
                return r;
            }
            return godot::RID();
        };

        if (p_props.has("pipeline_rid")) pipeline_rid = parse_rid(p_props["pipeline_rid"]);
        if (p_props.has("shader_rid"))   shader_rid   = parse_rid(p_props["shader_rid"]);
        if (p_props.has("shader_path"))  shader_path  = p_props["shader_path"];
        if (p_props.has("local_size_x")) local_size_x = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_x"]));
        if (p_props.has("dispatch_x"))   dispatch_x   = std::max<int32_t>(0, static_cast<int32_t>(p_props["dispatch_x"]));
        if (p_props.has("dispatch_y"))   dispatch_y   = std::max<int32_t>(1, static_cast<int32_t>(p_props["dispatch_y"]));
        if (p_props.has("dispatch_z"))   dispatch_z   = std::max<int32_t>(1, static_cast<int32_t>(p_props["dispatch_z"]));
        if (p_props.has("readback_write_parts")) {
            readback_write_parts = static_cast<bool>(p_props["readback_write_parts"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!p_context.grant || p_context.grant->part_count == 0) return;

            godot::RenderingDevice* rd = p_context.manager ? p_context.manager->get_rendering_device() : nullptr;
            if (!rd && godot::RenderingServer::get_singleton()) {
                rd = godot::RenderingServer::get_singleton()->get_rendering_device();
            }
            if (!rd) return;

            godot::RID active_shader   = shader_rid;
            godot::RID active_pipeline = pipeline_rid;
            bool owns_shader           = false;
            bool owns_pipeline         = false;

            if (!active_pipeline.is_valid()) {
                if (!active_shader.is_valid() && !shader_path.is_empty() && godot::ResourceLoader::get_singleton()) {
                    const godot::Ref<godot::RDShaderFile> sf = godot::ResourceLoader::get_singleton()->load(shader_path);
                    if (sf.is_valid()) {
                        const godot::Ref<godot::RDShaderSPIRV> spirv = sf->get_spirv();
                        if (spirv.is_valid()) {
                            active_shader = rd->shader_create_from_spirv(spirv);
                            owns_shader   = active_shader.is_valid();
                        }
                    }
                }
                if (active_shader.is_valid()) {
                    active_pipeline = rd->compute_pipeline_create(active_shader);
                    owns_pipeline   = active_pipeline.is_valid();
                }
            }
            if (!active_pipeline.is_valid()) {
                if (owns_shader && active_shader.is_valid()) rd->free_rid(active_shader);
                return;
            }

            uint32_t gx = static_cast<uint32_t>(std::max<int32_t>(1, dispatch_x));
            if (dispatch_x == 0) {
                const GrantPartPOD* p0 = p_context.get_grant_part(primary_buffer_id);
                if (!p0) p0 = &p_context.grant->parts[0];
                const int64_t elems = (p0->selection.element_count > 0)
                    ? p0->selection.element_count
                    : static_cast<int64_t>(p0->capacity_bytes / std::max<uint32_t>(4, p0->element_stride));
                gx = static_cast<uint32_t>(std::max<int64_t>(1, (elems + local_size_x - 1) / local_size_x));
            }
            const uint32_t gy = static_cast<uint32_t>(std::max<int32_t>(1, dispatch_y));
            const uint32_t gz = static_cast<uint32_t>(std::max<int32_t>(1, dispatch_z));

            if (p_context.grant->uniform_set_handle != 0) {
                godot::RID uniform_set;
                std::memcpy(&uniform_set, &p_context.grant->uniform_set_handle, sizeof(uint64_t));
                const int64_t cl = rd->compute_list_begin();
                rd->compute_list_bind_compute_pipeline(cl, active_pipeline);
                rd->compute_list_bind_uniform_set(cl, uniform_set, 0);
                rd->compute_list_dispatch(cl, gx, gy, gz);
                rd->compute_list_end();
            } else if (active_shader.is_valid()) {
                std::vector<godot::RID> temp_ssbos(p_context.grant->part_count);
                godot::TypedArray<godot::RDUniform> uniforms;

                for (uint32_t i = 0; i < p_context.grant->part_count; ++i) {
                    const GrantPartPOD& part = p_context.grant->parts[i];
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

                const godot::RID temp_uset = rd->uniform_set_create(uniforms, active_shader, 0);
                if (temp_uset.is_valid()) {
                    const int64_t cl = rd->compute_list_begin();
                    rd->compute_list_bind_compute_pipeline(cl, active_pipeline);
                    rd->compute_list_bind_uniform_set(cl, temp_uset, 0);
                    rd->compute_list_dispatch(cl, gx, gy, gz);
                    rd->compute_list_end();

                    if (readback_write_parts) {
                        for (uint32_t i = 0; i < p_context.grant->part_count; ++i) {
                            GrantPartPOD& part = p_context.grant->parts[i];
                            if (part.access_mode != BufferAccessMode::READ && temp_ssbos[i].is_valid()) {
                                const godot::PackedByteArray out_bytes = rd->buffer_get_data(temp_ssbos[i]);
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
            if (owns_shader && active_shader.is_valid())     rd->free_rid(active_shader);
        }
    }
};

static_assert(sizeof(RDComputePipelineDispatchTransformLogic<>) == 64, "RDComputePipelineDispatchTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDComputePipelineDispatchTransformLogic<>>, "RDComputePipelineDispatchTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
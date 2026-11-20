#pragma once

/**
 * cymatic_mesh_resonance_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CYM4`) for `IdeamCymatics`.
 * Displaces 3D mesh vertices along normals and computes nodal intensity from a cymatic field.
 */

#include "../../../../cymatics/cymatics_solver_core.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) CymaticMeshResonanceTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types = DataType::FLOAT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "CYM4 Mesh Resonance Displacement";
    static constexpr std::string_view ui_category  = "Cymatics";

    uint32_t vertex_buffer_id    = INVALID_ID;
    uint32_t normal_buffer_id    = INVALID_ID;
    uint32_t field_buffer_id     = INVALID_ID;
    uint32_t nodal_out_buffer_id = INVALID_ID;
    uint32_t grid_width          = 32;
    uint32_t grid_height         = 32;
    float cell_size              = 1.0f;
    float displacement_scale     = 0.25f;
    uint8_t reserved_padding[32] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add = [&](const char *n, godot::Variant::Type t, godot::Variant d) {
            godot::Dictionary dict;
            dict["name"]    = n;
            dict["type"]    = static_cast<int>(t);
            dict["default"] = d;
            props.push_back(dict);
        };
        add("vertex_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("normal_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("field_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("nodal_out_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("grid_width",          godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_height",         godot::Variant::INT,   static_cast<int64_t>(32));
        add("cell_size",           godot::Variant::FLOAT, 1.0);
        add("displacement_scale",  godot::Variant::FLOAT, 0.25);
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("vertex_buffer_id"))    vertex_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        if (p_props.has("normal_buffer_id"))    normal_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        if (p_props.has("field_buffer_id"))     field_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["field_buffer_id"]));
        if (p_props.has("nodal_out_buffer_id")) nodal_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["nodal_out_buffer_id"]));
        if (p_props.has("grid_width"))          grid_width          = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))         grid_height         = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
        if (p_props.has("cell_size"))           cell_size           = static_cast<float>(static_cast<double>(p_props["cell_size"]));
        if (p_props.has("displacement_scale"))  displacement_scale  = static_cast<float>(static_cast<double>(p_props["displacement_scale"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *vp = p_ctx.get_grant_part(vertex_buffer_id);
        const GrantPartPOD *fp = p_ctx.get_grant_part(field_buffer_id);
        if (!vp || !fp || !vp->raw_base_ptr || !fp->raw_base_ptr) {
            return;
        }
        if (fp->capacity_bytes / sizeof(float) < static_cast<size_t>(grid_width) * grid_height) {
            return;
        }
        const uint32_t v_count = static_cast<uint32_t>(vp->capacity_bytes / (sizeof(float) * 3u));
        const float *normals   = nullptr;
        if (const GrantPartPOD *np = p_ctx.get_grant_part(normal_buffer_id);
            np && np->raw_base_ptr && np->capacity_bytes >= vp->capacity_bytes) {
            normals = reinterpret_cast<const float *>(np->raw_base_ptr);
        }
        float *nodal_out = nullptr;
        if (const GrantPartPOD *op = p_ctx.get_grant_part(nodal_out_buffer_id);
            op && op->raw_base_ptr && op->capacity_bytes >= v_count * sizeof(float)) {
            nodal_out = reinterpret_cast<float *>(op->raw_base_ptr);
        }

        CymaticGridConfigPOD cfg;
        cfg.width     = grid_width;
        cfg.height    = grid_height;
        cfg.cell_size = cell_size;

        CymaticsSolverCore::displace_mesh_resonance(
            reinterpret_cast<float *>(vp->raw_base_ptr),
            normals,
            v_count,
            reinterpret_cast<const float *>(fp->raw_base_ptr),
            cfg,
            displacement_scale,
            nodal_out
        );
    }
};

static_assert(sizeof(CymaticMeshResonanceTransformLogic<>) == 64, "CYM4 must be 64 bytes!");
static_assert(IsTransformLogic<CymaticMeshResonanceTransformLogic<>>, "CYM4 must satisfy IsTransformLogic!");

} // namespace ideam::core

#pragma once

/**
 * tiling_instance_transform_logic.h
 * TileDOD Transform Task (T_T1): Wave 3A Parallel Crystallographic & Isohedral Instance Transform.
 *
 * Evaluates 2D affine lattice + aspect transforms (`TileTransformPOD`: `position`, `scale`,
 * `angle_rad`, `cell_id`, `custom_data`) across all active `IsoCellPOD` cells using the
 * prebaked `BakedPrototilePOD<float>` descriptor table (`prototile_descriptor_buffer_id`).
 */

#include "../../../../tiles/tile_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) TilingInstanceTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tiling Lattice Instance Transform";
    static constexpr std::string_view ui_category  = "Tiles";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id               = INVALID_ID; // 4B
    uint32_t prototile_descriptor_buffer_id = INVALID_ID; // 4B: BakedPrototilePOD<float>
    uint32_t iso_cell_buffer_id             = INVALID_ID; // 4B: IsoCellPOD (32B AoS)
    uint32_t instance_transform_buffer_id   = INVALID_ID; // 4B: TileTransformPOD (32B AoS)
    uint8_t reserved_padding[48]            = {0};        // 48B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("prototile_descriptor_buffer_id");
        add_buf("iso_cell_buffer_id");
        add_buf("instance_transform_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))               target_buffer_id               = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("prototile_descriptor_buffer_id")) prototile_descriptor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["prototile_descriptor_buffer_id"]));
        if (p_props.has("iso_cell_buffer_id"))             iso_cell_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["iso_cell_buffer_id"]));
        if (p_props.has("instance_transform_buffer_id"))   instance_transform_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["instance_transform_buffer_id"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : instance_transform_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *proto_part = p_context.get_grant_part(prototile_descriptor_buffer_id);
        const GrantPartPOD *cell_part  = p_context.get_grant_part(iso_cell_buffer_id);
        const GrantPartPOD *xf_part    = p_context.get_grant_part(instance_transform_buffer_id);

        if (!proto_part || !proto_part->raw_base_ptr ||
            !cell_part || !cell_part->raw_base_ptr ||
            !xf_part || !xf_part->raw_base_ptr) {
            return;
        }

        const int64_t proto_cap = proto_part->selection.capacity;
        const int64_t cell_cnt  = std::min(cell_part->selection.element_count, xf_part->selection.capacity);
        if (proto_cap <= 0 || cell_cnt <= 0) return;

        const auto *protos = reinterpret_cast<const BakedPrototilePOD<float> *>(proto_part->raw_base_ptr);
        const auto *cells  = reinterpret_cast<const IsoCellPOD *>(cell_part->raw_base_ptr);
        auto *xforms       = reinterpret_cast<TileTransformPOD *>(xf_part->raw_base_ptr);

        for (int64_t c = 0; c < cell_cnt; ++c) {
            const IsoCellPOD &cell = cells[c];
            const int64_t grp      = std::min<int64_t>(cell.group_slot, proto_cap - 1);
            const BakedPrototilePOD<float> &proto = protos[grp];

            const uint32_t asp = (proto.num_aspects > 0u)
                ? (static_cast<uint32_t>(cell.aspect_index) % static_cast<uint32_t>(proto.num_aspects))
                : 0u;

            const glm::vec2 lattice_offset =
                proto.lattice_basis.origin +
                proto.lattice_basis.u_axis * static_cast<float>(cell.lattice_i) +
                proto.lattice_basis.v_axis * static_cast<float>(cell.lattice_j);

            const glm::mat3 &asp_m = proto.aspect_xforms[asp];
            const glm::vec2 asp_translation(asp_m[2][0], asp_m[2][1]);

            TileTransformPOD out{};
            out.position      = lattice_offset + asp_translation;
            out.scale         = proto.aspect_scales[asp];
            out.angle_rad     = proto.aspect_angles_rad[asp];
            out.cell_id       = static_cast<uint32_t>(c);
            out.custom_data.x = static_cast<float>(cell.colour_id % 3u) * 0.5f;
            out.custom_data.y = static_cast<float>(cell.generation);
            xforms[c]         = out;
        }
    }
};

static_assert(
    sizeof(TilingInstanceTransformLogic<>) == 64,
    "TilingInstanceTransformLogic must be 64 bytes!"
);

} // namespace ideam::core

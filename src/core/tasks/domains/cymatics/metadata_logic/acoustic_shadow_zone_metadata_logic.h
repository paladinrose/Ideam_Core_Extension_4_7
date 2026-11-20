#pragma once

/**
 * acoustic_shadow_zone_metadata_logic.h
 * 64-byte cache-aligned MetadataLogic task struct (`CYM6`) for `IdeamCymatics`.
 * Classifies grid cells into Direct LOS (0), Diffracted Rim (1), Reverberant Chamber (2),
 * and Acoustic Shadow (3) in `r_selection.lod_levels`.
 */

#include "../../../../cymatics/cymatics_solver_core.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) AcousticShadowZoneMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "CYM6 Acoustic Shadow Zone Classifier";
    static constexpr std::string_view ui_category  = "Cymatics";

    uint32_t target_buffer_id       = INVALID_ID;
    uint32_t reflectivity_buffer_id = INVALID_ID;
    uint32_t out_class_buffer_id    = INVALID_ID;
    uint32_t grid_width             = 32;
    uint32_t grid_height            = 32;
    float source_grid_x             = 16.0f;
    float source_grid_y             = 16.0f;
    uint8_t reserved_padding[36]    = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add = [&](const char *n, godot::Variant::Type t, godot::Variant d) {
            godot::Dictionary dict;
            dict["name"]    = n;
            dict["type"]    = static_cast<int>(t);
            dict["default"] = d;
            props.push_back(dict);
        };
        add("target_buffer_id",       godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("reflectivity_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("out_class_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("grid_width",             godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_height",            godot::Variant::INT,   static_cast<int64_t>(32));
        add("source_grid_x",          godot::Variant::FLOAT, 16.0);
        add("source_grid_y",          godot::Variant::FLOAT, 16.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))       target_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("reflectivity_buffer_id")) reflectivity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["reflectivity_buffer_id"]));
        if (p_props.has("out_class_buffer_id"))    out_class_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_class_buffer_id"]));
        if (p_props.has("grid_width"))             grid_width             = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))            grid_height            = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
        if (p_props.has("source_grid_x"))          source_grid_x          = static_cast<float>(static_cast<double>(p_props["source_grid_x"]));
        if (p_props.has("source_grid_y"))          source_grid_y          = static_cast<float>(static_cast<double>(p_props["source_grid_y"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const size_t total = static_cast<size_t>(grid_width) * grid_height;
        if (total == 0u) {
            return;
        }
        const float *field = nullptr;
        if (const GrantPartPOD *fp = p_ctx.get_grant_part(target_buffer_id);
            fp && fp->raw_base_ptr && fp->capacity_bytes / sizeof(float) >= total) {
            field = reinterpret_cast<const float *>(fp->raw_base_ptr);
        }
        const float *refl = nullptr;
        if (const GrantPartPOD *rp = p_ctx.get_grant_part(reflectivity_buffer_id);
            rp && rp->raw_base_ptr && rp->capacity_bytes / sizeof(float) >= total) {
            refl = reinterpret_cast<const float *>(rp->raw_base_ptr);
        }

        uint8_t *out_cls = nullptr;
        if (const GrantPartPOD *op = p_ctx.get_grant_part(out_class_buffer_id);
            op && op->raw_base_ptr && op->capacity_bytes >= total) {
            out_cls = reinterpret_cast<uint8_t *>(op->raw_base_ptr);
        }

        CymaticGridConfigPOD cfg;
        cfg.width  = grid_width;
        cfg.height = grid_height;

        if (out_cls) {
            CymaticsSolverCore::classify_shadow_zones(field, refl, out_cls, cfg, source_grid_x, source_grid_y);
        }

        if (GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id))) {
            MemoryBufferSelectionPOD &sel = target_part->selection;
            if (sel.lod_levels && sel.element_count > 0) {
                const size_t lim = std::min<size_t>(total, static_cast<size_t>(sel.element_count));
                if (out_cls) {
                    for (size_t i = 0; i < lim; ++i) {
                        sel.lod_levels[i] = out_cls[i];
                    }
                } else {
                    CymaticsSolverCore::classify_shadow_zones(field, refl, sel.lod_levels, cfg, source_grid_x, source_grid_y);
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_ctx,
        T_ActiveView &p_view
    ) const noexcept {
        (void)r_selection;
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_metadata(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(AcousticShadowZoneMetadataLogic<>) == 64, "CYM6 must be 64 bytes!");
static_assert(IsMetadataLogic<AcousticShadowZoneMetadataLogic<>>, "CYM6 must satisfy IsMetadataLogic!");

} // namespace ideam::core

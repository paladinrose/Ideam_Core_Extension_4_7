#pragma once

/**
 * acoustic_shadow_zone_metadata_logic.h
 * 64-byte cache-aligned MetadataLogic task struct (`CYM6`) for `IdeamCymatics`.
 * Classifies selected grid cells into Direct LOS (0), Diffracted Rim (1),
 * Reverberant Chamber (2), and Acoustic Shadow (3) in `r_selection.lod_levels`.
 */

#include "../../../../cymatics/cymatics_solver_core.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cmath>
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
        auto add_buf = [&](const char *n) {
            godot::Dictionary dict;
            dict["name"]        = n;
            dict["type"]        = static_cast<int>(godot::Variant::INT);
            dict["hint"]        = static_cast<int>(godot::PROPERTY_HINT_NONE);
            dict["hint_string"] = "buffer_option";
            dict["default"]     = static_cast<int64_t>(INVALID_ID);
            props.push_back(dict);
        };
        auto add = [&](const char *n, godot::Variant::Type t, godot::Variant d) {
            godot::Dictionary dict;
            dict["name"]    = n;
            dict["type"]    = static_cast<int>(t);
            dict["default"] = d;
            props.push_back(dict);
        };
        add_buf("target_buffer_id");
        add_buf("reflectivity_buffer_id");
        add_buf("out_class_buffer_id");
        add("grid_width",    godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_height",   godot::Variant::INT,   static_cast<int64_t>(32));
        add("source_grid_x", godot::Variant::FLOAT, 16.0);
        add("source_grid_y", godot::Variant::FLOAT, 16.0);
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

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_ctx,
        const T_ActiveView &p_view
    ) const noexcept {
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

        if (!r_selection.lod_levels || r_selection.element_count <= 0) {
            return;
        }

        const int64_t limit = std::min<int64_t>(static_cast<int64_t>(total), r_selection.capacity);
        if (limit <= 0) {
            return;
        }

        if (r_selection.mode == SelectionMode::DENSE) {
            _dispatch_dense(r_selection, limit, field, refl, out_cls);
        } else if (r_selection.mode == SelectionMode::SPARSE) {
            _dispatch_sparse(r_selection, limit, field, refl, out_cls);
        } else if (r_selection.mode == SelectionMode::RANGE) {
            _dispatch_range(r_selection, limit, field, refl, out_cls, cfg);
        }
    }

private:
    [[nodiscard]] inline uint8_t _classify_cell(
        const float *p_field,
        const float *p_refl,
        size_t p_idx
    ) const noexcept {
        const uint32_t w = grid_width;
        const uint32_t h = grid_height;
        const uint32_t x = static_cast<uint32_t>(p_idx % w);
        const uint32_t y = static_cast<uint32_t>(p_idx / w);
        const float energy = p_field ? std::fabs(p_field[p_idx]) : 0.0f;

        const float dx  = static_cast<float>(x) - source_grid_x;
        const float dy  = static_cast<float>(y) - source_grid_y;
        const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
        bool occluded   = false;
        int obs_hits    = 0;

        if (p_refl) {
            for (int s = 1; s < steps; ++s) {
                const float t = static_cast<float>(s) / static_cast<float>(steps);
                const int sx  = std::clamp(static_cast<int>(std::lround(source_grid_x + dx * t)), 0, static_cast<int>(w - 1u));
                const int sy  = std::clamp(static_cast<int>(std::lround(source_grid_y + dy * t)), 0, static_cast<int>(h - 1u));
                if (p_refl[static_cast<uint32_t>(sy) * w + static_cast<uint32_t>(sx)] >= 0.5f) {
                    occluded = true;
                    ++obs_hits;
                }
            }
        }

        AcousticShadowClass cls = AcousticShadowClass::DIRECT_LOS;
        if (!occluded) {
            cls = (energy > 0.35f) ? AcousticShadowClass::DIRECT_LOS : AcousticShadowClass::REVERBERANT_CHAMBER;
        } else if (obs_hits == 1 && energy > 0.15f) {
            cls = AcousticShadowClass::DIFFRACTED_RIM;
        } else if (energy > 0.08f) {
            cls = AcousticShadowClass::REVERBERANT_CHAMBER;
        } else {
            cls = AcousticShadowClass::ACOUSTIC_SHADOW;
        }
        return static_cast<uint8_t>(cls);
    }

    inline void _dispatch_dense(
        MemoryBufferSelectionPOD &r_sel,
        int64_t p_limit,
        const float *p_field,
        const float *p_refl,
        const uint8_t *p_precomputed
    ) const noexcept {
        const uint64_t *bitset = r_sel.data.bitset;
        if (!bitset) return;
        uint8_t *lods = r_sel.lod_levels;

        for (int64_t i = 0; i < p_limit; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                lods[i] = p_precomputed ? p_precomputed[i] : _classify_cell(p_field, p_refl, static_cast<size_t>(i));
            }
        }
    }

    inline void _dispatch_sparse(
        MemoryBufferSelectionPOD &r_sel,
        int64_t p_limit,
        const float *p_field,
        const float *p_refl,
        const uint8_t *p_precomputed
    ) const noexcept {
        const int64_t *indices = r_sel.data.indices;
        if (!indices) return;
        uint8_t *lods       = r_sel.lod_levels;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            if (idx >= 0 && idx < p_limit) {
                lods[idx] = p_precomputed ? p_precomputed[idx] : _classify_cell(p_field, p_refl, static_cast<size_t>(idx));
            }
        }
    }

    inline void _dispatch_range(
        MemoryBufferSelectionPOD &r_sel,
        int64_t p_limit,
        const float *p_field,
        const float *p_refl,
        const uint8_t *p_precomputed,
        const CymaticGridConfigPOD &p_cfg
    ) const noexcept {
        const int64_t start = std::max<int64_t>(0, r_sel.start_index);
        const int64_t end   = std::min<int64_t>(p_limit, r_sel.start_index + r_sel.element_count);
        if (start >= end) return;

        uint8_t *lods = r_sel.lod_levels;
        const int64_t full_grid = static_cast<int64_t>(p_cfg.width) * static_cast<int64_t>(p_cfg.height);

        if (!p_precomputed && start == 0 && end == full_grid) {
            CymaticsSolverCore::classify_shadow_zones(p_field, p_refl, lods, p_cfg, source_grid_x, source_grid_y);
            return;
        }

        for (int64_t i = start; i < end; ++i) {
            lods[i] = p_precomputed ? p_precomputed[i] : _classify_cell(p_field, p_refl, static_cast<size_t>(i));
        }
    }
};

static_assert(sizeof(AcousticShadowZoneMetadataLogic<>) == 64, "CYM6 must be 64 bytes!");
static_assert(IsMetadataLogic<AcousticShadowZoneMetadataLogic<>>, "CYM6 must satisfy IsMetadataLogic!");

} // namespace ideam::core

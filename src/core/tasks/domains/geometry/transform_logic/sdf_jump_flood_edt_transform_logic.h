#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/swap_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * SDFJumpFloodEDTTransformLogic<T, T_Strategy, T_View> (GEO7)
 * Computes a 2D Euclidean Signed Distance Field (SDF) and optional closest-seed UV coordinates
 * over a 2D spatial grid or flat scalar/vector buffer using either:
 *   - Mode 0: Exact O(N) Separable Squared Distance Transform (Felzenszwalb-Huttenlocher lower envelope)
 *   - Mode 1: Jump Flooding Algorithm (JFA+1) propagation in transient workspace
 */
template <
    typename T = float,
    typename T_Strategy = Spatial2DStrategy,
    template <typename, typename> class T_View = SwapView
>
struct alignas(64) SDFJumpFloodEDTTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr int32_t MAX_EDT_GRID_DIM = 512;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS | ViewCapability::SWAP_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA |
        BufferLayoutType::SPARSE_SET | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types =
        DataType::FLOAT32 | DataType::VECTOR2 | DataType::VECTOR4;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes =
        (static_cast<size_t>(MAX_EDT_GRID_DIM) * static_cast<size_t>(MAX_EDT_GRID_DIM) * sizeof(float) * 4) +
        (static_cast<size_t>(MAX_EDT_GRID_DIM + 2) * sizeof(float) * 4) + 256;

    static constexpr std::string_view display_name = "SDF Jump-Flood & Separable EDT";
    static constexpr std::string_view ui_category  = "Geometry/SDF";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id     = INVALID_ID; // 4B
    uint32_t source_buffer_id     = INVALID_ID; // 4B
    uint32_t output_buffer_id     = INVALID_ID; // 4B
    int32_t grid_width            = 256;        // 4B -> 16B
    int32_t grid_height           = 256;        // 4B
    float cell_size               = 1.0f;       // 4B
    float iso_threshold           = 0.5f;       // 4B
    int32_t algorithm_mode        = 0;          // 4B -> 32B: 0 = Separable EDT, 1 = Jump Flooding (JFA+1)
    bool signed_distance          = true;       // 1B
    bool write_nearest_seed_uv    = false;      // 1B
    uint8_t reserved_padding[30]  = {0};        // 30B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("source_buffer_id");
        add_buf("output_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("grid_width", godot::Variant::INT, 256);
        add_prop("grid_height", godot::Variant::INT, 256);
        add_prop("cell_size", godot::Variant::FLOAT, 1.0f);
        add_prop("iso_threshold", godot::Variant::FLOAT, 0.5f);
        add_prop("algorithm_mode", godot::Variant::INT, 0, 2, "Separable Exact EDT,Jump Flooding (JFA+1)");
        add_prop("signed_distance", godot::Variant::BOOL, true);
        add_prop("write_nearest_seed_uv", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (output_buffer_id != INVALID_ID) ? output_buffer_id : source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("source_buffer_id")) source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        if (p_props.has("output_buffer_id")) output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_buffer_id"]));
        if (p_props.has("grid_width")) grid_width = std::clamp<int32_t>(static_cast<int32_t>(p_props["grid_width"]), 1, MAX_EDT_GRID_DIM);
        if (p_props.has("grid_height")) grid_height = std::clamp<int32_t>(static_cast<int32_t>(p_props["grid_height"]), 1, MAX_EDT_GRID_DIM);
        if (p_props.has("cell_size")) cell_size = std::max(1e-6f, static_cast<float>(p_props["cell_size"]));
        if (p_props.has("iso_threshold")) iso_threshold = static_cast<float>(p_props["iso_threshold"]);
        if (p_props.has("algorithm_mode")) algorithm_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["algorithm_mode"]), 0, 1);
        if (p_props.has("signed_distance")) signed_distance = static_cast<bool>(p_props["signed_distance"]);
        if (p_props.has("write_nearest_seed_uv")) write_nearest_seed_uv = static_cast<bool>(p_props["write_nearest_seed_uv"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    static void edt_1d_pass(const float* f, float* d, int32_t* v, float* z, int32_t n) noexcept {
        constexpr float INF = 1e20f;
        int32_t k = 0;
        v[0] = 0;
        z[0] = -INF;
        z[1] = +INF;

        for (int32_t q = 1; q < n; ++q) {
            float s = ((f[q] + static_cast<float>(q * q)) - (f[v[k]] + static_cast<float>(v[k] * v[k]))) /
                      static_cast<float>(2 * q - 2 * v[k]);
            while (s <= z[k]) {
                --k;
                s = ((f[q] + static_cast<float>(q * q)) - (f[v[k]] + static_cast<float>(v[k] * v[k]))) /
                    static_cast<float>(2 * q - 2 * v[k]);
            }
            ++k;
            v[k] = q;
            z[k] = s;
            z[k + 1] = +INF;
        }

        k = 0;
        for (int32_t q = 0; q < n; ++q) {
            while (z[k + 1] < static_cast<float>(q)) {
                ++k;
            }
            const float dx = static_cast<float>(q - v[k]);
            d[q] = dx * dx + f[v[k]];
        }
    }

    static void compute_edt_2d(float* grid, float* temp_row, int32_t* v, float* z, int32_t w, int32_t h) noexcept {
        for (int32_t y = 0; y < h; ++y) {
            float* row = grid + static_cast<size_t>(y) * w;
            edt_1d_pass(row, temp_row, v, z, w);
            for (int32_t x = 0; x < w; ++x) {
                row[x] = temp_row[x];
            }
        }
        for (int32_t x = 0; x < w; ++x) {
            for (int32_t y = 0; y < h; ++y) {
                temp_row[y] = grid[static_cast<size_t>(y) * w + x];
            }
            float* col_out = z + (h + 2);
            edt_1d_pass(temp_row, col_out, v, z, h);
            for (int32_t y = 0; y < h; ++y) {
                grid[static_cast<size_t>(y) * w + x] = col_out[y];
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const noexcept {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!p_ctx.local_workspace) return;

            const GrantPartPOD* src_part = (source_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(source_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            const GrantPartPOD* dst_part = (output_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(output_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : src_part);
            if (!src_part || !src_part->raw_base_ptr || !dst_part || !dst_part->raw_base_ptr) return;

            const int32_t w = std::clamp<int32_t>(grid_width, 1, MAX_EDT_GRID_DIM);
            const int32_t h = std::clamp<int32_t>(grid_height, 1, MAX_EDT_GRID_DIM);
            const size_t total_cells = static_cast<size_t>(w) * static_cast<size_t>(h);
            const size_t avail_elems = src_part->capacity_bytes / std::max<uint32_t>(1, src_part->element_stride);
            if (avail_elems < total_cells) return;

            constexpr float INF = 1e18f;
            uint8_t* ws = static_cast<uint8_t*>(p_ctx.local_workspace);
            float* d_out = reinterpret_cast<float*>(ws);
            float* d_in = d_out + total_cells;
            int32_t* seed_a = reinterpret_cast<int32_t*>(d_in + total_cells);
            int32_t* seed_b = seed_a + total_cells;

            const size_t max_dim = static_cast<size_t>(std::max(w, h)) + 2;
            float* temp_1d = reinterpret_cast<float*>(seed_b + total_cells);
            int32_t* v_buf = reinterpret_cast<int32_t*>(temp_1d + max_dim);
            float* z_buf = reinterpret_cast<float*>(v_buf + max_dim);

            const float* src_f = reinterpret_cast<const float*>(src_part->raw_base_ptr);
            const uint32_t src_floats_per_elem = std::max<uint32_t>(1, src_part->element_stride / sizeof(float));

            if (algorithm_mode == 0) {
                for (size_t i = 0; i < total_cells; ++i) {
                    const float val = src_f[i * src_floats_per_elem];
                    const bool inside = (val >= iso_threshold);
                    d_out[i] = inside ? 0.0f : INF;
                    d_in[i] = inside ? INF : 0.0f;
                }

                compute_edt_2d(d_out, temp_1d, v_buf, z_buf, w, h);
                if (signed_distance) {
                    compute_edt_2d(d_in, temp_1d, v_buf, z_buf, w, h);
                }

                float* dst_f = reinterpret_cast<float*>(dst_part->raw_base_ptr);
                const uint32_t dst_floats_per_elem = std::max<uint32_t>(1, dst_part->element_stride / sizeof(float));
                for (size_t i = 0; i < total_cells; ++i) {
                    const float dist_out = std::sqrt(std::max(0.0f, d_out[i]));
                    const float dist_in = signed_distance ? std::sqrt(std::max(0.0f, d_in[i])) : 0.0f;
                    const float sdf = (dist_out - dist_in) * cell_size;
                    dst_f[i * dst_floats_per_elem] = sdf;
                }
            } else {
                for (int32_t y = 0; y < h; ++y) {
                    for (int32_t x = 0; x < w; ++x) {
                        const size_t idx = static_cast<size_t>(y) * w + x;
                        const float val = src_f[idx * src_floats_per_elem];
                        const bool inside = (val >= iso_threshold);
                        seed_a[idx] = inside ? static_cast<int32_t>(idx) : -1;
                        seed_b[idx] = inside ? -1 : static_cast<int32_t>(idx);
                    }
                }

                auto run_jfa_pass = [&](int32_t* seeds, int32_t step) {
                    for (int32_t y = 0; y < h; ++y) {
                        for (int32_t x = 0; x < w; ++x) {
                            const size_t idx = static_cast<size_t>(y) * w + x;
                            int32_t best_seed = seeds[idx];
                            float best_d2 = INF;
                            if (best_seed >= 0) {
                                const int32_t sx = best_seed % w;
                                const int32_t sy = best_seed / w;
                                const float dx = static_cast<float>(x - sx);
                                const float dy = static_cast<float>(y - sy);
                                best_d2 = dx * dx + dy * dy;
                            }
                            for (int32_t oy = -1; oy <= 1; ++oy) {
                                const int32_t ny = y + oy * step;
                                if (ny < 0 || ny >= h) continue;
                                for (int32_t ox = -1; ox <= 1; ++ox) {
                                    if (ox == 0 && oy == 0) continue;
                                    const int32_t nx = x + ox * step;
                                    if (nx < 0 || nx >= w) continue;
                                    const int32_t cand = seeds[static_cast<size_t>(ny) * w + nx];
                                    if (cand < 0) continue;
                                    const int32_t cx = cand % w;
                                    const int32_t cy = cand / w;
                                    const float dx = static_cast<float>(x - cx);
                                    const float dy = static_cast<float>(y - cy);
                                    const float d2 = dx * dx + dy * dy;
                                    if (d2 < best_d2) {
                                        best_d2 = d2;
                                        best_seed = cand;
                                    }
                                }
                            }
                            seeds[idx] = best_seed;
                        }
                    }
                };

                int32_t max_span = std::max(w, h);
                int32_t step = 1;
                while (step * 2 < max_span) step *= 2;
                for (; step >= 1; step /= 2) {
                    run_jfa_pass(seed_a, step);
                    if (signed_distance) run_jfa_pass(seed_b, step);
                }
                run_jfa_pass(seed_a, 1);
                if (signed_distance) run_jfa_pass(seed_b, 1);

                float* dst_f = reinterpret_cast<float*>(dst_part->raw_base_ptr);
                const uint32_t dst_floats_per_elem = std::max<uint32_t>(1, dst_part->element_stride / sizeof(float));
                const float inv_w = 1.0f / static_cast<float>(w);
                const float inv_h = 1.0f / static_cast<float>(h);

                for (int32_t y = 0; y < h; ++y) {
                    for (int32_t x = 0; x < w; ++x) {
                        const size_t idx = static_cast<size_t>(y) * w + x;
                        float dist_out = 0.0f;
                        float su = static_cast<float>(x) * inv_w;
                        float sv = static_cast<float>(y) * inv_h;
                        if (seed_a[idx] >= 0) {
                            const int32_t sx = seed_a[idx] % w;
                            const int32_t sy = seed_a[idx] / w;
                            const float dx = static_cast<float>(x - sx);
                            const float dy = static_cast<float>(y - sy);
                            dist_out = std::sqrt(dx * dx + dy * dy);
                            su = static_cast<float>(sx) * inv_w;
                            sv = static_cast<float>(sy) * inv_h;
                        }
                        float dist_in = 0.0f;
                        if (signed_distance && seed_b[idx] >= 0) {
                            const int32_t sx = seed_b[idx] % w;
                            const int32_t sy = seed_b[idx] / w;
                            const float dx = static_cast<float>(x - sx);
                            const float dy = static_cast<float>(y - sy);
                            dist_in = std::sqrt(dx * dx + dy * dy);
                        }
                        const float sdf = (dist_out - dist_in) * cell_size;
                        float* elem_out = dst_f + idx * dst_floats_per_elem;
                        elem_out[0] = sdf;
                        if (write_nearest_seed_uv && dst_floats_per_elem >= 3) {
                            elem_out[1] = su;
                            elem_out[2] = sv;
                        }
                    }
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(SDFJumpFloodEDTTransformLogic<>) == 64, "SDFJumpFloodEDTTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SDFJumpFloodEDTTransformLogic<>>, "SDFJumpFloodEDTTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
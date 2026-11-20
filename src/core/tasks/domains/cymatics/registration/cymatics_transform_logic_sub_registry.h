#pragma once

#include "cymatics_transform_task_registry.h"
#include "../../../registration/task_view_bridge.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../transform_task.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

// --- Cymatics Transform Logics (Section 1.5: QD3) ---
#include "../transform_logic/acoustic_wave_propagation_transform_logic.h"
#include "../transform_logic/cymatic_chladni_modal_field_transform_logic.h"
#include "../transform_logic/cymatic_particle_nodal_drift_transform_logic.h"
#include "../transform_logic/cymatic_mesh_resonance_transform_logic.h"
#include "../transform_logic/acoustic_directional_sector_reduce_transform_logic.h"

// --- Views & Strategies ---
#include "../../../../memory/views/aosoa_view.h"
#include "../../../../memory/views/atomic_view.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/multi_element_view.h"
#include "../../../../memory/views/paged_view.h"
#include "../../../../memory/views/ring_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/sparse_set_view.h"
#include "../../../../memory/views/static_stencil_view.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/stencil_math.h"
#include "../../../../memory/views/swap_view.h"
#include "../../../../memory/views/strategies.h"

namespace ideam::core {

namespace {
    consteval size_t cymatics_trans_floor_pow2(size_t n) {
        if (n == 0) return 1;
        size_t res = 1;
        while ((res << 1) <= n) res <<= 1;
        return res;
    }

    template <MemoryTypes MemType, BufferAlignmentMode AlignMode, size_t TargetVectorWidthBytes>
    struct CymaticsTransformAOSOALaneCalculator {
        static constexpr DataType DataFlag = NativeMemoryTraits<MemType>::DataFlag;
        static constexpr size_t get_lane_width() {
            if constexpr (DataFlag == DataType::CUSTOM) return 4;
            else {
                constexpr size_t byte_size = MemoryUtilities::get_type_byte_size(DataFlag, AlignMode);
                if constexpr (byte_size == 0) return 1;
                constexpr size_t raw_lanes = TargetVectorWidthBytes / byte_size;
                return cymatics_trans_floor_pow2(raw_lanes > 0 ? raw_lanes : 1);
            }
        }
    };

    template <typename S, typename = void> struct CymaticsTransformStrategyDimExtractor { static constexpr size_t value = 1; };
    template <typename S> struct CymaticsTransformStrategyDimExtractor<S, std::void_t<decltype(S::dimensions)>> { static constexpr size_t value = S::dimensions; };

    template <typename T_Resolver, typename Enable = void> struct CymaticsTransformKernelExtractorImpl { static constexpr size_t value = 0; static constexpr bool has_kernel = false; };
    template <typename T_Resolver> struct CymaticsTransformKernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::K_SIZE)>> { static constexpr size_t value = T_Resolver::K_SIZE; static constexpr bool has_kernel = true; };

    // --- CymaticsTransformLogicResolver Specializations (Section 2.3: MND4) ---
    template <CymaticsTransformLogicID ID, typename T_Concrete, typename T_Strategy>
    struct CymaticsTransformLogicResolver { static constexpr bool is_valid = false; };
    template <typename C, typename S> struct CymaticsTransformLogicResolver<CymaticsTransformLogicID::AcousticWavePropagation, C, S> { using Type = AcousticWavePropagationTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct CymaticsTransformLogicResolver<CymaticsTransformLogicID::CymaticChladniModalField, C, S> { using Type = CymaticChladniModalFieldTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct CymaticsTransformLogicResolver<CymaticsTransformLogicID::CymaticParticleNodalDrift, C, S> { using Type = CymaticParticleNodalDriftTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct CymaticsTransformLogicResolver<CymaticsTransformLogicID::CymaticMeshResonance, C, S> { using Type = CymaticMeshResonanceTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct CymaticsTransformLogicResolver<CymaticsTransformLogicID::AcousticDirectionalSectorReduce, C, S> { using Type = AcousticDirectionalSectorReduceTransformLogic<C>; static constexpr bool is_valid = true; };

    template <CymaticsTransformLogicID LogicID, typename C, typename S>
    struct CymaticsTransformLogicKernelExtractor : CymaticsTransformKernelExtractorImpl<CymaticsTransformLogicResolver<LogicID, C, S>> {};

    template <MemoryStrategy ID> struct CymaticsTransformStrategyResolver { static constexpr bool is_valid = false; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::FlatStrategy> { using Type = FlatStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::SoAStrategy> { using Type = SoAStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::AoSStrategy> { using Type = AoSStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::Spatial2DStrategy> { using Type = Spatial2DStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::Spatial3DStrategy> { using Type = Spatial3DStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::Spatial4DStrategy> { using Type = Spatial4DStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::TiledSoAStrategy> { using Type = TiledSoAStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::RingStrategy> { using Type = RingStrategy; static constexpr bool is_valid = true; };
    template <> struct CymaticsTransformStrategyResolver<MemoryStrategy::PagedStrategy> { using Type = PagedStrategy; static constexpr bool is_valid = true; };

    template <MemoryView ViewID, CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct CymaticsTransformViewResolver { static constexpr bool is_valid = false; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = CymaticsTransformStrategyDimExtractor<S>::value; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = CymaticsTransformLogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = CymaticsTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = CymaticsTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = CymaticsTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = CymaticsTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = CymaticsTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <CymaticsTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct CymaticsTransformViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = CymaticsTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
} // namespace

template <CymaticsTransformLogicID L>
struct CymaticsTransformLogicSubRegistry {
    static std::array<CymaticsTransformTaskFactoryFn, CymaticsTransformTaskRegistry::SUB_MATRIX_SIZE> factories;

    static void init_execution_routing() {
        _fill_matrix<false>(std::make_index_sequence<CymaticsTransformTaskRegistry::SUB_MATRIX_SIZE>{}, nullptr);
    }

    static void cleanup_execution_routing() {
        factories.fill(nullptr);
    }

    static void generate_ui_matrices(godot::Dictionary &p_matrix) {
        _fill_matrix<true>(std::make_index_sequence<CymaticsTransformTaskRegistry::SUB_MATRIX_SIZE>{}, &p_matrix);
    }

private:
    template <bool BuildUI, size_t... Indices>
    static void _fill_matrix(std::index_sequence<Indices...>, godot::Dictionary *p_matrix) {
        (_fill_single<BuildUI, Indices>(p_matrix), ...);
    }

    template <bool BuildUI, size_t FlatIdx>
    static void _fill_single(godot::Dictionary *p_matrix) {
        constexpr size_t T_COUNT = CymaticsTransformTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = CymaticsTransformTaskRegistry::S_COUNT;

        constexpr size_t T = FlatIdx % T_COUNT;
        constexpr size_t S = (FlatIdx / T_COUNT) % S_COUNT;
        constexpr size_t V = FlatIdx / (T_COUNT * S_COUNT);

        constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
        constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
        constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);

        using Traits = NativeMemoryTraits<MemTypeEnum>;
        using ConcreteType = typename Traits::ConcreteType;

        using ResolvedStrategy = CymaticsTransformStrategyResolver<StrategyEnum>;
        if constexpr (!ResolvedStrategy::is_valid) return;
        using T_Strategy = typename ResolvedStrategy::Type;

        using ResolvedLogic = CymaticsTransformLogicResolver<L, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedLogic::is_valid) return;
        using L_Type = typename ResolvedLogic::Type;

        using ResolvedView = CymaticsTransformViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedView::is_valid) return;
        using V_Type = typename ResolvedView::Type;
        using V_Traits = ViewTraits<V_Type>;

        constexpr bool is_fully_valid = []() consteval {
            constexpr ViewStrategies iter_strategy_mask = to_view_strategy_mask(StrategyEnum);
            if ((V_Traits::supported_strategies & iter_strategy_mask) == ViewStrategies::NONE) return false;
            constexpr DataType iter_type_mask = Traits::DataFlag;
            if ((L_Type::required_types & iter_type_mask) == DataType::NONE) return false;
            if ((V_Traits::supported_types & iter_type_mask) == DataType::NONE) return false;
            return TransformLogicValidator::validate<L_Type, V_Type>();
        }();

        if constexpr (is_fully_valid) {
            if constexpr (!BuildUI) {
                factories[FlatIdx] = []() -> INativeTask * {
                    return new TransformTask<L_Type, V_Type, T_Strategy>();
                };
            } else if (p_matrix) {
                godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));
                if (!p_matrix->has(logic_key)) {
                    godot::Dictionary dict;
                    dict["properties"] = L_Type::get_ui_properties();
                    dict["valid_combinations"] = godot::PackedInt64Array();
                    dict["name"] = godot::String(L_Type::display_name.data());
                    constexpr std::string_view cat = LogicCategoryExtractor<L_Type>::value;
                    dict["category"] = godot::String(cat.data());
                    (*p_matrix)[logic_key] = dict;
                }
                godot::Dictionary dict = (*p_matrix)[logic_key];
                godot::PackedInt64Array combos = dict["valid_combinations"];
                combos.push_back(static_cast<int64_t>(FlatIdx));
                dict["valid_combinations"] = combos;
                (*p_matrix)[logic_key] = dict;
            }
        }
    }
};

template <CymaticsTransformLogicID L>
std::array<CymaticsTransformTaskFactoryFn, CymaticsTransformTaskRegistry::SUB_MATRIX_SIZE>
CymaticsTransformLogicSubRegistry<L>::factories = [] {
    std::array<CymaticsTransformTaskFactoryFn, CymaticsTransformTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core

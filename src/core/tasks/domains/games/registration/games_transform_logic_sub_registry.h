#pragma once

#include "games_transform_task_registry.h"
#include "../../../registration/task_view_bridge.h"
#include "../../../transform_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

// --- Games Transform Logics ---
#include "../transform_logic/agent_action_sequence_step_transform_logic.h"
#include "../transform_logic/agent_possession_transform_blend_transform_logic.h"
#include "../transform_logic/consequence_ring_drain_transform_logic.h"
#include "../transform_logic/control_map_input_filter_transform_logic.h"
#include "../transform_logic/entity_time_scale_cascade_transform_logic.h"
#include "../transform_logic/game_entity_snapshot_sync_transform_logic.h"
#include "../transform_logic/gameplay_rule_apply_transform_logic.h"
#include "../transform_logic/interaction_tug_of_war_resolve_transform_logic.h"
#include "../transform_logic/piece_action_score_refresh_transform_logic.h"
#include "../transform_logic/property_modifier_aggregate_transform_logic.h"
#include "../transform_logic/property_modifier_tick_transform_logic.h"
#include "../transform_logic/property_range_clamp_and_exhaust_transform_logic.h"

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
    consteval size_t games_transform_floor_pow2(size_t n) {
        if (n == 0) return 1;
        size_t res = 1;
        while ((res << 1) <= n) res <<= 1;
        return res;
    }

    template <MemoryTypes MemType, BufferAlignmentMode AlignMode, size_t TargetVectorWidthBytes>
    struct GamesTransformAOSOALaneCalculator {
        static constexpr DataType DataFlag = NativeMemoryTraits<MemType>::DataFlag;
        static constexpr size_t get_lane_width() {
            if constexpr (DataFlag == DataType::CUSTOM) return 4;
            else {
                constexpr size_t byte_size = MemoryUtilities::get_type_byte_size(DataFlag, AlignMode);
                if constexpr (byte_size == 0) return 1;
                constexpr size_t raw_lanes = TargetVectorWidthBytes / byte_size;
                return games_transform_floor_pow2(raw_lanes > 0 ? raw_lanes : 1);
            }
        }
    };

    template <typename S, typename = void> struct GamesTransformStrategyDimExtractor { static constexpr size_t value = 1; };
    template <typename S> struct GamesTransformStrategyDimExtractor<S, std::void_t<decltype(S::dimensions)>> { static constexpr size_t value = S::dimensions; };

    template <typename T_Resolver, typename Enable = void> struct GamesTransformKernelExtractorImpl { static constexpr size_t value = 0; static constexpr bool has_kernel = false; };
    template <typename T_Resolver> struct GamesTransformKernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::K_SIZE)>> { static constexpr size_t value = T_Resolver::K_SIZE; static constexpr bool has_kernel = true; };

    // --- GamesTransformLogicResolver Specializations ---
    template <GamesTransformLogicID ID, typename T_Concrete, typename T_Strategy>
    struct GamesTransformLogicResolver { static constexpr bool is_valid = false; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::AgentActionSequenceStep, C, S> { using Type = AgentActionSequenceStepTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::AgentPossessionTransformBlend, C, S> { using Type = AgentPossessionTransformBlendTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::ConsequenceRingDrain, C, S> { using Type = ConsequenceRingDrainTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::ControlMapInputFilter, C, S> { using Type = ControlMapInputFilterTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::EntityTimeScaleCascade, C, S> { using Type = EntityTimeScaleCascadeTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::GameEntitySnapshotSync, C, S> { using Type = GameEntitySnapshotSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::GameplayRuleApply, C, S> { using Type = GameplayRuleApplyTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::InteractionTugOfWarResolve, C, S> { using Type = InteractionTugOfWarResolveTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::PieceActionScoreRefresh, C, S> { using Type = PieceActionScoreRefreshTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::PropertyModifierAggregate, C, S> { using Type = PropertyModifierAggregateTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::PropertyModifierTick, C, S> { using Type = PropertyModifierTickTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct GamesTransformLogicResolver<GamesTransformLogicID::PropertyRangeClampAndExhaust, C, S> { using Type = PropertyRangeClampAndExhaustTransformLogic<C>; static constexpr bool is_valid = true; };

    template <GamesTransformLogicID LogicID, typename C, typename S>
    struct GamesTransformLogicKernelExtractor : GamesTransformKernelExtractorImpl<GamesTransformLogicResolver<LogicID, C, S>> {};

    template <MemoryStrategy ID> struct GamesTransformStrategyResolver { static constexpr bool is_valid = false; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::FlatStrategy> { using Type = FlatStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::SoAStrategy> { using Type = SoAStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::AoSStrategy> { using Type = AoSStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::Spatial2DStrategy> { using Type = Spatial2DStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::Spatial3DStrategy> { using Type = Spatial3DStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::Spatial4DStrategy> { using Type = Spatial4DStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::TiledSoAStrategy> { using Type = TiledSoAStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::RingStrategy> { using Type = RingStrategy; static constexpr bool is_valid = true; };
    template <> struct GamesTransformStrategyResolver<MemoryStrategy::PagedStrategy> { using Type = PagedStrategy; static constexpr bool is_valid = true; };

    template <MemoryView ViewID, GamesTransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct GamesTransformViewResolver { static constexpr bool is_valid = false; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = GamesTransformStrategyDimExtractor<S>::value; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = GamesTransformLogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GamesTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GamesTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GamesTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GamesTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GamesTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GamesTransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GamesTransformViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GamesTransformAOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
} // namespace

template <GamesTransformLogicID L>
struct GamesTransformLogicSubRegistry {
    static std::array<GamesTransformTaskFactoryFn, GamesTransformTaskRegistry::SUB_MATRIX_SIZE> factories;

    // Fast-path: Instantiates C++ function pointers for runtime execution
    static void init_execution_routing() {
        _fill_matrix<false>(std::make_index_sequence<GamesTransformTaskRegistry::SUB_MATRIX_SIZE>{}, nullptr);
    }

    static void cleanup_execution_routing() {
        factories.fill(nullptr);
    }

    // Heavy-path: Allocates Godot Dictionaries for the Editor UI
    static void generate_ui_matrices(godot::Dictionary& p_matrix) {
        IdeamTaskRegistry::log_with_registry(godot::vformat("    -> [GamesTransform] Evaluating LogicID: %d", static_cast<int64_t>(L)));
        _fill_matrix<true>(std::make_index_sequence<GamesTransformTaskRegistry::SUB_MATRIX_SIZE>{}, &p_matrix);

        // POST-EVALUATION METRIC GATHERING
        godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));
        if (p_matrix.has(logic_key)) {
            godot::Dictionary dict = p_matrix[logic_key];
            godot::PackedInt64Array combos = dict["valid_combinations"];
            IdeamTaskRegistry::log_with_registry(godot::vformat("      -> SUCCESS: LogicID %d appended with %d valid combinations.", static_cast<int64_t>(L), combos.size()));
        } else {
            IdeamTaskRegistry::log_with_registry(godot::vformat("      -> FAILURE: LogicID %d resulted in 0 valid combinations. Key not created.", static_cast<int64_t>(L)));
        }
    }

private:
    template <bool BuildUI, size_t... Indices>
    static void _fill_matrix(std::index_sequence<Indices...>, godot::Dictionary* p_matrix) {
        (_fill_single<BuildUI, Indices>(p_matrix), ...);
    }

    template <bool BuildUI, size_t FlatIdx>
    static void _fill_single(godot::Dictionary* p_matrix) {
        constexpr size_t T_COUNT = GamesTransformTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = GamesTransformTaskRegistry::S_COUNT;

        constexpr size_t T = FlatIdx % T_COUNT;
        constexpr size_t S = (FlatIdx / T_COUNT) % S_COUNT;
        constexpr size_t V = FlatIdx / (T_COUNT * S_COUNT);

        constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
        constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
        constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);

        using Traits = NativeMemoryTraits<MemTypeEnum>;
        using ConcreteType = typename Traits::ConcreteType;

        // --- FAST-FAIL TIER 1: Base Resolution ---
        using ResolvedStrategy = GamesTransformStrategyResolver<StrategyEnum>;
        if constexpr (!ResolvedStrategy::is_valid) return;
        using T_Strategy = typename ResolvedStrategy::Type;

        using ResolvedLogic = GamesTransformLogicResolver<L, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedLogic::is_valid) return;
        using L_Type = typename ResolvedLogic::Type;

        using ResolvedView = GamesTransformViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedView::is_valid) return;
        using V_Type = typename ResolvedView::Type;
        using V_Traits = ViewTraits<V_Type>;

        // --- FAST-FAIL TIER 2: Deep DOD Intersection ---
        constexpr bool is_fully_valid = []() consteval {
            // 1. Does the View support this Strategy at all?
            constexpr ViewStrategies iter_strategy_mask = to_view_strategy_mask(StrategyEnum);
            if ((V_Traits::supported_strategies & iter_strategy_mask) == ViewStrategies::NONE) return false;

            // 2. Does the primitive type satisfy both the View and the Logic?
            constexpr DataType iter_type_mask = Traits::DataFlag;
            if ((L_Type::required_types & iter_type_mask) == DataType::NONE) return false;
            if ((V_Traits::supported_types & iter_type_mask) == DataType::NONE) return false;

            // 3. Deep DOD Validation via Types (Capabilities, Layouts, Types, Dimensions, Kernels)
            return TransformLogicValidator::validate<L_Type, V_Type>();
        }();

        // --- FACTORY GENERATION ---
        if constexpr (is_fully_valid) {
            if constexpr (!BuildUI) {
                // Hot-Path Execution Routing
                factories[FlatIdx] = []() -> INativeTask* {
                    return new TransformTask<L_Type, V_Type, T_Strategy>();
                };
            } else {
                // Cold-Path UI Collection
                if (p_matrix) {
                    IdeamTaskRegistry::log_with_registry(
                        godot::vformat("        -> Valid Combo: T=%d, S=%d, V=%d(FlatIdx: %d)", T, S, V, FlatIdx)
                    );

                    godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));

                    if (!p_matrix->has(logic_key)) {
                        godot::Dictionary dict;
                        // Instantiate the property array exactly once per logic struct type
                        dict["properties"] = L_Type::get_ui_properties();
                        dict["valid_combinations"] = godot::PackedInt64Array();
                        dict["name"] = godot::String(L_Type::display_name.data());

                        constexpr std::string_view cat = LogicCategoryExtractor<L_Type>::value;
                        dict["category"] = godot::String(cat.data());

                        const auto& variant_info = GamesTransformTaskRegistry::logic_variants[static_cast<size_t>(L)];
                        if (variant_info.is_primary && variant_info.variant_count > 1) {
                            dict["variant_count"] = variant_info.variant_count;

                            godot::Array ui_labels;
                            for (uint32_t i = 0; i < variant_info.variant_count; ++i) {
                                ui_labels.push_back(godot::String(variant_info.labels[i]));
                            }
                            dict["variant_labels"] = ui_labels;
                        }

                        (*p_matrix)[logic_key] = dict;
                    }

                    // Map this valid 3D configuration hash (FlatIdx) so the UI knows it's an allowed permutation
                    godot::Dictionary dict = (*p_matrix)[logic_key];
                    godot::PackedInt64Array combos = dict["valid_combinations"];
                    combos.push_back(static_cast<int64_t>(FlatIdx));
                    dict["valid_combinations"] = combos;
                    (*p_matrix)[logic_key] = dict;
                }
            }
        }
    }
};

template <GamesTransformLogicID L>
std::array<GamesTransformTaskFactoryFn, GamesTransformTaskRegistry::SUB_MATRIX_SIZE>
GamesTransformLogicSubRegistry<L>::factories = [] {
    std::array<GamesTransformTaskFactoryFn, GamesTransformTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core
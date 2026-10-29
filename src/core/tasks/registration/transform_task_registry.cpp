#include "transform_task_registry.h"
#include "transform_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

namespace { // Translation Unit Firewall
    // --- UI Labels for OptionButtons ---
    constexpr const char* stencil_labels[] = { 
        "Moore (R1)", "Moore (R2)", "Moore (R3)", 
        "Von Neumann (R1)", "Von Neumann (R2)", "Von Neumann (R3)" 
    };

    constexpr const char* blend_shape_labels[] = {
    "1 Shape", "2 Shapes", "4 Shapes", "8 Shapes"
    };
} // namespace

// --- Populate the Variant Map ---
const std::array<LogicVariantGroup, TransformTaskRegistry::L_COUNT> TransformTaskRegistry::logic_variants = {{
    // BlendShapeEvaluate_1Shape (0)
    { true, 4, blend_shape_labels },
    // BlendShapeEvaluate_2Shapes, 4Shapes, 8Shapes (1, 2, 3)
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },

    // BoundaryConstraint (4)
    { true, 1, nullptr },
    // BoundsExtraction (5)
    { true, 1, nullptr },
    // CollisionShapeMeshSync (6)
    { true, 1, nullptr },
    // CpuLinearBlendSkinning (7)
    { true, 1, nullptr },
    // DataScatter (8)
    { true, 1, nullptr },
    // DataSort (9)
    { true, 1, nullptr },
    // EulerIntegration (10)
    { true, 1, nullptr },
    // FabrikChainIK (11)
    { true, 1, nullptr },
    // FaceCenterAndArea (12)
    { true, 1, nullptr },
    // FastNoiseLite (13)
    { true, 1, nullptr },
    // ForwardKinematicsChain (14)
    { true, 1, nullptr },

    // ImageChannelExtract (15)
    { true, 1, nullptr },
    // ImageChannelPack (16)
    { true, 1, nullptr },
    // ImageNormalizeScalar (17)
    { true, 1, nullptr },
    // ImagePaletteLut (18)
    { true, 1, nullptr },
    // ImageRawExport (19)
    { true, 1, nullptr },
    // ImageRawImport (20)
    { true, 1, nullptr },
    // ImageSpatial2DExport (21)
    { true, 1, nullptr },
    // ImageSpatial2DImport (22)
    { true, 1, nullptr },

    // MeshAttributeRegionCommit (23)
    { true, 1, nullptr },
    // MeshBoundsSync (24)
    { true, 1, nullptr },
    // MeshSkinRegionCommit (25)
    { true, 1, nullptr },
    // MeshStreamPack (26)
    { true, 1, nullptr },
    // MeshStreamUnpack (27)
    { true, 1, nullptr },
    // MeshSurfaceExport (28)
    { true, 1, nullptr },
    // MeshSurfaceImport (29)
    { true, 1, nullptr },
    // MeshSurfaceScatter (30)
    { true, 1, nullptr },
    // MeshUVProjection (31)
    { true, 1, nullptr },
    // MeshVertexColorMap (32)
    { true, 1, nullptr },
    // MeshVertexDeform (33)
    { true, 1, nullptr },
    // MeshVertexRegionCommit (34)
    { true, 1, nullptr },
    // MultiMeshBufferSync (35)
    { true, 1, nullptr },

    // NoiseInjection (36)
    { true, 1, nullptr },
    // NormalRecalc (37)
    { true, 1, nullptr },
    // PackedArrayExport (38)
    { true, 1, nullptr },
    // SkeletonPoseBridge (39)
    { true, 1, nullptr },
    // SpringBoneChain (40)
    { true, 1, nullptr },

    // Stencil_Moore_R1 (41)
    { true, 6, stencil_labels },
    // Stencil_Moore_R2, R3 (42, 43)
    { false, 0, nullptr },
    { false, 0, nullptr },
    // Stencil_VonNeumann_R1, R2, R3 (44, 45, 46)
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },

    // TangentRecalc (47)
    { true, 1, nullptr },
    // Transform3DBatch (48)
    { true, 1, nullptr },
    // TwoBoneIK (49)
    { true, 1, nullptr },
    // ValueAccumulation (50)
    { true, 1, nullptr }
}};

std::array<const TransformTaskRegistry::SubMatrix*, TransformTaskRegistry::L_COUNT> TransformTaskRegistry::logic_matrices = {};

namespace { // Translation Unit Firewall
    // --- Execution Routing (Fast Path) ---
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::init_execution_routing(), ...);
        ((TransformTaskRegistry::logic_matrices[Is] = &TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    // --- UI Matrices (Heavy Path) ---
    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary& p_matrix, std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void TransformTaskRegistry::init_execution_routing() {
    // Compiles into consecutive fast-path init calls and pointer assignments
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void TransformTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void TransformTaskRegistry::generate_ui_matrices(godot::Dictionary& p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> TransformTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    // Rips through valid template permutations to build the Godot Inspector UI
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> TransformTaskRegistry: Template unrolling complete.");
    
}


// --- The O(1) Dispatcher ---
std::unique_ptr<INativeTask> TransformTaskRegistry::create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id) {
    if (p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }

    // 1. O(1) Cache-line hit for the sub-registry pointer
    const SubMatrix* sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    // 2. O(1) Flat Index Math (Dropped the L dimension)
    size_t flat_idx = 
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }

    return nullptr;
}

} // namespace ideam::core
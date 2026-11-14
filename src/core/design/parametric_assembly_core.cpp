#include "parametric_assembly_core.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::core {

void ParametricAssemblyRecipe::clear() noexcept {
    part_templates_.clear();
    parameters_.clear();
    parameter_names_.clear();
}

uint32_t ParametricAssemblyRecipe::add_part_template(const AssemblyPartTemplatePOD &p_part) {
    const uint32_t idx = static_cast<uint32_t>(part_templates_.size());
    part_templates_.push_back(p_part);
    return idx;
}

uint32_t ParametricAssemblyRecipe::add_parameter(
    std::string_view p_name,
    const AssemblyParameterSlotPOD &p_param
) {
    const uint32_t hash = hash_reflection_symbol(p_name);
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i].param_name_hash == hash) {
            parameters_[i]       = p_param;
            parameters_[i].param_name_hash = hash;
            parameter_names_[i]  = std::string(p_name);
            return static_cast<uint32_t>(i);
        }
    }
    const uint32_t idx = static_cast<uint32_t>(parameters_.size());
    AssemblyParameterSlotPOD copy = p_param;
    copy.param_name_hash          = hash;
    copy.current_value            = std::clamp(copy.current_value, copy.min_value, copy.max_value);
    parameters_.push_back(copy);
    parameter_names_.emplace_back(p_name);
    return idx;
}

bool ParametricAssemblyRecipe::set_parameter_value(std::string_view p_name, float p_value) noexcept {
    const uint32_t hash = hash_reflection_symbol(p_name);
    for (auto &param : parameters_) {
        if (param.param_name_hash == hash) {
            param.current_value = std::clamp(p_value, param.min_value, param.max_value);
            return true;
        }
    }
    return false;
}

float ParametricAssemblyRecipe::get_parameter_value(std::string_view p_name, float p_default) const noexcept {
    const uint32_t hash = hash_reflection_symbol(p_name);
    for (const auto &param : parameters_) {
        if (param.param_name_hash == hash) {
            return param.current_value;
        }
    }
    return p_default;
}

void ParametricAssemblyRecipe::synthesize_to_layout_solver(
    StructureLayoutSolver &r_solver,
    float p_dream_morph_factor,
    uint64_t p_seed
) const {
    SeedStreamPOD rng(p_seed);

    // 1. Add Root Assembly Node
    StructureNodePod root_node{};
    root_node.parent_id                 = 0xFFFFFFFFu;
    root_node.semantic_tag_id           = root_semantic_role_id_;
    root_node.nuclear_seme_mask_lo32    = root_nuclear_seme_mask_;
    root_node.peripheral_seme_mask_lo32 = root_peripheral_seme_mask_;
    root_node.container_mode            = DesignContainerLayoutMode::LAYOUT_LINEAR_BOX;
    root_node.rest_obb_center           = glm::vec3(0.0f);
    root_node.rest_obb_extents          = glm::vec3(0.5f, 0.5f, 0.5f);
    const uint32_t root_id              = r_solver.add_structure_node(root_node);

    // Track spawned node IDs per part_role_id so child parts (e.g. CasterWheel under Leg)
    // can parent and socket-mate to each instance of their parent part!
    struct RoleSpawnGroup {
        uint32_t role_id = 0u;
        std::vector<uint32_t> node_ids;
    };
    std::vector<RoleSpawnGroup> spawned_groups;
    spawned_groups.push_back({ root_semantic_role_id_, { root_id } });

    auto find_spawned_for_role = [&](uint32_t p_role) -> std::vector<uint32_t> {
        for (const auto &g : spawned_groups) {
            if (g.role_id == p_role && !g.node_ids.empty()) {
                return g.node_ids;
            }
        }
        return { root_id };
    };

    for (const AssemblyPartTemplatePOD &tmpl : part_templates_) {
        // Check if required_sibling_role is satisfied
        if (tmpl.required_sibling_role != SEMANTIC_ROLE_NONE) {
            bool sib_present = false;
            for (const auto &g : spawned_groups) {
                if (g.role_id == tmpl.required_sibling_role && !g.node_ids.empty()) {
                    sib_present = true;
                    break;
                }
            }
            if (!sib_present) continue;
        }

        // Evaluate parameters targeting this part_role_id
        int32_t count         = static_cast<int32_t>(tmpl.default_count);
        glm::vec3 scale_mul   = glm::vec3(1.0f);
        glm::vec3 offset_add  = glm::vec3(0.0f);
        float radial_radius   = tmpl.radial_ring_radius;
        bool toggled_enabled  = (tmpl.default_count > 0u);

        for (const AssemblyParameterSlotPOD &param : parameters_) {
            if (match_semantic_role_hierarchy(param.target_part_role_id, tmpl.part_role_id) == 0u) {
                continue;
            }
            const float morph_delta = (p_dream_morph_factor != 0.0f && param.dream_morph_weight != 0.0f)
                ? (p_dream_morph_factor * param.dream_morph_weight * (rng.next_unit_float() * 2.0f - 1.0f))
                : 0.0f;
            const float val = std::clamp(param.current_value + morph_delta, param.min_value, param.max_value);

            switch (param.operation) {
                case AssemblyParamTargetOp::PARAM_OP_PART_COUNT:
                    count = static_cast<int32_t>(std::lround(val));
                    toggled_enabled = (count > 0);
                    break;
                case AssemblyParamTargetOp::PARAM_OP_SCALE_X:
                    scale_mul.x *= std::max(0.05f, val);
                    break;
                case AssemblyParamTargetOp::PARAM_OP_SCALE_Y:
                    scale_mul.y *= std::max(0.05f, val);
                    break;
                case AssemblyParamTargetOp::PARAM_OP_SCALE_Z:
                    scale_mul.z *= std::max(0.05f, val);
                    break;
                case AssemblyParamTargetOp::PARAM_OP_OFFSET_Y:
                    offset_add.y += val;
                    break;
                case AssemblyParamTargetOp::PARAM_OP_RADIAL_RADIUS:
                    radial_radius = std::max(0.0f, val);
                    break;
                case AssemblyParamTargetOp::PARAM_OP_TOGGLE_PART:
                    toggled_enabled = (val >= 0.5f);
                    if (toggled_enabled && count == 0) {
                        count = std::max<int32_t>(1, static_cast<int32_t>(tmpl.max_count));
                    } else if (!toggled_enabled) {
                        count = 0;
                    }
                    break;
            }
        }

        if (!toggled_enabled || count <= 0) {
            continue;
        }
        const uint32_t clamped_count = static_cast<uint32_t>(std::clamp<int32_t>(
            count,
            static_cast<int32_t>(tmpl.min_count),
            static_cast<int32_t>(std::max<uint16_t>(tmpl.min_count, tmpl.max_count))
        ));
        if (clamped_count == 0u) {
            continue;
        }

        const std::vector<uint32_t> parent_ids = find_spawned_for_role(tmpl.parent_part_role_id);
        RoleSpawnGroup group{ tmpl.part_role_id, {} };

        // If the parent part has multiple instances (e.g., 4 Legs) and this part attaches 1-per-parent
        // (e.g., CasterWheel under Leg), spawn 1 child under each parent instance!
        if (parent_ids.size() > 1u && tmpl.parent_part_role_id != root_semantic_role_id_) {
            for (uint32_t p_node_id : parent_ids) {
                StructureNodePod part_node{};
                part_node.parent_id                 = p_node_id;
                part_node.semantic_tag_id           = tmpl.part_role_id;
                part_node.style_slot_id             = tmpl.style_slot_id;
                part_node.nuclear_seme_mask_lo32    = tmpl.nuclear_seme_mask | root_nuclear_seme_mask_;
                part_node.peripheral_seme_mask_lo32 = tmpl.peripheral_seme_mask | root_peripheral_seme_mask_;
                part_node.scale_mode                = tmpl.scale_mode;
                part_node.edge_snap_policy          = tmpl.edge_snap_policy;
                part_node.size_flags_y              = tmpl.size_flags_y;
                part_node.container_mode            = tmpl.container_mode;
                part_node.rest_obb_center           = glm::vec3(
                    tmpl.base_center_offset.x + offset_add.x,
                    tmpl.base_center_offset.y + offset_add.y,
                    tmpl.base_center_offset.z + offset_add.z
                );
                part_node.rest_obb_extents          = glm::vec3(
                    tmpl.base_half_extents.x * scale_mul.x,
                    tmpl.base_half_extents.y * scale_mul.y,
                    tmpl.base_half_extents.z * scale_mul.z
                );
                const uint32_t new_id = r_solver.add_structure_node(part_node);
                group.node_ids.push_back(new_id);

                if (tmpl.spawns_socket_pair != 0u) {
                    SocketPod host_sock{};
                    host_sock.owner_node_id        = p_node_id;
                    host_sock.compatibility_bitmask = static_cast<uint64_t>(tmpl.part_role_id);
                    host_sock.position             = part_node.rest_obb_center;
                    host_sock.normal               = glm::vec3(0.0f, -1.0f, 0.0f);
                    host_sock.physics_binding_mode = tmpl.socket_physics_mode;
                    host_sock.is_male_plug         = 0u;
                    const uint32_t hs_id           = r_solver.add_socket(host_sock);

                    SocketPod plug_sock{};
                    plug_sock.owner_node_id        = new_id;
                    plug_sock.compatibility_bitmask = static_cast<uint64_t>(tmpl.part_role_id);
                    plug_sock.position             = glm::vec3(0.0f);
                    plug_sock.normal               = glm::vec3(0.0f, 1.0f, 0.0f);
                    plug_sock.physics_binding_mode = tmpl.socket_physics_mode;
                    plug_sock.is_male_plug         = 1u;
                    plug_sock.mated_socket_id      = hs_id;
                    plug_sock.is_occupied          = 1u;
                    const uint32_t ps_id           = r_solver.add_socket(plug_sock);
                    r_solver.sockets_mut()[hs_id].mated_socket_id = ps_id;
                    r_solver.sockets_mut()[hs_id].is_occupied     = 1u;
                }
            }
        } else {
            const uint32_t p_node_id = parent_ids.front();
            for (uint32_t idx = 0; idx < clamped_count; ++idx) {
                StructureNodePod part_node{};
                part_node.parent_id                 = p_node_id;
                part_node.semantic_tag_id           = tmpl.part_role_id;
                part_node.style_slot_id             = tmpl.style_slot_id;
                part_node.nuclear_seme_mask_lo32    = tmpl.nuclear_seme_mask | root_nuclear_seme_mask_;
                part_node.peripheral_seme_mask_lo32 = tmpl.peripheral_seme_mask | root_peripheral_seme_mask_;
                part_node.scale_mode                = tmpl.scale_mode;
                part_node.edge_snap_policy          = tmpl.edge_snap_policy;
                part_node.size_flags_y              = tmpl.size_flags_y;
                part_node.container_mode            = tmpl.container_mode;

                glm::vec3 center = glm::vec3(
                    tmpl.base_center_offset.x + offset_add.x,
                    tmpl.base_center_offset.y + offset_add.y,
                    tmpl.base_center_offset.z + offset_add.z
                );

                if (clamped_count > 1u && radial_radius > 0.0f) {
                    const float angle = (2.0f * std::numbers::pi_v<float> * static_cast<float>(idx)) / static_cast<float>(clamped_count);
                    center.x += std::cos(angle) * radial_radius;
                    center.z += std::sin(angle) * radial_radius;
                } else if (clamped_count == 2u) {
                    // Symmetric left/right placement (e.g. Armrests)
                    center.x = (idx == 0u) ? -std::abs(center.x) : std::abs(center.x);
                }

                part_node.rest_obb_center  = center;
                part_node.rest_obb_extents = glm::vec3(
                    tmpl.base_half_extents.x * scale_mul.x,
                    tmpl.base_half_extents.y * scale_mul.y,
                    tmpl.base_half_extents.z * scale_mul.z
                );
                const uint32_t new_id = r_solver.add_structure_node(part_node);
                group.node_ids.push_back(new_id);

                if (tmpl.spawns_socket_pair != 0u) {
                    SocketPod host_sock{};
                    host_sock.owner_node_id        = p_node_id;
                    host_sock.compatibility_bitmask = static_cast<uint64_t>(tmpl.part_role_id);
                    host_sock.position             = center;
                    host_sock.normal               = glm::vec3(0.0f, 1.0f, 0.0f);
                    host_sock.physics_binding_mode = tmpl.socket_physics_mode;
                    host_sock.is_male_plug         = 0u;
                    const uint32_t hs_id           = r_solver.add_socket(host_sock);

                    SocketPod plug_sock{};
                    plug_sock.owner_node_id        = new_id;
                    plug_sock.compatibility_bitmask = static_cast<uint64_t>(tmpl.part_role_id);
                    plug_sock.position             = glm::vec3(0.0f);
                    plug_sock.normal               = glm::vec3(0.0f, -1.0f, 0.0f);
                    plug_sock.physics_binding_mode = tmpl.socket_physics_mode;
                    plug_sock.is_male_plug         = 1u;
                    plug_sock.mated_socket_id      = hs_id;
                    plug_sock.is_occupied          = 1u;
                    const uint32_t ps_id           = r_solver.add_socket(plug_sock);
                    r_solver.sockets_mut()[hs_id].mated_socket_id = ps_id;
                    r_solver.sockets_mut()[hs_id].is_occupied     = 1u;
                }
            }
        }
        spawned_groups.push_back(std::move(group));
    }

    r_solver.solve_layout_constraints();
}

ParametricAssemblyRecipe ParametricAssemblyRecipe::create_chair_recipe() {
    ParametricAssemblyRecipe r;
    r.set_archetype_name("Chair");
    r.set_root_semantic_role_id(SEMANTIC_ROLE_SURFACE_CHAIR);
    r.set_root_seme_masks(0x0101u, 0x1000u);

    // 1. Seat Base [1..1]
    {
        AssemblyPartTemplatePOD seat{};
        seat.part_role_id        = SEMANTIC_ROLE_SURFACE_SEAT;
        seat.parent_part_role_id = SEMANTIC_ROLE_SURFACE_CHAIR;
        seat.min_count           = 1u;
        seat.max_count           = 1u;
        seat.default_count       = 1u;
        seat.base_center_offset  = glm::vec3(0.0f, 0.45f, 0.0f);
        seat.base_half_extents   = glm::vec3(0.25f, 0.04f, 0.25f);
        seat.socket_physics_mode = SocketPhysicsBindingMode::PHYSICS_BIND_STATIC_WELD;
        r.add_part_template(seat);
    }
    // 2. Legs [3..6, default 4] arranged radially under Seat
    {
        AssemblyPartTemplatePOD leg{};
        leg.part_role_id        = SEMANTIC_ROLE_COMPONENT_LEG;
        leg.parent_part_role_id = SEMANTIC_ROLE_SURFACE_SEAT;
        leg.min_count           = 3u;
        leg.max_count           = 6u;
        leg.default_count       = 4u;
        leg.container_mode      = DesignContainerLayoutMode::LAYOUT_RADIAL_RING;
        leg.base_center_offset  = glm::vec3(0.0f, -0.22f, 0.0f);
        leg.base_half_extents   = glm::vec3(0.03f, 0.22f, 0.03f);
        leg.radial_ring_radius  = 0.20f;
        leg.socket_physics_mode = SocketPhysicsBindingMode::PHYSICS_BIND_STATIC_WELD;
        r.add_part_template(leg);
    }
    // 3. Backrest [0..1, default 1] attached to Seat
    {
        AssemblyPartTemplatePOD back{};
        back.part_role_id        = SEMANTIC_ROLE_COMPONENT_BACKREST;
        back.parent_part_role_id = SEMANTIC_ROLE_SURFACE_SEAT;
        back.min_count           = 0u;
        back.max_count           = 1u;
        back.default_count       = 1u;
        back.base_center_offset  = glm::vec3(0.0f, 0.28f, -0.22f);
        back.base_half_extents   = glm::vec3(0.24f, 0.25f, 0.03f);
        back.socket_physics_mode = SocketPhysicsBindingMode::PHYSICS_BIND_STATIC_WELD;
        r.add_part_template(back);
    }
    // 4. Armrests [0..2, default 2]
    {
        AssemblyPartTemplatePOD arm{};
        arm.part_role_id        = SEMANTIC_ROLE_COMPONENT_ARMREST;
        arm.parent_part_role_id = SEMANTIC_ROLE_SURFACE_SEAT;
        arm.min_count           = 0u;
        arm.max_count           = 2u;
        arm.default_count       = 2u;
        arm.base_center_offset  = glm::vec3(0.24f, 0.16f, 0.0f);
        arm.base_half_extents   = glm::vec3(0.03f, 0.08f, 0.20f);
        arm.socket_physics_mode = SocketPhysicsBindingMode::PHYSICS_BIND_STATIC_WELD;
        r.add_part_template(arm);
    }
    // 5. Caster Wheels [0..6, default 0; requires Leg, uses PinJoint3D swivel socket!]
    {
        AssemblyPartTemplatePOD wheel{};
        wheel.part_role_id          = SEMANTIC_ROLE_COMPONENT_CASTER_WHEEL;
        wheel.parent_part_role_id   = SEMANTIC_ROLE_COMPONENT_LEG;
        wheel.required_sibling_role = SEMANTIC_ROLE_COMPONENT_LEG;
        wheel.min_count             = 0u;
        wheel.max_count             = 6u;
        wheel.default_count         = 0u;
        wheel.base_center_offset    = glm::vec3(0.0f, -0.24f, 0.0f);
        wheel.base_half_extents     = glm::vec3(0.03f, 0.03f, 0.03f);
        wheel.socket_physics_mode   = SocketPhysicsBindingMode::PHYSICS_BIND_PIN_JOINT_3D;
        r.add_part_template(wheel);
    }

    // Exposed Chair Parameters
    r.add_parameter("leg_count",       { 0u, SEMANTIC_ROLE_COMPONENT_LEG,          4.0f, 3.0f, 6.0f, 1.0f, 1.0f, AssemblyParamTargetOp::PARAM_OP_PART_COUNT });
    r.add_parameter("has_wheels",      { 0u, SEMANTIC_ROLE_COMPONENT_CASTER_WHEEL, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, AssemblyParamTargetOp::PARAM_OP_TOGGLE_PART });
    r.add_parameter("has_armrests",    { 0u, SEMANTIC_ROLE_COMPONENT_ARMREST,      1.0f, 0.0f, 1.0f, 1.0f, 0.0f, AssemblyParamTargetOp::PARAM_OP_TOGGLE_PART });
    r.add_parameter("seat_height",     { 0u, SEMANTIC_ROLE_SURFACE_SEAT,           0.0f,-0.2f, 0.4f, 0.05f,0.2f, AssemblyParamTargetOp::PARAM_OP_OFFSET_Y });
    r.add_parameter("backrest_height", { 0u, SEMANTIC_ROLE_COMPONENT_BACKREST,     1.0f, 0.4f, 2.5f, 0.1f, 0.8f, AssemblyParamTargetOp::PARAM_OP_SCALE_Y });

    return r;
}

ParametricAssemblyRecipe ParametricAssemblyRecipe::create_table_recipe() {
    ParametricAssemblyRecipe r;
    r.set_archetype_name("Table");
    r.set_root_semantic_role_id(SEMANTIC_ROLE_SURFACE_TABLE);
    r.set_root_seme_masks(0x0102u, 0x1000u);

    AssemblyPartTemplatePOD top{};
    top.part_role_id        = SEMANTIC_ROLE_SURFACE_SUPPORT;
    top.parent_part_role_id = SEMANTIC_ROLE_SURFACE_TABLE;
    top.default_count       = 1u;
    top.base_center_offset  = glm::vec3(0.0f, 0.75f, 0.0f);
    top.base_half_extents   = glm::vec3(0.60f, 0.04f, 0.40f);
    r.add_part_template(top);

    AssemblyPartTemplatePOD leg{};
    leg.part_role_id        = SEMANTIC_ROLE_COMPONENT_LEG;
    leg.parent_part_role_id = SEMANTIC_ROLE_SURFACE_SUPPORT;
    leg.min_count           = 1u;
    leg.max_count           = 8u;
    leg.default_count       = 4u;
    leg.container_mode      = DesignContainerLayoutMode::LAYOUT_RADIAL_RING;
    leg.base_center_offset  = glm::vec3(0.0f, -0.37f, 0.0f);
    leg.base_half_extents   = glm::vec3(0.04f, 0.37f, 0.04f);
    leg.radial_ring_radius  = 0.45f;
    r.add_part_template(leg);

    r.add_parameter("leg_count",   { 0u, SEMANTIC_ROLE_COMPONENT_LEG,   4.0f, 1.0f, 8.0f, 1.0f, 1.0f, AssemblyParamTargetOp::PARAM_OP_PART_COUNT });
    r.add_parameter("table_width", { 0u, SEMANTIC_ROLE_SURFACE_SUPPORT, 1.0f, 0.5f, 3.0f, 0.1f, 0.5f, AssemblyParamTargetOp::PARAM_OP_SCALE_X });
    return r;
}

ParametricAssemblyRecipe ParametricAssemblyRecipe::create_organic_tree_recipe() {
    ParametricAssemblyRecipe r;
    r.set_archetype_name("OrganicTree");
    r.set_root_semantic_role_id(SEMANTIC_ROLE_ASSEMBLY_ORGANIC);
    r.set_root_seme_masks(0x0501u, 0x2000u);

    AssemblyPartTemplatePOD trunk{};
    trunk.part_role_id        = SEMANTIC_ROLE_ORGANIC_TRUNK;
    trunk.parent_part_role_id = SEMANTIC_ROLE_ASSEMBLY_ORGANIC;
    trunk.default_count       = 1u;
    trunk.scale_mode          = DesignScaleMode::SPLINE_LOFT_DEFORM;
    trunk.base_center_offset  = glm::vec3(0.0f, 1.5f, 0.0f);
    trunk.base_half_extents   = glm::vec3(0.25f, 1.5f, 0.25f);
    r.add_part_template(trunk);

    AssemblyPartTemplatePOD branch{};
    branch.part_role_id        = SEMANTIC_ROLE_ORGANIC_BRANCH;
    branch.parent_part_role_id = SEMANTIC_ROLE_ORGANIC_TRUNK;
    branch.min_count           = 2u;
    branch.max_count           = 8u;
    branch.default_count       = 4u;
    branch.container_mode      = DesignContainerLayoutMode::LAYOUT_RADIAL_RING;
    branch.scale_mode          = DesignScaleMode::SPLINE_LOFT_DEFORM;
    branch.base_center_offset  = glm::vec3(0.0f, 1.2f, 0.0f);
    branch.base_half_extents   = glm::vec3(0.10f, 0.8f, 0.10f);
    branch.radial_ring_radius  = 0.65f;
    r.add_part_template(branch);

    AssemblyPartTemplatePOD canopy{};
    canopy.part_role_id        = SEMANTIC_ROLE_ORGANIC_FOLIAGE_CANOPY;
    canopy.parent_part_role_id = SEMANTIC_ROLE_ORGANIC_BRANCH;
    canopy.default_count       = 1u;
    canopy.base_center_offset  = glm::vec3(0.0f, 0.6f, 0.0f);
    canopy.base_half_extents   = glm::vec3(0.55f, 0.45f, 0.55f);
    r.add_part_template(canopy);

    r.add_parameter("branch_count", { 0u, SEMANTIC_ROLE_ORGANIC_BRANCH, 4.0f, 2.0f, 8.0f, 1.0f, 2.0f, AssemblyParamTargetOp::PARAM_OP_PART_COUNT });
    r.add_parameter("trunk_height", { 0u, SEMANTIC_ROLE_ORGANIC_TRUNK,  1.0f, 0.5f, 3.0f, 0.1f, 0.6f, AssemblyParamTargetOp::PARAM_OP_SCALE_Y });
    return r;
}

ParametricAssemblyRecipe ParametricAssemblyRecipe::create_geologic_outcrop_recipe() {
    ParametricAssemblyRecipe r;
    r.set_archetype_name("GeologicOutcrop");
    r.set_root_semantic_role_id(SEMANTIC_ROLE_ASSEMBLY_GEOLOGIC);
    r.set_root_seme_masks(0x0601u, 0x4000u);

    AssemblyPartTemplatePOD strata{};
    strata.part_role_id        = SEMANTIC_ROLE_GEOLOGIC_STRATA;
    strata.parent_part_role_id = SEMANTIC_ROLE_ASSEMBLY_GEOLOGIC;
    strata.min_count           = 1u;
    strata.max_count           = 5u;
    strata.default_count       = 3u;
    strata.container_mode      = DesignContainerLayoutMode::LAYOUT_LINEAR_BOX;
    strata.size_flags_y        = DESIGN_SIZE_EXPAND;
    strata.base_half_extents   = glm::vec3(1.2f, 0.35f, 1.0f);
    r.add_part_template(strata);

    AssemblyPartTemplatePOD boulder{};
    boulder.part_role_id        = SEMANTIC_ROLE_GEOLOGIC_BOULDER;
    boulder.parent_part_role_id = SEMANTIC_ROLE_GEOLOGIC_STRATA;
    boulder.default_count       = 1u;
    boulder.edge_snap_policy    = EdgeSnapPolicy::SNAP_SURFACE_NORMAL_DRAPE;
    boulder.base_center_offset  = glm::vec3(0.0f, 0.45f, 0.0f);
    boulder.base_half_extents   = glm::vec3(0.65f, 0.40f, 0.50f);
    r.add_part_template(boulder);

    r.add_parameter("strata_layers", { 0u, SEMANTIC_ROLE_GEOLOGIC_STRATA, 3.0f, 1.0f, 5.0f, 1.0f, 1.0f, AssemblyParamTargetOp::PARAM_OP_PART_COUNT });
    return r;
}

} // namespace ideam::core

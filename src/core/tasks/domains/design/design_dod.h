#pragma once

/**
 * design_dod.h
 * DOD Orchestrator for `IdeamDesign` (`src/core/tasks/domains/design/`).
 */

#include "design_dod_common.h"
#include "metadata_logic/parametric_assembly_morph_metadata_logic.h"
#include "transform_logic/structure_socket_constraint_solve_transform_logic.h"
#include "transform_logic/style_3d_theme_reskin_transform_logic.h"
#include "transform_logic/aperture_host_cutout_bridge_transform_logic.h"
#include <vector>

namespace ideam::core {

class DesignDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

    DesignBufferCapacities capacities_{};
    DesignBufferManifest   manifest_{};

    StructureLayoutSolver solver_{};
    float dream_morph_factor_   = 0.0f;
    float seat_height_override_ = -1.0f;
    float global_scale_factor_  = 1.0f;

public:
    DesignDOD() = default;
    ~DesignDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }

    [[nodiscard]] StructureLayoutSolver &solver() noexcept { return solver_; }
    [[nodiscard]] const StructureLayoutSolver &solver() const noexcept { return solver_; }

    void set_capacities(const DesignBufferCapacities &p_caps) noexcept { capacities_ = p_caps; }
    [[nodiscard]] const DesignBufferCapacities &get_capacities() const noexcept { return capacities_; }

    [[nodiscard]] const DesignBufferManifest &get_manifest() const noexcept { return manifest_; }
    [[nodiscard]] DesignBufferManifest &get_manifest_mut() noexcept { return manifest_; }

    void set_dream_morph_factor(float p_factor) noexcept { dream_morph_factor_ = p_factor; }
    [[nodiscard]] float get_dream_morph_factor() const noexcept { return dream_morph_factor_; }

    void set_seat_height_override(float p_height) noexcept { seat_height_override_ = p_height; }
    [[nodiscard]] float get_seat_height_override() const noexcept { return seat_height_override_; }

    void set_global_scale_factor(float p_scale) noexcept { global_scale_factor_ = p_scale; }
    [[nodiscard]] float get_global_scale_factor() const noexcept { return global_scale_factor_; }

    // --- Buffer Initialization & Staging ---
    bool initialize_design_memory(MemoryManagerDOD *p_mem);
    [[nodiscard]] uint32_t get_buffer_id(DesignBufferSlot p_slot) const noexcept {
        return manifest_.get(p_slot);
    }

    bool stage_solver_to_dod(const StructureLayoutSolver &p_solver);
    bool stage_solver_to_dod() { return stage_solver_to_dod(solver_); }

    // --- Direct Synchronous Execution (`DSG4 -> DSG2 -> DSG1 -> DSG3`) ---
    bool execute_full_pipeline_direct(uint32_t p_stage_mask = DESIGN_STAGE_DEFAULT_PIPELINE_MASK);

    // --- Readback & Sync Helpers ---
    void read_structure_nodes(std::vector<StructureNodePod> &r_out) const;
    void read_sockets(std::vector<SocketPod> &r_out) const;
    void read_style_rules(std::vector<Style3DRulePod> &r_out) const;
    void read_aperture_cutouts(std::vector<ApertureCutoutPod> &r_out) const;
    void read_node_status_masks(std::vector<uint32_t> &r_out) const;
    void sync_dod_back_to_solver();
};

} // namespace ideam::core

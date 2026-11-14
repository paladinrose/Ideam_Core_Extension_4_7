#pragma once

/**
 * structure_layout_solver.h
 * Pure C++26 1D/2D/3D Container Box-Model, Socket Contact Constraint, CSS 3D Theme Cascade,
 * and Aperture Host Boolean Cutout Solver (`src/core/design/`).
 */

#include "design_common.h"
#include <span>
#include <vector>

namespace ideam::core {

class StructureLayoutSolver {
private:
    std::vector<StructureNodePod> nodes_;
    std::vector<SocketPod> sockets_;
    std::vector<Style3DRulePod> style_rules_;
    std::vector<ApertureCutoutPod> aperture_cutouts_;

public:
    StructureLayoutSolver() = default;
    ~StructureLayoutSolver() = default;

    void clear() noexcept;

    uint32_t add_structure_node(const StructureNodePod &p_node);
    uint32_t add_socket(const SocketPod &p_socket);
    uint32_t add_style_rule(const Style3DRulePod &p_rule);
    uint32_t add_aperture_cutout(const ApertureCutoutPod &p_cutout);

    [[nodiscard]] const std::vector<StructureNodePod> &nodes() const noexcept { return nodes_; }
    [[nodiscard]] std::vector<StructureNodePod> &nodes_mut() noexcept { return nodes_; }

    [[nodiscard]] const std::vector<SocketPod> &sockets() const noexcept { return sockets_; }
    [[nodiscard]] std::vector<SocketPod> &sockets_mut() noexcept { return sockets_; }

    [[nodiscard]] const std::vector<Style3DRulePod> &style_rules() const noexcept { return style_rules_; }
    [[nodiscard]] std::vector<Style3DRulePod> &style_rules_mut() noexcept { return style_rules_; }

    [[nodiscard]] const std::vector<ApertureCutoutPod> &aperture_cutouts() const noexcept { return aperture_cutouts_; }
    [[nodiscard]] std::vector<ApertureCutoutPod> &aperture_cutouts_mut() noexcept { return aperture_cutouts_; }

    // --- 3D CSS Cascade & Specificity (`DSG2`) ---
    [[nodiscard]] static uint16_t compute_style_specificity(const Style3DRulePod &p_rule) noexcept;
    static void sort_style_rules_by_cascade(std::span<Style3DRulePod> r_rules) noexcept;
    void apply_theme_cascade_to_nodes() noexcept;

    // --- 3D Container & Socket Layout Solving (`DSG1`) ---
    void solve_layout_constraints();
    void solve_socket_contacts() noexcept;

    [[nodiscard]] static bool are_sockets_compatible(const SocketPod &p_a, const SocketPod &p_b) noexcept;

    // --- Aperture Host Wall Boolean Cutout Bridge (`DSG3`) ---
    uint32_t evaluate_aperture_cutouts();
};

} // namespace ideam::core

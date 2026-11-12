#include "wfc_solver_core.h"
#include <queue>

namespace ideam::core {

void WFCSolverCore::clear_rules_and_edges() noexcept {
    rule_tensor_.clear();
    adjacency_edges_.clear();
}

void WFCSolverCore::add_rule(uint32_t p_port_id, uint32_t p_source_state, const BitmaskDomain256POD &p_allowed_mask) {
    rule_tensor_.push_back({p_port_id, p_source_state, p_allowed_mask});
}

void WFCSolverCore::add_directed_edge(uint32_t p_from_slot, uint32_t p_to_slot, uint32_t p_port_id) {
    adjacency_edges_.push_back({p_from_slot, p_to_slot, p_port_id});
}

bool WFCSolverCore::propagate_arc_consistency() {
    std::queue<uint32_t> worklist;
    for (uint32_t i = 0; i < static_cast<uint32_t>(slot_domains_.size()); ++i) {
        worklist.push(i);
    }

    while (!worklist.empty()) {
        const uint32_t u = worklist.front();
        worklist.pop();

        for (const auto &edge : adjacency_edges_) {
            if (edge.from_slot != u || edge.to_slot >= slot_domains_.size()) {
                continue;
            }
            BitmaskDomain256POD union_allowed{};
            bool has_port_rules = false;
            for (const auto &rule : rule_tensor_) {
                if (rule.port_id == edge.port_id && slot_domains_[u].test_bit(rule.source_state)) {
                    has_port_rules = true;
                    for (size_t w = 0; w < 4; ++w) {
                        union_allowed.words[w] |= rule.allowed_mask.words[w];
                    }
                }
            }
            if (!has_port_rules) {
                continue;
            }

            auto &target_dom = slot_domains_[edge.to_slot];
            bool changed = false;
            for (size_t w = 0; w < 4; ++w) {
                const uint64_t restricted = target_dom.words[w] & union_allowed.words[w];
                if (restricted != target_dom.words[w]) {
                    target_dom.words[w] = restricted;
                    changed = true;
                }
            }
            if (target_dom.empty()) {
                status_ = ProcGenSolverStatus::CONTRADICTION;
                return false;
            }
            if (changed) {
                worklist.push(edge.to_slot);
            }
        }
    }
    return true;
}

ProcGenSolverStatus WFCSolverCore::solve_all(uint32_t p_max_steps) {
    status_ = ProcGenSolverStatus::IN_PROGRESS;
    if (!apply_pinned_landmarks() || !propagate_arc_consistency()) {
        return status_;
    }

    for (uint32_t step = 0; step < p_max_steps; ++step) {
        const int32_t slot = select_lowest_entropy_slot();
        if (status_ == ProcGenSolverStatus::CONTRADICTION) {
            return status_;
        }
        if (slot < 0) {
            status_ = ProcGenSolverStatus::SOLVED;
            return status_;
        }

        const int32_t chosen_state = slot_domains_[static_cast<size_t>(slot)].first_set_state();
        if (chosen_state < 0) {
            status_ = ProcGenSolverStatus::CONTRADICTION;
            return status_;
        }
        slot_domains_[static_cast<size_t>(slot)].clear();
        slot_domains_[static_cast<size_t>(slot)].set_bit(static_cast<uint32_t>(chosen_state));

        if (!propagate_arc_consistency()) {
            return status_;
        }
    }
    return status_;
}

} // namespace ideam::core

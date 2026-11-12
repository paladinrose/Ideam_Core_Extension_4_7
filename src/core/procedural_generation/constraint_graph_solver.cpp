#include "constraint_graph_solver.h"
#include <algorithm>
#include <limits>

namespace ideam::core {

void ConstraintGraphSolver::configure_slots(uint32_t p_slot_count, uint32_t p_state_count, uint64_t p_seed) {
    state_count_ = std::min<uint32_t>(p_state_count, 256u);
    rng_         = ProcGenSeedStreamPOD(p_seed);
    slot_domains_.resize(p_slot_count);
    for (auto &dom : slot_domains_) {
        dom.fill_first_n(state_count_);
    }
    status_ = ProcGenSolverStatus::UNSOLVED;
}

void ConstraintGraphSolver::clear_pinned_landmarks() noexcept {
    pinned_landmarks_.clear();
}

void ConstraintGraphSolver::add_pinned_landmark(uint32_t p_slot_id, uint32_t p_state_id) {
    pinned_landmarks_.push_back({p_slot_id, p_state_id});
}

int32_t ConstraintGraphSolver::collapsed_state_at(uint32_t p_slot_id) const noexcept {
    if (p_slot_id >= slot_domains_.size()) {
        return -1;
    }
    const auto &dom = slot_domains_[p_slot_id];
    return (dom.popcount() == 1u) ? dom.first_set_state() : -1;
}

bool ConstraintGraphSolver::apply_pinned_landmarks() {
    for (const auto &pin : pinned_landmarks_) {
        if (pin.slot_id >= slot_domains_.size() || pin.pinned_state >= state_count_) {
            continue;
        }
        slot_domains_[pin.slot_id].clear();
        slot_domains_[pin.slot_id].set_bit(pin.pinned_state);
        if (slot_domains_[pin.slot_id].empty()) {
            status_ = ProcGenSolverStatus::CONTRADICTION;
            return false;
        }
    }
    return true;
}

int32_t ConstraintGraphSolver::select_lowest_entropy_slot() noexcept {
    uint32_t best_count = std::numeric_limits<uint32_t>::max();
    int32_t best_slot   = -1;

    for (uint32_t i = 0; i < static_cast<uint32_t>(slot_domains_.size()); ++i) {
        const uint32_t c = slot_domains_[i].popcount();
        if (c == 0u) {
            status_ = ProcGenSolverStatus::CONTRADICTION;
            return -1;
        }
        if (c > 1u && c < best_count) {
            best_count = c;
            best_slot  = static_cast<int32_t>(i);
        }
    }
    return best_slot;
}

} // namespace ideam::core

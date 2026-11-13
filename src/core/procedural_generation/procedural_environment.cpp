#include "procedural_environment.h"

namespace ideam::core {

void ProceduralEnvironment::clear() noexcept {
    tokens_.clear();
    slots_.clear();
    child_slot_csr_.clear();
    active_rules_.clear();
    rule_token_masks_.clear();
}

void ProceduralEnvironment::configure_slots(uint32_t p_slot_count, uint32_t p_token_count) {
    slots_.resize(p_slot_count);
    for (uint32_t i = 0; i < p_slot_count; ++i) {
        slots_[i] = EnvironmentSlotPOD{};
        slots_[i].slot_id = i;
        slots_[i].domain.fill_first_n(p_token_count);
    }
}

} // namespace ideam::core

#pragma once

/**
 * communication_medium_resource.h
 * `CommunicationMediumResource` (`src/godot/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Godot `Resource` representing the physical or logical transmission medium and
 * its noise profile (succeeding `DiscourseMedium.cs` and `AudioEnvironment.cs`).
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../core/communications/communications_common.h"

namespace ideam::godot_ext {

class CommunicationMediumResource : public godot::Resource {
    GDCLASS(CommunicationMediumResource, godot::Resource)

private:
    float ambient_noise_db_       = 50.0f;
    float attenuation_db_per_m_   = 0.6f;
    float occlusion_damping_db_   = 0.0f;

protected:
    static void _bind_methods();

public:
    CommunicationMediumResource() = default;
    ~CommunicationMediumResource() override = default;

    void set_ambient_noise_db(float p_db) { ambient_noise_db_ = p_db; emit_changed(); }
    [[nodiscard]] float get_ambient_noise_db() const noexcept { return ambient_noise_db_; }

    void set_attenuation_db_per_m(float p_att) { attenuation_db_per_m_ = p_att; emit_changed(); }
    [[nodiscard]] float get_attenuation_db_per_m() const noexcept { return attenuation_db_per_m_; }

    void set_occlusion_damping_db(float p_occ) { occlusion_damping_db_ = p_occ; emit_changed(); }
    [[nodiscard]] float get_occlusion_damping_db() const noexcept { return occlusion_damping_db_; }
};

} // namespace ideam::godot_ext

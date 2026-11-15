#pragma once

/**
 * @file thermodynamic_sounding_skewt_control.h
 * @brief Radiosonde / Skew-T vertical sounding & supercell hodograph inspector widget for IdeamClimatology.
 *
 * Displays surface temperature, upper-troposphere temperature lapse, Lifting Condensation Level (LCL),
 * Convective Available Potential Energy (CAPE), 0-3 km Storm-Relative Helicity (SRH), Köppen-Geiger
 * classification, and active tornado vortex EF rating at the selected `(x, y)` cell.
 */

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "godot/climatology/climatology_weather_resource.h"

namespace ideam::climatology {

class ThermodynamicSoundingSkewTControl : public godot::Control {
    GDCLASS(ThermodynamicSoundingSkewTControl, godot::Control)

private:
    godot::Ref<ClimatologyAtmosphereResource> atmosphere_resource_;
    godot::Vector2i inspected_cell_{32, 32};
    godot::Dictionary cached_sounding_;

protected:
    static void _bind_methods();

public:
    ThermodynamicSoundingSkewTControl();
    ~ThermodynamicSoundingSkewTControl() override = default;

    void set_atmosphere_resource(const godot::Ref<ClimatologyAtmosphereResource>& p_resource);
    godot::Ref<ClimatologyAtmosphereResource> get_atmosphere_resource() const;

    void set_inspected_cell(const godot::Vector2i& p_cell);
    godot::Vector2i get_inspected_cell() const;

    void refresh_sounding_view();
    void _draw() override;
};

} // namespace ideam::climatology

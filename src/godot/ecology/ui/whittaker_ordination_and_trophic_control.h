#pragma once

/**
 * @file whittaker_ordination_and_trophic_control.h
 * @brief Quadrat ecology, Whittaker biome ordination & trophic pyramid inspector widget for IdeamEcology.
 */

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "godot/ecology/ecology_biosphere_resource.h"

namespace ideam::godot_ext {

class WhittakerOrdinationAndTrophicControl : public godot::Control {
    GDCLASS(WhittakerOrdinationAndTrophicControl, godot::Control)

private:
    godot::Ref<EcologyBiosphereResource> biosphere_resource_;
    godot::Vector2i inspected_cell_{32, 32};
    godot::Dictionary cached_quadrat_;

protected:
    static void _bind_methods();

public:
    WhittakerOrdinationAndTrophicControl();
    ~WhittakerOrdinationAndTrophicControl() override = default;

    void set_biosphere_resource(const godot::Ref<EcologyBiosphereResource>& p_resource);
    godot::Ref<EcologyBiosphereResource> get_biosphere_resource() const;

    void set_inspected_cell(const godot::Vector2i& p_cell);
    godot::Vector2i get_inspected_cell() const;

    void refresh_quadrat_view();
    void _draw() override;
};

} // namespace ideam::godot_ext
